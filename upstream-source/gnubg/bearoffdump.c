/*
 * Copyright (C) 2004 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2004-2020 the AUTHORS
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
 * $Id: bearoffdump.c,v 1.29 2022/01/27 22:14:49 plm Exp $
 */

#include "config.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include "positionid.h"
#include "bearoff.h"
#include "multithread.h"
#include "backgammon.h"
#include "drawboard.h"

extern void
MT_CloseThreads(void)
{
    return;
}

extern int
main(int argc, char **argv)
{

    char *filename, *szPosID = NULL;
    unsigned int id = 0;
    bearoffcontext *pbc;
    char sz[4096];
    TanBoard anBoard;

    GOptionEntry ao[] = {
        {"index", 'n', 0, G_OPTION_ARG_INT, &id,
         N_("index"), NULL},
        {"posid", 'p', 0, G_OPTION_ARG_STRING, &szPosID,
         N_("Position ID"), NULL},
        {NULL, 0, 0, (GOptionArg) 0, NULL, NULL, NULL}
    };
    GError *error = NULL;
    GOptionContext *context;

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    g_set_print_handler(print_utf8_to_locale);
    g_set_printerr_handler(print_utf8_to_locale);

    context = g_option_context_new("file");
    g_option_context_add_main_entries(context, ao, PACKAGE);
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);
    if (error) {
        g_printerr("%s\n", error->message);
        exit(EXIT_FAILURE);
    }

    if ((szPosID && id) || (!szPosID && !id)) {
        g_printerr(_("Either Position ID or index is required.\n" "For more help try `bearoffdump --help'\n"));
        exit(EXIT_FAILURE);
    }

    if (argc != 2) {
        g_printerr(_("A bearoff database file should be given as an argument\n"
                   "For more help try `bearoffdump --help'\n"));
        exit(EXIT_FAILURE);
    }
    filename = argv[1];

    g_print(_("Bearoff database: %s\n"), filename);
    if (!id) {
        g_print(_("Position ID     : %s\n"), szPosID);
    } else {
        g_print(_("Position number : %u\n"), id);
    }

    /* This is needed since we call ReadBearoffFile() from bearoff.c */
    MT_InitThreads();

    if (!(pbc = BearoffInit(filename, BO_NONE, NULL))) {
        g_print(_("Failed to initialise bearoff database %s\n"), filename);
        exit(-1);
    }

    /* information about bearoff database */

    g_print(_("\n" "Information about database:\n\n"));

    *sz = 0;
    BearoffStatus(pbc, sz);
    puts(sz);

    /* set up board */

    memset(anBoard, 0, sizeof anBoard);

    if (!id) {
        g_print(_("\n" "Dump of position ID: %s\n\n"), szPosID);

        PositionFromID(anBoard, szPosID);
    } else {
        unsigned int n, nUs, nThem;

        g_print(_("\n" "Dump of position#: %u\n\n"), id);

        n = Combination(pbc->nPoints + pbc->nChequers, pbc->nPoints);
        nUs = id / n;
        nThem = id % n;
        PositionFromBearoff(anBoard[0], nThem, pbc->nPoints, pbc->nChequers);
        PositionFromBearoff(anBoard[1], nUs, pbc->nPoints, pbc->nChequers);
    }

    /* board */

    {
        char szOut[2048];
        char *apc[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        int nc = pbc->bt == BEAROFF_HYPERGAMMON ? pbc->nChequers : 15;

        puts(DrawBoard(szOut, (ConstTanBoard) anBoard, TRUE, apc, NULL, nc));
    }

    /* dump req. position */

    *sz = 0;
    BearoffDump(pbc, (ConstTanBoard) anBoard, sz);
    puts(sz);

    BearoffClose(pbc);

    return 0;

}
