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
                pipCountHuman = Engine.pipCount(startBoard)[1],
                pipCountEngine = Engine.pipCount(startBoard)[0]
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
                android.util.Log.d("gnubg-ui", "board[0..24]=${state.board.slice(0..24)}")
                android.util.Log.d("gnubg-ui", "board[25..49]=${state.board.slice(25..49)}")
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
            // Use gnubg legal moves exclusively — no manual move computation
            val humanOnBar = state.board[49]
            val gnubgSrc = if (humanOnBar > 0) 24 else point - 1

            val nLegal = state.legalMoves.size / 8
            android.util.Log.d("gnubg-move", "tap point=$point gnubgSrc=$gnubgSrc nLegal=$nLegal")

            // Find legal sub-move from gnubgSrc using the first remaining die
            val die = state.remainingDice.first()
            var matchedMove: IntArray? = null
            outer@ for (i in 0 until nLegal) {
                val m = state.legalMoves.sliceArray(i * 8 until (i + 1) * 8)
                for (j in 0..3) {
                    val src = m[j*2]; val dst = m[j*2+1]
                    if (src == gnubgSrc && dst >= 0 && src - dst == die) {
                        matchedMove = intArrayOf(src, dst, -1, -1, -1, -1, -1, -1)
                        break@outer
                    }
                }
            }
            // Fallback: match by source only if die-exact match not found
            if (matchedMove == null) {
                outer@ for (i in 0 until nLegal) {
                    val m = state.legalMoves.sliceArray(i * 8 until (i + 1) * 8)
                    for (j in 0..3) {
                        if (m[j*2] == gnubgSrc && m[j*2+1] >= 0) {
                            matchedMove = intArrayOf(m[j*2], m[j*2+1], -1, -1, -1, -1, -1, -1)
                            break@outer
                        }
                    }
                }
            }

            if (matchedMove == null) {
                android.util.Log.d("gnubg-move", "no legal move from $gnubgSrc")
                return@launch
            }

            android.util.Log.d("gnubg-move", "applying ${matchedMove[0]}->${matchedMove[1]}")
            val newBoard = Engine.applyMove(state.board, matchedMove)
            val newRemaining = state.remainingDice.drop(1)

            val winner = Engine.isGameOver(newBoard)
            if (winner != 0) {
                _gameState.value = state.copy(board = newBoard,
                    phase = GamePhase.GAME_OVER,
                    winner = if (winner == 1) 0 else 1)
                return@launch
            }

            val newLegal = when {
                newRemaining.size >= 2 -> Engine.getLegalMoves(newBoard, newRemaining[0], newRemaining[1])
                newRemaining.size == 1 -> Engine.getLegalMoves(newBoard, newRemaining[0], newRemaining[0])
                else -> IntArray(0)
            }
            _gameState.value = state.copy(
                board = newBoard,
                remainingDice = newRemaining,
                legalMoves = newLegal,
                moveHistory = state.moveHistory + listOf(state.board),
                diceHistory = state.diceHistory + listOf(state.remainingDice),
                pipCountHuman = Engine.pipCount(newBoard)[1],
                pipCountEngine = Engine.pipCount(newBoard)[0]
            )
        }
    }


    fun swapDice() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        val dice = state.dice ?: return
        if (dice.first == dice.second) return
        val newRemaining = state.remainingDice.reversed()
        viewModelScope.launch(engineThread) {
            val newLegal = if (newRemaining.size >= 2)
                Engine.getLegalMoves(state.board, newRemaining[0], newRemaining[1])
            else if (newRemaining.size == 1)
                Engine.getLegalMoves(state.board, newRemaining[0], newRemaining[0])
            else IntArray(0)
            _gameState.value = state.copy(
                dice = Pair(dice.second, dice.first),
                remainingDice = newRemaining,
                legalMoves = newLegal
            )
        }
    }

    fun cancelMove() {
        val state = _gameState.value
        if (!state.canCancel) return
        val prevBoard = state.moveHistory.last()
        val prevDice = state.diceHistory.last()
        viewModelScope.launch(engineThread) {
            val legal = if (prevDice.size >= 2)
                Engine.getLegalMoves(prevBoard, prevDice[0], prevDice[1])
            else if (prevDice.size == 1)
                Engine.getLegalMoves(prevBoard, prevDice[0], prevDice[0])
            else IntArray(0)
            _gameState.value = state.copy(
                board = prevBoard,
                remainingDice = prevDice,
                legalMoves = legal,
                moveHistory = state.moveHistory.dropLast(1),
                diceHistory = state.diceHistory.dropLast(1),
                pipCountHuman = Engine.pipCount(prevBoard)[1],
                pipCountEngine = Engine.pipCount(prevBoard)[0]
            )
        }
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
                pipCountHuman = Engine.pipCount(state.board)[1],
                pipCountEngine = Engine.pipCount(state.board)[0]
            )
            delay(500)
            rollDice()
        }
    }

    private suspend fun engineMove(d0: Int, d1: Int) {
        val state = _gameState.value
        android.util.Log.d("gnubg-engine", "engineMove d0=$d0 d1=$d1")
        android.util.Log.d("gnubg-engine", "board[0..24]=${state.board.slice(0..24)}")
        android.util.Log.d("gnubg-engine", "board[25..49]=${state.board.slice(25..49)}")
        val swapped = Engine.swapBoard(state.board)
        android.util.Log.d("gnubg-engine", "swapped[0..24]=${swapped.slice(0..24)}")
        android.util.Log.d("gnubg-engine", "swapped[25..49]=${swapped.slice(25..49)}")
        val swappedLegal = Engine.getLegalMoves(swapped, d0, d1)
        android.util.Log.d("gnubg-engine", "swapped legal moves: ${swappedLegal.size / 8}")
        val move = Engine.findBestMove(swapped, d0, d1) ?: return
        // Apply move to swapped board, then swap back
        val newBoardSwapped = Engine.applyMove(swapped, move)
        val newBoard = Engine.swapBoard(newBoardSwapped)
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
            pipCountHuman = Engine.pipCount(newBoard)[1],
            pipCountEngine = Engine.pipCount(newBoard)[0]
        )
    }


    fun newGame() {
        viewModelScope.launch(engineThread) {
            val startBoard = Engine.newGame()
            _gameState.value = BoardState(
                board = startBoard,
                turn = 0,
                phase = GamePhase.WAITING_FOR_ROLL,
                pipCountHuman = Engine.pipCount(startBoard)[1],
                pipCountEngine = Engine.pipCount(startBoard)[0]
            )
        }
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