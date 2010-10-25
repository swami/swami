/* File mark table */
CREATE TABLE FileMarks (
  UserId SMALLINT UNSIGNED NOT NULL,    /* user who marked file */
  PatchId INT UNSIGNED NOT NULL,        /* file the user marked */
  MarkFlags SET('Downloaded', 'Want') DEFAULT "Downloaded",  /* Downloaded: User downloaded? Want: User wants? */
  MarkTime TIMESTAMP NOT NULL DEFAULT 0,          /* the time that the file was downloaded */
  UNIQUE (UserId, PatchId)
);
