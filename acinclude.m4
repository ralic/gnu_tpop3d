dnl This is copyright Erez Zadok, taken straight from his am-utils-6.0.7
dnl package.  The license for am-utils-6.0.7 is reproduced below:


dnl Copyright (c) 1997-2001 Erez Zadok
dnl Copyright (c) 1989 Jan-Simon Pendry
dnl Copyright (c) 1989 Imperial College of Science, Technology & Medicine
dnl Copyright (c) 1989 The Regents of the University of California.
dnl All rights reserved.
dnl 
dnl This code is derived from software contributed to Berkeley by
dnl Jan-Simon Pendry at Imperial College, London.
dnl 
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl 3. All advertising materials mentioning features or use of this software
dnl    must display the following acknowledgment:
dnl      This product includes software developed by the University of
dnl      California, Berkeley and its contributors, as well as the Trustees of
dnl      Columbia University.
dnl 4. Neither the name of the University nor the names of its contributors
dnl    may be used to endorse or promote products derived from this software
dnl    without specific prior written permission.
dnl 
dnl THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
dnl ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
dnl FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
dnl OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
dnl HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
dnl LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
dnl OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
dnl SUCH DAMAGE.

dnl ---------------------------------------------------------------------------

dnl
dnl I have been advised by Erek Zadoc, via Mark Longair, that it is OK to
dnl distribute this as part of a GPL'd package, tpop3d. For completeness, I
dnl include the relevant email:
dnl
dnl Message-Id: <200204091948.g39JmhSc026604@shekel.mcl.cs.columbia.edu>
dnl In-Reply-To: Your message of
dnl     "Tue, 09 Apr 2002 17:22:52 BST."
dnl     <15539.5468.644567.908003@gin.house.local>
dnl From: Erez Zadok <ezk@cs.columbia.edu>
dnl To: Mark Longair <mhl@pobox.com>
dnl Cc: Erez Zadok <ezk@cs.columbia.edu>
dnl Subject: Re: license for check_lib2.m4 ?
dnl Date: Tue, 9 Apr 2002 15:48:43 -0400 (EDT)
dnl MIME-Version: 1.0
dnl 
dnl In message <15539.5468.644567.908003@gin.house.local>, Mark Longair writes:
dnl > Erez,
dnl > 
dnl > A while ago I added autoconf and automake support for a POP3 server
dnl > called tpop3d [1] which is released under the GPL.  It turns out that a
dnl > problem with figuring out which libraries are required on Solaris is
dnl > very neatly solved by your code in check_lib2.m4 from am-utils.
dnl > However, I'm not sure whether the license of am-utils would allow me to
dnl > include that file in a GPL-ed package - could you possibly clarify that
dnl > for me?
dnl > 
dnl > many thanks,
dnl > mark
dnl > 
dnl > [1] http://www.ex-parrot.com/~chris/tpop3d/
dnl 
dnl I haven't thought about it specifically, but since some of my macros have
dnl gone into other autotools, and I'd like some others to go there too, I think
dnl GPL makes the most sense.  Is GPL ok w/ you?
dnl 
dnl There's no risk making the macros GPL'ed even for people who don't like the
dnl GPL, b/c to use a package, one only needs the configure script that's
dnl generated out of these macros, not the .m4 code itself.
dnl 
dnl As for this check_lib2.m4 function, a few of us tried to convince the
dnl autoconf maintainers that our version makes more sense as the default
dnl behavior; but they still wanted to keep the older macro w/ the older
dnl behavior, which I think is just wrong.  Sooner or later the official
dnl check_lib will change or there'd a new macro offered; more and more people
dnl like the more logical behavior that my macro has.
dnl 
dnl Cheers,
dnl Erez.
dnl 

dnl ---------------------------------------------------------------------------

dnl a bug-fixed version of autoconf 2.12.
dnl first try to link library without $5, and only of that failed,
dnl try with $5 if specified.
dnl it adds $5 to $LIBS if it was needed -Erez.
dnl AC_CHECK_LIB2(LIBRARY, FUNCTION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl              [, OTHER-LIBRARIES]]])
AC_DEFUN(AC_CHECK_LIB2,
[AC_MSG_CHECKING([for $2 in -l$1])
dnl Use a cache variable name containing both the library and function name,
dnl because the test really is for library $1 defining function $2, not
dnl just for library $1.  Separate tests with the same $1 and different $2s
dnl may have different results.
ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ac_cv_lib_$ac_lib_var,
[ac_save_LIBS="$LIBS"

# first try with base library, without auxiliary library
LIBS="-l$1 $LIBS"
AC_TRY_LINK(dnl
ifelse([$2], [main], , dnl Avoid conflicting decl of main.
[/* Override any gcc2 internal prototype to avoid an error.  */
]
[/* We use char because int might match the return type of a gcc2
    builtin and then its argument prototype would still apply.  */
char $2();
]),
	    [$2()],
	    eval "ac_cv_lib_$ac_lib_var=\"$1\"",
	    eval "ac_cv_lib_$ac_lib_var=no")

# if OK, set to no auxiliary library, else try auxiliary library
if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = no"; then
 LIBS="-l$1 $5 $LIBS"
 AC_TRY_LINK(dnl
 ifelse([$2], [main], , dnl Avoid conflicting decl of main.
 [/* Override any gcc2 internal prototype to avoid an error.  */
 ]
 [/* We use char because int might match the return type of a gcc2
     builtin and then its argument prototype would still apply.  */
 char $2();
 ]),
 	    [$2()],
 	    eval "ac_cv_lib_$ac_lib_var=\"$1 $5\"",
 	    eval "ac_cv_lib_$ac_lib_var=no")
fi

LIBS="$ac_save_LIBS"
])dnl
ac_tmp="`eval echo '$''{ac_cv_lib_'$ac_lib_var'}'`"
if test "${ac_tmp}" != no; then
  AC_MSG_RESULT(-l$ac_tmp)
  ifelse([$3], ,
[changequote(, )dnl
  ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED($ac_tr_lib)
  LIBS="-l$ac_tmp $LIBS"
], [$3])
else
  AC_MSG_RESULT(no)
ifelse([$4], , , [$4
])dnl
fi

])

