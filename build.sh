
NAME=ghostbiff
TARGET=$NAME.dll
OBJS=$NAME.o
PKG=sylpheed-$NAME
LIBSYLPH=./lib/libsylph-0-1.a
LIBSYLPHEED=./lib/libsylpheed-plugin-0-1.a
#LIBS=" -lglib-2.0-0  -lintl"
LIBS=" `pkg-config --libs glib-2.0 gobject-2.0 gtk+-2.0 gthread-2.0`"
INC=" -I. -I../../ -I../../libsylph -I../../src `pkg-config --cflags glib-2.0 cairo gdk-2.0 gtk+-2.0 gthread-2.0`"

USE_AQUESTALK=1
DEF=" -DHAVE_CONFIG_H -DUNICODE -D_UNICODE "
DEBUG=0
LIB_AQUESTALK=""
if [ "$USE_AQUESTALK" -eq 1 ]; then
    DEF="$DEF -DUSE_AQUESTALK -DMULTI_STR_CODE"
    LIB_AQUESTALK=" -lAquesTalk2 -lAquesTalk2Da -lAqKanji2Koe AquesTalk2Da.dll AqKanji2Koe.dll"
fi

function build()
{

    if [ $DEBUG -eq 1 ]; then
        com="gcc -std=c99 -Wall -g -c $DEF $INC $NAME.c"
    else
        com="gcc -std=c99 -Wall -c $DEF $INC $NAME.c"
    fi
    echo $com
    eval $com
    if [ $? != 0 ]; then
        echo "compile error"
        exit
    fi
    com="gcc -std=c99 -shared -o $TARGET $OBJS -L./lib $LIBSYLPH $LIBSYLPHEED $LIBS -lssleay32 -leay32 -lws2_32 -liconv -lonig "
    echo $com
    eval $com
    if [ $? != 0 ]; then
        echo "done"
    else
        DEST="/C/Users/$LOGNAME/AppData/Roaming/Sylpheed/plugins"
        if [ -d "$DEST" ]; then
            com="cp $TARGET $DEST/ghostbiff.dll"
            echo $com
            eval $com
        fi
        DEST="/C/Documents and Settings/$LOGNAME/Application Data/Sylpheed/plugins"
        if [ -d "$DEST" ]; then
            com="cp $TARGET \"$DEST/ghostbiff.dll\""
            echo $com
            eval $com
        fi
    fi
}

if [ ! -z "$1" ]; then
  case "$1" in
      pot)
          mkdir -p po
          com="xgettext $NAME.c -k_ -kN_ -o po/$NAME.pot"
          ;;
      po)
          com="msgmerge po/ja.po po/$NAME.pot -o po/ja.po"
          ;;
      mo)
          com="msgfmt po/ja.po -o po/$NAME.mo"
          echo $com
          eval $com
          DEST="/C/apps/Sylpheed/lib/locale/ja/LC_MESSAGES"
          if [ -d "$DEST" ]; then
              com="cp po/$NAME.mo $DEST/ghostbiff.mo"
              echo $com
              eval $com
          fi
          exit
          ;;
      sstp)
          com="gcc -o directsstp.exe directsstp.c $INC $LIBS"
          ;;
      hello)
          com="gcc -o hellotalk.exe hellotalk.c $DEF $INC $LIBS -L./lib $LIB_AQUESTALK"
          ;;
      release)
          zip $PKG-$2.zip $TARGET
          zip -r $PKG-$2.zip README.ja.txt
          #zip -r $PKG-$2.zip ghostbiff.c
          #zip -r $PKG-$2.zip po/ghostbiff.mo
          zip -r $PKG-$2.zip *.xpm
          ;;
      debug)
          DEBUG=1
          DEF="$DEF -DDEBUG"
          build
          ;;
  esac
  echo $com
  eval $com
else
    build
fi