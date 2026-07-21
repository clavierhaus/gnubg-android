package com.clavierhaus.gnubg.engine

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.clavierhaus.gnubg.Engine
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.drop
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.util.concurrent.Executors

class GameViewModel(application: Application) : AndroidViewModel(application) {

    private val _settings = MutableStateFlow(GameSettings())
    val settings: StateFlow<GameSettings> = _settings.asStateFlow()

    private val _gameState = MutableStateFlow(BoardState())
    /* ONE STATE, ONE WRITER (contract order): _gameState is written ONLY on
     * the engine thread -- readMatchState (the projection) plus the sanctioned
     * transient writers (the tap pipeline's sub-move draft, the busy/hold
     * overlay setters below). The live-dice watcher runs on another thread and
     * therefore NEVER touches _gameState: it owns its own leaf flow, merged
     * read-only here. No field has two writers; no writer crosses a thread. */
    private val _liveEngineDice = MutableStateFlow<Pair<Int, Int>?>(null)
    val gameState: StateFlow<BoardState> =
        combine(_gameState, _liveEngineDice) { st, live ->
            // Show the engine's roll while it is thinking AND on the end screen:
            // a game-ending engine move transitions straight to GAME_OVER, and
            // the roll that ended the game must remain visible (it is the record
            // of what happened). Any later phase clears it via _liveEngineDice.
            if ((st.phase == GamePhase.ENGINE_THINKING || st.phase == GamePhase.GAME_OVER) && live != null)
                st.copy(engineDice = live) else st
        }.stateIn(viewModelScope, SharingStarted.Eagerly, BoardState())

    /* What the engine work is, while phase == ENGINE_THINKING -- the one
     * genuinely app-side distinction the panel needs (judging the player's
     * move vs delivering it for GNU's reply). Set only by beginEngineWork /
     * settleFromEngine on the engine thread. */
    private val _busyKind = MutableStateFlow(BusyKind.NONE)
    val busyKind: StateFlow<BusyKind> = _busyKind.asStateFlow()

    private var lastTutorAnalysis: TutorAnalysis? = null
    private var lastAnalysisDetail: MoveAnalysisDetail? = null

    /* Coach session (docs/COACH.md). While active, confirm() runs ONE analysis
     * per move -- the coach verdict -- instead of the Play/Tutor pair
     * (tutorAnalyze + analyzePlayedMove). Field report: all three stacked on
     * the single engine thread made a doubles-roll turn take 30+ seconds and
     * GNU looked stuck. One move, one evaluation (vision C6). */
    private var coachSession = false
    /* Coach setup (M4): the mode shows a setup screen -- strength + length --
     * before the board, mirroring the tournament setup, so match length > 1 is
     * selectable now that cube coaching exists. Independent of the Play/tutor
     * settings; the coach owns its own choices. */
    private val _showCoachSetup = MutableStateFlow(true)
    val showCoachSetup: StateFlow<Boolean> = _showCoachSetup.asStateFlow()
    private val _coachLength = MutableStateFlow(1)
    val coachLength: StateFlow<Int> = _coachLength.asStateFlow()
    private val _coachDifficulty = MutableStateFlow(Difficulty.EXPERT)
    val coachDifficulty: StateFlow<Difficulty> = _coachDifficulty.asStateFlow()
    fun setCoachLength(n: Int) { _coachLength.value = n.coerceIn(1, 25) }
    fun setCoachDifficulty(d: Difficulty) { _coachDifficulty.value = d }
    fun openCoachSetup() { _showCoachSetup.value = true }

    private val _coachGlance = MutableStateFlow<IntArray?>(null)
    val coachGlance: StateFlow<IntArray?> = _coachGlance.asStateFlow()

    /* Coach: the committed move is HELD here, not yet given to gnubg, while the
     * player studies the verdict (maintainer design: GNU must not move -- must
     * not even have ROLLED -- while alternatives are on the table, since its
     * dice would change the position under discussion). gnubg's state stays
     * pre-move, which is also exactly what makes the alternative views
     * consistent with the live board. continueCoachTurn() delivers it. */
    private var pendingCoachMove: String? = null
    // Terminal latch (root fix, 2026-07-20): GAME_OVER is sticky. gnubg's
    // NextTurn(TRUE) auto-advances into the next game the instant a game ends,
    // so any settle that reads the engine after a win sees a live next game and
    // would overwrite the end screen. Once GAME_OVER is projected, readMatchState
    // refuses to replace the phase until newGame() (the player's acknowledgment)
    // clears this. One owner of the terminal transition, enforced at the writer.
    private var gameOverLatched = false

    /* Coach cube coaching (M4): the parallel of pendingCoachMove for cube
     * decisions. When the human makes a cube decision in a coach session, the
     * verdict is judged and HELD here; continueCoachCube() then carries out the
     * action the human chose. _coachCubeGlance is the 10-int coachCubeVerdict
     * payload (gnubg values only) the panel reads. */
    private val _coachCubeGlance = MutableStateFlow<IntArray?>(null)
    val coachCubeGlance: StateFlow<IntArray?> = _coachCubeGlance.asStateFlow()

    /** GNU's answer to the PLAYER's double (coach only): null = none pending,
     *  first = took (true) / dropped (false), second = points won on a drop.
     *  A take is shown until the player commits their next action (wiped in
     *  beginEngineWork, one rule); a drop is shown through GAME_OVER and
     *  cleared with the acknowledged new game. */
    private val _coachCubeAnswer = MutableStateFlow<Pair<Boolean, Int>?>(null)
    val coachCubeAnswer: StateFlow<Pair<Boolean, Int>?> = _coachCubeAnswer.asStateFlow()
    // 0 no-double(roll), 1 doubled, 2 took, 3 dropped -- the action to carry
    // out on continue. -1 = nothing pending.
    private var pendingCubeAction: Int = -1

    companion object {
        /** Position-SENSITIVE fingerprint (a plain sum is invariant: every
         *  legal position sums to 30). Weighted by index squared so any
         *  checker relocation changes the value. */
        fun fpOf(b: IntArray): Int = b.withIndex().sumOf { (i, x) -> x * (i * i + 1) }
    }

    private val _engineReady = MutableStateFlow(false)
    val engineReady: StateFlow<Boolean> = _engineReady.asStateFlow()

    private val _showMatchSetup = MutableStateFlow(true)
    val showMatchSetup: StateFlow<Boolean> = _showMatchSetup.asStateFlow()

    private val engineThread = Executors.newSingleThreadExecutor { r ->
        Thread(r, "gnubg-engine-thread")
    }.asCoroutineDispatcher()
    private val actionInProgress = java.util.concurrent.atomic.AtomicBoolean(false)

    /**
     * Write the match gnubg currently holds to [file], as gnubg's native SGF.
     * Returns true only if a non-empty file resulted.
     *
     * PORT: CommandSaveMatch (sgf.c:2365), reached via gnubg_mobile_save_match.
     * Three of its properties dictate how it must be called, all recorded in
     * docs/TECHNICAL-NOTES.md:
     *
     *  - It begins with NextToken(&sz), which splits the path on whitespace, so a
     *    path containing a space is silently truncated. The caller must pass a
     *    space-free path; this method refuses one that is not.
     *  - It returns void, and FACADE_FILE_OP returns 1 unconditionally, so
     *    Engine.saveMatch()'s Boolean says only that the call was made. Success is
     *    established here, by the file existing and being non-empty.
     *  - It refuses when plGame is NULL ("No game in progress"), printing to the
     *    log. That surfaces here as a false return, not an exception.
     *
     * gnubg writes the whole match -- every game so far -- not just the current
     * game, so this is meaningful at any point once a game has started.
     */
    suspend fun saveMatchToFile(file: java.io.File): Boolean = withContext(engineThread) {
        if (file.absolutePath.any { it.isWhitespace() }) return@withContext false
        if (file.exists() && !file.delete()) return@withContext false
        Engine.saveMatch(file.absolutePath)
        file.exists() && file.length() > 0L
    }

