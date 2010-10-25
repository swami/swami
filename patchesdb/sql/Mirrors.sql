/* Table of download mirrors */
CREATE TABLE Mirrors (
  MirrorId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  /* Active: Mirror is active?, CRAM: Has CRAM files?, zip: Has zip files? */
  Flags SET ("Active", "CRAM", "zip") NOT NULL DEFAULT "CRAM,zip",
  BaseURL VARCHAR(255) NOT NULL DEFAULT "",  /* base URL of files (ftp:// or http://) */
  DownloadClicks INT UNSIGNED NOT NULL DEFAULT 0,   /* Number of download clicks */
  DownloadSize BIGINT UNSIGNED NOT NULL DEFAULT 0,  /* Size of downloaded data since last mirror was added */
  DownloadTotal BIGINT UNSIGNED NOT NULL DEFAULT 0, /* Total size of downloaded data for mirror */
  Name VARCHAR(255) NOT NULL DEFAULT "",     /* descr. name of site ex: "resonance.org" */
  Location VARCHAR(255) NOT NULL DEFAULT "", /* location ex: "North America:California" */
  ContactEmail VARCHAR(255) NOT NULL DEFAULT "",  /* admin contact email address */
  ContactName VARCHAR(255) NOT NULL DEFAULT ""    /* name of admin */
);
