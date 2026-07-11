#ifndef GNUBG_MOBILE_H
#define GNUBG_MOBILE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-neutral GNUbg mobile facade.
 *
 * This layer is intentionally plain C.
 * Android JNI must call into this facade.
 * Future non-Android adapters should call the same facade.
 *
 * The facade must not expose JNI, Kotlin, Swift,
 * Objective-C, Android lifecycle, or UI types.
 */

const char *gnubg_mobile_facade_version(void);

int gnubg_mobile_command_new_game(void);
int gnubg_mobile_command_new_match(int match_length);
int gnubg_mobile_set_auto_crawford(int on);
int gnubg_mobile_set_jacoby(int on);
int gnubg_mobile_set_cube_use(int on);
int gnubg_mobile_set_met(const char *path);
int gnubg_mobile_set_auto_doubles(int n);
int gnubg_mobile_set_beavers(int n);
int gnubg_mobile_command_new_session(int games);
int gnubg_mobile_command_end_game(void);
int gnubg_mobile_command_resign(const char *value);
int gnubg_mobile_command_next(const char *argument);
int gnubg_mobile_command_previous(const char *argument);  /* PORT: CommandPrevious */
int gnubg_mobile_command_accept(void);
int gnubg_mobile_command_reject(void);
int gnubg_mobile_command_decline(void);
int gnubg_mobile_command_agree(void);
int gnubg_mobile_command_redouble(void);
int gnubg_mobile_command_double(void);
int gnubg_mobile_can_double(void);   /* 1 if a double would succeed; 0 otherwise */
int gnubg_mobile_command_take(void);
int gnubg_mobile_command_drop(void);
/* Roll for the human. Returns 1 if every one of CommandRoll's preconditions held
 * (play.c:4048), 0 if gnubg was always going to refuse -- in which case the
 * reason is logged under the gnubg-roll tag. CommandRoll itself returns void and
 * refuses silently, so without this a refused roll looks like a successful one
 * and the UI loops back to WAITING_FOR_ROLL, which presents as a stuck game. */
int gnubg_mobile_command_roll(void);
/* Resignation offered by GNU: 0 none, 1 normal, 2 gammon, 3 backgammon.
 * PORT: ms.fResigned. GNU offers it itself (play.c:1335); CommandRoll refuses
 * until the human answers with agree or decline. */
int gnubg_mobile_get_resignation(void);
int gnubg_mobile_command_move(const char *move);
int gnubg_mobile_start_match(int match_length);
int gnubg_mobile_next_game(void);

/* -- State readers (Tier 1) ------------------------------------------------- */
int gnubg_mobile_get_board(int out_board[50]);
int gnubg_mobile_get_board_human(int out_board[50]);
int gnubg_mobile_get_match_state(int out_state[13]);
int gnubg_mobile_get_cube_info(int out_cube[3]);
int gnubg_mobile_get_dice(int out_dice[2]);
int gnubg_mobile_get_move_record_dice(int out_dice[2]);
int gnubg_mobile_get_game_result(int out_result[2]);
int gnubg_mobile_get_match_score(int out_score[3]);
int gnubg_mobile_get_match_winner(void);

/* -- Board utilities (Tier 1) ----------------------------------------------- */
int gnubg_mobile_swap_board(const int in_board[50], int out_board[50]);
int gnubg_mobile_pip_count(const int in_board[50], int out_pips[2]);
int gnubg_mobile_apply_sub_move(const int in_board[50], int i_src, int n_roll,
                                int out_board[50]);
int gnubg_mobile_format_move(const int in_board[50], const int in_move[8],
                             char *out_text, int out_capacity);

/* -- File / SGF ops (Tier 1) ------------------------------------------------ */
int gnubg_mobile_load_game(const char *path);
int gnubg_mobile_save_game(const char *path);
int gnubg_mobile_load_match(const char *path);
int gnubg_mobile_save_match(const char *path);
int gnubg_mobile_load_position(const char *path);
int gnubg_mobile_save_position(const char *path);

/* -- Engine algorithms (Tier 2) --------------------------------------------- */
int gnubg_mobile_get_legal_moves(const int board[50], int d0, int d1,
                                 int f_partial, int *out_moves, int out_cap);
/* Set engine chequer-play strength to a gnubg preset (0=Beginner,1=Casual,
 * 2=Intermediate,3=Advanced). See android-app.c. */
void gnubg_mobile_set_engine_strength(int idx);
int gnubg_mobile_find_move(const int old_board[50], const int cur_board[50],
                           int d0, int d1, char *out_text, int out_cap);
