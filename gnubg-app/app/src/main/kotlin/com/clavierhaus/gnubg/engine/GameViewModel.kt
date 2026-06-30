package com.clavierhaus.gnubg.engine

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.clavierhaus.gnubg.Engine
import com.clavierhaus.gnubg.tutor.BEAVER_DECISIONS
import com.clavierhaus.gnubg.tutor.CubeAction
import com.clavierhaus.gnubg.tutor.CubeDecision
import com.clavierhaus.gnubg.tutor.cubeDecisionAction
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import java.util.concurrent.Executors

class GameViewModel(application: Application) : AndroidViewModel(application) {

    private val _settings = MutableStateFlow(GameSettings())
    val settings: StateFlow<GameSettings> = _settings.asStateFlow()

    private val _gameState = MutableStateFlow(BoardState())
    val gameState: StateFlow<BoardState> = _gameState.asStateFlow()

    private val _engineReady = MutableStateFlow(false)
    val engineReady: StateFlow<Boolean> = _engineReady.asStateFlow()

    private val _showMatchSetup = MutableStateFlow(true)
    val showMatchSetup: StateFlow<Boolean> = _showMatchSetup.asStateFlow()

    private val engineThread = Executors.newSingleThreadExecutor { r ->
        Thread(r, "gnubg-engine-thread")
    }.asCoroutineDispatcher()
    private val actionInProgress = java.util.concurrent.atomic.AtomicBoolean(false)

    init {
        viewModelScope.launch {
            PreferencesManager.boardThemeFlow(application).collect { theme ->
                _settings.value = _settings.value.copy(boardTheme = theme)
            }
        }
        viewModelScope.launch(engineThread) {
            val weightsPath = AssetExtractor.extractWeights(application)
            Engine.initialise(weightsPath)
            // Apply the default engine strength (gnubg preset) before any game,
            // so the first move uses it rather than the hardcoded 2-ply setup.
            Engine.setEngineStrength(_settings.value.difficulty.settingIndex)
            // Push saved tournament rule preferences into the engine globals so
            // the first match honors them. These are safe global toggles
            // (set.c); the engine decides applicability per game/match.
            val s = _settings.value
            Engine.setAutoCrawford(s.crawford)
            Engine.setJacoby(s.jacoby)
            Engine.setAutoDoubles(s.automaticDoubles)
            Engine.setBeavers(if (s.beavers) 3 else 0)
            _engineReady.value = true
        }
    }

