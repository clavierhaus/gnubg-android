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
 * $Id: randomorg.h,v 1.4 2020/02/23 21:07:07 plm Exp $
 */

#ifndef RANDOMORG_H
#define RANDOMORG_H

#include <stdlib.h>

/*#define RANDOMORG_DEBUG 1 */

#define STRINGIZEAUX(num) #num
#define STRINGIZE(num) STRINGIZEAUX(num)

#define RANDOMORGSITE "www.random.org"
#define BUFLENGTH 500
#define BUFLENGTH_STR STRINGIZE(BUFLENGTH)

#define RANDOMORG_URL "https://" RANDOMORGSITE "/integers/?num=" \
	BUFLENGTH_STR "&min=0&max=5&col=1&base=10&format=plain&rnd=new"
#define RANDOMORG_USERAGENT "GNUbg/" VERSION " (" PACKAGE_BUGREPORT ")"
#define RANDOMORG_CERTPATH ".\\etc\\ssl\\"
#define RANDOMORG_CABUNDLE "ca-bundle.crt"

typedef struct {
    size_t nNumRolls;
    int nCurrent;
    unsigned int anBuf[BUFLENGTH];
} RandomData;

extern unsigned int getDiceRandomDotOrg(void);

#endif
