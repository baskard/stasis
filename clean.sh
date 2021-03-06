#! /bin/sh
make distclean
rm -rf Makefile.in  aclocal.m4  autom4te.cache  config.h.in  configure  depcomp  hello.c  install-sh  missing doc/api doc/developers doc/coverage autoscan.log configure.scan
find . | grep \~$ | xargs rm -f
find . -name '*.bb' | xargs rm -f
find . -name '*.bbg' | xargs rm -f
find . -name '*.da' | xargs rm -f
find . -name '*.o'  | xargs rm -f
find . -name '*.lo'  | xargs rm -f
find . -name '*.Plo'  | xargs rm -f
find . | perl -ne 'print if (/\/core(\.\d+)?$/)' | xargs rm -f
find . | perl -ne 'print if (/\/Makefile.in$/)' | xargs rm -f
find . | perl -ne 'print if (/\/storefile.txt$/)' | xargs rm -f
find . | perl -ne 'print if (/\/logfile.txt$/)' | xargs rm -f
find . | perl -ne 'print if (/\/blob._file.txt$/)' | xargs rm -f
rm -f test/gmon.out test/lladd/gmon.out
rm -f configure.in
rm -f config.guess config.h config.log config.status config.sub install-sh ltmain.sh mkinstalldirs py-compile
rm -f m4/ltsugar.m4 m4/libtool.m4 m4/ltversion.m4 m4/lt~obsolete.m4 m4/ltoptions.m4 missing stamp-h1
