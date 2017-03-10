
# ---------------------------------------------------------------------
# Copyright (c) 2009, William Fonkou Tambe
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http:#www.gnu.org/licenses/>.
# ---------------------------------------------------------------------


# The options -fdata-sections -ffunction-sections -Wl,--gc-sections
# are used to respectively keep data and functions in separate sections
# which then allow --gc-sections to remove unused sections.
# The option -Wl,-s strip debug information and cannot be used with -g.
CFLAGS = -Werror -Wno-unused-result -fdata-sections -ffunction-sections -Wl,--gc-sections -Os -Wl,-s

LIBS = byt file mm pamsyn parsearg arrayuint arrayu8 string

BINS = srep

all: ${BINS}
	ls -lh ${BINS}

srep: src/srep.c
	for i in ${LIBS}; do I="$${I} src/lib/$${i}/$${i}.c"; done; \
	gcc ${CFLAGS} -I src/lib/inc -I src/lib/inc -o srep src/srep.c $${I};

package:
	PKGNAME=$$(basename "$$(pwd)"); cd .. && tar -cJf $${PKGNAME}.tar.xz -h $${PKGNAME}

install:
	for i in ${BINS}; do cp "$$(pwd)/$${i}" /bin/; done

uninstall:
	for i in ${BINS}; do rm -f /bin/$${i}; done

clean:
	rm ${BINS}; true
