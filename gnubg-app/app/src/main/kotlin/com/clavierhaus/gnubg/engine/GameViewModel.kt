package com.clavierhaus.gnubg.engine

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.clavierhaus.gnubg.Engine
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.util.concurrent.Executors

enum class BoardTheme { OCEAN, CLASSIC, FOREST, SYSTEM }

enum class Difficulty(val ply: Int, val label: String, val subtitle: String) {
    BEGINNER(2,     "Beginner",     "2-ply evaluation"),
    INTERMEDIATE(3, "Intermediate", "3-ply evaluation"),
    ADVANCED(4,     "Advanced",     "4-ply evaluation"),
    EXPERT(0,       "Expert",       "Rollout-based")
}

data class GameSettings(
    val matchLength: Int          = 7,
    val crawford: Boolean         = true,
    val jacoby: Boolean           = false,
    val automaticDoubles: Int     = 0,
    val beavers: Boolean          = false,
    val boardTheme: BoardTheme    = BoardTheme.OCEAN,
    val showPointNumbers: Boolean = true,
    val showPipCount: Boolean     = true,
    val difficulty: Difficulty    = Difficulty.ADVANCED,
    val tutorMode: Boolean        = false,
    val hint: Boolean             = true,
    val showEquity: Boolean       = true,
    val showMWC: Boolean          = false,
    val thresholdDoubtful: Float  = 0.010f,
    val thresholdBad: Float       = 0.050f,
    val thresholdVeryBad: Float   = 0.100f
)

class GameViewModel(application: Application) : AndroidViewModel(application) {
    private val _settings = MutableStateFlow(GameSettings())
    val settings: StateFlow<GameSettings> = _settings.asStateFlow()

    private val _gameState = MutableStateFlow(BoardState())
    val gameState: StateFlow<BoardState> = _gameState.asStateFlow()

    private val _engineReady = MutableStateFlow(false)
    val engineReady: StateFlow<Boolean> = _engineReady.asStateFlow()

    // Single persistent thread for all engine calls
    // gnubg TLS is bound to the thread that calls TLSGet — must always be same thread
    private val engineThread = Executors.newSingleThreadExecutor { r ->
        Thread(r, "gnubg-engine-thread")
    }.asCoroutineDispatcher()

    init {
        // Load persisted theme on startup
        viewModelScope.launch {
            PreferencesManager.boardThemeFlow(application).collect { theme ->
                _settings.value = _settings.value.copy(boardTheme = theme)
            }
        }
        // Initialise engine on background thread then start new game
        viewModelScope.launch(engineThread) {
            val weightsPath = AssetExtractor.extractWeights(application)
            Engine.initialise(weightsPath)
            _engineReady.value = true
            val startBoard = Engine.newGame()
            android.util.Log.d("gnubg-board", "b0=${startBoard.slice(0..24)}")
            android.util.Log.d("gnubg-board", "b1=${startBoard.slice(25..49)}")
            _gameState.value = BoardState(
                board = startBoard,
                turn = 0,
                phase = GamePhase.WAITING_FOR_ROLL,
                pipCountHuman = calcPips(startBoard, 0),
                pipCountEngine = calcPips(startBoard, 1)
            )
        }
    }

    // ── Game loop ─────────────────────────────────────────────────────────────

    fun rollDice() {
        val state = _gameState.value
        if (state.phase != GamePhase.WAITING_FOR_ROLL) return
        if (_engineReady.value.not()) return

        viewModelScope.launch(engineThread) {
            val dice = Engine.rollDice()
            val d0 = dice[0]; val d1 = dice[1]

            if (state.turn == 0) {
                val remaining = if (d0 == d1) listOf(d0, d0, d0, d0) else listOf(d0, d1)
                val legal = Engine.getLegalMoves(state.board, d0, d1)
                android.util.Log.d("gnubg-ui", "rolled d0=$d0 d1=$d1 legal.size=${legal.size}")
                _gameState.value = state.copy(
                    dice = Pair(d0, d1),
                    remainingDice = remaining,
                    blockedDice = emptyList(),
                    legalMoves = legal,
                    moveHistory = emptyList(),
                    diceHistory = emptyList(),
                    phase = GamePhase.HUMAN_MOVING
                )
            } else {
                _gameState.value = state.copy(
                    dice = Pair(d0, d1),
                    phase = GamePhase.ENGINE_THINKING
                )
                delay(500)
                engineMove(d0, d1)
            }
        }
    }

