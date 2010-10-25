/* CRAM archive files table */
CREATE TABLE Files (
  FileId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,      /* Id of file */
  PatchId INT UNSIGNED NOT NULL,                /* PatchId which owns this file */
  FileName VARCHAR(255) NOT NULL DEFAULT "",    /* Name of file */
  FileSize INT UNSIGNED NOT NULL DEFAULT 0,     /* Size of raw file in bytes */
  ComprSize INT UNSIGNED NOT NULL DEFAULT 0,    /* CRAM size of file in bytes */
  Md5 CHAR(32) NOT NULL DEFAULT ""              /* MD5 sum of file */
);
