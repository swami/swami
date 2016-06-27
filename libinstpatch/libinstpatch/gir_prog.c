#include <glib.h>
#include <girepository.h>
#include <libinstpatch/libinstpatch.h>


int
main (int argc, char *argv[])
{
  GError *err = NULL;
  char *irdump = NULL;
  GOptionContext *context;

  GOptionEntry entries[] = {
    { "introspect-dump", 'i', 0, G_OPTION_ARG_STRING, &irdump, NULL, NULL },
    { NULL }
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, "libinstpatch-gir-program");

  if (!g_option_context_parse (context, &argc, &argv, &err))
  {
    g_print ("option parsing failed: %s\n", err->message);
    return (1);
  }

  ipatch_init ();

  if (!g_irepository_dump (irdump, &err))
  {
    g_print ("g_irepository_dump() failed: %s\n", err->message);
    return (1);
  }

  return (0);
}

