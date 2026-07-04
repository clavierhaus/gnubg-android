package com.clavierhaus.gnubg.engine

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.clavierhaus.gnubg.Engine
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

    private var lastTutorAnalysis: TutorAnalysis? = null

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
            tutorAnalysis  = lastTutorAnalysis
        )
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
                originalDice  = Pair(d0, d1)
            )
        } else {
            val ed = Engine.getMoveRecordDice()
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL,
                engineDice = if (ed[0] > 0) Pair(ed[0], ed[1]) else null)
        }
    }

    fun newGame() {
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
                android.util.Log.i("gnubg-vm", "rollDice legal d0=$d0 d1=$d1 moves=${allMoves.size/8}")
                readMatchState(
                    phase         = GamePhase.HUMAN_MOVING,
                    remainingDice = remaining,
                    legalMoves    = allMoves,
                    oldBoard      = board,
                    originalDice  = Pair(d0, d1),
                    engineDice    = null
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
            // from == 0 is the bar signal (matches tapSource): the human re-enters
            // from gnubg point 24. Entry for die d lands on board point 25 - d, i.e.
            // gnubg 0-based 24 - d, so the same src - d == to - 1 test below applies.
            val src = if (from == 0) 24 else from - 1
            var newBoard = IntArray(0)
            val usedDice = ArrayList<Int>()
            for (d in state.remainingDice.distinct()) {
                if (src - d == to - 1 || (to == 0 && src - d < 0)) {
                    val b = Engine.applySubMove(state.board, src, d)
                    if (b.isNotEmpty()) { newBoard = b; usedDice.add(d); break }
                }
            }
            if (newBoard.isEmpty() && state.remainingDice.size >= 2) {
                val dice = state.remainingDice
                outer@ for (i in dice.indices) {
                    for (j in dice.indices) {
                        if (i == j) continue
                        val dA = dice[i]; val dB = dice[j]
                        val b1 = Engine.applySubMove(state.board, src, dA)
                        if (b1.isEmpty()) continue
                        val mid = src - dA
                        if (mid < 0) continue
                        if (mid - dB == to - 1 || (to == 0 && mid - dB < 0)) {
                            val b2 = Engine.applySubMove(b1, mid, dB)
                            if (b2.isNotEmpty()) {
                                newBoard = b2; usedDice.add(dA); usedDice.add(dB); break@outer
                            }
                        }
                    }
                }
            }
            if (newBoard.isEmpty()) return@launch
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
            // Capture the match score before the move. A game-ending move triggers
            // gnubgs NextTurn(TRUE) inside command_move, which scores the game AND
            // auto-starts the next one -- so by the time we read state, ms.gs is
            // back to GAME_PLAYING and getGameResult() reads the NEW games empty
            // MOVE_GAMEINFO (fWinner=-1). The score delta survives the auto-advance
            // (play.c:291 updates anScore synchronously before the new game starts),
            // so it is the reliable game-over signal -- same pattern as commandResign.
            val scoreBefore = Engine.getMatchScore()
            _gameState.value = _gameState.value.copy(phase = GamePhase.ENGINE_THINKING)
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
            val scoreAfter = Engine.getMatchScore()
            val humanDelta  = scoreAfter[0] - scoreBefore[0]
            val engineDelta = scoreAfter[1] - scoreBefore[1]
            if (humanDelta != 0 || engineDelta != 0) {
                val winner = if (humanDelta > engineDelta) 0 else 1
                val points = kotlin.math.abs(humanDelta - engineDelta).coerceAtLeast(1)
                readMatchState(phase = GamePhase.GAME_OVER, winner = winner, nPoints = points)
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
            analyzeMoveInBackground(state.oldBoard)
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
                    // Refresh the panel with the new verdict. Unguarded, single
                    // targeted update -- no race (we never cleared to null, so the
                    // worst case is the panel briefly shows the prior verdict).
                    _gameState.value = _gameState.value.copy(tutorAnalysis = lastTutorAnalysis)
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
                if (gr[0] >= 0) {
                    android.util.Log.i(
                        "gnubg-vm",
                        "offerDouble: engine dropped; result=${gr.joinToString(",")}"
                    )
                    readMatchState(
                        phase = GamePhase.GAME_OVER,
                        winner = gr[0],
                        nPoints = gr[1]
                    )
                } else {
                    android.util.Log.i("gnubg-vm", "offerDouble: engine took; play continues")
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
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
