/* Additional info for Patch items */
CREATE TABLE PatchInfo (
  PatchId INT UNSIGNED UNIQUE NOT NULL,   /* Id of item in Items table */

  /* flags (author: UserId is author? NOTE: Must be first flag) */
  PatchFlags SET ("Author") NOT NULL DEFAULT "",

  /* state of this patch file: Importing (in progress), Imported (imported but
     not yet active), Activating (queued to be activated), Active (is public),
     Disabled (disabled) */
  State ENUM ("Importing", "Imported", "Activating", "Active", "Disabled") NOT NULL DEFAULT "Importing",

  UserId SMALLINT UNSIGNED NOT NULL, /* owner of this patch */
  PatchAuthor VARCHAR(255) NOT NULL DEFAULT "", /* if ! 'Author' flag - author's name */
  FileName VARCHAR(255) NOT NULL,    /* archive file name without extensions */
  PatchType ENUM ("unknown", "sf2", "dls", "gig") NOT NULL DEFAULT "unknown",
  FileSize INT UNSIGNED NOT NULL DEFAULT 0,   /* total raw size of file(s) */
  PatchName VARCHAR(255) NOT NULL DEFAULT "", /* Name of patch file (title) */
  PatchDescr TEXT NOT NULL DEFAULT "",        /* description of the patch file */

  CramSize INT UNSIGNED NOT NULL DEFAULT 0,   /* size of CRAM compressed file */
  CramMd5 CHAR(32) NOT NULL DEFAULT "",       /* MD5 of CRAM file */
  CramClicks INT UNSIGNED NOT NULL DEFAULT 0, /* Number of CRAM file download clicks */

  ZipSize INT UNSIGNED NOT NULL DEFAULT 0,    /* Size of ZIP compressed file */
  ZipMd5 CHAR(32) NOT NULL DEFAULT "",	      /* MD5 of ZIP file */
  ZipClicks INT UNSIGNED NOT NULL DEFAULT 0,  /* Number of ZIP file download clicks */

  InfoClicks INT UNSIGNED NOT NULL DEFAULT 0, /* Number of info view clicks */

  CategoryId SMALLINT UNSIGNED NOT NULL DEFAULT 1,  /* Category of patch */
  CategoryId2 SMALLINT UNSIGNED NOT NULL DEFAULT 0,  /* 2nd Category of patch or 0 */
  CategoryId3 SMALLINT UNSIGNED NOT NULL DEFAULT 0,  /* 3rd Category of patch or 0 */
  LicenseId SMALLINT UNSIGNED NOT NULL DEFAULT 1,   /* License of the patch */
  Version VARCHAR(8) NOT NULL DEFAULT "",	/* an optional file version */
  WebSite VARCHAR(255) NOT NULL DEFAULT "", /* web site address */
  Email VARCHAR(255) NOT NULL DEFAULT "",   /* contact email address */
  RateTotal SMALLINT UNSIGNED NOT NULL DEFAULT 0, /* sum of all ratings */
  RateCount SMALLINT UNSIGNED NOT NULL DEFAULT 0, /* number of ratings */
  UserModified SMALLINT UNSIGNED NOT NULL DEFAULT 0,  /* user last modified this entry */
  UserImported SMALLINT UNSIGNED NOT NULL DEFAULT 0,  /* user that imported this entry */
  DateModified TIMESTAMP NOT NULL DEFAULT 0,  /* date of last modification */
  DateImported TIMESTAMP NOT NULL DEFAULT 0,  /* creation date */
  KEY (PatchName)
);
