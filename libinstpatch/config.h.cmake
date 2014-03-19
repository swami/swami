#ifndef CONFIG_H
#define CONFIG_H

/* Define to 1 if you have the <errno.h> header file. */
#cmakedefine HAVE_ERRNO_H @HAVE_ERRNO_H@

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H @HAVE_FCNTL_H@

/* Define to 1 if you have the <math.h> header file. */
#cmakedefine HAVE_MATH_H @HAVE_MATH_H@

/* Define to 1 if you have the <stdarg.h> header file. */
#cmakedefine HAVE_STDARG_H @HAVE_STDARG_H@

/* Define to 1 if you have the <stdio.h> header file. */
#cmakedefine HAVE_STDIO_H @HAVE_STDIO_H@

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H @HAVE_STDLIB_H@

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H @HAVE_STRING_H@

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H @HAVE_SYS_STAT_H@

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H @HAVE_UNISTD_H@

/* Define to 1 if you have the <windows.h> header file. */
#cmakedefine HAVE_WINDOWS_H @HAVE_WINDOWS_H@

/* Define if using the MinGW32 environment */
#cmakedefine MINGW32 @MINGW32@

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
#cmakedefine NO_MINUS_C_MINUS_O @NO_MINUS_C_MINUS_O@

/* Name of package */
#cmakedefine PACKAGE "@PACKAGE@"

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT @PACKAGE_BUGREPORT@

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME @PACKAGE_NAME@

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING @PACKAGE_STRING@

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME @PACKAGE_TARNAME@

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION @PACKAGE_VERSION@

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS @STDC_HEADERS@

/* Version number of package */
#cmakedefine VERSION @IPATCH_VERSION@

/* Enable this to include extra debug functions */
#cmakedefine IPATCH_DEBUG @IPATCH_DEBUG@

#endif /* CONFIG_H */

