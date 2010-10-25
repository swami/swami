/* User messages */
CREATE TABLE Messages (
  ToUserId SMALLINT UNSIGNED NOT NULL,        /* recipient UserId */
  FromUserId SMALLINT UNSIGNED NOT NULL,  /* sender UserId or 0 if anonymous */
  FromEmail VARCHAR(255) NOT NULL DEFAULT "",            /* anonymous user's from email */
  Flags SET('Received', 'Replied') NOT NULL DEFAULT "",
  Subject VARCHAR(255) NOT NULL DEFAULT "",              /* subject of the message */
  Body TEXT NOT NULL,                         /* body of the message */

  CreatedDate TIMESTAMP NOT NULL              /* message creation date */
);
