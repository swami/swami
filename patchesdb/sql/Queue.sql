/* Action queue */
CREATE TABLE Queue (
  QueueId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, /* Id of action */

  /* Type of action.  Import: File import, Activate: CRAM and activate patch */
  Type ENUM ("Import", "Activate", "Email") NOT NULL DEFAULT "Import",

  /* Operation status  New: Not yet active, Queued: waiting for process,
     Running: executing, Error: an error occurred */
  Status ENUM ("New", "Queued", "Running", "Error") NOT NULL DEFAULT "Queued",

  ErrorMsg VARCHAR (255) NOT NULL DEFAULT "",   /* Error message (if Status = Error) */
  Log TEXT NOT NULL,                 /* log of operation (if Status >= Running) */

  UserId SMALLINT UNSIGNED NOT NULL DEFAULT 0, /* Owner of this action */

  ItemId INT UNSIGNED NOT NULL DEFAULT 0,      /* Item this action pertains to (or 0 if no item) */
  FileName VARCHAR (255) NOT NULL DEFAULT "",   /* file name of action (or "" if no file name) */

  Content TEXT NOT NULL,  /* only for Email Type currently, the email content */

  Date TIMESTAMP NOT NULL            /* Date the action was started */
);
