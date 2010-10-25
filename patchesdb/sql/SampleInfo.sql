/* Table of all samples */
CREATE TABLE SampleInfo (
  SampleId INT UNSIGNED UNIQUE NOT NULL,  /* ID in Items table */
  PatchId INT UNSIGNED NOT NULL,          /* parent patch file */
  Format ENUM ("8bit", "16bit", "24bit", "32bit", "float", "double") NOT NULL DEFAULT "16bit",
  SampleSize INT UNSIGNED NOT NULL DEFAULT 0, /* sample size */
  Channels TINYINT NOT NULL DEFAULT 1,  /* number of channels (1=mono, 2=stereo, etc) */
  RootNote TINYINT NOT NULL DEFAULT 60,  /* root MIDI note */
  SampleRate INT UNSIGNED NOT NULL DEFAULT 44100, /* sample rate in Hz */
  SampleName VARCHAR(255) NOT NULL, /* Sample name */
  KEY (SampleName)
);
