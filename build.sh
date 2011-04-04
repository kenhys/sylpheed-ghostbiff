
TARGET=ghostbiff.dll
OBJS=ghostbiff.o
PKG=sylpheed-ghostbiff
LIBSYLPH=./lib/libsylph-0-1.a
LIBSYLPHEED=./lib/libsylpheed-plugin-0-1.a
#LIBS=" -lglib-2.0-0  -lintl"
LIBS=" `pkg-config --libs glib-2.0 gobject-2.0 gtk+-2.0`"
INC=" -I. -I../../ -I../../libsylph -I../../src `pkg-config --cflags glib-2.0 cairo gdk-2.0 gtk+-2.0`"
DEF=" -DHAVE_CONFIG_H"
if [ -z "$1" ]; then
    com="gcc -Wall -c $DEF $INC ghostbiff.c"
    echo $com
    eval $com
    if [ $? != 0 ]; then
        echo "compile error"
        exit
    fi
    com="gcc -shared -o $TARGET $OBJS -L./lib $LIBSYLPH $LIBSYLPHEED $LIBS -lssleay32 -leay32 -lws2_32 -liconv -lonig"
    echo $com
    eval $com
fi

if [ ! -z "$1" ]; then
  case "$1" in
      pot)
          mkdir -p po
          com="xgettext ghostbiff.c -k_ -kN_ -o po/ghostbiff.pot"
          ;;
      po)
          com="msgmerge po/ja.po po/ghostbiff.pot -o po/ja.po"
          ;;
      mo)
          com="msgfmt po/ja.po -o po/ghostbiff.mo"
          ;;
      sstp)
          com="gcc -o directsstp.exe directsstp.c $INC $LIBS"
          ;;
      release)
          zip $PKG-$2.zip ghostbiff.dll
          zip -r $PKG-$2.zip README.ja.txt
          #zip -r $PKG-$2.zip ghostbiff.c
          #zip -r $PKG-$2.zip po/ghostbiff.mo
          zip -r $PKG-$2.zip *.xpm
          ;;
  esac
  echo $com
  eval $com
fi