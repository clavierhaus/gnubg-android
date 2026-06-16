package com.clavierhaus.gnubg.engine

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.clavierhaus.gnubg.Engine
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.delay
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

    // Snapshot ms into BoardState.
    // Board swapped when fTurn==1 so human is always at bottom.
    // Dice from remainingDice — ms.anDice is 0 after TurnDone().
    private fun readMatchState(
        phase: GamePhase,
        remainingDice: List<Int> = emptyList(),
        legalMoves: IntArray = IntArray(0),
        oldBoard: IntArray = IntArray(50),
        originalDice: Pair<Int, Int>? = null,
        engineDice: Pair<Int, Int>? = null,
        winner: Int = -1
    ) {
        val turn     = Engine.getMatchTurn()
        val rawBoard = Engine.getMatchBoard()
        val board    = if (turn == 1) Engine.swapBoard(rawBoard) else rawBoard
        val pips     = Engine.pipCount(board)
        // Always show both dice from originalDice for display; usedCount handles graying
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
            winner         = winner
        )
    }

    private fun startNewGame() {
        Engine.newGame()
        val turn = Engine.getMatchTurn()
        val dice = Engine.getMatchDice()
        val d0 = dice[0]; val d1 = dice[1]
        if (turn == 0 && d0 > 0) {
            val board = Engine.getMatchBoard()
            readMatchState(
                phase         = GamePhase.HUMAN_MOVING,
                remainingDice = listOf(d0, d1),
                legalMoves    = Engine.getLegalMoves(board, d0, d1),
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
        viewModelScope.launch(engineThread) { startNewGame() }
    }

    fun rollDice() {
        if (_gameState.value.phase != GamePhase.WAITING_FOR_ROLL) return
        if (!_engineReady.value) return
        viewModelScope.launch(engineThread) {
            Engine.rollDice()
            val turn = Engine.getMatchTurn()
            val dice = Engine.getMatchDice()
            val d0 = dice[0]; val d1 = dice[1]
            android.util.Log.i("gnubg-vm", "rollDice: turn=$turn d0=$d0 d1=$d1 gs=${Engine.getMatchStatus()}")
            if (turn == 0 && d0 > 0) {
                val board = Engine.getMatchBoard()
                val remaining = if (d0 == d1) listOf(d0, d0, d0, d0) else listOf(d0, d1)
                readMatchState(
                    phase         = GamePhase.HUMAN_MOVING,
                    remainingDice = remaining,
                    legalMoves    = Engine.getLegalMoves(board, d0, d1),
                    oldBoard      = board,
                    originalDice  = Pair(d0, d1)
                )
            } else {
                if (Engine.getMatchStatus() >= 2)
                    readMatchState(phase = GamePhase.GAME_OVER, winner = Engine.getMatchWinner())
                else {
                    val ed = Engine.getMoveRecordDice()
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL,
                        engineDice = if (ed[0] > 0) Pair(ed[0], ed[1]) else null)
                }
            }
        }
    }

    // Single tap on source: try first die, then second via ApplySubMove.
    // Mirrors button_release_event single-click in gtkboard.c.
    // Updates display board only — no match record, no TurnDone.
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
            // Keep original dice pair for display — graying handled by usedCount
            val dicePair = state.originalDice
            val nextMoves = if (newRemaining.isNotEmpty()) {
                val d0 = newRemaining[0]
                val d1 = if (newRemaining.size > 1) newRemaining[1] else d0
                Engine.getLegalMoves(newBoard, d0, d1)
            } else IntArray(0)

            _gameState.value = state.copy(
                board          = newBoard,
                remainingDice  = newRemaining,
                legalMoves     = nextMoves,
                pipCountHuman  = pips[0],
                pipCountEngine = pips[1],
                dice           = dicePair
            )
        }
    }

    // Undo: restore board to start of turn (oldBoard) and reset remaining dice
    fun undo() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        val origDice = state.originalDice ?: return
        val d0 = origDice.first; val d1 = origDice.second
        val pips = Engine.pipCount(state.oldBoard)
        val remaining = if (d0 == d1) listOf(d0, d0, d0, d0) else listOf(d0, d1)
        _gameState.value = state.copy(
            board          = state.oldBoard,
            remainingDice  = remaining,
            legalMoves     = Engine.getLegalMoves(state.oldBoard, d0, d1),
            pipCountHuman  = pips[0],
            pipCountEngine = pips[1],
            dice           = origDice
        )
    }

    // Swap dice order — mirrors clicking dice in gtkboard.c (swaps diceRoll[0] and diceRoll[1])
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

    // Commit: mirrors Confirm(bd)/update_move() in gtkboard.c.
    // findMove locates the complete move by position key comparison,
    // then CommandMove records it and calls TurnDone().
    fun confirm() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        if (state.remainingDice.isNotEmpty()) return  // not all dice played yet
        viewModelScope.launch(engineThread) {
            val origDice = state.originalDice ?: return@launch
            // findMove only works when all dice have been used (board differs from oldBoard).
            // If remainingDice is non-empty, the human has not finished — ignore.
            if (state.board.contentEquals(state.oldBoard)) { android.util.Log.e("gnubg-vm", "confirm: board unchanged"); return@launch }
            val moveStr = Engine.findMove(state.oldBoard, state.board, origDice.first, origDice.second)
            android.util.Log.i("gnubg-vm", "confirm: findMove='$moveStr' dice=${origDice.first},${origDice.second} remaining=${state.remainingDice}")
            if (moveStr.isEmpty()) { android.util.Log.e("gnubg-vm", "confirm: findMove empty"); return@launch }
            // Guard against duplicate commits — re-check phase under engine lock
            if (_gameState.value.phase != GamePhase.HUMAN_MOVING) return@launch
            _gameState.value = _gameState.value.copy(phase = GamePhase.ENGINE_THINKING)
            Engine.applyMoveString(moveStr)
            if (Engine.getMatchStatus() >= 2) {
                readMatchState(phase = GamePhase.GAME_OVER, winner = Engine.getMatchWinner())
                return@launch
            }
            // Engine already moved inside NextTurn(TRUE) in applyMoveString.
            // ms.fTurn=0 now. Just read engine dice from move record and show WAITING_FOR_ROLL.
            val mrd = Engine.getMoveRecordDice()
            val engDice = if (mrd[0] > 0) Pair(mrd[0], mrd[1]) else null
            if (Engine.getMatchStatus() >= 2)
                readMatchState(phase=GamePhase.GAME_OVER, winner=Engine.getMatchWinner())
            else
                readMatchState(phase=GamePhase.WAITING_FOR_ROLL, engineDice=engDice)
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