    init {
        viewModelScope.launch(engineThread) {
            // 1. Load persisted settings BEFORE the engine reads them, so the
            //    first match honors the user's saved rules/strength/MET.
            val saved = PreferencesManager.settingsFlow(application).first()
            _settings.value = saved

            val weightsPath = AssetExtractor.extractWeights(application)
            Engine.initialise(weightsPath)
            // Apply the saved engine strength (gnubg preset) before any game,
            // so the first move uses it rather than the hardcoded 2-ply setup.
            Engine.setEngineStrength(saved.difficulty.settingIndex)
            // Push saved tournament rule preferences into the engine globals so
            // the first match honors them. These are safe global toggles
            // (set.c); the engine decides applicability per game/match.
            Engine.setAutoCrawford(saved.crawford)
            Engine.setJacoby(saved.jacoby)
            Engine.setAutoDoubles(saved.automaticDoubles)
            Engine.setBeavers(if (saved.beavers) 3 else 0)
            Engine.setCubeUse(saved.cubeUse)
            // Extract the bundled match-equity tables and load the saved choice
            // (overriding the built-in Zadeh default set during initialise()).
            val metDir = AssetExtractor.extractMets(application)
            Engine.setMet("$metDir/${saved.metTable.fileName}")
            _engineReady.value = true

            // 2. Persist every subsequent settings change. One observer on the
            //    whole settings object means no per-setter save calls -- any
            //    mutation via copy(...) is caught and written. drop(1) skips the
            //    value we just loaded.
            _settings.drop(1).collect { s ->
                PreferencesManager.saveSettings(application, s)
            }
        }
    }

    fun startMatch(length: Int) {
        _settings.value = _settings.value.copy(matchLength = length)
        _showMatchSetup.value = false
        viewModelScope.launch(engineThread) {
            startNewGame(isNewMatch = true)
        }
    }

    /**
     * The set of die faces (from remainingDice) that gnubg move generator
     * never uses -- dice with no legal play, which the UI greys out.
     *
     * gnubg authority: legalMoves is GenerateMoves (fPartial=0), the complete
     * maximal legal moves. A die is PLAYABLE iff some legal move contains a
     * sub-move using it; the generator already enforces must-use-both and
     * use-the-larger, so an unplayable die appears in no move. Each sub-move die
     * comes from gnubg encoding: in-board dest>=0 gives die=src-dest; a bear-off
     * sub-move clamps dest to -1 (die not recoverable from the list), so we
     * resolve it via the engine -- the remaining die that applySubMove accepts
     * from that source (facade LegalMove gate). No die is invented here.
     */
    /**
     * Die faces for which gnubg lists no legal play.
     *
     * The bear-off branch is NOT a heuristic that could be replaced by
     * `src - dest`. gnubg clamps every negative destination to -1 in SaveMoves
     * (eval.c: `pm->anMove[i] = anMoves[i] > -1 ? anMoves[i] : -1`), so -1 means
     * "off" and carries no information about which die was used: bearing off
     * point 1 with a 1 and point 3 with a 6 both store (src, -1). The die must be
     * recovered by asking gnubg -- Engine.applySubMove is LegalMove -- against
     * the board as it stands at that sub-move, which is why the working board is
     * replayed.
     *
     * A previous version of this file replaced all of that with
     * `playable.add(src - dest)`, on the stated grounds that GenerateMovesSub
     * writes `i - anRoll`. It does; SaveMoves then clamps it. That change made
     * bear-offs report the wrong die, and the same false premise, carried into
     * tapSource, made a legal bear-off with a 6 impossible to play.
     */
    private fun unplayableDiceFor(board: IntArray, moves: IntArray, remaining: List<Int>): Set<Int> {
        if (remaining.isEmpty()) return emptySet()
        val distinct = remaining.distinct()
        if (moves.isEmpty()) return distinct.toSet()  // no legal move: all unplayable

        val playable = mutableSetOf<Int>()
        val nMoves = moves.size / 8
        for (i in 0 until nMoves) {
            // Replay this move sub-moves on a working board so a bear-off die
            // can be resolved against the board state at that sub-move.
            var work = board
            for (j in 0..3) {
                val src  = moves[i * 8 + j * 2]
                val dest = moves[i * 8 + j * 2 + 1]
                if (src < 0) break            // -1 source = move terminator
                if (dest >= 0) {
                    playable.add(src - dest)  // in-board: die is exact
                    val nb = Engine.applySubMove(work, src, src - dest)
                    if (nb.isNotEmpty()) work = nb
                } else {
                    // bear-off (dest clamped to -1): the die is whichever remaining
                    // value the engine accepts from this source on the working board.
                    // Prefer a not-yet-seen die so each bear-off widens coverage; fall
                    // back to any accepted die to keep the working board advancing.
                    var chosen = -1
                    for (d in distinct) {
                        if (Engine.applySubMove(work, src, d).isNotEmpty()) {
                            if (d !in playable) { chosen = d; break }
                            if (chosen < 0) chosen = d
                        }
                    }
                    if (chosen >= 0) {
                        playable.add(chosen)
                        val nb = Engine.applySubMove(work, src, chosen)
                        if (nb.isNotEmpty()) work = nb
                    }
                }
                if (playable.containsAll(distinct)) return emptySet()
            }
        }
        return distinct.filterNot { it in playable }.toSet()
    }

    private fun readMatchState(
        phase: GamePhase,
        remainingDice: List<Int> = emptyList(),
        legalMoves: IntArray = IntArray(0),
        oldBoard: IntArray = IntArray(50),
        originalDice: Pair<Int, Int>? = null,
        engineDice: Pair<Int, Int>? = null,
        winner: Int = -1,
        nPoints: Int = 1,
        moveHistory: List<MoveSnapshot> = emptyList()
    ) {
        // Terminal latch: hold GAME_OVER against any non-terminal projection
        // until the player acknowledges (newGame). A fresh GAME_OVER (winner
        // supplied) re-latches; everything else is refused while latched.
        if (phase == GamePhase.GAME_OVER) {
            gameOverLatched = true
        } else if (gameOverLatched) {
            android.util.Log.i("gnubg-vm",
                "readMatchState: held at GAME_OVER (refused phase=$phase while latched)")
            return
        }
        val score       = Engine.getMatchScore()
        val matchLength = Engine.getMatchLength()
        val cubeInfo  = Engine.getMatchCubeInfo()
        val fDoubled  = cubeInfo[0] == 1
        val cubeOwner = cubeInfo[1]
        val cubeValue = cubeInfo[2]
        // Engine is the sole authority on cube tappability (gnubg_can_double,
        // play.c:2397 preconditions: Crawford, cube use, ownership, dead cube,
        // match-value cap). The UI reads this rather than reimplementing any
        // subset of the rule.
        val canDouble = Engine.canDouble()
        val turn      = Engine.getMatchTurn()
        // Display board comes pre-oriented to a STABLE human (player-0) frame from
        // a single atomic facade call. The old code read the board and an
        // orientation field (fTurn) in two separate locked JNI calls and swapped
        // in Kotlin; between the two reads the engine thread could change fMove
        // and re-orient ms.anBoard, so the swap was applied against a board from a
        // different instant -- flipping the display mid-game. Resolving
        // orientation under one lock in the facade removes that race entirely; the
        // UI does no swapping.
        val board     = Engine.getMatchBoardHuman()
        val pips      = Engine.pipCount(board)
        val dicePair: Pair<Int, Int>? = originalDice ?: when {
            remainingDice.size >= 2 -> Pair(remainingDice[0], remainingDice[1])
            remainingDice.size == 1 -> Pair(remainingDice[0], remainingDice[0])
            else -> null
        }
        // Die faces gnubg lists no legal play for (greyed in the UI). Computed
        // against oldBoard -- the exact board legalMoves was generated from --
        // only when it is the human turn with dice in play.
        val unplayableDice =
            if (turn == 0 && remainingDice.isNotEmpty())
                unplayableDiceFor(oldBoard, legalMoves, remainingDice)
            else emptySet()
        _gameState.value = BoardState(
            matchScore     = score,
            matchLength    = matchLength,
            board          = board,
            oldBoard       = oldBoard,
            turn           = turn,
            dice           = dicePair,
            originalDice   = originalDice ?: dicePair,
            remainingDice  = remainingDice,
            legalMoves     = legalMoves,
            pipCountHuman  = pips[0],
            pipCountEngine = pips[1],
            phase          = phase,
            engineDice     = engineDice,
            winner         = winner,
            nPoints        = nPoints,
            humanScore     = score[0],
            engineScore    = score[1],
            cubeValue      = cubeValue,
            cubeOwner      = cubeOwner,
            fDoubled       = fDoubled,
            canDouble      = canDouble,
            unplayableDice = unplayableDice,
            resignation    = Engine.getResignation(),
            tutorAnalysis  = lastTutorAnalysis,
            analysisDetail = lastAnalysisDetail
        )
    }

    private fun startNewGame(isNewMatch: Boolean = true) {
        // The player has acknowledged the end screen: release the terminal
        // latch so the next game's projection can be written.
        gameOverLatched = false
        // Before driving gnubg's new game (which may hand the opening to the
        // engine and compute its move synchronously), show a thinking state and
        // arm the live-dice watcher. Otherwise, when the engine wins the opening
        // roll at a slow level, the board sits static for 7-9s and looks frozen
        // -- a field report. The watcher surfaces the opening roll as it lands.
        beginEngineWork(BusyKind.REPLYING)
        if (isNewMatch) Engine.newGame(_settings.value.matchLength) else Engine.nextGame()
        settleFromEngine()
    }

