-- Don't modify this! Create a new migration instead--see docs/schema-migrations.adoc
BEGIN TRANSACTION;

CREATE TABLE installation_result(unique_mark INTEGER PRIMARY KEY CHECK (unique_mark = 0), id TEXT, result_code INTEGER NOT NULL DEFAULT 0, result_text TEXT);

DELETE FROM version;
INSERT INTO version VALUES(7);

COMMIT TRANSACTION;
