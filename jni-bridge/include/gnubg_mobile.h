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
int gnubg_mobile_set_auto_doubles(int n);
int gnubg_mobile_set_beavers(int n);
int gnubg_mobile_command_new_session(int games);
int gnubg_mobile_command_end_game(void);
int gnubg_mobile_command_resign(const char *value);
int gnubg_mobile_command_next(const char *argument);
int gnubg_mobile_command_accept(void);
int gnubg_mobile_command_reject(void);
int gnubg_mobile_command_decline(void);
int gnubg_mobile_command_agree(void);
int gnubg_mobile_command_redouble(void);
int gnubg_mobile_command_double(void);
int gnubg_mobile_can_double(void);   /* 1 if a double would succeed; 0 otherwise */
int gnubg_mobile_command_take(void);
int gnubg_mobile_command_drop(void);
int gnubg_mobile_command_roll(void);
int gnubg_mobile_command_move(const char *move);
int gnubg_mobile_start_match(int match_length);
int gnubg_mobile_next_game(void);

/* -- State readers (Tier 1) ------------------------------------------------- */
int gnubg_mobile_get_board(int out_board[50]);
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
int gnubg_mobile_tutor_analyze(const int old_board[50], int out[52]);
int gnubg_mobile_evaluate(const int board[50], float *out, int out_cap);
int gnubg_mobile_classify(const int board[50]);

/* Return up to n_max ranked candidates for the position after rolling d0/d1.
 * out_moves : flat int array, n_max * 8 ints (one anMove[8] per candidate).
 * out_equities: float array, n_max floats (cubeless equity, best-first).
 * Returns the number of candidates written (0 if no legal moves), -1 on error.
 * Candidates are ordered best-first by 1-ply evaluation.
 * The caller owns both output buffers. */
int gnubg_mobile_get_candidates(const int board[50], int d0, int d1,
                                int *out_moves, float *out_equities,
                                int n_max);
int gnubg_mobile_cube_decision(const int board[50],
                               float *out, int out_cap, int *out_decision);
int gnubg_mobile_rollout(const int board[50], int trials,
                         float *out, int out_cap);

/* Full engine initialisation (Tier C). Pass the neural-net weights path.
 * Runs EvalInitialise/RNG/TLD/rollout setup, creates the match list, and
 * configures the Human-vs-GNU players. Returns 1 on success. */
int gnubg_mobile_initialise(const char *weights_path);

/* Engine responds to a human double already on the table (take!=0 -> take). */
int gnubg_mobile_engine_cube_response(int take);

#ifdef __cplusplus
}
#endif

#endif
