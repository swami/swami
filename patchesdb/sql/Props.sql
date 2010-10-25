/* Table of all extended property names */
CREATE TABLE Props (
  PropId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  PropName VARCHAR(255) NOT NULL,   /* name of the property */
  PropDescr VARCHAR(255) NOT NULL DEFAULT "",  /* description of the property */
  UNIQUE (PropName)
);