    fun newGame() {
        // The GAME_OVER acknowledgment consumes any pending cube answer.
        _coachCubeAnswer.value = null
        viewModelScope.launch(engineThread) {
            val matchLength = Engine.getMatchLength()
            // Match-over is gnubg decision, not a UI score comparison.
            // getMatchWinner() (facade mirrors play.c:2816) returns 0/1 once a
            // player has reached nMatchTo, else -1. It returns -1 for a single
            // game (nMatchTo <= 1), so that case is handled explicitly -- a
            // 1-pointer has no next game, so we always return to setup.
            val matchOver = Engine.getMatchWinner() >= 0 || matchLength <= 1
            if (matchOver) {
                _showMatchSetup.value = true
            } else {
                startNewGame(isNewMatch = false)
            }
        }
    }

    /**
     * Accept GNU's resignation. PORT: CommandAgree. The game ends; gnubg builds
     * the MOVE_RESIGN record and awards the points.
     */
    fun acceptResignation() {
        viewModelScope.launch(engineThread) {
            val scoreBefore = Engine.getMatchScore()
            Engine.commandAgree()
            settleFromEngine(result = gameResultFromScoreDelta(scoreBefore))
        }
    }

    /**
     * Refuse it and play on. PORT: CommandDecline. gnubg records
     * ms.fResignationDeclined so GNU will not offer the same level again.
     */
    fun declineResignation() {
        viewModelScope.launch(engineThread) {
            Engine.commandDecline()
            settleFromEngine()
        }
    }

    fun rollDice() {
        if (_gameState.value.phase != GamePhase.WAITING_FOR_ROLL) return
        viewModelScope.launch(engineThread) {
            doRollNow()
        }
    }

    /** The actual roll, on engineThread. gnubg refuses to roll while a
     *  resignation is unanswered (play.c:4048), so surface that WITHOUT
     *  rolling; otherwise roll and let the projection derive whatever gnubg
     *  now says the state is. No phase is picked here (contract order). */
    private suspend fun doRollNow() {
        if (Engine.getResignation() > 0 && Engine.getMatchTurn() == 0) {
            settleFromEngine()
            return
        }
        Engine.rollDice()
        settleFromEngine()
    }


    private fun tryDestinationStackMove(state: BoardState, point: Int): BoardState? {
        if (point !in 1..24) return null
        if (state.remainingDice.size < 2) return null

        // If a checker is on the bar, the normal bar-entry rules must stay explicit.
        if (state.board[49] > 0) return null

        // Destination shortcut is only for empty points or opponent blots/points.
        // If this point contains our checker, a tap is a normal source tap.
        if (state.board[24 + point] > 0) return null

        val dest = point - 1
        val results = mutableListOf<Pair<IntArray, List<Int>>>()

        state.remainingDice.forEachIndexed { i, dieA ->
            val srcA = dest + dieA
            if (srcA in 0..23) {
                val b1 = Engine.applySubMove(state.board, srcA, dieA)
                if (b1.isNotEmpty()) {
                    val rem1 = state.remainingDice.toMutableList().also { it.removeAt(i) }

                    rem1.forEachIndexed { j, dieB ->
                        val srcB = dest + dieB
                        if (srcB in 0..23) {
                            val b2 = Engine.applySubMove(b1, srcB, dieB)
                            if (b2.isNotEmpty()) {
                                val rem2 = rem1.toMutableList().also { it.removeAt(j) }
                                results.add(Pair(b2, rem2))
                            }
                        }
                    }
                }
            }
        }

        if (results.isEmpty()) return null

        val unique = results.distinctBy { (board, remaining) ->
            board.joinToString(",") + "|" + remaining.joinToString(",")
        }

        if (unique.size != 1) {
            android.util.Log.i(
                "gnubg-vm",
                "destinationStack: ambiguous point=$point candidates=${unique.size}"
            )
            return null
        }

        val newBoard = unique[0].first
        val rawRemaining = unique[0].second

        // Legality is already gnubg's, at both granularities: each of the two
        // applySubMove hops above routes through LegalMove (eval.c:2732), and
        // the COMPLETE turn is validated by confirm() via Engine.findMove
        // (GenerateMoves + cMaxMoves/cMaxPips match) when the player commits.
        // A destination stack is a PARTIAL move -- on doubles it plays two of
        // four dice -- so it must NOT be gated against findMove here (that
        // demands a maximum move and wrongly rejects the legal partial: e.g.
        // double sixes, stacking two checkers 21->15 leaves two sixes still to
        // play). No extra check belongs here.

        val rawNextMoves = if (rawRemaining.isNotEmpty()) {
            val r0 = rawRemaining[0]
            val r1 = if (rawRemaining.size > 1) rawRemaining[1] else r0
            Engine.getLegalMoves(newBoard, r0, r1)
        } else IntArray(0)

        val newRemaining =
            if (rawRemaining.isNotEmpty() && rawNextMoves.isEmpty()) emptyList()
            else rawRemaining

        val nextMoves = if (newRemaining.isNotEmpty()) rawNextMoves else IntArray(0)
        val pips = Engine.pipCount(newBoard)

        val snapshot = MoveSnapshot(
            board = state.board.copyOf(),
            remainingDice = state.remainingDice,
            legalMoves = state.legalMoves.copyOf(),
            pipCountHuman = state.pipCountHuman,
            pipCountEngine = state.pipCountEngine
        )

        android.util.Log.i(
            "gnubg-vm",
            "destinationStack: point=$point oldRemaining=${state.remainingDice} newRemaining=$newRemaining"
        )

        return state.copy(
            board          = newBoard,
            remainingDice  = newRemaining,
            legalMoves     = nextMoves,
            pipCountHuman  = pips[0],
            pipCountEngine = pips[1],
            dice           = state.dice,
            moveHistory    = state.moveHistory + snapshot
        )
    }

    fun dragMove(from: Int, to: Int) {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        if (state.remainingDice.isEmpty()) return
        if (from !in 0..24 || to !in 0..24) return
        viewModelScope.launch(engineThread) {
            // Resolve the drag through gnubg's OWN enumerated move paths, exactly
            // as the landing-point highlight does (Board.landingPointsForSource).
            // Each legal move is a path of up to four sub-moves (anMove[8] = four
            // src/dst pairs; src < 0 terminates). We follow the dragged checker
            // from its origin along each path, collecting the sub-moves it takes,
            // and accept the FIRST path that walks it to `to` -- including
            // multi-hop (e.g. double 2s from 24 reaching 18 in three hops). The
            // old code enumerated only single-die and same-target two-die moves,
            // so multi-hop drags previewed (the highlight walks these paths) but
            // committed nothing (this resolver had no branch for them): preview
            // from gnubg, commit from hand arithmetic, disagreeing on multi-hop.
            // Now both read the same authority.
            //
            // from == 0 is the bar signal; the checker re-enters from gnubg point
            // 24 (0-based), which is where the legal-move paths already start it.
            val origin = if (from == 0) 24 else from - 1
            val target = to - 1                  // gnubg 0-based; to == 0 -> -1 (bear off)
            val moves = state.legalMoves
            var newBoard = IntArray(0)
            val usedDice = ArrayList<Int>()

            var m = 0
            walk@ while (m + 7 < moves.size) {
                var here = origin
                var moving = false
                val pathSub = ArrayList<Pair<Int, Int>>()   // (src, dst) hops of OUR checker
                var reached = false
                for (j in 0..3) {
                    val s = moves[m + j * 2]
                    val d = moves[m + j * 2 + 1]
                    if (s < 0) break
                    if (!moving && s == origin) {
                        moving = true; pathSub.add(s to d); here = d
                    } else if (moving && s == here) {
                        pathSub.add(s to d); here = d
                    } else continue           // a different checker's sub-move
                    if (d == target || (to == 0 && d < 0)) { reached = true; break }
                }
                if (reached && pathSub.isNotEmpty()) {
                    // Apply this checker's hops in order via the engine. The roll
                    // for a hop is gnubg's own expression, src - dst (eval.c:2525
                    // applies anMove paths as ApplySubMove(.., src, src - dst)).
                    // For a bear-off hop gnubg marks dst with a negative sentinel,
                    // where src - dst would be wrong; there we try each remaining
                    // die and let the engine's LegalMove gate accept the right one
                    // (bear-off, including overage, lives in the engine, not here).
                    var b = state.board
                    val used = ArrayList<Int>()
                    val avail = state.remainingDice.toMutableList()
                    var ok = true
                    for ((s, d) in pathSub) {
                        if (d >= 0) {
                            val roll = s - d
                            val nb = Engine.applySubMove(b, s, roll)
                            if (nb.isEmpty() || !avail.remove(roll)) { ok = false; break }
                            b = nb; used.add(roll)
                        } else {
                            // bear-off: try remaining dice, smallest legal first
                            var done = false
                            for (roll in avail.sorted()) {
                                val nb = Engine.applySubMove(b, s, roll)
                                if (nb.isNotEmpty()) {
                                    b = nb; used.add(roll); avail.remove(roll); done = true; break
                                }
                            }
                            if (!done) { ok = false; break }
                        }
                    }
                    if (ok) { newBoard = b; usedDice.addAll(used); break@walk }
                }
                m += 8
            }
            if (newBoard.isEmpty()) {
                android.util.Log.i("gnubg-vm", "dragMove: no legal path from=$from to=$to -- ignored")
                return@launch
            }
            val rawRemaining = state.remainingDice.toMutableList()
            usedDice.forEach { rawRemaining.remove(it) }
            val pips = Engine.pipCount(newBoard)
            val rawNextMoves = if (rawRemaining.isNotEmpty()) {
                val r0 = rawRemaining[0]
                val r1 = if (rawRemaining.size > 1) rawRemaining[1] else r0
                Engine.getLegalMoves(newBoard, r0, r1)
            } else IntArray(0)
            val newRemaining =
                if (rawRemaining.isNotEmpty() && rawNextMoves.isEmpty()) emptyList()
                else rawRemaining
            val nextMoves = if (newRemaining.isNotEmpty()) rawNextMoves else IntArray(0)
            android.util.Log.i("gnubg-vm", "dragMove: from=$from to=$to usedDice=$usedDice newRemaining=$newRemaining")
            val snapshot = MoveSnapshot(
                board = state.board.copyOf(),
                remainingDice = state.remainingDice,
                legalMoves = state.legalMoves.copyOf(),
                pipCountHuman = state.pipCountHuman,
                pipCountEngine = state.pipCountEngine
            )
            _gameState.value = state.copy(
                board          = newBoard,
                remainingDice  = newRemaining,
                legalMoves     = nextMoves,
                pipCountHuman  = pips[0],
                pipCountEngine = pips[1],
                dice           = state.dice,
                moveHistory    = state.moveHistory + snapshot
            )
        }
    }

