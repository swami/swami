/* Pending email confirmations */
CREATE TABLE Confirm (
  UserId SMALLINT UNSIGNED NOT NULL,    /* User ID of confirmation */
  /* Action being confirmed: Register: user registration, ResetPass: password reset */
  ConfirmAction ENUM('Register', 'ResetPass') NOT NULL DEFAULT "Register",
  HashKey CHAR (40) NOT NULL DEFAULT "",           /* SHA1 hash key */
  DateCreated TIMESTAMP NOT NULL        /* date entry was created */
);
