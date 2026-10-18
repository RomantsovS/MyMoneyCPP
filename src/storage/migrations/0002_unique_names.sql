-- Account names and category names are unique, each globally (ADR-0014):
-- commands refer to them by name. Case-sensitive: SQLite's NOCASE would fold
-- only ASCII letters, not Cyrillic ones.

CREATE UNIQUE INDEX account_name_unique ON account (name);
CREATE UNIQUE INDEX category_name_unique ON category (name);
