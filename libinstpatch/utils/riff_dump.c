/* 
 * riff_dump.c - Command line utility to dump info about RIFF files
 *
 * riff_dump utility (licensed separately from libInstPatch)
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Public Domain, use as you please.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libinstpatch/libinstpatch.h>

#define _GNU_SOURCE
#include <getopt.h>

void usage (void);
gboolean recurse_riff_chunks (IpatchRiff *riff, char *indent,
			      GError **err);
void display_chunk (IpatchRiff *riff, char *indent);
gboolean dump_chunk (IpatchRiff *riff, GError **err);

/* chunk index - so commands can be run on specific chunks */
int chunk_index = 0;
int dump_index = -1;	 /* set to chunk index if chunk dump requested */
char *dump_type = NULL;	/* set to 4 char string if dumping a chunk type */
gboolean raw_dump = FALSE;	/* set to TRUE for raw byte dumps */
gboolean display = TRUE;      /* set to FALSE to not display chunks */
gboolean stop = FALSE;		/* set to TRUE to stop recursion */

extern char *optarg;
extern int optind, opterr, optopt;

int
main (int argc, char *argv[])
{
  IpatchRiff *riff;
  IpatchRiffChunk *chunk;
  IpatchFile *file;
  IpatchFileHandle *fhandle;
  char indent_buf[256] = "";
  GError *err = NULL;
  char *file_name = NULL;
  int option_index = 0;
  int c;

  static struct option long_options[] =
    {
      {"dump", 1, 0, 'd'},
      {"dump-type", 1, 0, 't'},
      {"raw", 0, 0, 'r'},
      {NULL, 0, NULL, 0}
    };

  while (TRUE)
    {
      c = getopt_long (argc, argv, "rd:t:", long_options, &option_index);
      if (c == -1) break;

      switch (c)
	{
	case 'd':		/* dump chunk? */
	  dump_index = atoi (optarg); /* get chunk index */
	  display = FALSE;  /* we enable display when we find chunk */
	  break;
	case 't':
	  dump_type = g_strndup (optarg, 4);
	  display = FALSE;
	  break;
	case 'r':
	  raw_dump = TRUE;
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

  if (optind >= argc)
    {
      usage ();
      exit (1);
    }

  file_name = argv[optind];

  g_type_init ();
  ipatch_init ();

  file = ipatch_file_new ();
  if (!(fhandle = ipatch_file_open (file, file_name, "r", &err)))
    {
      fprintf (stderr, "Failed to open file '%s': %s\n",
	       file_name, err ? err->message : "<no details>");
      exit (1);
    }

  riff = ipatch_riff_new (fhandle);

  if (!(chunk = ipatch_riff_start_read (riff, &err)))
    {
      fprintf (stderr, "Failed to start RIFF parse of file '%s': %s\n",
	       file_name, err ? err->message : "<no details>");
      exit (1);
    }

  /* if a dump of chunk 0 requested or type matches, display everything */
  if (dump_index == 0
      || (dump_type && strncmp (dump_type, chunk->idstr, 4) == 0))
    display = TRUE;

  if (display) display_chunk (riff, indent_buf);
  chunk_index++;
  strcat (indent_buf, "  ");

  if (!recurse_riff_chunks (riff, indent_buf, &err))
    {
      fprintf (stderr, "%s\n", ipatch_riff_message_detail
	       (riff, -1, "Error while parsing RIFF file '%s': %s",
		file_name, err ? err->message : "<no details>"));
      exit (1);
    }

  exit (0);
}

void
usage (void)
{
  fprintf (stderr, "Usage: riff_dump [OPTION]... FILE\n");
  fprintf (stderr, "  -d, --dump=CHUNK_INDEX       "
	   "Dump a chunk by index\n");
  fprintf (stderr, "  -t, --dump-type='CHNK'       "
	   "Dump a chunk by RIFF FOURCC ID\n");
  fprintf (stderr, "  -r, --raw                    "
	   "Do raw dump rather than formatted hex dump\n\n");
  fprintf (stderr, "CHUNK_INDEX - The chunk index (number in brackets [])\n");
}

gboolean
recurse_riff_chunks (IpatchRiff *riff, char *indent, GError **err)
{
  IpatchRiffChunk *chunk;
  gboolean retval;

  while (!stop && (chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (dump_index == chunk_index) /* dump by chunk index match? */
	{
	  if (chunk->type != IPATCH_RIFF_CHUNK_SUB) /* list chunk? */
	    {
	      display_chunk (riff, indent);

	      strcat (indent, "  ");
	      display = TRUE;
	      retval = recurse_riff_chunks (riff, indent, err);

	      stop = TRUE;
	      return (retval);
	    }
	  else
	    {
	      retval = dump_chunk (riff, err); /* hex dump of sub chunk */
	      stop = TRUE;
	      return (retval);
	    }
	} /* dump by type match? */
      else if (dump_type && strncmp (dump_type, chunk->idstr, 4) == 0)
	{
	  if (chunk->type != IPATCH_RIFF_CHUNK_SUB) /* list chunk? */
	    {
	      display = TRUE;
	      strcat (indent, "  ");
	      recurse_riff_chunks (riff, indent, err);
	      indent[strlen (indent) - 2] = '\0';
	      display = FALSE;
	    }
	  else dump_chunk (riff, err); /* hex dump of sub chunk */
	}
      else			/* no dump match, just do stuff */
	{
	  if (display) display_chunk (riff, indent);
	  chunk_index++;		/* advance chunk index */

	  if (chunk->type != IPATCH_RIFF_CHUNK_SUB)	/* list chunk? */
	    {
	      strcat (indent, "  ");
	      if (!recurse_riff_chunks (riff, indent, err)) return (FALSE);
	      indent[strlen (indent) - 2] = '\0';
	    }
	}

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
    }

  return (ipatch_riff_get_error (riff, NULL));
}

void
display_chunk (IpatchRiff *riff, char *indent)
{
  IpatchRiffChunk *chunk;
  int filepos;

  chunk = ipatch_riff_get_chunk (riff, -1);
  filepos = ipatch_riff_get_position (riff);

  if (chunk->type == IPATCH_RIFF_CHUNK_SUB)
    printf ("%s(%.4s)[%4d] (ofs = 0x%x, size = %d)\n", indent,
	    chunk->idstr, chunk_index,
	    filepos - (chunk->position + IPATCH_RIFF_HEADER_SIZE),
	    chunk->size);
  else	/* list chunk */
    printf ("%s<%.4s>[%4d] (ofs = 0x%x, size = %d)\n", indent,
	    chunk->idstr, chunk_index,
	    filepos - (chunk->position + IPATCH_RIFF_HEADER_SIZE),
	    chunk->size);
}

#define BUFFER_SIZE (16 * 1024)

/* hex dump of a sub chunk */
gboolean
dump_chunk (IpatchRiff *riff, GError **err)
{
  IpatchRiffChunk *chunk;
  guint8 buf[BUFFER_SIZE];
  int filepos, read_size, bytes_left, i;

  chunk = ipatch_riff_get_chunk (riff, -1);
  filepos = ipatch_riff_get_position (riff);

  if (!raw_dump)
    {
      printf ("Dump chunk: (%.4s)[%4d] (ofs = 0x%x, size = %d)",
	      chunk->idstr, chunk_index,
	      filepos - (chunk->position + IPATCH_RIFF_HEADER_SIZE),
	      chunk->size);

      i = filepos & ~0xF;   /* round down to nearest 16 byte offset */
      while (i < filepos) /* advance to start point in 16 byte block */
	{
	  if (!(i & 0xF)) printf ("\n%08u  ", i); /* print file position */
	  else if (!(i & 0x3)) printf (" |  "); /* print divider */
	  printf ("   ");		/* skip 1 byte character */
	  i++;
	}
    }

  read_size = BUFFER_SIZE;
  bytes_left = chunk->size;
  while (bytes_left)		/* loop until chunk exhausted */
    {
      if (bytes_left < BUFFER_SIZE) read_size = bytes_left;
      if (!ipatch_file_read (riff->handle, &buf, read_size, err))
	return (FALSE);

      for (i = 0; i < read_size; i++, filepos++)
	{
	  if (!raw_dump)
	    {
	      if (!(filepos & 0xF))
		printf ("\n%08u  ", filepos); /* print file position */
	      else if (!(filepos & 0x3)) printf (" |  "); /* print divider */
	    }

	  printf ("%02X ", buf[i]);
	}
      bytes_left -= read_size;
    }

  printf ("\n");

  return (TRUE);
}
