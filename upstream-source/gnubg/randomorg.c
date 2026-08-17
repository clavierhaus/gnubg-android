/*
 * Copyright (C) 2015 Michael Petch <mpetch@capp-sysware.com>
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
 * $Id: randomorg.c,v 1.8 2020/08/25 17:29:27 plm Exp $
 */

#include "config.h"

#if defined(LIBCURL_PROTOCOL_HTTPS)

#include <ctype.h>
#include <curl/curl.h>

#include "backgammon.h"
#include "randomorg.h"


/*
 * Fetch random numbers from www.random.org
 */

static size_t
RandomOrgCallBack(void *pvRawData, size_t nSize, size_t nNumMemb, void *pvUserData)
{
    size_t nNewDataLen = nSize * nNumMemb;
    RandomData *pvRandomData = (RandomData *) pvUserData;
    unsigned int i;
    size_t iNumRead = pvRandomData->nNumRolls;
    char *szRawData = (char *) pvRawData;

#if defined(RANDOMORG_DEBUG)
    int count = 0;

    if (iNumRead > 0)
        outputl("");

    output("Random rolls received:");
#endif
    for (i = 0; i < nNewDataLen; i++) {
        if (iNumRead >= BUFLENGTH) {
            /* Prevent buffer overflow if random.org sent more than we asked */
            break;
        }
        if ((szRawData[i] >= '0') && (szRawData[i] <= '5')) {
            /* Get a number */
            pvRandomData->anBuf[iNumRead] = 1 + (unsigned int) (szRawData[i] - '0');
#if defined(RANDOMORG_DEBUG)
            if (count++ % 100 == 0)
                outputl("");

            outputf("%d", pvRandomData->anBuf[iNumRead]);
#endif
            iNumRead++;
        } else if (isspace(szRawData[i])) {
            /* Skip white space */
            continue;
        } else {
            /* Any other characters suggest invalid data
             * exit with error (0) */
            return 0;
        }
    }

    pvRandomData->nNumRolls = iNumRead;
    return nNewDataLen;
}

static RandomData randomData = { 0, -1, {0} };

unsigned int
getDiceRandomDotOrg(void)
{
    CURL *curl_handle;
    CURLcode nRetVal;
#if defined(WIN32)
    gchar *szWIN32_cert_path = NULL;
#endif
    if ((randomData.nCurrent >= 0) && ((unsigned int) randomData.nCurrent < randomData.nNumRolls)) {
        return randomData.anBuf[randomData.nCurrent++];
    }

    randomData.nNumRolls = 0;
    outputf(_("Fetching %d random numbers from <%s>\n"), BUFLENGTH, RANDOMORGSITE);
    outputx();

    /* Initialize the curl session */
    curl_handle = curl_easy_init();

#if defined(WIN32)
    /* Set location of certificate bundle */
    szWIN32_cert_path = g_build_filename(RANDOMORG_CERTPATH, RANDOMORG_CABUNDLE, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_CAINFO, szWIN32_cert_path);
#endif

#if defined(RANDOMORG_DEBUG)
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
#endif
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_URL, RANDOMORG_URL);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, RandomOrgCallBack);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &randomData);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, RANDOMORG_USERAGENT);

    nRetVal = curl_easy_perform(curl_handle);
#if defined(RANDOMORG_DEBUG)
    outputf("\n\n");
#endif

#if defined(WIN32)
    g_free(szWIN32_cert_path);
#endif

    /* check if an error condition exists */
    if (nRetVal != CURLE_OK) {
        curl_easy_cleanup(curl_handle);
        outputerrf("curl_easy_perform() failed: %s\n", curl_easy_strerror(nRetVal));
        randomData.nCurrent = -1;
        return 0;
    }

    /* Cleanup curl session */
    curl_easy_cleanup(curl_handle);

    randomData.nCurrent = 1;
    return randomData.anBuf[0];
}

#endif
