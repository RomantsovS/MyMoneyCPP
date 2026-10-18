-- Initial schema (ADR-0006, ADR-0013).
-- STRICT tables reject values of the wrong type; CHECK constraints repeat the
-- most important rules, so the database defends itself against any writer.

CREATE TABLE currency (
  code         TEXT    PRIMARY KEY CHECK (code GLOB '[A-Z][A-Z][A-Z]'),
  minor_digits INTEGER NOT NULL    CHECK (minor_digits BETWEEN 0 AND 4)
) STRICT;

CREATE TABLE account (
  id       INTEGER PRIMARY KEY,
  name     TEXT    NOT NULL CHECK (trim(name) <> ''),
  currency TEXT    NOT NULL REFERENCES currency (code),
  archived INTEGER NOT NULL DEFAULT 0 CHECK (archived IN (0, 1))
) STRICT;

CREATE TABLE category (
  id        INTEGER PRIMARY KEY,
  name      TEXT    NOT NULL CHECK (trim(name) <> ''),
  kind      TEXT    NOT NULL CHECK (kind IN ('expense', 'income')),
  parent_id INTEGER REFERENCES category (id)
) STRICT;

CREATE TABLE operation (
  id      INTEGER PRIMARY KEY,
  date    TEXT    NOT NULL
          CHECK (date GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'),
  type    TEXT    NOT NULL CHECK (type IN ('expense', 'income', 'transfer')),
  comment TEXT    NOT NULL DEFAULT ''
) STRICT;

-- One signed change of one account's balance, in the account's currency.
CREATE TABLE movement (
  id           INTEGER PRIMARY KEY,
  operation_id INTEGER NOT NULL REFERENCES operation (id) ON DELETE CASCADE,
  account_id   INTEGER NOT NULL REFERENCES account (id),
  category_id  INTEGER REFERENCES category (id),
  amount_minor INTEGER NOT NULL CHECK (amount_minor <> 0)
) STRICT;

CREATE INDEX movement_by_account ON movement (account_id);
CREATE INDEX movement_by_operation ON movement (operation_id);
CREATE INDEX operation_by_date ON operation (date);
