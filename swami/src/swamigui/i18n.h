#ifndef __SWAMIGUI_I18N_H__
#define __SWAMIGUI_I18N_H__

#include <config.h>

#ifndef _
#if defined(ENABLE_NLS)
#  include <libintl.h>
#  define _(x) gettext(x)
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

#endif /* __SWAMIGUI_I18N_H__ */
