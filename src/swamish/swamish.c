/*
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */
#include <stdio.h>
#include <string.h>
#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <i18n.h>

typedef struct _SwamishCmd SwamishCmd;

typedef void (*SwamishCmdCallback)(SwamishCmd *command,
                                   char **args, int count);

struct _SwamishCmd
{
    char *command;		/* text of command */
    SwamishCmdCallback callback;	/* command callback function */
    char *syntax;			/* syntax description of command */
    char *descr;			/* description of command */
    char *help;			/* detailed help on command */
};


static void swamish_cmd_ls(SwamishCmd *command, char **args, int count);
static void swamish_cmd_pwd(SwamishCmd *command, char **args, int count);
static void swamish_cmd_quit(SwamishCmd *command, char **args, int count);


static SwamishCmd swamish_commands[] =
{
    {
        "cd", NULL,
        N_("cd PATH"), N_("Change current object"),
        N_("Change the current object\n"
           "The `PATH' parameter is the directory or object to change to.")
    },
    {
        "close", NULL,
        N_("close PATH [PATH2]..."), N_("Close instrument files"),
        N_("Close one or more files.\n"
           "`PATH' is a path to an instrument file.")
    },
    {
        "cp", NULL,
        N_("cp SRC [SRC2]... DEST"), N_("Copy objects"),
        N_("Copy one or more objects to a destination.\n"
           "`SRC' and `DEST' are paths to objects or directories.")
    },
    {
        "get", NULL,
        N_("get PATH [PATH2]... [NAME]..."), N_("Get object properties"),
        N_("Get an instrument object's property values.\n"
           "`PATH' is the path to an instrument object.\n"
           "Property names can be specified, all are listed if not given.")
    },
    {
        "help", NULL,
        N_("help"), N_("Get help"),
        N_("When you don't know what to do, get some help.")
    },
    {
        "ls", swamish_cmd_ls,
        N_("ls [PATH]..."), N_("List directory or object contents"),
        N_("List directory or instrument object children.\n"
           "The optional parameters can be objects and/or directories.\n"
           "If no parameters are given, the current directory is displayed.")
    },
    {
        "new", NULL,
        N_("new [TYPE]"), N_("Create a new instrument object"),
        N_("Create a new instrument object within the current path.\n"
           "`TYPE' is the type of object to create.\n"
           "Available types are displayed if not specified.")
    },
    {
        "pwd", swamish_cmd_pwd,
        N_("pwd"), N_("Print current object path"),
        N_("Displays the current directory or object path.")
    },
    {
        "quit", swamish_cmd_quit,
        N_("quit"), N_("Quit"),
        N_("Exit the Swami Shell")
    },
    {
        "rm", NULL,
        N_("rm PATH [PATH2]..."), N_("Remove files or objects"),
        N_("Remove one or more objects or files.\n"
           "`PATH' is a path to a directory or object.")
    },
    {
        "save", NULL,
        N_("save PATH [PATH2]..."), N_("Save instrument files"),
        N_("Save one or more instrument files.\n"
           "`PATH' is a path to an instrument file.")
    },
    {
        "saveas", NULL,
        N_("saveas PATH NEWPATH"), N_("Save instrument file as another file"),
        N_("Save an instrument file to a different name.\n"
           "`PATH' is a path to an instrument file.\n"
           "`NEWPATH' is a new file path to save to.")
    },
    {
        "set", NULL,
        N_("set PATH [PATH2]... NAME=VALUE..."), N_("Set object properties"),
        N_("Set properties of an instrument object.\n"
           "`PATH' is the path to an instrument object.\n"
           "One or more property `NAME=VALUE' pairs may be given.")
    },
};


gboolean exit_swamish = FALSE; // Set to TRUE to exit
char *current_dir = NULL;      // Current directory of current path
char *current_obj = NULL;      // Current obj of path or NULL (appended to dir)


rl_compentry_func_t *complete_command()
{
}

char *
get_cmd(void)
{
    char *line;

    line = readline("swami> ");

    if(line && *line)	   /* add line to history if it has any text */
    {
        add_history(line);
    }

    return (line);
}

int
main(void)
{
    char *line;
    SwamishCmd *cmdinfo;
    int i;

    current_dir = g_get_current_dir();

    do
    {
        line = get_cmd();

        for(i = 0; i < G_N_ELEMENTS(swamish_commands); i++)
        {
            cmdinfo = &swamish_commands[i];

            // Command matches?
            if(strcmp(cmdinfo->command, line) == 0)
            {
                break;
            }
        }

        free(line);

        // Was there a match and has a callback?
        if(i < G_N_ELEMENTS(swamish_commands))
        {
            if(cmdinfo->callback)
            {
                cmdinfo->callback(cmdinfo, NULL, 0);
            }
        }
        else
        {
            printf("Unknown command\n");
        }
    }
    while(!exit_swamish);

    printf("See ya!\n");

    return 0;
}

/* get a file listing for a directory (excluding '.' and '..' entries),
   returns NULL terminated array of strings which should be freed with
   g_strfreev() */
char **
get_path_contents(char *path, GError **err)
{
    GDir *dh;
    const char *fname;
    GPtrArray *file_array;
    char **retptr;

    g_return_val_if_fail(path != NULL, NULL);
    g_return_val_if_fail(!err || !*err, NULL);

    dh = g_dir_open(path, 0, err);

    if(!dh)
    {
        return (NULL);
    }

    file_array = g_ptr_array_new();

    while((fname = g_dir_read_name(dh)))
    {
        g_ptr_array_add(file_array, g_strdup(fname));
    }

    g_ptr_array_sort(file_array, (GCompareFunc)strcmp);   /* sort the array */
    g_ptr_array_add(file_array, NULL);   /* NULL terminate */

    g_dir_close(dh);

    retptr = (char **)(file_array->pdata);
    g_ptr_array_free(file_array, FALSE);

    return (retptr);
}

#if 0
static gboolean
parse_path(const char *path, char **ret_dir, char **ret_obj)
{
    char *fullpath = NULL;

    g_return_val_if_fail(path != NULL && *path, FALSE);

    if(!g_path_is_absolute(path))
    {
        fullpath = g_build_filename(current_dir, path, NULL);
        path = fullpath;
    }



    if(fullpath)
    {
        g_free(fullpath);
    }

    return (TRUE);
}

static void
swamish_cmd_cd(SwamishCmd *command, char **args, int count)
{

}
#endif

static void
swamish_cmd_ls(SwamishCmd *command, char **args, int count)
{
    char **files, **s;
    GError *err = NULL;

    if(!current_obj)	// Is current path a directory?
    {
        files = get_path_contents(current_dir, &err);

        if(!files)
        {
            g_critical("Error while getting directory listing: %s\n",
                       ipatch_gerror_message(err));
            g_clear_error(&err);
            return;
        }
    }
    else
    {
        return;
    }

    s = files;

    while(*s)
    {
        printf("%s\n", *s);
        s++;
    }

    g_strfreev(files);
}

static void
swamish_cmd_pwd(SwamishCmd *command, char **args, int count)
{
    char *path;

    path = g_build_filename(current_dir, current_obj, NULL);
    printf("%s\n", path);
    g_free(path);
}

static void
swamish_cmd_quit(SwamishCmd *command, char **args, int count)
{
    exit_swamish = TRUE;
}
