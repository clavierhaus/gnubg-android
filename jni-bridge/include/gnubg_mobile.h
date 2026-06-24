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
int gnubg_mobile_command_take(void);
int gnubg_mobile_command_drop(void);
int gnubg_mobile_command_roll(void);
int gnubg_mobile_command_move(const char *move);
int gnubg_mobile_start_match(int match_length);
int gnubg_mobile_next_game(void);

/* ── State readers (Tier 1) ───────────────────────────────────────────────── */
int gnubg_mobile_get_board(int out_board[50]);
int gnubg_mobile_get_match_state(int out_state[13]);
int gnubg_mobile_get_cube_info(int out_cube[3]);
int gnubg_mobile_get_dice(int out_dice[2]);
int gnubg_mobile_get_move_record_dice(int out_dice[2]);
int gnubg_mobile_get_game_result(int out_result[2]);
int gnubg_mobile_get_match_score(int out_score[3]);
int gnubg_mobile_get_match_winner(void);

/* ── Board utilities (Tier 1) ─────────────────────────────────────────────── */
int gnubg_mobile_swap_board(const int in_board[50], int out_board[50]);
int gnubg_mobile_pip_count(const int in_board[50], int out_pips[2]);
int gnubg_mobile_apply_sub_move(const int in_board[50], int i_src, int n_roll,
                                int out_board[50]);
int gnubg_mobile_format_move(const int in_board[50], const int in_move[8],
                             char *out_text, int out_capacity);

/* ── File / SGF ops (Tier 1) ──────────────────────────────────────────────── */
int gnubg_mobile_load_game(const char *path);
int gnubg_mobile_save_game(const char *path);
int gnubg_mobile_load_match(const char *path);
int gnubg_mobile_save_match(const char *path);
int gnubg_mobile_load_position(const char *path);
int gnubg_mobile_save_position(const char *path);

/* ── Engine algorithms (Tier 2) ───────────────────────────────────────────── */
int gnubg_mobile_get_legal_moves(const int board[50], int d0, int d1,
                                 int f_partial, int *out_moves, int out_cap);
int gnubg_mobile_find_move(const int old_board[50], const int cur_board[50],
                           int d0, int d1, char *out_text, int out_cap);
int gnubg_mobile_evaluate(const int board[50], float *out, int out_cap);
int gnubg_mobile_classify(const int board[50]);
int gnubg_mobile_cube_decision(const int board[50], int cube_value,
                               int cube_owner, int f_move, int match_to,
                               int score0, int score1, int crawford,
                               float *out, int out_cap, int *out_decision);
int gnubg_mobile_rollout(const int board[50], int trials,
                         float *out, int out_cap);

/* Initialise the facade's default cubeinfo (call once at engine init). */
void gnubg_mobile_set_default_cubeinfo(void);

#ifdef __cplusplus
}
#endif

#endif
