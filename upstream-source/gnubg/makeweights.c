/*
 * Copyright (C) 2000-2001 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2002-2019 the AUTHORS
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
 * $Id: makeweights.c,v 1.31 2022/02/02 22:04:12 plm Exp $
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <locale.h>

#include "eval.h"               /* for WEIGHTS_VERSION */
#include "output.h"

static void
usage(char *prog)
{
    g_printerr(_("Usage: %s [[-f] outputfile [inputfile]]\n"
            "  outputfile: Output to file instead of stdout\n"
            "  inputfile: Input from file instead of stdin\n"), prog);
    exit(1);
}

extern int
main(int argc, /*lint -e{818} */ char *argv[])
{
    neuralnet nn;
    char szFileVersion[16];
    static float ar[2] = { WEIGHTS_MAGIC_BINARY, WEIGHTS_VERSION_BINARY };
    int c;
    FILE *in = stdin, *out = stdout;

    if (!setlocale(LC_ALL, "C") || !bindtextdomain(PACKAGE, LOCALEDIR) || !textdomain(PACKAGE)) {
        perror("setting locale failed");
        exit(1);
    }

    g_set_printerr_handler(print_utf8_to_locale);

    if (argc > 1) {
        int arg = 1;
        if (!StrCaseCmp(argv[1], "-f"))
            arg++;              /* Skip */

        if (argc > arg + 2)
            usage(argv[0]);
        if ((out = g_fopen(argv[arg], "wb")) == 0) {
	    g_printerr(_("Can't open output file"));
            exit(1);
        }
        if (argc == arg + 2) {
            if ((in = g_fopen(argv[arg + 1], "r")) == 0) {
	        g_printerr(_("Can't open input file"));
                exit(1);
            }
        }
    }

    /* generate weights */

    if (fscanf(in, "GNU Backgammon %15s\n", szFileVersion) != 1) {
        g_printerr(_("%s: invalid weights file\n"), argv[0]);
        fclose(in);
        fclose(out);
        return EXIT_FAILURE;
    }

    if (StrCaseCmp(szFileVersion, WEIGHTS_VERSION)) {
        g_printerr(_("%s: incorrect weights version\n"
                     "(version %s is required, "
                     "but these weights are %s)\n"), argv[0], WEIGHTS_VERSION, szFileVersion);
        fclose(in);
        fclose(out);
        return EXIT_FAILURE;
    }

    if (fwrite(ar, sizeof(ar[0]), 2, out) != 2) {
        g_printerr(_("Failed to write neural net!"));
        fclose(in);
        fclose(out);
        return EXIT_FAILURE;
    }

    for (c = 0; !feof(in); c++) {
        if (NeuralNetLoad(&nn, in) == -1) {
            g_printerr(_("Failed to load neural net!"));
            fclose(in);
            fclose(out);
            return EXIT_FAILURE;
        }
        if (NeuralNetSaveBinary(&nn, out) == -1) {
            g_printerr(_("Failed to save neural net!"));
            fclose(in);
            fclose(out);
            return EXIT_FAILURE;
        }
        NeuralNetDestroy(&nn);
    }

    g_printerr(_("%d nets converted\n"), c);

    fclose(in);
    fclose(out);

    return EXIT_SUCCESS;

}