    fun startMatch(length: Int) {
        _settings.value = _settings.value.copy(matchLength = length)
        _showMatchSetup.value = false
        viewModelScope.launch(engineThread) {
            startNewGame(isNewMatch = true)
        }
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
        blockedDice: Set<Int> = emptySet(),
        moveHistory: List<MoveSnapshot> = emptyList()
    ) {
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
        val rawBoard  = Engine.getMatchBoard()
        // Swap when the engine is the current MOVER (fMove=1), not just when fTurn=1.
        // They diverge during an engine double: the engine doubles (fMove stays 1)
        // but fTurn flips to 0 so the human can take/drop.  Without the extra guard
        // the raw board (engine's coordinate frame) is used as-is, putting engine
        // checkers in board[25..49] and human checkers in board[0..24] -- the tray
        // then reads the engine's borne-off count as the human's.
        val board     = if (turn == 1 || (fDoubled && turn == 0)) Engine.swapBoard(rawBoard) else rawBoard
        val pips      = Engine.pipCount(board)
        val dicePair: Pair<Int, Int>? = originalDice ?: when {
            remainingDice.size >= 2 -> Pair(remainingDice[0], remainingDice[1])
            remainingDice.size == 1 -> Pair(remainingDice[0], remainingDice[0])
            else -> null
        }
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
            blockedDice    = blockedDice,
            humanScore     = score[0],
            engineScore    = score[1],
            cubeValue      = cubeValue,
            cubeOwner      = cubeOwner,
            fDoubled       = fDoubled,
            canDouble      = canDouble
        )
    }

    private fun blockedDiceFor(moves: IntArray, d0: Int, d1: Int): Set<Int> {
        if (d0 == d1) return emptySet()
        val blocked = mutableSetOf<Int>()
        val s0 = (0 until moves.size step 8).any { m ->
            (0 until 8 step 2).any { j -> moves[m+j] >= 0 && moves[m+j] - moves[m+j+1] == d0 } }
        val s1 = (0 until moves.size step 8).any { m ->
            (0 until 8 step 2).any { j -> moves[m+j] >= 0 && moves[m+j] - moves[m+j+1] == d1 } }
        return blocked
    }

    private fun startNewGame(isNewMatch: Boolean = true) {
        if (isNewMatch) Engine.newGame(_settings.value.matchLength) else Engine.nextGame()
        val turn = Engine.getMatchTurn()
        val dice = Engine.getMatchDice()
        val d0 = dice[0]; val d1 = dice[1]
        if (turn == 0 && d0 > 0) {
            val board    = Engine.getMatchBoard()
            val allMoves = Engine.getLegalMoves(board, d0, d1)
            readMatchState(
                phase         = GamePhase.HUMAN_MOVING,
                remainingDice = if (d0 == d1) listOf(d0,d0,d0,d0) else listOf(d0,d1),
                legalMoves    = allMoves,
                oldBoard      = board,
                originalDice  = Pair(d0, d1),
                blockedDice   = blockedDiceFor(allMoves, d0, d1)
            )
        } else {
            val ed = Engine.getMoveRecordDice()
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL,
                engineDice = if (ed[0] > 0) Pair(ed[0], ed[1]) else null)
        }
    }

    fun newGame() {
        viewModelScope.launch(engineThread) {
            val score = Engine.getMatchScore()
            val matchLength = Engine.getMatchLength()
            if (score[0] >= matchLength || score[1] >= matchLength || matchLength <= 1) {
                _showMatchSetup.value = true
            } else {
                startNewGame(isNewMatch = false)
            }
        }
    }

    fun rollDice() {
        if (_gameState.value.phase != GamePhase.WAITING_FOR_ROLL) return
        viewModelScope.launch(engineThread) {
            Engine.rollDice()
            val cubeAfter = Engine.getMatchCubeInfo()
            if (cubeAfter[0] == 1 && Engine.getMatchTurn() == 0) {
                readMatchState(phase = GamePhase.CUBE_OFFERED)
                return@launch
            }
            val turn = Engine.getMatchTurn()
            val dice = Engine.getMatchDice()
            val d0 = dice[0]; val d1 = dice[1]
            android.util.Log.i("gnubg-vm", "rollDice: turn=$turn d0=$d0 d1=$d1 gs=${Engine.getMatchStatus()}")
            if (turn == 0 && d0 > 0) {
                val board    = Engine.getMatchBoard()
                val remaining = if (d0 == d1) listOf(d0,d0,d0,d0) else listOf(d0,d1)
                val allMoves = Engine.getLegalMoves(board, d0, d1)
                android.util.Log.i("gnubg-vm", "blocked check d0=$d0 d1=$d1 moves=${allMoves.size/8}")
                readMatchState(
                    phase         = GamePhase.HUMAN_MOVING,
                    remainingDice = remaining,
                    legalMoves    = allMoves,
                    oldBoard      = board,
                    originalDice  = Pair(d0, d1),
                    engineDice    = null,
                    blockedDice   = blockedDiceFor(allMoves, d0, d1)
                )
            } else {
                if (Engine.getMatchStatus() >= 2)
                    Engine.getGameResult().let { gr -> readMatchState(phase = GamePhase.GAME_OVER, winner = gr[0], nPoints = gr[1]) }
                else {
                    val ed = Engine.getMoveRecordDice()
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL,
                        engineDice = if (ed[0] > 0) Pair(ed[0], ed[1]) else null)
                }
            }
        }
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

        val newBlocked = if (newRemaining.size >= 2)
            blockedDiceFor(nextMoves, newRemaining[0], newRemaining[1])
        else emptySet()

        val snapshot = MoveSnapshot(
            board = state.board.copyOf(),
            remainingDice = state.remainingDice,
            legalMoves = state.legalMoves.copyOf(),
            blockedDice = state.blockedDice,
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
            blockedDice    = newBlocked,
            pipCountHuman  = pips[0],
            pipCountEngine = pips[1],
            dice           = state.dice,
            moveHistory    = state.moveHistory + snapshot
        )
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

            // Collect the dice gnubg itself authored for this src. Every legal
            // move (state.legalMoves = GenerateMoves, fPartial=0) encodes its
            // sub-moves as [src,dest, src,dest, ...] pairs across slots 0..3.
            // A sub-move from src may sit in ANY slot (gnubg stores one
            // dedup ordering per final position), so scan all four. The die
            // is src-dest, taken from gnubgs own encoding -- never guessed.
            val nLegal = state.legalMoves.size / 8
            val candidateDice = LinkedHashSet<Int>()
            for (i in 0 until nLegal) {
                for (j in 0..3) {
                    val sSlot = state.legalMoves[i * 8 + j * 2]
                    val dSlot = state.legalMoves[i * 8 + j * 2 + 1]
                    if (sSlot == src && dSlot >= 0) candidateDice.add(sSlot - dSlot)
                }
            }
            if (candidateDice.isEmpty()) return@launch

            // Try dice in remainingDice order so a dice swap actually changes
            // which die plays first. Only dice gnubg authored for src
            // (candidateDice) are eligible; the engine then has final say via
            // applySubMove (facade LegalMove gate, engine-core/eval.c seam).
            // distinct() collapses the doubles list [d,d,d,d] to one trial.
            var newBoard = IntArray(0)
            var usedDie  = -1
            for (d in state.remainingDice.distinct()) {
                if (d !in candidateDice) continue
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

            val newBlocked = if (newRemaining.size >= 2)
                blockedDiceFor(nextMoves, newRemaining[0], newRemaining[1])
            else emptySet()

            val snapshot = MoveSnapshot(
                board = state.board.copyOf(),
                remainingDice = state.remainingDice,
                legalMoves = state.legalMoves.copyOf(),
                blockedDice = state.blockedDice,
                pipCountHuman = state.pipCountHuman,
                pipCountEngine = state.pipCountEngine
            )

            _gameState.value = state.copy(
                board          = newBoard,
                remainingDice  = newRemaining,
                legalMoves     = nextMoves,
                blockedDice    = newBlocked,
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
            blockedDice    = snapshot.blockedDice,
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
    fun passTurn() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        if (state.legalMoves.isNotEmpty()) return
        if (!state.board.contentEquals(state.oldBoard)) return
        viewModelScope.launch(engineThread) {
            if (_gameState.value.phase != GamePhase.HUMAN_MOVING) return@launch
            _gameState.value = _gameState.value.copy(phase = GamePhase.ENGINE_THINKING)
            Engine.applyMoveString("")
            if (Engine.getMatchStatus() >= 2) {
                Engine.getGameResult().let { gr -> readMatchState(phase = GamePhase.GAME_OVER, winner = gr[0], nPoints = gr[1]) }
                return@launch
            }
            val cubeInfo = Engine.getMatchCubeInfo()
            if (cubeInfo[0] == 1 && Engine.getMatchTurn() == 0) {
                readMatchState(phase = GamePhase.CUBE_OFFERED)
                return@launch
            }
            val mrd = Engine.getMoveRecordDice()
            val engDice = if (mrd[0] > 0) Pair(mrd[0], mrd[1]) else null
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL, engineDice = engDice)
        }
    }

    fun confirm() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        viewModelScope.launch(engineThread) {
            val origDice = state.originalDice ?: return@launch
            if (state.board.contentEquals(state.oldBoard)) return@launch
            val moveStr = Engine.findMove(state.oldBoard, state.board, origDice.first, origDice.second)
            android.util.Log.i("gnubg-vm", "confirm: findMove='$moveStr' dice=${origDice.first},${origDice.second} remaining=${state.remainingDice}")
            if (moveStr.isEmpty()) { android.util.Log.e("gnubg-vm", "confirm: findMove empty"); return@launch }
            if (_gameState.value.phase != GamePhase.HUMAN_MOVING) return@launch
            _gameState.value = _gameState.value.copy(phase = GamePhase.ENGINE_THINKING)
            Engine.applyMoveString(moveStr)

            // --- Tutor analysis: gnubg's own AnalyzeMove on the played move ---
            // Runs AFTER applyMoveString (the move record must be in plGame).
            // Wrapped in try/catch: never affects gameplay.
            try {
                val raw = Engine.tutorAnalyze(state.oldBoard)
                if (raw.size == 52) {
                    val playedEquity = Float.fromBits(raw[0])
                    val bestEquity   = Float.fromBits(raw[1])
                    val bestBoard    = raw.copyOfRange(2, 52)
                    val equityLoss   = (bestEquity - playedEquity).coerceAtLeast(0f)
                    val level        = com.clavierhaus.gnubg.tutor.BlunderClassifier.classify(equityLoss)
                    val playedFV     = com.clavierhaus.gnubg.tutor.FeatureExtractor.extract(state.board)
                    val bestFV       = com.clavierhaus.gnubg.tutor.FeatureExtractor.extract(bestBoard)
                    val comparison   = com.clavierhaus.gnubg.tutor.FeatureExtractor.compare(playedFV, bestFV)
                    val notable      = comparison.notableDeltas.take(3)
                        .joinToString("; ") { "${it.feature} ${it.playedValue}->${it.bestValue}" }
                    android.util.Log.i("gnubg-tutor",
                        "level=$level loss=${"%.4f".format(equityLoss)} " +
                        "best=${"%.4f".format(bestEquity)} played=${"%.4f".format(playedEquity)} | " +
                        notable.ifEmpty { "no notable deltas" })
                } else {
                    android.util.Log.i("gnubg-tutor", "no analysis (raw.size=${raw.size})")
                }
            } catch (t: Throwable) {
                android.util.Log.e("gnubg-tutor", "analysis failed: ${t.message}")
            }

            if (Engine.getMatchStatus() >= 2) {
                Engine.getGameResult().let { gr -> readMatchState(phase = GamePhase.GAME_OVER, winner = gr[0], nPoints = gr[1]) }
                return@launch
            }
            val cubeInfo = Engine.getMatchCubeInfo()
            if (cubeInfo[0] == 1 && Engine.getMatchTurn() == 0) {
                readMatchState(phase = GamePhase.CUBE_OFFERED)
                return@launch
            }
            val mrd = Engine.getMoveRecordDice()
            val engDice = if (mrd[0] > 0) Pair(mrd[0], mrd[1]) else null
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL, engineDice = engDice)
        }
    }

    fun offerDouble() {
        val state = _gameState.value
        if (state.phase != GamePhase.WAITING_FOR_ROLL) {
            android.util.Log.i("gnubg-vm", "offerDouble: ignored phase=${state.phase}")
            return
        }

        _gameState.value = state.copy(phase = GamePhase.ENGINE_THINKING)

        viewModelScope.launch(engineThread) {
            try {
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
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                    return@launch
                }

                val matchBoard = Engine.getMatchBoard()
                val cd = Engine.cubeDecision(matchBoard)

                if (cd == null || cd.isEmpty()) {
                    android.util.Log.e("gnubg-vm", "offerDouble: cubeDecision returned null/empty")
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                    return@launch
                }

                /* Layout (cd.size == 20):
                 *   cd[0..6]   = aarOutput[0][0..6] float-bits (primary eval)
                 *   cd[7..13]  = aarOutput[1][0..6] float-bits (secondary eval)
                 *   cd[14..17] = arDouble[OPTIMAL,NODOUBLE,TAKE,DROP] float-bits
                 *   cd[18]     = cubedecision enum ordinal
                 *   cd[19]     = reserved */
                val decision = when {
                    cd.size >= 19 -> cd[18]
                    cd.size >= 15 -> cd[14]
                    else          -> cd[0]
                }
                if (cd.size >= 18) {
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
                android.util.Log.i(
                    "gnubg-vm",
                    "offerDouble: cubeDecision enum=$decision rawSize=${cd.size} raw0=${cd[0]}"
                )

                val cdEnum = CubeDecision.fromOrdinal(decision)
                if (cdEnum == null) {
                    android.util.Log.e(
                        "gnubg-vm",
                        "offerDouble: cubeDecision returned unknown ordinal=$decision (eval.h has 21 values: 0..20)"
                    )
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                    return@launch
                }

                val action = cubeDecisionAction(cdEnum)
                android.util.Log.i(
                    "gnubg-vm",
                    "offerDouble: gnubg cd=$cdEnum (ordinal=$decision) -> action=$action"
                )

                when (action) {
                    CubeAction.TAKE -> {
                        if (cdEnum in BEAVER_DECISIONS) {
                            android.util.Log.i(
                                "gnubg-vm",
                                "offerDouble: beaver $cdEnum collapsed to take (no beaver UI yet)"
                            )
                        }
                        // Register the human double through the engine, then let
                        // the engine take it (CommandTake runs for the engine
                        // player only while fComputerDecision is set, which
                        // engineCubeResponse does).
                        Engine.commandDouble()
                        Engine.engineCubeResponse(true)
                        android.util.Log.i(
                            "gnubg-vm",
                            "offerDouble: engine took human double ($cdEnum)"
                        )
                        readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                    }

                    CubeAction.DROP -> {
                        Engine.commandDouble()
                        Engine.engineCubeResponse(false)
                        val gr = Engine.getGameResult()
                        android.util.Log.i(
                            "gnubg-vm",
                            "offerDouble: engine dropped human double ($cdEnum) result=${gr.joinToString(",")}"
                        )
                        readMatchState(
                            phase = GamePhase.GAME_OVER,
                            winner = gr[0],
                            nPoints = gr[1]
                        )
                    }

                    CubeAction.NONE -> {
                        // NOT_AVAILABLE: cube cannot be offered. Engine.canDouble()
                        // earlier in this function should have caught this; if
                        // we got here, that gating is wrong.
                        android.util.Log.e(
                            "gnubg-vm",
                            "offerDouble: cd=$cdEnum has no action; Engine.canDouble() gating failed?"
                        )
                        readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                    }
                }
            } catch (t: Throwable) {
                android.util.Log.e("gnubg-vm", "offerDouble: failed", t)
                readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
            } finally {
                actionInProgress.set(false)
            }
        }
    }

    fun acceptDouble() {
        viewModelScope.launch(engineThread) {
            Engine.commandTake()
            val ed = Engine.getMoveRecordDice()
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL,
                engineDice = if (ed[0] > 0) Pair(ed[0], ed[1]) else null)
        }
    }

    fun dropDouble() {
        viewModelScope.launch(engineThread) {
            Engine.commandDrop()
            Engine.getGameResult().let { gr ->
                readMatchState(phase = GamePhase.GAME_OVER, winner = gr[0], nPoints = gr[1])
            }
        }
    }


    private fun refreshFromEngineAfterControl() {
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
            cubeInfo[0] == 1 && turn == 0 -> GamePhase.CUBE_OFFERED
            dice[0] > 0 && turn == 0 -> GamePhase.HUMAN_MOVING
            else -> GamePhase.WAITING_FOR_ROLL
        }

        readMatchState(phase = phase)
    }

    fun commandNewGame() {
        viewModelScope.launch(engineThread) {
            Engine.commandNewGame()
            refreshFromEngineAfterControl()
        }
    }

    fun commandNewMatch(length: Int = _settings.value.matchLength) {
        _settings.value = _settings.value.copy(matchLength = length)
        _showMatchSetup.value = false
        viewModelScope.launch(engineThread) {
            Engine.commandNewMatch(length)
            Engine.commandNewGame()
            refreshFromEngineAfterControl()
        }
    }

    fun commandNewSession(games: Int = 0) {
        viewModelScope.launch(engineThread) {
            Engine.commandNewSession(games)
            refreshFromEngineAfterControl()
        }
    }

    fun commandEndGame() {
        viewModelScope.launch(engineThread) {
            Engine.commandEndGame()
            refreshFromEngineAfterControl()
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

                readMatchState(
                    phase = GamePhase.GAME_OVER,
                    winner = winner,
                    nPoints = points
                )
            } else {
                refreshFromEngineAfterControl()
            }
        }
    }

    fun commandNext(argument: String = "") {
        viewModelScope.launch(engineThread) {
            Engine.commandNext(argument)
            refreshFromEngineAfterControl()
        }
    }

    fun commandAccept() {
        viewModelScope.launch(engineThread) {
            Engine.commandAccept()
            refreshFromEngineAfterControl()
        }
    }

    fun commandReject() {
        viewModelScope.launch(engineThread) {
            Engine.commandReject()
            refreshFromEngineAfterControl()
        }
    }

    fun commandDecline() {
        viewModelScope.launch(engineThread) {
            Engine.commandDecline()
            refreshFromEngineAfterControl()
        }
    }

    fun commandAgree() {
        viewModelScope.launch(engineThread) {
            Engine.commandAgree()
            refreshFromEngineAfterControl()
        }
    }

    fun commandRedouble() {
        viewModelScope.launch(engineThread) {
            Engine.commandRedouble()
            refreshFromEngineAfterControl()
        }
    }

    fun loadGame(path: String) {
        viewModelScope.launch(engineThread) {
            Engine.loadGame(path)
            refreshFromEngineAfterControl()
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
            refreshFromEngineAfterControl()
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
            refreshFromEngineAfterControl()
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
        viewModelScope.launch { PreferencesManager.saveBoardTheme(getApplication(), t) }
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
        // Local-only until GNUbg Settings command timing is made lifecycle-safe.
        _settings.value = _settings.value.copy(tutorMode = on)
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
