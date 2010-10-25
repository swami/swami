/* Table of all item extra property values */
CREATE TABLE ItemProps (
  ItemId INT UNSIGNED NOT NULL, /* Item ID owning this property */
  PropId INT UNSIGNED NOT NULL, /* Property ID */
  PropValue TEXT NOT NULL,      /* Property value */
  UNIQUE KEY (ItemId, PropId)
);