    fun tapSource(point: Int) {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        if (state.remainingDice.isEmpty()) return
        viewModelScope.launch(engineThread) {
            val humanOnBar = state.board[49]

            if (humanOnBar == 0 && point in 1..24 && state.board[24 + point] == 0) {
                val stacked = tryDestinationStackMove(state, point)
                if (stacked != null) {
                    _gameState.value = stacked
                    return@launch
                }
            }

            val src  = if (humanOnBar > 0 || point == 0) 24 else point - 1

            // Apply one sub-move that gnubg LegalMove accepts from src. Iterate
            // the actual remaining dice in order (so a swap changes which die is
            // tried first); Engine.applySubMove routes through the facade
            // gnubg_legal_sub_move -> LegalMove (eval.c:2732), gnubgs authority
            // for both in-board legality (Chris rule) and bear-off (all home +
            // exact/highest die). No die is derived in Kotlin; the engine accepts
            // or rejects each (src, die). Bear-off needs no special case: its
            // legality is entirely inside LegalMove.
            //
            // The must-use-both / maximization rule is a property of the COMPLETE
            // turn, enforced by confirm() via Engine.findMove (GenerateMoves +
            // cMaxMoves/cMaxPips match). A tap sequence that cannot complete to a
            // maximal legal move is rejected at confirm() and the player undoes --
            // exactly as gnubgs own board UI behaves. It is NOT reimplemented here.
            var newBoard = IntArray(0)
            var usedDie  = -1
            for (d in state.remainingDice.distinct()) {
                val b = Engine.applySubMove(state.board, src, d)
                if (b.isNotEmpty()) { newBoard = b; usedDie = d; break }
            }
            if (newBoard.isEmpty()) return@launch

            val rawRemaining = state.remainingDice.toMutableList().also { it.remove(usedDie) }
            val pips = Engine.pipCount(newBoard)

            val rawNextMoves = if (rawRemaining.isNotEmpty()) {
                val r0 = rawRemaining[0]
                val r1 = if (rawRemaining.size > 1) rawRemaining[1] else r0
                Engine.getLegalMoves(newBoard, r0, r1)
            } else IntArray(0)

            // If GNUbg says no legal continuation exists with the remaining dice,
            // consume/grey the impossible dice. This handles doubles where only
            // 1, 2, or 3 of the 4 dice can legally be played.
            val newRemaining =
                if (rawRemaining.isNotEmpty() && rawNextMoves.isEmpty()) emptyList()
                else rawRemaining

            val nextMoves = if (newRemaining.isNotEmpty()) rawNextMoves else IntArray(0)

            android.util.Log.i(
                "gnubg-vm",
                "tapSource: usedDie=$usedDie rawRemaining=$rawRemaining rawNextMoves=${rawNextMoves.size / 8} newRemaining=$newRemaining"
            )

            val snapshot = MoveSnapshot(
                board = state.board.copyOf(),
                remainingDice = state.remainingDice,
                legalMoves = state.legalMoves.copyOf(),
                pipCountHuman = state.pipCountHuman,
                pipCountEngine = state.pipCountEngine
            )

            _gameState.value = state.copy(
                board          = newBoard,
                remainingDice  = newRemaining,
                legalMoves     = nextMoves,
                pipCountHuman  = pips[0],
                pipCountEngine = pips[1],
                dice           = state.dice,
                moveHistory    = state.moveHistory + snapshot
            )
        }
    }

    fun undo() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return

        val snapshot = state.moveHistory.lastOrNull() ?: return
        _gameState.value = state.copy(
            board          = snapshot.board.copyOf(),
            remainingDice  = snapshot.remainingDice,
            legalMoves     = snapshot.legalMoves.copyOf(),
            pipCountHuman  = snapshot.pipCountHuman,
            pipCountEngine = snapshot.pipCountEngine,
            dice           = state.dice,
            moveHistory    = state.moveHistory.dropLast(1)
        )

