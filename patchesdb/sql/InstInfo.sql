/* Table of all instruments */
CREATE TABLE InstInfo (
  InstId INT UNSIGNED UNIQUE NOT NULL,
  PatchId INT UNSIGNED NOT NULL,  /* parent patch file */
  Bank SMALLINT UNSIGNED NOT NULL DEFAULT 0,    /* MIDI bank number */
  Program SMALLINT UNSIGNED NOT NULL DEFAULT 0, /* MIDI program number */
  InstName VARCHAR(255) NOT NULL DEFAULT "", /* Instrument name */
  KEY (InstName)
);
