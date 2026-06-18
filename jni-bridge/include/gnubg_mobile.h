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
int gnubg_mobile_command_roll(void);

#ifdef __cplusplus
}
#endif

#endif