        android.util.Log.i(
            "gnubg-vm",
            "undo: restored one submove remainingDice=${snapshot.remainingDice} historyLeft=${state.moveHistory.size - 1}"
        )
    }

    fun swapDice() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        if (state.remainingDice.size < 2) return
        if (state.remainingDice[0] == state.remainingDice[1]) return
        _gameState.value = state.copy(
            remainingDice = state.remainingDice.reversed(),
            dice = state.dice?.let { Pair(it.second, it.first) }
        )
    }

    /* Forfeit the roll when no legal move exists (e.g. close-out on the
     * bar, or own home board fully blocked). The engine seam
     * gnubg_set_suppress_auto_forfeit defers gnubg built-in CommandRoll
     * auto-pass so the UI can show the dice + a Continue button.
     * Engine.applyMoveString("") then makes CommandMove see
     * GenerateMoves == 0, add a no-move record, and call TurnDone. */
    /* Poll the lock-free live-dice channel while the engine thinks, so the
     * player sees WHAT it is thinking about. gnubg rolls early in its turn
     * (play.c DiceRolled -> port hook) and only then searches; desktop gnubg
     * shows the dice at that same moment. baseSeq: snapshot from before the
     * turn -- dice count as fresh only once the seq has advanced, so a stale
     * previous roll can never be mistaken for this one. Runs off the engine
     * thread, which is blocked inside the turn; stops the moment dice arrive
     * or the phase leaves ENGINE_THINKING (e.g. the engine doubled instead). */
    private fun watchEngineDice(baseSeq: Int) {
        viewModelScope.launch(kotlinx.coroutines.Dispatchers.Default) {
            while (_gameState.value.phase == GamePhase.ENGINE_THINKING) {
                val v = Engine.peekLiveDice()
                if (v.size == 3 && v[0] != baseSeq && v[1] in 1..6 && v[2] in 1..6) {
                    // Contract order: this coroutine is on Dispatchers.Default;
                    // it must NEVER write _gameState (the read-copy-write here
                    // once clobbered a just-settled WAITING_FOR_ROLL back to
                    // ENGINE_THINKING -- the frozen-board bug). It owns its own
                    // leaf flow; the public gameState merges it, gated on the
                    // thinking phase, so a late write is displayed nowhere.
                    _liveEngineDice.value = Pair(v[1], v[2])
                    break
                }
                kotlinx.coroutines.delay(100)
            }
            // Keep the roll on the end screen: if the engine's move ended the
            // game, GAME_OVER must still show the dice that ended it. Clear only
            // for other exits (the next game's beginEngineWork resets cleanly).
            val p = _gameState.value.phase
            if (p != GamePhase.ENGINE_THINKING && p != GamePhase.GAME_OVER)
                _liveEngineDice.value = null
        }
    }

    /* The single entry into engine work. Clears the coach verdicts (any new
     * engine work invalidates the verdict on screen -- this ONE rule replaces
     * the per-path clears that kept being forgotten), records what the work
     * is, shows the thinking overlay, and arms the live-dice watcher. Engine
     * thread only. */
    private fun beginEngineWork(kind: BusyKind) {
        // Terminal latch first: after a win, this is a clean no-op -- never
        // clear glances, set busy, or flash a thinking state over the end
        // screen (the coach win path calls this before its settle).
        if (gameOverLatched) return
        _coachGlance.value = null
        _coachCubeGlance.value = null
        _coachCubeAnswer.value = null
        pendingCubeAction = -1
        _busyKind.value = kind
        val diceBase = Engine.peekLiveDice().let { if (it.size == 3) it[0] else 0 }
        _gameState.value = _gameState.value.copy(phase = GamePhase.ENGINE_THINKING, engineDice = null)
        watchEngineDice(diceBase)
    }

    /* The coach HOLD -- the one genuinely app-side phase (a decision judged
     * but deliberately not yet given to gnubg). Engine thread only. */
    private fun holdCoachReview() {
        if (gameOverLatched) return
        _busyKind.value = BusyKind.NONE
        _gameState.value = _gameState.value.copy(phase = GamePhase.COACH_REVIEW)
    }

    fun passTurn() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        if (state.legalMoves.isNotEmpty()) return
        if (!state.board.contentEquals(state.oldBoard)) return
        viewModelScope.launch(engineThread) {
            if (_gameState.value.phase != GamePhase.HUMAN_MOVING) return@launch
            val scoreBefore = Engine.getMatchScore()
            // A no-move turn has nothing to judge, so no hold -- but the same
            // one busy entry applies: it clears any stale verdict and labels
            // the wait. The projection derives whatever gnubg says comes next
            // (including a game the forced pass just lost).
            beginEngineWork(BusyKind.REPLYING)
            Engine.applyMoveString("")
            settleFromEngine(result = gameResultFromScoreDelta(scoreBefore))
        }
    }

    fun confirm() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        viewModelScope.launch(engineThread) {
            val origDice = state.originalDice ?: return@launch
            if (state.board.contentEquals(state.oldBoard)) return@launch
            val moveStr = Engine.findMove(state.oldBoard, state.board, origDice.first, origDice.second)
            android.util.Log.i("gnubg-vm", "confirm: findMove='$moveStr' dice=${origDice.first},${origDice.second} remaining=${state.remainingDice} " +
                "fp(old)=${fpOf(state.oldBoard)} fp(new)=${fpOf(state.board)}")
            if (moveStr.isEmpty()) { android.util.Log.e("gnubg-vm", "confirm: findMove empty"); return@launch }
            if (_gameState.value.phase != GamePhase.HUMAN_MOVING) return@launch
            // Capture the match score before the move. A game-ending move triggers
            // gnubgs NextTurn(TRUE) inside command_move, which scores the game AND
            // auto-starts the next one -- so by the time we read state, ms.gs is
            // back to GAME_PLAYING and getGameResult() reads the NEW games empty
            // MOVE_GAMEINFO (fWinner=-1). The score delta survives the auto-advance
            // (play.c:291 updates anScore synchronously before the new game starts),
            // so it is the reliable game-over signal -- same pattern as commandResign.
            val scoreBefore = Engine.getMatchScore()

            // Coach: the verdict is computed and shown BEFORE gnubg's reply
            // (maintainer order) -- board-based against the live pre-move
            // state, so no record walking. beginEngineWork(JUDGING) is the ONE
            // clear point: it wipes both stale glances so a chequer verdict is
            // never masked by an old cube one, and labels the wait honestly.
            if (coachSession) {
                beginEngineWork(BusyKind.JUDGING)
                val t0 = android.os.SystemClock.elapsedRealtime()
                val cv = Engine.coachVerdictPre(state.oldBoard, origDice.first, origDice.second, state.board)
                val cms = android.os.SystemClock.elapsedRealtime() - t0
                if (cv.size >= 168) {
                    android.util.Log.i("gnubg-coach",
                        "verdict(pre) ${cms}ms rank=${cv[0]} of=${cv[1]} skill=${cv[4]}")
                    _coachGlance.value = cv
                } else {
                    android.util.Log.i("gnubg-coach", "verdict(pre) ${cms}ms: none")
                }
                // HOLD: the move is not yet gnubg's. The board keeps showing
                // the player's position and dice; the game waits for the
                // player's active "GNU's turn".
                pendingCoachMove = moveStr
                holdCoachReview()
                return@launch
            }

            beginEngineWork(BusyKind.REPLYING)
            Engine.applyMoveString(moveStr)

            // Tutor 2-ply analysis (~2s) is DECOUPLED: it runs in
            // analyzeMoveInBackground() after the turn completes, so it never
            // blocks the return to WAITING_FOR_ROLL. The panel keeps showing the
            // PREVIOUS move's verdict until the new one lands (no empty flash).

            // Game-over detection via score delta (survives the NextTurn
            // auto-advance; see scoreBefore comment above). gnubg computed the
            // score, so the delta -- and the winner/points derived from it -- is
            // gnubg-authoritative. getMatchStatus()/getGameResult() are NOT used
            // here because the auto-advance resets ms.gs and the game-info record.
            settleFromEngine(result = gameResultFromScoreDelta(scoreBefore))
            if (!coachSession) analyzeMoveInBackground(state.oldBoard)
        }
    }

    /** Decoupled tutor analysis: gnubg's 2-ply AnalyzeMove of the played move,
     *  run off the turn-completion critical path so the player can roll
     *  immediately. The previous verdict stays visible until this overwrites it
     *  (lastTutorAnalysis is never cleared to null). Never affects gameplay. */
    private fun analyzeMoveInBackground(oldBoard: IntArray) {
        viewModelScope.launch(engineThread) {
            try {
                val raw = Engine.tutorAnalyze(oldBoard)
                if (raw.size == 52) {
                    val playedEquity = Float.fromBits(raw[0])
                    val bestEquity   = Float.fromBits(raw[1])
                    val equityLoss   = (bestEquity - playedEquity).coerceAtLeast(0f)
                    val level        = com.clavierhaus.gnubg.tutor.BlunderClassifier.classify(equityLoss)
                    android.util.Log.i("gnubg-tutor",
                        "level=$level loss=${"%.4f".format(equityLoss)} " +
                        "best=${"%.4f".format(bestEquity)} played=${"%.4f".format(playedEquity)}")
                    lastTutorAnalysis = TutorAnalysis(
                        level = level,
                        equityLoss = equityLoss,
                        bestEquity = bestEquity,
                        playedEquity = playedEquity
                    )
                    // Analysis-mode detail: gnubg's probability breakdown for the
                    // played move (Win/Wg/Wbg/Lg/Lbg + equities). Same replay, so
                    // fetch alongside the verdict.
                    val det = Engine.analyzePlayedMove(oldBoard)
                    lastAnalysisDetail = if (det.size == 7) {
                        MoveAnalysisDetail(
                            win = det[0], winGammon = det[1], winBackgammon = det[2],
                            loseGammon = det[3], loseBackgammon = det[4],
                            equityCubeful = det[5], equityCubeless = det[6]
                        )
                    } else null
                    android.util.Log.i("gnubg-analysis",
                        if (det.size == 7)
                            "win=${"%.4f".format(det[0])} wg=${"%.4f".format(det[1])} " +
                            "lg=${"%.4f".format(det[3])} eqCubeful=${"%.4f".format(det[5])} " +
                            "eqCubeless=${"%.4f".format(det[6])}"
                        else "no detail (size=${det.size})")
                    // Refresh the panel with the new verdict + detail. Unguarded,
                    // single targeted update -- no race (we never cleared to null).
                    _gameState.value = _gameState.value.copy(
                        tutorAnalysis = lastTutorAnalysis,
                        analysisDetail = lastAnalysisDetail
                    )
                } else {
                    android.util.Log.i("gnubg-tutor", "no analysis (raw.size=${raw.size})")
                }
            } catch (t: Throwable) {
                android.util.Log.e("gnubg-tutor", "analysis failed: ${t.message}")
            }
        }
    }

    fun offerDouble() {
        val state = _gameState.value
        if (state.phase != GamePhase.WAITING_FOR_ROLL) {
            android.util.Log.i("gnubg-vm", "offerDouble: ignored phase=${state.phase}")
            return
        }
        // Coach: doubling is a cube decision -- judge and hold before offering.
        if (coachSession) {
            viewModelScope.launch(engineThread) {
                val cd = Engine.canDouble()
                android.util.Log.i("gnubg-cube",
                    "coach offerDouble: canDouble=$cd cubeInfo=${Engine.getMatchCubeInfo().joinToString(",")}")
                if (cd) judgeAndHoldCube(Engine.getMatchBoard(), 1)
                else android.util.Log.i("gnubg-cube", "coach offerDouble: gnubg refuses the double here")
            }
            return
        }

        viewModelScope.launch(engineThread) { offerDoubleNow() }
    }

    /** The double itself, on the engine thread. Called by offerDouble (Play,
     *  or Coach carrying out a held decision via continueCoachCube) -- the
     *  continuation calls it DIRECTLY rather than faking a phase to satisfy
     *  the public guard (contract order: no handler patches the phase). */
    private suspend fun offerDoubleNow() {
            try {
                val scoreBefore = Engine.getMatchScore()
                beginEngineWork(BusyKind.REPLYING)
                val dbg = Engine.getCubeDebugState()
                android.util.Log.i(
                    "gnubg-vm",
                    "offerDouble: dbg gs=${dbg[0]} turn=${dbg[1]} move=${dbg[2]} " +
                        "dice=${dbg[3]},${dbg[4]} doubled=${dbg[5]} owner=${dbg[6]} cube=${dbg[7]} " +
                        "crawford=${dbg[8]} cubeUse=${dbg[9]} score=${dbg[10]}-${dbg[11]} match=${dbg[12]}"
                )

                // PORT (audit V5): legality check is now Engine.canDouble(),
                // which routes through facade verb gnubg_mobile_can_double() to
                // engine seam gnubg_can_double() in engine-core/play.c. The seam
                // mirrors CommandDouble's preconditions (play.c:2369) exactly
                // (minus the desktop-only move_not_last_in_match_ok prompt), so
                // the Kotlin side is no longer reimplementing cube legality.
                if (!Engine.canDouble()) {
                    android.util.Log.i("gnubg-vm", "offerDouble: rejected by engine canDouble()")
                    settleFromEngine()
                    return
                }

                val matchBoard = Engine.getMatchBoard()
                val cd = Engine.cubeDecision(matchBoard)

                // Cube-debug logging retained: shows gnubg's cube evaluation even
                // though the port no longer ACTS on it (gnubg decides take/drop
                // itself, below). Purely diagnostic.
                if (cd != null && cd.size >= 18) {
                    val win   = Float.fromBits(cd[0])
                    val wg    = Float.fromBits(cd[1])
                    val wbg   = Float.fromBits(cd[2])
                    val lg    = Float.fromBits(cd[3])
                    val lbg   = Float.fromBits(cd[4])
                    val eqLess = Float.fromBits(cd[5])
                    val eqFul  = Float.fromBits(cd[6])
                    val optEq  = Float.fromBits(cd[14])
                    val noDbl  = Float.fromBits(cd[15])
                    val take   = Float.fromBits(cd[16])
                    val drop   = Float.fromBits(cd[17])
                    val winT   = Float.fromBits(cd[7])
                    val eqFulT = Float.fromBits(cd[13])
                    android.util.Log.i("gnubg-cube",
                        "probs[0]: win=%.4f wg=%.4f wbg=%.4f lg=%.4f lbg=%.4f eq_cubeless=%.4f eq_cubeful=%.4f"
                            .format(win, wg, wbg, lg, lbg, eqLess, eqFul))
                    android.util.Log.i("gnubg-cube",
                        "probs[1]: win=%.4f eq_cubeful=%.4f (take branch)"
                            .format(winT, eqFulT))
                    android.util.Log.i("gnubg-cube",
                        "arDouble: optimal=%.4f nodouble=%.4f take=%.4f drop=%.4f | nodbl-take=%+.4f take-drop=%+.4f"
                            .format(optEq, noDbl, take, drop, noDbl - take, take - drop))
                }

                // Offer the cube and let gnubg's own ComputerTurn decide the
                // engine's response. commandDouble() drains the turn loop, so the
                // take/drop/beaver decision is gnubg's -- the port no longer
                // computes it in Kotlin or forces CommandTake/CommandDrop. gnubg
                // natively collapses an inapplicable beaver to take (play.c), so
                // no beaver handling is needed here.
                Engine.commandDouble()

                // Read what gnubg decided: a winner means it dropped (game over);
                // no winner means it took and play continues. (getGameResult
                // returns result[0] = -1 while the game is still in progress.)
                val gr = Engine.getGameResult()
                android.util.Log.i("gnubg-vm",
                    if (gr[0] >= 0) "offerDouble: engine dropped; result=${gr.joinToString(",")}"
                    else "offerDouble: engine took; play continues")
                // Score delta is the uniform gnubg-authoritative end signal
                // (survives NextTurn auto-advance); a drop changes the score.
                val res = gameResultFromScoreDelta(scoreBefore)
                // Coach: GNU's answer to the player's double is part of the
                // lesson -- surface it (take: visible until the next committed
                // action; drop: through the GAME_OVER acknowledgment).
                if (coachSession) {
                    _coachCubeAnswer.value =
                        if (res == null) Pair(true, 0) else Pair(false, res.second)
                }
                settleFromEngine(result = res)
            } catch (t: Throwable) {
                android.util.Log.e("gnubg-vm", "offerDouble: failed", t)
                settleFromEngine()
            } finally {
                actionInProgress.set(false)
            }
    }

    fun acceptDouble() {
        // Coach: taking is a cube decision -- judge and hold; the take itself
        // happens in takeNow() when the player continues.
        if (coachSession) {
            viewModelScope.launch(engineThread) { judgeAndHoldCube(Engine.getMatchBoard(), 2) }
            return
        }
        viewModelScope.launch(engineThread) { takeNow() }
    }

    /** The take itself, on the engine thread. commandTake hands the turn
     *  straight to the engine, which rolls and thinks inside it -- the busy
     *  overlay + dice watcher show the wait; the projection derives whatever
     *  gnubg says follows (including a game GNU's ensuing move ended). */
    private suspend fun takeNow() {
        val scoreBefore = Engine.getMatchScore()
        beginEngineWork(BusyKind.REPLYING)
        Engine.commandTake()
        settleFromEngine(result = gameResultFromScoreDelta(scoreBefore))
    }

    fun dropDouble() {
        if (coachSession) {
            viewModelScope.launch(engineThread) { judgeAndHoldCube(Engine.getMatchBoard(), 3) }
            return
        }
        viewModelScope.launch(engineThread) { dropNow() }
    }

    private suspend fun dropNow() {
        val scoreBefore = Engine.getMatchScore()
        Engine.commandDrop()
        settleFromEngine(result = gameResultFromScoreDelta(scoreBefore))
    }


    /* THE settle (contract order: one projection, every path). After ANY
     * engine operation, this derives the phase FROM gnubg -- game over,
     * resignation, cube, a rolled human turn, or waiting -- and projects it via
     * readMatchState, the sole writer. `result` carries the one thing gnubg
     * cannot answer after the fact: a game that ended by score delta, because
     * NextTurn(TRUE) auto-starts the next game and resets ms.gs/game-info
     * before we can read them (play.c:291; the confirm() comment documents
     * it). No caller picks a phase; gnubg IS the phase. */
    /* Game-over by score delta: gnubg updates anScore synchronously before
     * NextTurn(TRUE) auto-starts the next game (play.c:291), so the delta is
     * the gnubg-authoritative end-of-game signal that survives the advance.
     * Returns (winner, points) or null if no game ended. */
    private fun gameResultFromScoreDelta(scoreBefore: IntArray): Pair<Int, Int>? {
        val after = Engine.getMatchScore()
        val humanDelta  = after[0] - scoreBefore[0]
        val engineDelta = after[1] - scoreBefore[1]
        if (humanDelta == 0 && engineDelta == 0) return null
        val winner = if (humanDelta > engineDelta) 0 else 1
        val points = kotlin.math.abs(humanDelta - engineDelta).coerceAtLeast(1)
        return Pair(winner, points)
    }

    private fun settleFromEngine(result: Pair<Int, Int>? = null) {
        _busyKind.value = BusyKind.NONE
        if (result != null) {
            android.util.Log.i("gnubg-vm", "settle: GAME_OVER winner=${result.first} pts=${result.second}")
            readMatchState(phase = GamePhase.GAME_OVER, winner = result.first, nPoints = result.second)
            return
        }
        val status = Engine.getMatchStatus()
        if (status >= 2) {
            Engine.getGameResult().let { gr ->
                readMatchState(
                    phase = GamePhase.GAME_OVER,
                    winner = gr[0],
                    nPoints = gr[1]
                )
            }
            return
        }

        val cubeInfo = Engine.getMatchCubeInfo()
        val dice = Engine.getMatchDice()
        val turn = Engine.getMatchTurn()

        val phase = when {
            // gnubg refuses CommandRoll while a resignation is on the table
            // ("Please resolve the resignation first", play.c:4048), so nothing
            // else can be offered until this is answered.
            Engine.getResignation() > 0 && turn == 0 -> GamePhase.RESIGNATION_OFFERED
            cubeInfo[0] == 1 && turn == 0 -> GamePhase.CUBE_OFFERED
            dice[0] > 0 && turn == 0 -> GamePhase.HUMAN_MOVING
            else -> GamePhase.WAITING_FOR_ROLL
        }

        // HUMAN_MOVING needs the move-entry state (dice, legal moves, the board
        // they were generated from), exactly as startNewGame builds it. A bare
        // readMatchState(phase) left the player in HUMAN_MOVING with NO dice --
        // Undo/Commit and nothing to play. Field report from the Coach opening;
        // the same hole was latent under Play's "New match" whenever the human
        // won the opening roll, since commandNewMatch also lands here.
        android.util.Log.i("gnubg-vm",
            "settle: phase=$phase turn=$turn dice=${dice[0]},${dice[1]} " +
                "cube=${cubeInfo[0]},${cubeInfo[1]},${cubeInfo[2]}")
        if (phase == GamePhase.HUMAN_MOVING) {
            val d0 = dice[0]; val d1 = dice[1]
            val board    = Engine.getMatchBoard()
            val allMoves = Engine.getLegalMoves(board, d0, d1)
            android.util.Log.i("gnubg-vm", "settle: human turn d0=$d0 d1=$d1 moves=${allMoves.size / 8}")
            readMatchState(
                phase         = GamePhase.HUMAN_MOVING,
                remainingDice = if (d0 == d1) listOf(d0, d0, d0, d0) else listOf(d0, d1),
                legalMoves    = allMoves,
                oldBoard      = board,
                originalDice  = Pair(d0, d1)
            )
        } else if (phase == GamePhase.WAITING_FOR_ROLL && turn == 0) {
            // Show what the engine rolled for its completed move (same as
            // startNewGame's engine-opening branch).
            val ed = Engine.getMoveRecordDice()
            readMatchState(phase = phase,
                engineDice = if (ed[0] > 0) Pair(ed[0], ed[1]) else null)
        } else {
            readMatchState(phase = phase)
        }
    }

    fun commandNewGame() {
        gameOverLatched = false
        viewModelScope.launch(engineThread) {
            Engine.commandNewGame()
            settleFromEngine()
        }
    }

    /** Leave the current match to the hub AND re-open the setup screen, so the
     *  next entry to Play offers strength and length again -- rather than
     *  dropping back into the abandoned match. This is what makes "Home" mean
     *  "end this match", consistent with the first-launch flow. gnubg keeps the
     *  match state until a new match/game command replaces it; nothing is
     *  corrupted by leaving. */
    fun leaveMatch() {
        _showMatchSetup.value = true
    }

    /** Coach mode (docs/COACH.md): start the contained single game. A 1-point
     *  game against gnubg's Expert preset (0-ply, no noise -- instant replies
     *  keep the coaching rhythm tight), started with DIRECT engine calls so the
     *  player's saved Play settings (strength, match length) are never touched.
     *  In a 1-point match the cube is dead by the rules, so no cube UI can
     *  arise -- V1's "no cube" falls out of match play itself. */
    /** The player's active continuation (maintainer design: dice rolling is
     *  actively initiated for both sides in this mode). Delivers the held move
     *  to gnubg; GNU then rolls -- live dice via the watcher -- and replies.
     *  Tail mirrors confirm()'s: gnubg-authoritative game-over via score delta,
     *  then back to the player. */
    /* Judge a human cube ACTION before carrying it out, and HOLD -- the cube
     * mirror of confirm()'s chequer branch. board = the pre-decision match
     * board; action = what the human chose (0 no-double,1 doubled,2 took,
     * 3 dropped). Publishes the 10-int gnubg verdict and parks in COACH_REVIEW;
     * continueCoachCube() then performs the action. Runs on engineThread. */
    private suspend fun judgeAndHoldCube(board: IntArray, action: Int) {
        val t0 = android.os.SystemClock.elapsedRealtime()
        val v = Engine.coachCubeVerdict(board, action)
        val ms = android.os.SystemClock.elapsedRealtime() - t0
        if (v.size >= 10) {
            android.util.Log.i("gnubg-coach",
                "cube verdict ${ms}ms action=$action isBest=${v[0]} cd=${v[1]} skill=${v[4]}")
            _coachCubeGlance.value = v
        } else {
            android.util.Log.i("gnubg-coach", "cube verdict ${ms}ms: none")
            _coachCubeGlance.value = null
        }
        pendingCubeAction = action
        holdCoachReview()
    }

    /* The player's active continuation for a cube decision (mirrors
     * continueCoachTurn). Carries out the held action; GNU then responds. */
    fun continueCoachCube() {
        viewModelScope.launch(engineThread) {
            val action = pendingCubeAction
            if (action < 0 || _gameState.value.phase != GamePhase.COACH_REVIEW) return@launch
            pendingCubeAction = -1
            _coachCubeGlance.value = null
            // Carry out the held decision via the SAME engine-thread bodies the
            // non-coach path runs -- called DIRECTLY, so no phase is faked to
            // slip past a public guard and no re-judging flag is needed (the
            // diversions live only in the public functions). Contract order:
            // handlers never patch the phase; the projection derives it.
            when (action) {
                0 -> doRollNow()
                1 -> offerDoubleNow()
                2 -> takeNow()
                3 -> dropNow()
            }
        }
    }

    fun continueCoachTurn() {
        viewModelScope.launch(engineThread) {
            val mv = pendingCoachMove ?: return@launch
            if (_gameState.value.phase != GamePhase.COACH_REVIEW) return@launch
            pendingCoachMove = null
            val scoreBefore = Engine.getMatchScore()
            // beginEngineWork clears the studied verdict (it belonged to the
            // held move, now being committed -- one rule, every path) and
            // labels the wait as GNU's reply; settleFromEngine derives
            // whatever gnubg says comes next. No phase is picked here.
            beginEngineWork(BusyKind.REPLYING)
            android.util.Log.i("gnubg-coach", "continue: delivering '$mv'")
            Engine.applyMoveString(mv)
            settleFromEngine(result = gameResultFromScoreDelta(scoreBefore))
        }
    }

    /** Coach Undo (Plus UI, reached only from the Plus coach rail): discard
     *  the HELD move and hand the turn back to the player, same roll.
     *
     *  No engine rewind exists or is needed: the coach hold PRECEDES
     *  applyMoveString (see the confirm path -- "the move is not yet
     *  gnubg's"), so gnubg still sits at the pre-move position with the dice
     *  rolled. Clearing the hold and re-settling from the engine re-derives
     *  HUMAN_MOVING with the same dice and a fresh move entry -- the
     *  projection restores the pre-move board because that IS gnubg's board.
     *  The discarded verdict is cleared with the move it judged; nothing is
     *  faked and no phase is picked here (contract order: the projection
     *  derives it).
     *
     *  Deliberately chequer-only: a held CUBE decision (pendingCubeAction)
     *  is not a move and is not undone by this; the guard on
     *  pendingCoachMove excludes it. */
    fun undoCoachMove() {
        viewModelScope.launch(engineThread) {
            if (_gameState.value.phase != GamePhase.COACH_REVIEW) return@launch
            if (pendingCoachMove == null) return@launch
            android.util.Log.i("gnubg-coach", "undo: discarding held '$pendingCoachMove'")
            pendingCoachMove = null
            _coachGlance.value = null
            settleFromEngine()
        }
    }

    fun startCoachGame(
        length: Int = _coachLength.value,
        difficulty: Difficulty = _coachDifficulty.value
    ) {
        coachSession = true
        _coachLength.value = length
        _coachDifficulty.value = difficulty
        _showCoachSetup.value = false
        _coachGlance.value = null
        pendingCoachMove = null
        _coachCubeGlance.value = null
        _coachCubeAnswer.value = null
        pendingCubeAction = -1
        _busyKind.value = BusyKind.NONE
        _liveEngineDice.value = null
        gameOverLatched = false
        viewModelScope.launch(engineThread) {
            Engine.setEngineStrength(difficulty.settingIndex)
            Engine.commandNewMatch(length)
            Engine.commandNewGame()
            settleFromEngine()
        }
    }

    /** Leaving Coach: restore the player's saved strength (Coach forced
     *  Expert), and re-open Play's setup so a later "Play" never resumes the
     *  coach game as if it were a tournament match. */
    fun endCoachSession() {
        coachSession = false
        _coachGlance.value = null
        pendingCoachMove = null
        _coachCubeGlance.value = null
        _coachCubeAnswer.value = null
        pendingCubeAction = -1
        _busyKind.value = BusyKind.NONE
        _liveEngineDice.value = null
        _showMatchSetup.value = true
        // Next Coach entry starts at its setup screen, not straight into a game.
        _showCoachSetup.value = true
        viewModelScope.launch(engineThread) {
            Engine.setEngineStrength(_settings.value.difficulty.settingIndex)
        }
    }

    fun commandNewMatch(length: Int = _settings.value.matchLength) {
        _settings.value = _settings.value.copy(matchLength = length)
        _showMatchSetup.value = false
        gameOverLatched = false
        viewModelScope.launch(engineThread) {
            Engine.commandNewMatch(length)
            Engine.commandNewGame()
            settleFromEngine()
        }
    }

    fun commandNewSession(games: Int = 0) {
        gameOverLatched = false
        viewModelScope.launch(engineThread) {
            Engine.commandNewSession(games)
            settleFromEngine()
        }
    }

    fun commandEndGame() {
        viewModelScope.launch(engineThread) {
            Engine.commandEndGame()
            settleFromEngine()
        }
    }

    fun commandResign(value: String) {
        viewModelScope.launch(engineThread) {
            val beforeScore = Engine.getMatchScore()
            Engine.commandResign(value)
            val afterScore = Engine.getMatchScore()

            val humanDelta = afterScore[0] - beforeScore[0]
            val engineDelta = afterScore[1] - beforeScore[1]

            if (humanDelta != 0 || engineDelta != 0) {
                val winner = if (humanDelta > engineDelta) 0 else 1
                val points = kotlin.math.abs(humanDelta - engineDelta)
                    .coerceAtLeast(1)
                settleFromEngine(result = Pair(winner, points))
            } else {
                settleFromEngine()
            }
        }
    }

    fun commandNext(argument: String = "") {
        viewModelScope.launch(engineThread) {
            Engine.commandNext(argument)
            settleFromEngine()
        }
    }

    fun commandAccept() {
        viewModelScope.launch(engineThread) {
            Engine.commandAccept()
            settleFromEngine()
        }
    }

    fun commandReject() {
        viewModelScope.launch(engineThread) {
            Engine.commandReject()
            settleFromEngine()
        }
    }

    // commandAgree() / commandDecline() wrappers were removed here. They had no
    // caller, and they routed through settleFromEngine(), which has
    // no GAME_OVER branch -- so accepting a resignation through them would have
    // left the phase at WAITING_FOR_ROLL after gnubg ended the game. Use
    // acceptResignation() / declineResignation(), which read gnubg's match status.

    fun commandRedouble() {
        viewModelScope.launch(engineThread) {
            Engine.commandRedouble()
            settleFromEngine()
        }
    }

    fun loadGame(path: String) {
        viewModelScope.launch(engineThread) {
            Engine.loadGame(path)
            settleFromEngine()
        }
    }

    fun saveGame(path: String) {
        viewModelScope.launch(engineThread) {
            Engine.saveGame(path)
        }
    }

    fun loadMatch(path: String) {
        viewModelScope.launch(engineThread) {
            Engine.loadMatch(path)
            settleFromEngine()
        }
    }

    fun saveMatch(path: String) {
        viewModelScope.launch(engineThread) {
            Engine.saveMatch(path)
        }
    }

    fun loadPosition(path: String) {
        viewModelScope.launch(engineThread) {
            Engine.loadPosition(path)
            settleFromEngine()
        }
    }

    fun savePosition(path: String) {
        viewModelScope.launch(engineThread) {
            Engine.savePosition(path)
        }
    }


    private fun runSettingsCommand(command: String) {
        viewModelScope.launch(engineThread) {
            val ok = Engine.runCommand(command)
            android.util.Log.i("gnubg-vm", "settings command '$command' ok=$ok")
        }
    }

    private fun onOff(on: Boolean): String = if (on) "on" else "off"

    fun setMatchLength(n: Int) {
        // Local Android match setup value.
        // Applied by startMatch()/Engine.newGame(), not through the generic GNUbg command bridge.
        _settings.value = _settings.value.copy(matchLength = n)
    }
    fun setCrawford(on: Boolean) {
        // Crawford is auto-managed by gnubg per game from the match score
        // (play.c:864). The user rule is fAutoCrawford, toggled via the SAFE
        // CommandSetAutoCrawford -- never CommandSetCrawford (the in-match
        // per-game command that asserts a player is 1-away and crashed when
        // called from Settings). Set on the engine thread; rule state only.
        _settings.value = _settings.value.copy(crawford = on)
        viewModelScope.launch(engineThread) { Engine.setAutoCrawford(on) }
    }
    fun setJacoby(on: Boolean) {
        _settings.value = _settings.value.copy(jacoby = on)
        viewModelScope.launch(engineThread) { Engine.setJacoby(on) }
    }
    fun setCubeUse(on: Boolean) {
        // gnubg CommandSetCubeUse -- safe global toggle (fCubeUse), same class
        // as Jacoby. Applies immediately at the engine level.
        _settings.value = _settings.value.copy(cubeUse = on)
        viewModelScope.launch(engineThread) { Engine.setCubeUse(on) }
    }
    fun setMet(t: MatchEquityTable) {
        // Load the chosen match equity table. InitMatchEquity + eval-cache flush
        // (safe: touches only MET tables, not live match state).
        _settings.value = _settings.value.copy(metTable = t)
        viewModelScope.launch(engineThread) {
            val metDir = AssetExtractor.extractMets(getApplication())
            Engine.setMet("$metDir/${t.fileName}")
        }
    }

    fun setAutomaticDoubles(n: Int) {
        _settings.value = _settings.value.copy(automaticDoubles = n)
        viewModelScope.launch(engineThread) { Engine.setAutoDoubles(n) }
    }
    fun setBeavers(on: Boolean) {
        // gnubg stores a beaver COUNT (nBeavers). The UI toggle maps on->3
        // (gnubgs standard allowance) / off->0, mirroring set.c:4690.
        _settings.value = _settings.value.copy(beavers = on)
        viewModelScope.launch(engineThread) { Engine.setBeavers(if (on) 3 else 0) }
    }
    fun setBoardTheme(t: BoardTheme)    {
        _settings.value = _settings.value.copy(boardTheme = t)
    }
    fun setShowPointNumbers(on: Boolean) { _settings.value = _settings.value.copy(showPointNumbers = on) }
    fun setShowPipCount(on: Boolean)     { _settings.value = _settings.value.copy(showPipCount = on) }
    fun setDifficulty(d: Difficulty) {
        _settings.value = _settings.value.copy(difficulty = d)
        // Apply the gnubg preset to the engine player's chequer eval context.
        // Setting the eval setup is safe at any time -- it only affects the
        // NEXT engine move evaluation, not in-flight match state.
        viewModelScope.launch(engineThread) {
            Engine.setEngineStrength(d.settingIndex)
        }
    }
    fun setTutorMode(on: Boolean) {
        // The chequer-play tutor is a single game, and that is a game fact, not
        // an app convention: at a 1-point match gnubg_can_double (play.c:156)
        // already refuses -- nCube >= nMatchTo - score -- because you cannot
        // double past the match. So the tutored game has no cube decisions to
        // comment on, which is exactly the intent (cube tutoring needs a
        // different interaction and does not exist yet).
        //
        // Pin the length in the same copy() so the state can never be tutor-on
        // with a 7-point length. The setup screen shows the pinned value and
        // says why, rather than the old behaviour of silently substituting 1
        // at startMatch time.
        _settings.value =
            if (on) _settings.value.copy(tutorMode = true, matchLength = 1)
            else _settings.value.copy(tutorMode = false)
    }
    fun setHint(on: Boolean) {
        // Local-only until GNUbg Settings command timing is made lifecycle-safe.
        _settings.value = _settings.value.copy(hint = on)
    }
    fun setShowEquity(on: Boolean) {
        // Local-only until GNUbg Settings command timing is made lifecycle-safe.
        _settings.value = _settings.value.copy(showEquity = on)
    }
    fun setShowMWC(on: Boolean) {
        // Local-only until GNUbg Settings command timing is made lifecycle-safe.
        _settings.value = _settings.value.copy(showMWC = on)
    }
    fun setThresholdDoubtful(v: Float) {
        // Local-only until GNUbg Settings command timing is made lifecycle-safe.
        _settings.value = _settings.value.copy(thresholdDoubtful = v)
    }
    fun setThresholdBad(v: Float) {
        // Local-only until GNUbg Settings command timing is made lifecycle-safe.
        _settings.value = _settings.value.copy(thresholdBad = v)
    }
    fun setThresholdVeryBad(v: Float) {
        // Local-only until GNUbg Settings command timing is made lifecycle-safe.
        _settings.value = _settings.value.copy(thresholdVeryBad = v)
    }
}
