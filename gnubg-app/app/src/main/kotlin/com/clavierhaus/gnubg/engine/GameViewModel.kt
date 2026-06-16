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

    private val engineThread = Executors.newSingleThreadExecutor { r ->
        Thread(r, "gnubg-engine-thread")
    }.asCoroutineDispatcher()

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
            startNewGame()
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
        blockedDice: Set<Int> = emptySet()
    ) {
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
        if (!s0) blocked.add(0)
        if (!s1) blocked.add(1)
        return blocked
    }

    private fun startNewGame() {
        Engine.newGame()
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
        viewModelScope.launch(engineThread) { startNewGame() }
    }

    fun rollDice() {
        if (_gameState.value.phase != GamePhase.WAITING_FOR_ROLL) return
        if (!_engineReady.value) return
        viewModelScope.launch(engineThread) {
            Engine.rollDice()
            // Check if engine doubled before rolling
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

    fun tapSource(point: Int) {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        if (state.remainingDice.isEmpty()) return
        viewModelScope.launch(engineThread) {
            val humanOnBar = state.board[49]
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

            val newRemaining = state.remainingDice.toMutableList().also { it.remove(usedDie) }
            val pips = Engine.pipCount(newBoard)
            val nextMoves = if (newRemaining.isNotEmpty()) {
                val r0 = newRemaining[0]
                val r1 = if (newRemaining.size > 1) newRemaining[1] else r0
                Engine.getLegalMoves(newBoard, r0, r1)
            } else IntArray(0)
            val newBlocked = if (newRemaining.size >= 2)
                blockedDiceFor(nextMoves, newRemaining[0], newRemaining[1])
            else emptySet()

            _gameState.value = state.copy(
                board          = newBoard,
                remainingDice  = newRemaining,
                legalMoves     = nextMoves,
                blockedDice    = newBlocked,
                pipCountHuman  = pips[0],
                pipCountEngine = pips[1],
                dice           = state.originalDice
            )
        }
    }

    fun undo() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        val origDice = state.originalDice ?: return
        val d0 = origDice.first; val d1 = origDice.second
        val pips = Engine.pipCount(state.oldBoard)
        val remaining = if (d0 == d1) listOf(d0,d0,d0,d0) else listOf(d0,d1)
        _gameState.value = state.copy(
            board          = state.oldBoard,
            remainingDice  = remaining,
            legalMoves     = Engine.getLegalMoves(state.oldBoard, d0, d1),
            pipCountHuman  = pips[0],
            pipCountEngine = pips[1],
            dice           = origDice
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
            val mrd = Engine.getMoveRecordDice()
            val engDice = if (mrd[0] > 0) Pair(mrd[0], mrd[1]) else null
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL, engineDice = engDice)
        }
    }

    fun offerDouble() {
        if (_gameState.value.phase != GamePhase.WAITING_FOR_ROLL) return
        viewModelScope.launch(engineThread) {
            Engine.commandDouble()
            val cubeInfo = Engine.getMatchCubeInfo()
            if (Engine.getMatchStatus() >= 2)
                Engine.getGameResult().let { gr -> readMatchState(phase = GamePhase.GAME_OVER, winner = gr[0], nPoints = gr[1]) }
            else if (cubeInfo[0] == 0) {
                // Engine took — now it's engine's turn to roll
                rollDice()
            }
            // If cubeInfo[0]==1 engine doubled back (beaver) — handle later
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

    // Settings
    fun setMatchLength(n: Int)           { _settings.value = _settings.value.copy(matchLength = n) }
    fun setCrawford(on: Boolean)         { _settings.value = _settings.value.copy(crawford = on) }
    fun setJacoby(on: Boolean)           { _settings.value = _settings.value.copy(jacoby = on) }
    fun setAutomaticDoubles(n: Int)      { _settings.value = _settings.value.copy(automaticDoubles = n) }
    fun setBeavers(on: Boolean)          { _settings.value = _settings.value.copy(beavers = on) }
    fun setBoardTheme(t: BoardTheme)     {
        _settings.value = _settings.value.copy(boardTheme = t)
        viewModelScope.launch { PreferencesManager.saveBoardTheme(getApplication(), t) }
    }
    fun setShowPointNumbers(on: Boolean) { _settings.value = _settings.value.copy(showPointNumbers = on) }
    fun setShowPipCount(on: Boolean)     { _settings.value = _settings.value.copy(showPipCount = on) }
    fun setDifficulty(d: Difficulty)     { _settings.value = _settings.value.copy(difficulty = d) }
    fun setTutorMode(on: Boolean)        { _settings.value = _settings.value.copy(tutorMode = on) }
    fun setHint(on: Boolean)             { _settings.value = _settings.value.copy(hint = on) }
    fun setShowEquity(on: Boolean)       { _settings.value = _settings.value.copy(showEquity = on) }
    fun setShowMWC(on: Boolean)          { _settings.value = _settings.value.copy(showMWC = on) }
    fun setThresholdDoubtful(v: Float)   { _settings.value = _settings.value.copy(thresholdDoubtful = v) }
    fun setThresholdBad(v: Float)        { _settings.value = _settings.value.copy(thresholdBad = v) }
    fun setThresholdVeryBad(v: Float)    { _settings.value = _settings.value.copy(thresholdVeryBad = v) }
}
