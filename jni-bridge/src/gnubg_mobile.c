#include <pthread.h>
#include <stdio.h>

#include "gnubg_mobile.h"

#include "backgammon.h"

extern void CommandNewGame(char *);
extern void CommandNewMatch(char *);
extern void CommandNewSession(char *);
extern void CommandEndGame(char *);
extern void CommandResign(char *);
extern void CommandNext(char *);
extern void CommandAccept(char *);
extern void CommandReject(char *);
extern void CommandDecline(char *);
extern void CommandAgree(char *);
extern void CommandRedouble(char *);
extern void CommandDouble(char *);
extern void CommandTake(char *);
extern void CommandDrop(char *);
extern void CommandRoll(char *);
extern void CommandMove(char *);
extern int NextTurn(int fPlayNext);
extern void ClearMatch(void);
extern int ListCreate(listOLD *pl);
extern listOLD lMatch;
extern int fNextTurn;
extern pthread_mutex_t gnubg_lock;

const char *gnubg_mobile_facade_version(void) {
    return "gnubg-mobile-facade-v0";
}

static void gnubg_mobile_drain_next_turns(void) {
    while (fNextTurn) {
        NextTurn(TRUE);
    }
}

int gnubg_mobile_command_new_game(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandNewGame(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_new_match(int match_length) {
    char sz_match[16];

    snprintf(sz_match, sizeof(sz_match), "%d", match_length);

    pthread_mutex_lock(&gnubg_lock);
    CommandNewMatch(sz_match);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_new_session(int games) {
    char sz_games[16];

    snprintf(sz_games, sizeof(sz_games), "%d", games);

    pthread_mutex_lock(&gnubg_lock);
    CommandNewSession(sz_games);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_end_game(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandEndGame(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_resign(const char *value) {
    pthread_mutex_lock(&gnubg_lock);
    CommandResign((char *)(value ? value : ""));
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_next(const char *argument) {
    pthread_mutex_lock(&gnubg_lock);
    CommandNext((char *)(argument ? argument : ""));
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_accept(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandAccept(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_reject(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandReject(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_decline(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandDecline(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_agree(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandAgree(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_redouble(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandRedouble(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_double(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandDouble(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_roll(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandRoll(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_move(const char *move) {
    pthread_mutex_lock(&gnubg_lock);
    CommandMove((char *)(move ? move : ""));
    NextTurn(TRUE);
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_take(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandTake(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_drop(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandDrop(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_start_match(int match_length) {
    char szMatch[16];

    snprintf(szMatch, sizeof(szMatch), "%d", match_length);

    pthread_mutex_lock(&gnubg_lock);
    ListCreate(&lMatch);
    ClearMatch();
    CommandNewMatch(szMatch);
    CommandNewGame(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_next_game(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandNext("");
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

