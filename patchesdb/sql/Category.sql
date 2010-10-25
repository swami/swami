/* Table of categories */
CREATE TABLE Category (
  CategoryId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  CategoryName VARCHAR(255) NOT NULL,
  CategoryDescr VARCHAR(255) NOT NULL DEFAULT "",
  UNIQUE (CategoryName)
);

INSERT INTO Category (CategoryId, CategoryName) VALUES (1, "Unknown");
