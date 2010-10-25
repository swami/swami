#ifndef __I18N_H__
#define __I18N_H__

#include <config.h>

#ifndef _
#if defined(ENABLE_NLS)
#  include <libintl.h>
#  define _(x) dgettext(PACKAGE, x)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define N_(String) (String)
#  define _(x) (x)
#  define gettext(x) (x)
#endif
#endif

#endif
