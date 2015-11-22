#!/bin/bash
set -e
CC=gcc
hash colorgcc 2>/dev/null && { CC=colorgcc; }
CFLAGS="-Wall -Wextra -pedantic -std=gnu99 -fPIC -g `pkg-config --cflags sdl2 gl` -I. -lSDL2_image -lSDL2_mixer -lm"
LDLIBS="`pkg-config --libs sdl2 gl` -lSDL2_image -lSDL2_mixer -lm"

cat <<EOF >Makefile
.PHONY: default clean

CC=$CC
CFLAGS=$CFLAGS
LDLIBS=$LDLIBS

OBJECTS= gut.o
EOF

printf "DEMOS=" >>Makefile
REFFILES=`cd demo && find . -name '*.c'`
for i in $REFFILES; do
	printf " \\\\\n\tdemo/%s" `echo $i | cut -c3- | sed -e 's/\.c$//'` >>Makefile
done
cat <<EOF >>Makefile

default: libgut.a libgut.so \$(DEMOS)
libgut.a: \$(OBJECTS)
	ar -cr libgut.a \$(OBJECTS)
libgut.so: \$(OBJECTS)
	\$(CC) -shared -Wl,-soname,libgut.so -o libgut.so \$(OBJECTS)
%.o: %.c
	\$(CC) -c \$< -o \$@ \$(CFLAGS)
EOF

for i in $REFFILES; do
	j=`echo $i | cut -c3-`
	o=`echo $j | sed -e 's/\.c$//'`
	echo "demo/$o: demo/$o.o libgut.a" >>Makefile
	echo "demo/`$CC -MM -I. demo/$j`" >>Makefile
done

cat <<EOF >>Makefile

source:
EOF

printf '\ttar zcf src.tgz build.sh' >>Makefile
for i in `find . -type f | grep -P "\.(c|h)$" | sed -e 's/\.\///'`; do
	printf " \\\\\n\t$i" >>Makefile
done

cat <<EOF >>Makefile

clean:
	rm -f libgut.a libgut.so src.tgz
	rm -f \$(OBJECTS) *.o \$(DEMOS)
EOF
for i in $REFFILES; do
	j=`echo $i | cut -c3- | sed -e 's/c$/o/'`
	echo "	rm -f demo/$j" >>Makefile
done