/* Tutor analysis via gnubg's AnalyzeMove on the last played move (in plGame).
 * Call AFTER applyMoveString. old_board = pre-move board (for best-move board).
 * out[52]: [0]=played equity bits, [1]=best equity bits, [2..51]=best board.
 * Returns 1 ok, 0 no record, -1 error. */
int gnubg_mobile_skill(float equity_delta);
int gnubg_mobile_tutor_analyze(const int old_board[50], int out[52]);
int gnubg_mobile_analyze_played_move(const int old_board[50], float out[7]);
int gnubg_mobile_apply_move(const int board[50], const int anMove[8], int out[50]); /* PORT: ApplyMove for the candidate explorer */
int gnubg_mobile_coach_verdict_pre(const int old_board[50], int d0, int d1, const int new_board[50], int out[168]); /* PORT: Coach, judged BEFORE apply; [166,167]=dice */
int gnubg_mobile_coach_verdict(int out[166]);  /* PORT: Coach M0, last human move + candidates */
int gnubg_mobile_review_verdict(int out[71]);  /* PORT: verdict at plLastMove */
void gnubg_mobile_on_dice_rolled(void);        /* PORT: called by play.c DiceRolled */
int gnubg_mobile_peek_live_dice(int out[3]);   /* PORT: lock-free {seq,d0,d1} */

/* gnubg own position-feature inputs (CalculateHalfInputs), both sides.
 * out holds 2*MORE_INPUTS floats (MORE_INPUTS from eval.h). Raw normalised. */
int gnubg_mobile_position_features(const int board[50], float out[]);
int gnubg_mobile_evaluate(const int board[50], float *out, int out_cap);
int gnubg_mobile_classify(const int board[50]);


int gnubg_mobile_cube_decision(const int board[50],
                               float *out, int out_cap, int *out_decision);
int gnubg_mobile_rollout(const int board[50], int trials,
                         float *out, int out_cap);

/* Full engine initialisation (Tier C). Pass the neural-net weights path.
 * Runs EvalInitialise/RNG/TLD/rollout setup, creates the match list, and
 * configures the Human-vs-GNU players. Returns 1 on success. */
int gnubg_mobile_initialise(const char *weights_path);

/* ---------------------------------------------------------------------------
 * Position entry (Analyse Position). Thin wrappers over gnubg's own routines.
 * ------------------------------------------------------------------------- */

/* Install a position from a GNU BG ID ("PositionID:MatchID") or an XGID.
 * PORT: SetGNUbgID (backgammon.h:519). Returns gnubg's code unchanged:
 *   0 installed, 1 no valid IDs found, 2 installed but player on roll is on
 *   top (the UI must offer a swap), -1 bad argument.
 * The Command wrapper is deliberately not used: it answers the swap question
 * through GetInputYN, which always returns TRUE in this port. */
/* Encode an edited position + context as "PositionID:MatchID" with gnubg's own
 * encoders. Install the result via gnubg_mobile_set_gnubg_id. Dice 0,0 = not
 * rolled = a cube decision. cube_owner: -1 centred, 0 human, 1 engine.
 * match_to 0 = money game. */
int gnubg_mobile_ids_from_state(const int board[50], int d0, int d1, int turn,
                                int score_h, int score_e, int match_to,
                                int cube, int cube_owner, int crawford,
                                char *out, int out_cap);
/* gnubg's own text for a cubedecision value (GetCubeRecommendation). */
int gnubg_mobile_cube_recommendation(int cd, char *out, int out_cap);

int gnubg_mobile_set_gnubg_id(const char *id);

/* The user's yes to the swap offered after a return of 2.
 * PORT: CommandSwapPlayers (backgammon.h:1024). */
int gnubg_mobile_swap_players(void);

/* gnubg's own renderings of the current state, copied out separately.
 * PORT: PositionID (positionid.h:27), MatchIDFromMatchState (matchid.h:51).
 * Returns 1, or -1 on bad argument. */
int gnubg_mobile_current_ids(char *out_pos, int pos_cap,
                             char *out_match, int match_cap);

/* Ranked chequer-play candidates for the position currently loaded in ms.
 * PORT: FindnSaveBestMoves with esAnalysisChequer.ec and aamfAnalysis, the same
 * named instances the tutor uses. Fills out_equity[n] and out_moves[n*8].
 * Returns n, 0 when the position has no dice, or -1 on error. */
int gnubg_mobile_hint_moves(int max_n, float out_equity[], int out_moves[]);

/* Engine responds to a human double already on the table (take!=0 -> take). */

#ifdef __cplusplus
}
#endif

#endif
