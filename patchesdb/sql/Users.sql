/* Users of the system */
CREATE TABLE Users (
  UserId SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,  /* User ID */

  /* Admin: Admin user, NoLogin: Disabled login, Notified: Email notify sent,
     Confirm: Waiting for email confirmation */
  Flags SET('Admin','NoLogin','Notified','Confirm') NOT NULL DEFAULT "",

  /* EmailNews: receive news, NotifyRatings: notify on rating/comment */
  UserFlags SET('EmailNews','NotifyRatings') NOT NULL DEFAULT "",
  Login VARCHAR(32) NOT NULL,     /* user login name */
  Password CHAR(40) NOT NULL,  /* user password (SHA1) */
  UserName VARCHAR(32) NOT NULL,   /* user's public name */
  RealName VARCHAR(255) NOT NULL DEFAULT "", /* user's real name */
  UserEmail VARCHAR(255) NOT NULL,/* email address */

  WebSite VARCHAR(255) NOT NULL DEFAULT "",  /* users web site */

  /* visible to No one, other Users, All */
  EmailVisible ENUM('No', 'Users', 'All') NOT NULL DEFAULT "No",     /* email visibility */
  RealNameVisible ENUM('No', 'Users', 'All') NOT NULL DEFAULT "No",  /* real name visibility */

  LastLogin TIMESTAMP NOT NULL DEFAULT 0,   /* date of last login */
  DateCreated TIMESTAMP NOT NULL DEFAULT 0  /* user creation date */
);

INSERT INTO Users (UserId, Flags, Login, UserName, LastLogin, DateCreated)
  VALUES (1, "NoLogin", "anonymous", "Anonymous", 0, NULL);
