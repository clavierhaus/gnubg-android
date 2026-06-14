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

    // Single persistent thread for all engine calls — gnubg is not thread-safe
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

    // ── Read ms state into BoardState ─────────────────────────────────────────
    // Single source of truth: ms owns board, dice, turn, game status.
    // This function snapshots ms into the UI state after every engine call.
    private fun readMatchState(
        phase: GamePhase,
        remainingDice: List<Int> = emptyList(),
        legalMoves: IntArray = IntArray(0),
        moveHistory: List<IntArray> = emptyList(),
        diceHistory: List<List<Int>> = emptyList(),
        winner: Int = -1
    ) {
        val turn  = Engine.getMatchTurn()
        val dice  = Engine.getMatchDice()
        val d0 = dice[0]; val d1 = dice[1]

        // gnubg always puts the moving player's checkers in anBoard[1].
        // For display we always show human (player 0) from the bottom.
        // When it is the engine's turn (fTurn=1), anBoard[1]=engine, anBoard[0]=human —
        // swap to restore the fixed human-bottom perspective.
        val rawBoard = Engine.getMatchBoard()
        val board = if (turn == 1) Engine.swapBoard(rawBoard) else rawBoard

        val pips = Engine.pipCount(board)
        _gameState.value = BoardState(
            board          = board,
            turn           = turn,
            dice           = if (d0 > 0) Pair(d0, d1) else null,
            remainingDice  = remainingDice,
            legalMoves     = legalMoves,
            moveHistory    = moveHistory,
            diceHistory    = diceHistory,
            pipCountHuman  = pips[0],
            pipCountEngine = pips[1],
            phase          = phase,
            winner         = winner
        )
    }

    // ── New game ──────────────────────────────────────────────────────────────
    // CommandNewGame does the opening roll and sets ms.fTurn to the winner.
    // fTurn==1 → human (anBoard[1]). fTurn==0 → engine; CommandRoll auto-plays it.
    private fun startNewGame() {
        Engine.newGame()
        val turn = Engine.getMatchTurn()
        val dice = Engine.getMatchDice()
        val d0 = dice[0]; val d1 = dice[1]
        if (turn == 0) {
            // Human won opening roll — use those dice immediately
            val board = Engine.getMatchBoard()
            val legal = Engine.getLegalMoves(board, d0, d1)
            val remaining = listOf(d0, d1)
            readMatchState(phase = GamePhase.HUMAN_MOVING, remainingDice = remaining, legalMoves = legal)
        } else {
            // Engine won opening roll and already moved — human rolls next
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
        }
    }

    fun newGame() {
        viewModelScope.launch(engineThread) {
            startNewGame()
        }
    }

    // ── Roll dice ─────────────────────────────────────────────────────────────
    // CommandRoll rolls dice and — if ap[ms.fTurn].pt == PLAYER_GNU —
    // automatically calls ComputerTurn which finds and applies the best move.
    // We just read ms state afterwards.
    fun rollDice() {
        if (_gameState.value.phase != GamePhase.WAITING_FOR_ROLL) return
        if (!_engineReady.value) return

        viewModelScope.launch(engineThread) {
            Engine.rollDice()
            val turn = Engine.getMatchTurn()
            val dice = Engine.getMatchDice()
            val d0 = dice[0]; val d1 = dice[1]

            if (turn == 0) {
                // Human turn (fTurn==0 = ap[0] = PLAYER_HUMAN)
                val board = Engine.getMatchBoard()
                val legal = Engine.getLegalMoves(board, d0, d1)
                val remaining = if (d0 == d1) listOf(d0, d0, d0, d0) else listOf(d0, d1)
                readMatchState(phase = GamePhase.HUMAN_MOVING, remainingDice = remaining, legalMoves = legal)
            } else {
                // Engine already moved via ComputerTurn inside CommandRoll
                if (Engine.getMatchStatus() >= 2) {
                    readMatchState(phase = GamePhase.GAME_OVER, winner = 1)
                } else {
                    readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
                }
            }
        }
    }

    // ── Human move: tap a source point ───────────────────────────────────────
    // Finds the first complete legal move from the tapped point in gnubg's
    // move list, formats it via FormatMove, and applies the full turn via
    // CommandMove. CommandMove calls TurnDone() internally — ms owns all state.
    fun tapSource(point: Int) {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return

        viewModelScope.launch(engineThread) {
            val humanOnBar = state.board[49]  // anBoard[1][24]
            val gnubgSrc = if (humanOnBar > 0) 24 else point - 1

            // Find the first complete legal move containing a sub-move from gnubgSrc.
            // gnubg encodes each move as up to 4 sub-move pairs: [src0,dst0,src1,dst1,...,-1,-1]
            // We match any sub-move slot, not just the first.
            val nMoves = state.legalMoves.size / 8
            var matchedMove: IntArray? = null
            outer@ for (i in 0 until nMoves) {
                val m = state.legalMoves.sliceArray(i * 8 until (i + 1) * 8)
                for (j in 0..3) {
                    if (m[j * 2] == gnubgSrc && m[j * 2 + 1] >= 0) {
                        matchedMove = m
                        break@outer
                    }
                }
            }
            if (matchedMove == null) return@launch

            // Format the complete move and apply via CommandMove
            // CommandMove applies all sub-moves, records MOVE_NORMAL, calls TurnDone()
            val moveStr = Engine.formatMove(state.board, matchedMove)
            Engine.applyMoveString(moveStr)

            if (Engine.getMatchStatus() >= 2) {
                readMatchState(phase = GamePhase.GAME_OVER, winner = 0)
                return@launch
            }

            // Turn is done — hand off to engine via rollDice()
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
            delay(500)
            rollDice()
        }
    }

    // ── Swap dice order ───────────────────────────────────────────────────────
    // GenerateMoves is order-independent; we just swap the display order
    // and re-fetch legal moves for the new leading die.
    fun swapDice() {
        val state = _gameState.value
        if (state.phase != GamePhase.HUMAN_MOVING) return
        val dice = state.dice ?: return
        if (dice.first == dice.second) return
        // GenerateMoves is order-independent — legalMoves covers all dice combinations
        _gameState.value = state.copy(
            dice          = Pair(dice.second, dice.first),
            remainingDice = state.remainingDice.reversed()
        )
    }

    // ── Cancel last sub-move ──────────────────────────────────────────────────
    fun cancelMove() {
        val state = _gameState.value
        if (!state.canCancel) return
        val prevBoard = state.moveHistory.last()
        val prevDice  = state.diceHistory.last()

        viewModelScope.launch(engineThread) {
            val legal = when {
                prevDice.size >= 2 -> Engine.getLegalMoves(prevBoard, prevDice[0], prevDice[1])
                prevDice.size == 1 -> Engine.getLegalMoves(prevBoard, prevDice[0], prevDice[0])
                else -> IntArray(0)
            }
            val pips = Engine.pipCount(prevBoard)
            _gameState.value = state.copy(
                board          = prevBoard,
                remainingDice  = prevDice,
                legalMoves     = legal,
                moveHistory    = state.moveHistory.dropLast(1),
                diceHistory    = state.diceHistory.dropLast(1),
                pipCountHuman  = pips[1],
                pipCountEngine = pips[0]
            )
        }
    }

    // ── Commit human move — hand off to engine ────────────────────────────────
    fun commitMove() {
        val state = _gameState.value
        if (!state.canCommit) return

        viewModelScope.launch(engineThread) {
            readMatchState(phase = GamePhase.WAITING_FOR_ROLL)
            delay(500)
            rollDice()
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
