dnl XMMS_FUNC_POSIX(FUNCTION... [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for functions in posix libraries, and define POSIX_LIBS
AC_DEFUN([XMMS_FUNC_POSIX],
[dnl
    xmms_posix4=no
    xmms_rt=no
    for xmms_func in $1
    do
        xmms_have_func=no
        AC_CHECK_FUNC($xmms_func,xmms_have_func=yes,[
            dnl try in libposix4 if not found in current LIBS
            AC_CHECK_LIB(posix4,$xmms_func,[xmms_have_func=yes xmms_posix4=yes],[
                dnl try in librt, if not found so far
                AC_CHECK_LIB(rt,$xmms_func,[xmms_have_func=yes xmms_rt=yes])
            ])
        ])
        if test $xmms_have_func = yes; then
            changequote(, )dnl
            xmms_tr_func=HAVE_`echo $xmms_func | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
            changequote([, ])dnl
            AC_DEFINE_UNQUOTED($xmms_tr_func)
            ifelse([$2], , :, [$2])
        else
            ifelse([$3], , :, [$3])
        fi
    done
    POSIX_LIBS=
    if test $xmms_posix4 = yes; then
        POSIX_LIBS=-lposix4
    fi
    if test $xmms_rt = yes; then
        POSIX_LIBS="$POSIX_LIBS -lrt"
    fi
    AC_SUBST(POSIX_LIBS)
])

dnl Checks whether the stack can be marked nonexecutable by passing an option
dnl to the C-compiler when acting on .s files. Appends that option to CCASFLAGS.
dnl This macro is adapted from one found in GLIBC-2.3.5.
AC_DEFUN([XMMS_AS_NOEXECSTACK],[
AC_CACHE_CHECK([whether assembler supports --noexecstack option],
xmms_cv_as_noexecstack, [dnl
  cat > conftest.c <<EOF
void foo() {}
EOF
  if AC_TRY_COMMAND([${CC} $CFLAGS $CPPFLAGS
                     -S -o conftest.s conftest.c >/dev/null]) \
     && grep -q .note.GNU-stack conftest.s \
     && AC_TRY_COMMAND([${CC} $CFLAGS $CPPFLAGS -Wa,--noexecstack
                       -c -o conftest.o conftest.s >/dev/null])
  then
    xmms_cv_as_noexecstack=yes
  else
    xmms_cv_as_noexecstack=no
  fi
  rm -f conftest*])
  if test "$xmms_cv_as_noexecstack" = yes; then
    CCASFLAGS="$CCASFLAGS -Wa,--noexecstack"
  fi
])

