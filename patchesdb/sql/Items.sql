/* Table of all items (Patches, Instruments and Samples) */
CREATE TABLE Items (
  ItemId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ItemType ENUM ("Patch", "Inst", "Sample") NOT NULL DEFAULT "Patch"   /* Item type */
);
