-- Category groups (ADR-0015): a group is always at the root and only contains
-- categories; a category has operations and no children.

ALTER TABLE category
  ADD COLUMN is_group INTEGER NOT NULL DEFAULT 0 CHECK (is_group IN (0, 1));

-- Convert the data of schema 2. Its only writer, the M3 CategoryService,
-- allowed at most two levels and children of the parent's kind, so every
-- category with children is at the root and can become a group.
UPDATE category SET is_group = 1
WHERE id IN (SELECT parent_id FROM category WHERE parent_id IS NOT NULL);

-- A new group may still have operations of its own. They move to a new
-- category "<group> (other)" in that group, so no balance or total changes.
-- If that name is already taken, the UNIQUE index stops the migration.
INSERT INTO category (name, kind, parent_id)
SELECT name || ' (other)', kind, id FROM category AS grp
WHERE is_group = 1
  AND EXISTS (SELECT 1 FROM movement WHERE category_id = grp.id);

UPDATE movement
SET category_id = (
  SELECT other.id FROM category AS other
  JOIN category AS grp ON grp.id = other.parent_id
  WHERE grp.id = movement.category_id AND other.name = grp.name || ' (other)')
WHERE category_id IN (SELECT id FROM category WHERE is_group = 1);

-- From now on the database refuses data that breaks the rules. RAISE(ABORT)
-- fails the statement with this message; the services check the same rules
-- first, so the app normally never sees one.

CREATE TRIGGER category_insert_structure BEFORE INSERT ON category
WHEN NEW.parent_id IS NOT NULL
BEGIN
  SELECT RAISE(ABORT, 'a group must be at the root')
  WHERE NEW.is_group = 1;
  SELECT RAISE(ABORT, 'a category can only be inside a group of the same kind')
  WHERE NOT EXISTS (SELECT 1 FROM category
                    WHERE id = NEW.parent_id AND is_group = 1
                      AND kind = NEW.kind);
END;

CREATE TRIGGER category_update_structure
BEFORE UPDATE OF parent_id, kind, is_group ON category
WHEN NEW.parent_id IS NOT NULL
BEGIN
  SELECT RAISE(ABORT, 'a group must be at the root')
  WHERE NEW.is_group = 1;
  SELECT RAISE(ABORT, 'a category can only be inside a group of the same kind')
  WHERE NOT EXISTS (SELECT 1 FROM category
                    WHERE id = NEW.parent_id AND is_group = 1
                      AND kind = NEW.kind);
END;

-- A group with categories cannot become a category, a category with
-- operations cannot become a group, and neither can change its kind.
CREATE TRIGGER category_update_in_use BEFORE UPDATE OF kind, is_group ON category
WHEN (NEW.kind <> OLD.kind OR NEW.is_group <> OLD.is_group)
  AND (EXISTS (SELECT 1 FROM category WHERE parent_id = OLD.id)
       OR EXISTS (SELECT 1 FROM movement WHERE category_id = OLD.id))
BEGIN
  SELECT RAISE(ABORT, 'a category or group in use cannot change its kind or role');
END;

CREATE TRIGGER movement_insert_not_on_group BEFORE INSERT ON movement
WHEN EXISTS (SELECT 1 FROM category WHERE id = NEW.category_id AND is_group = 1)
BEGIN
  SELECT RAISE(ABORT, 'a group cannot have operations');
END;

CREATE TRIGGER movement_update_not_on_group BEFORE UPDATE OF category_id ON movement
WHEN EXISTS (SELECT 1 FROM category WHERE id = NEW.category_id AND is_group = 1)
BEGIN
  SELECT RAISE(ABORT, 'a group cannot have operations');
END;