    fun tapSource(point: Int) {
        val state = _gameState.value
        android.util.Log.d("gnubg-ui", "tapSource: point=$point phase=${state.phase} remainingDice=${state.remainingDice}")
        if (state.phase != GamePhase.HUMAN_MOVING) return
        if (state.remainingDice.isEmpty()) return

        viewModelScope.launch(engineThread) {
            val die = state.remainingDice.first()
            val isDoubles = state.dice?.let { it.first == it.second } ?: false
            // Human (anBoard[1]) moves from high UI points to low
            // gnubg move encoding: src = UI point - 1 (0-indexed), dest = src - die
            val gnubgSrc = point - 1
            val gnubgDest = gnubgSrc - die
            if (gnubgDest < 0) return@launch

            val move = intArrayOf(gnubgSrc, gnubgDest, -1, -1, -1, -1, -1, -1)
            val newBoard = Engine.applyMove(state.board, move)

            if (isDoubles && state.remainingDice.size >= 2) {
                val dest2 = gnubgDest - die
                if (dest2 >= 0) {
                    val move2 = intArrayOf(gnubgSrc, dest2, -1, -1, -1, -1, -1, -1)
                    val newBoard2 = Engine.applyMove(newBoard, move2)
                    val newRemaining = state.remainingDice.drop(2)
                    val winner = Engine.isGameOver(newBoard2)
                    if (winner != 0) {
                        _gameState.value = state.copy(board = newBoard2,
                            phase = GamePhase.GAME_OVER,
                            winner = if (winner == 1) 0 else 1)
                        return@launch
                    }
                    _gameState.value = state.copy(
                        board = newBoard2,
                        remainingDice = newRemaining,
                        moveHistory = state.moveHistory + listOf(state.board),
                        diceHistory = state.diceHistory + listOf(state.remainingDice),
                        pipCountHuman = calcPips(newBoard2, 0),
                        pipCountEngine = calcPips(newBoard2, 1)
                    )
                    return@launch
                }
            }

            // Single die move
            val newRemaining = state.remainingDice.drop(1)
            val winner = Engine.isGameOver(newBoard)
            if (winner != 0) {
                _gameState.value = state.copy(board = newBoard,
                    phase = GamePhase.GAME_OVER,
                    winner = if (winner == 1) 0 else 1)
                return@launch
            }
            _gameState.value = state.copy(
                board = newBoard,
                remainingDice = newRemaining,
                moveHistory = state.moveHistory + listOf(state.board),
                diceHistory = state.diceHistory + listOf(state.remainingDice),
                pipCountHuman = calcPips(newBoard, 0),
                pipCountEngine = calcPips(newBoard, 1)
            )
        }
    }

    fun swapDice() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        val dice = state.dice ?: return
        if (dice.first == dice.second) return
        _gameState.value = state.copy(
            dice = Pair(dice.second, dice.first),
            remainingDice = state.remainingDice.reversed()
        )
    }

    fun cancelMove() {
        val state = _gameState.value
        if (!state.canCancel) return
        val prevBoard = state.moveHistory.last()
        val prevDice = state.diceHistory.last()
        _gameState.value = state.copy(
            board = prevBoard,
            remainingDice = prevDice,
            moveHistory = state.moveHistory.dropLast(1),
            diceHistory = state.diceHistory.dropLast(1),
            pipCountHuman = calcPips(prevBoard, 0),
            pipCountEngine = calcPips(prevBoard, 1)
        )
    }

    fun commitMove() {
        val state = _gameState.value
        if (!state.canCommit) return
        viewModelScope.launch(engineThread) {
            _gameState.value = state.copy(
                turn = 1,
                dice = null,
                remainingDice = emptyList(),
                moveHistory = emptyList(),
                diceHistory = emptyList(),
                phase = GamePhase.WAITING_FOR_ROLL,
                pipCountHuman = calcPips(state.board, 0),
                pipCountEngine = calcPips(state.board, 1)
            )
            delay(500)
            rollDice()
        }
    }

    private suspend fun engineMove(d0: Int, d1: Int) {
        val state = _gameState.value
        val move = Engine.findBestMove(state.board, d0, d1) ?: return
        val newBoard = Engine.applyMove(state.board, move)
        val winner = Engine.isGameOver(newBoard)
        if (winner != 0) {
            _gameState.value = state.copy(board = newBoard,
                phase = GamePhase.GAME_OVER,
                winner = if (winner == 1) 0 else 1)
            return
        }
        _gameState.value = state.copy(
            board = newBoard,
            turn = 0,
            dice = null,
            remainingDice = emptyList(),
            phase = GamePhase.WAITING_FOR_ROLL,
            pipCountHuman = calcPips(newBoard, 0),
            pipCountEngine = calcPips(newBoard, 1)
        )
    }

    private fun calcPips(board: IntArray, player: Int): Int {
        var pips = 0
        val offset = player * 25
        for (i in 0 until 24) { pips += board[offset + i] * (i + 1) }
        pips += board[offset + 24] * 25
        return pips
    }

    // ── Settings ──────────────────────────────────────────────────────────────
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