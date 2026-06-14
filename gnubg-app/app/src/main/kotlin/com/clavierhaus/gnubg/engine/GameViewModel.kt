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

        viewModelScope.launch(engineThread) {
            val dice = Engine.rollDice()
            val d0 = dice[0]; val d1 = dice[1]
            val legal = Engine.getLegalMoves(state.board, d0, d1)

            if (state.turn == 0) {
                // Human turn
                _gameState.value = state.copy(
                    dice = Pair(d0, d1),
                    legalMoves = legal,
                    phase = if (legal.isEmpty()) GamePhase.WAITING_FOR_ROLL else GamePhase.HUMAN_MOVING
                )
            } else {
                // Engine turn
                _gameState.value = state.copy(
                    dice = Pair(d0, d1),
                    phase = GamePhase.ENGINE_THINKING
                )
                delay(500) // brief pause so player can see the roll
                engineMove(d0, d1)
            }
        }
    }

    fun selectPoint(point: Int) {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        _gameState.value = state.copy(selectedPoint = point)
    }

    fun moveTo(destPoint: Int) {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        val from = state.selectedPoint
        if (from < 0) return

        val (d0, d1) = state.dice ?: return

        // Find the matching legal move
        val nMoves = state.legalMoves.size / 8
        for (i in 0 until nMoves) {
            val move = state.legalMoves.sliceArray(i * 8 until (i + 1) * 8)
            // Check if first sub-move matches from→dest
            if (move[0] == from && move[1] == destPoint) {
                viewModelScope.launch(engineThread) {
                    val newBoard = Engine.applyMove(state.board, move)
                    val winner = Engine.isGameOver(newBoard)
                    if (winner != 0) {
                        _gameState.value = state.copy(
                            board = newBoard,
                            phase = GamePhase.GAME_OVER,
                            winner = if (winner == 1) 0 else 1,
                            selectedPoint = -1
                        )
                        return@launch
                    }
                    // Switch to engine turn
                    _gameState.value = state.copy(
                        board = newBoard,
                        turn = 1,
                        dice = null,
                        selectedPoint = -1,
                        phase = GamePhase.WAITING_FOR_ROLL,
                        pipCountHuman = calcPips(newBoard, 0),
                        pipCountEngine = calcPips(newBoard, 1)
                    )
                }
                return
            }
        }
        // No matching move — deselect
        _gameState.value = state.copy(selectedPoint = -1)
    }

    private suspend fun engineMove(d0: Int, d1: Int) {
        val state = _gameState.value
        val move = withContext(engineThread) {
            Engine.findBestMove(state.board, d0, d1)
        } ?: return

        val newBoard = withContext(engineThread) {
            Engine.applyMove(state.board, move)
        }

        val winner = Engine.isGameOver(newBoard)
        if (winner != 0) {
            _gameState.value = state.copy(
                board = newBoard,
                phase = GamePhase.GAME_OVER,
                winner = if (winner == 1) 0 else 1
            )
            return
        }

        _gameState.value = state.copy(
            board = newBoard,
            turn = 0,
            dice = null,
            phase = GamePhase.WAITING_FOR_ROLL,
            pipCountHuman = calcPips(newBoard, 0),
            pipCountEngine = calcPips(newBoard, 1)
        )
    }

    // Pip count: sum of (point_index + 1) * checker_count for each point
    private fun calcPips(board: IntArray, player: Int): Int {
        var pips = 0
        val offset = player * 25
        for (i in 0 until 24) {
            pips += board[offset + i] * (i + 1)
        }
        // Bar checkers count as 25
        pips += board[offset + 24] * 25
        return pips
    }

    fun setMatchLength(n: Int)           { _settings.value = _settings.value.copy(matchLength = n) }
    fun setCrawford(on: Boolean)         { _settings.value = _settings.value.copy(crawford = on) }
    fun setJacoby(on: Boolean)           { _settings.value = _settings.value.copy(jacoby = on) }
    fun setAutomaticDoubles(n: Int)      { _settings.value = _settings.value.copy(automaticDoubles = n) }
    fun setBeavers(on: Boolean)          { _settings.value = _settings.value.copy(beavers = on) }
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

    fun setBoardTheme(t: BoardTheme) {
        _settings.value = _settings.value.copy(boardTheme = t)
        viewModelScope.launch {
            PreferencesManager.saveBoardTheme(getApplication(), t)
        }
    }
}
