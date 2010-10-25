/* Patch file user ratings table */
CREATE TABLE Ratings (
  PatchId INT UNSIGNED NOT NULL,  /* Id of patch file item */
  UserId INT UNSIGNED NOT NULL DEFAULT 1, /* Owner of rating (1=Anonymous)*/
  Date TIMESTAMP NOT NULL,        /* Date rating was created */
  Rating TINYINT NOT NULL DEFAULT 0,        /* 0-5 rating (0=no rating or anonymous) */
  Comment TEXT NOT NULL,          /* Comment to go along with rating */
  KEY (PatchId, UserId)
);
