/* News entries */
CREATE TABLE News (
  NewsId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  Date timestamp,                    /* date of the news entry */
  UserId SMALLINT UNSIGNED NOT NULL DEFAULT 0, /* user who submitted news */
  Title VARCHAR(255) NOT NULL DEFAULT "",       /* title of news item */
  Content TEXT NOT NULL              /* content of the news */
);
