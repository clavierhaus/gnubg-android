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
        val turn      = Engine.getMatchTurn()
        val rawBoard  = Engine.getMatchBoard()
        val board     = if (turn == 1) Engine.swapBoard(rawBoard) else rawBoard
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
            fDoubled       = fDoubled
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
            val die0 = state.remainingDice[0]
            val die1 = if (state.remainingDice.size > 1) state.remainingDice[1] else -1

            var newBoard = Engine.applySubMove(state.board, src, die0)
            var usedDie  = die0
            if (newBoard.isEmpty() && die1 > 0) {
                newBoard = Engine.applySubMove(state.board, src, die1)
                usedDie  = die1
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

                val legalCubeWindow =
                    dbg[0] == 1 &&          // GAME_PLAYING
                    dbg[1] == 0 &&          // human turn
                    dbg[2] == 0 &&          // before roll
                    dbg[3] == 0 && dbg[4] == 0 &&
                    dbg[5] == 0 &&          // no pending double
                    dbg[8] == 0 &&          // not Crawford
                    dbg[9] != 0 &&          // cube enabled
                    (dbg[6] == -1 || dbg[6] == 0) // centred or human-owned

                if (!legalCubeWindow) {
                    android.util.Log.i("gnubg-vm", "offerDouble: rejected by legal cube window")
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                    return@launch
                }

                val matchBoard = Engine.getMatchBoard()
                val cd = Engine.cubeDecision(
                    matchBoard,
                    dbg[7],   // cube value
                    dbg[6],   // cube owner
                    dbg[2],   // fMove
                    dbg[12],  // match length
                    dbg[10],  // score0
                    dbg[11],  // score1
                    dbg[8]    // crawford
                )

                if (cd == null || cd.isEmpty()) {
                    android.util.Log.e("gnubg-vm", "offerDouble: cubeDecision returned null/empty")
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                    return@launch
                }

                val decision = if (cd.size >= 15) cd[14] else cd[0]
                android.util.Log.i(
                    "gnubg-vm",
                    "offerDouble: cubeDecision enum=$decision rawSize=${cd.size} raw0=${cd[0]}"
                )

                when (decision) {
                    // Engine accepts/takes the human double.
                    //
                    // 0  DOUBLE_TAKE
                    // 2  NODOUBLE_TAKE
                    // 4  DOUBLE_BEAVER       -> no beaver UI/path yet; collapse to TAKE
                    // 5  NODOUBLE_BEAVER     -> no beaver UI/path yet; collapse to TAKE
                    // 14 OPTIONAL_DOUBLE_TAKE
                    // 16 OPTIONAL_DOUBLE_BEAVER -> no beaver UI/path yet; collapse to TAKE
                    //
                    // Do not call CommandDouble().
                    0, 2, 4, 5, 14, 16 -> {
                        if (decision == 4 || decision == 5 || decision == 16) {
                            android.util.Log.i(
                                "gnubg-vm",
                                "offerDouble: beaver decision cd=$decision collapsed to take; beaver UI/path not implemented"
                            )
                        }

                        val applied = Engine.applyHumanDoubleTake()
                        android.util.Log.i(
                            "gnubg-vm",
                            "offerDouble: applyHumanDoubleTake result=${applied.joinToString(",")}"
                        )

                        if (applied.size >= 8 && applied[0] == 1) {
                            val after = _gameState.value.copy(
                                phase = GamePhase.WAITING_FOR_ROLL,
                                turn = applied[1],
                                fDoubled = applied[5] != 0,
                                cubeOwner = applied[6],
                                cubeValue = applied[7],
                                engineScore = if (applied.size >= 10) applied[8] else _gameState.value.engineScore,
                                humanScore = if (applied.size >= 10) applied[9] else _gameState.value.humanScore
                            )
                            _gameState.value = after
                            android.util.Log.i(
                                "gnubg-vm",
                                "offerDouble: UI cube state updated from apply result " +
                                    "turn=${after.turn} fDoubled=${after.fDoubled} " +
                                    "cubeOwner=${after.cubeOwner} cubeValue=${after.cubeValue} " +
                                    "score=${after.engineScore}-${after.humanScore}"
                            )
                        } else {
                            android.util.Log.e("gnubg-vm", "offerDouble: applyHumanDoubleTake failed/short result")
                            readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                        }
                    }

                    // Engine passes/drops.
                    //
                    // 1  DOUBLE_PASS
                    // 17 OPTIONAL_DOUBLE_PASS
                    //
                    // Pass/drop path is not implemented yet, so do not leave UI stuck thinking.
                    1, 17 -> {
                        android.util.Log.i("gnubg-vm", "offerDouble: engine pass/drop path not implemented yet for cd=$decision")
                        readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                    }

                    else -> {
                        android.util.Log.i("gnubg-vm", "offerDouble: no double/take action for cd=$decision")
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
            Engine.commandResign(value)
            refreshFromEngineAfterControl()
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

    fun setMatchLength(n: Int)          { _settings.value = _settings.value.copy(matchLength = n) }
    fun setCrawford(on: Boolean)        { _settings.value = _settings.value.copy(crawford = on) }
    fun setJacoby(on: Boolean)          { _settings.value = _settings.value.copy(jacoby = on) }
    fun setAutomaticDoubles(n: Int)     { _settings.value = _settings.value.copy(automaticDoubles = n) }
    fun setBeavers(on: Boolean)         { _settings.value = _settings.value.copy(beavers = on) }
    fun setBoardTheme(t: BoardTheme)    {
        _settings.value = _settings.value.copy(boardTheme = t)
        viewModelScope.launch { PreferencesManager.saveBoardTheme(getApplication(), t) }
    }
    fun setShowPointNumbers(on: Boolean) { _settings.value = _settings.value.copy(showPointNumbers = on) }
    fun setShowPipCount(on: Boolean)     { _settings.value = _settings.value.copy(showPipCount = on) }
    fun setDifficulty(d: Difficulty)    { _settings.value = _settings.value.copy(difficulty = d) }
    fun setTutorMode(on: Boolean)       { _settings.value = _settings.value.copy(tutorMode = on) }
    fun setHint(on: Boolean)            { _settings.value = _settings.value.copy(hint = on) }
    fun setShowEquity(on: Boolean)       { _settings.value = _settings.value.copy(showEquity = on) }
    fun setShowMWC(on: Boolean)         { _settings.value = _settings.value.copy(showMWC = on) }
    fun setThresholdDoubtful(v: Float)  { _settings.value = _settings.value.copy(thresholdDoubtful = v) }
    fun setThresholdBad(v: Float)       { _settings.value = _settings.value.copy(thresholdBad = v) }
    fun setThresholdVeryBad(v: Float)   { _settings.value = _settings.value.copy(thresholdVeryBad = v) }
}
