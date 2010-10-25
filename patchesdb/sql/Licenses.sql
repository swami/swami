/* Table of licenses */
CREATE TABLE Licenses (
  LicenseId SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  LicenseName VARCHAR(255) NOT NULL,
  LicenseDescr TEXT NOT NULL DEFAULT "",
  LicenseURL VARCHAR(255) NOT NULL DEFAULT "",
  UNIQUE (LicenseName)
);

INSERT INTO Licenses (LicenseId, LicenseName, LicenseDescr) VALUES
(1, "Unknown", "License type is not known");
