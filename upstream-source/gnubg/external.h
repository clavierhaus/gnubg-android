/*
 * Copyright (C) 2001-2002 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2003-2020 the AUTHORS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * $Id: external.h,v 1.26 2021/06/30 21:37:08 plm Exp $
 */

#ifndef EXTERNAL_H
#define EXTERNAL_H

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <glib.h>
#include <glib-object.h>
#include "backgammon.h"

/* Stuff for the yacc/lex parser */
extern void ExtStartParse(void *scanner, const char *szCommand);
extern int ExtInitParse(void **scancontext);
extern void ExtDestroyParse(void *scancontext);

#define MAX_RFBF_ELEMENTS 53

#define KEY_STR_BEAVERS "beavers"
#define KEY_STR_RESIGNATION "resignation"
#define KEY_STR_DETERMINISTIC "deterministic"
#define KEY_STR_JACOBYRULE "jacobyrule"
#define KEY_STR_CRAWFORDRULE "crawfordrule"
#define KEY_STR_PRUNE "prune"
#define KEY_STR_NOISE "noise"
#define KEY_STR_CUBEFUL "cubeful"
#define KEY_STR_PLIES "plies"
#define KEY_STR_NEWINTERFACE "newinterface"
#define KEY_STR_DEBUG "debug"
#define KEY_STR_PROMPT "prompt"

typedef enum {
    COMMAND_NONE = 0,
    COMMAND_FIBSBOARD = 1,
    COMMAND_EVALUATION = 2,
    COMMAND_EXIT = 3,
    COMMAND_VERSION = 4,
    COMMAND_SET = 5,
    COMMAND_HELP = 6,
    COMMAND_LIST = 7
} cmdtype;

typedef struct {
    cmdtype cmdType;
    void *pvData;
} commandinfo;

/* 
 * When sending positions to be evaluated by GNUbg as external player,
 * they are sent using the struct below in the following way:
 * - anFIBSBoard is from player 1 point of view in all cases
 * - the unnamed player in the struct fields is the external player and Opp
 *   is the other player from the instance connecting to it,
 *   except in the take case (where what is really evaluated is the full
 *   doubling decision, take/pass and take/drop, from the doubling side)
 *
 * For instance, with the external player as player 0 winning the opening roll,
 * the short game 52 (split) / 55 / fans / double / pass would have the strings
 * starting with "board:" sent to it:
 * 
 * player0 : 52
 * board:<player0>:<player1>:0:0:0:0:-2:0:0:0:0:5:0:3:0:0:0:-5:5:0:0:0:-3:0:-5:0:0:0:0:2:0:-1:5:2:5:2:1:1:1:0:1:-1:0:25:0:0:0:0:0:0:0:1
 * 24/22 13/8
 * 
 * player1 : 55
 * 13/8*(2) 6/1*(2)
 * 
 * player0 : double ?
 * board:<player0>:<player1>:0:0:0:-2:2:0:2:0:0:3:0:1:0:0:0:-4:5:0:0:0:-4:0:-5:0:0:0:0:2:0:-1:0:0:0:0:1:1:1:0:1:-1:0:25:0:0:0:0:0:0:0:1
 * no double
 * 
 * player0 : 66
 * board:<player0>:<player1>:0:0:0:-2:2:0:2:0:0:3:0:1:0:0:0:-4:5:0:0:0:-4:0:-5:0:0:0:0:2:0:-1:6:6:6:6:1:1:1:0:1:-1:0:25:0:0:0:0:0:0:0:1
 * fans
 * 
 * player1 : double
 * 
 * player0 : take ?
 * board:<player1>:<player0>:0:0:0:-2:2:0:2:0:0:3:0:1:0:0:0:-4:5:0:0:0:-4:0:-5:0:0:0:0:2:0:1:0:0:0:0:1:1:1:1:1:-1:0:25:0:0:0:0:0:0:0:1
 */

typedef struct {
    /*
     * These must be in the same order than the FIBS board string definition,
     * starting at Match Length.
     * See description at http://www.fibs.com/fibs_interface.html#board_state
     */
    int nMatchTo;
    int nScore;
    int nScoreOpp;
    int anFIBSBoard[26];
    int nTurn;
    int anDice[2];
    int anOppDice[2];
    int nCube;
    int fCanDouble;
    int fOppCanDouble;
    int fDoubled;
    int nColor;
    int nDirection;
    int nHome;                  /* unused, set to  0 */
    int nBar;                   /* unused, set to 25 */
    int nOnHome;                /* unused */
    int nOnHomeOpp;             /* unused */
    int nOnBar;                 /* unused */
    int nOnBarOpp;              /* unused */
    int nCanMove;               /* unused */
    int fForcedMOve;            /* unused */
    int fPostCrawford;
    /*
     * Redoubles from FIBS board is not usable by GNUbg.
     * A flag for match play is put at the same place
     */
    int fNonCrawford;
    int nVersion;
    int padding[51];

    /* These must be last */
    GString *gsName;
    GString *gsOpp;
} FIBSBoardInfo;

typedef struct {
    char szPlayer[MAX_NAME_LEN];
    char szOpp[MAX_NAME_LEN];
    int nMatchTo;
    int nScore;
    int nScoreOpp;
    int anDice[2];
    int nCube;
    int fCubeOwner;
    int fDoubled;
    int fCrawford;
    int fJacoby;
    int nResignation;
    TanBoard anBoard;
} ProcessedFIBSBoard;

typedef struct scancontext {
    /* scanner ptr must be first element in structure */
    void *scanner;
    void (*ExtErrorHandler) (struct scancontext *, const char *);
    int fError;
    int fDebug;
    int fNewInterface;
    char *szError;

    /* command type */
    cmdtype ct;
    void *pCmdData;

    /* evalcontext */
    int nPlies;
    float rNoise;
    int fDeterministic;
    int fCubeful;
    int fUsePrune;

    /* session rules */
    int fJacobyRule;
    int fCrawfordRule;
    int nResignation;
    int fBeavers;

    /* fibs board */
    union {
        FIBSBoardInfo bi;
        int anList[50];
    };
} scancontext;

#if HAVE_SOCKETS

#ifndef WIN32

#define closesocket close

#if HAVE_SYS_SOCKET_H
#include <sys/types.h>
#include <sys/socket.h>
#endif                          /* #if HAVE_SYS_SOCKET_H */

#else                           /* #ifndef WIN32 */
#include <winsock2.h>
#include <ws2tcpip.h>
#endif                          /* #ifndef WIN32 */

#define EXTERNAL_INTERFACE_VERSION "2"
#define RFBF_VERSION_SUPPORTED "0"

extern int ExternalSocket(struct sockaddr **ppsa, socklen_t *pcb, char *sz);
extern int ExternalRead(int h, char *pch, size_t cch);
extern int ExternalWrite(int h, char *pch, size_t cch);
#ifdef WIN32
extern void OutputWin32SocketError(const char *action);
#define SockErr OutputWin32SocketError
#else
#define SockErr outputerr
#endif

#endif                          /* #if HAVE_SOCKETS */

#endif                          /* #ifndef EXTERNAL_H */
