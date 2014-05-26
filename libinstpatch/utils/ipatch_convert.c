/* in progress, i.e. not yet working */


/*
 * instpatch.c - A command line interface to libInstPatch
 *
 * libInstPatch
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
#include <libinstpatch/libinstpatch.h>

#define _GNU_SOURCE
#include <getopt.h>

static void usage (void);

extern char *optarg;
extern int optind, opterr, optopt;

int
main (int argc, char *argv[])
{
  int option_index = 0;
  int c;

  static struct option long_options[] =
    {
      {"convert", 1, 0, 'c'},
      {"output", 1, 0, 'o'},
      {NULL, 0, NULL, 0}
    };

  while (TRUE)
    {
      c = getopt_long (argc, argv, "c:o:", long_options, &option_index);
      if (c == -1) break;

      switch (c)
	{
	case 'c':		/* conversion type */
	  break;
	case 'o':		/* conversion output file name/dir */
	  break;
	case ':':		/* missing option */
	  fprintf (stderr, "Missing parameter for option '-%c, %s'\n",
		   (char)(long_options[option_index].val),
		   long_options[option_index].name);
	  usage();
	  exit (1);
	  break;
	case '?':		/* unknown switch */
	  usage();
	  exit(1);
	  break;
	default:
	  fprintf (stderr, "Unknown getopt return val '%d'\n", c);
	  exit (1);
	  break;
	}
    }
}

#define U(text) fprintf (stderr, text);

static void
usage (void)
{
  U ("libInstPatch utility for MIDI instrument patch file processing\n");
  U ("Copyright (C) 1999-2005 Josh Green <jgreen@users.sourceforge.net>\n");
  U ("Distributed under the LGPL license\n\n");

  U ("Usage: instpatch [OPTION]... [FILE]...\n\n");

  U ("Examples:\n");
  U ("  instpatch -n sf2 -o newfile.sf2  # Create a new SoundFont file.\n");
  U ("  instpatch -c dls2 *.sf2          # Convert all SoundFont files to DLS2.\n");
  U ("  instpatch -ql myfile.gig         # List items in a GigaSampler file.\n");
  U ("  instpatch -qp -i 32 example.dls  # Get properties of item 32 in DLS file.\n");
  U ("\n");

  U ("Query options (with -q or --query):\n");
  U ("  -i, --item=ID                Select item by numeric ID\n");
  U ("  -l, --list                   List items\n");
  U ("  -p, --prop                   Query properties\n");
  U ("\n");

  U ("Conversion options (with -c or --convert):\n");
  U ("  -t, --type=TYPE              File TYPE to convert to\n");

  U ("  -n, --new=TYPE               Create a new patch object of TYPE\n");
  U ("  -c, --convert=TYPE           Convert type of file(s)\n");
  U ("  -o, --output=file/dir        Specify output file name and/or directory for --new and --convert\n");
}
