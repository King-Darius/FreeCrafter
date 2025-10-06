# Legacy document loading notes

## What changed in `Document::loadFromFile`

FreeCrafter stores geometric primitives in a `GeometryKernel` and keeps scene-specific state (tags, section planes, object hierarchy, etc.) alongside it.  Prior to the regression fix, the legacy import path (`version <= 1`) executed `geometryKernel.loadFromFile()` and then called `reset()`, which clears the kernel and every derived index.  The subsequent `synchronizeWithGeometry()` therefore rebuilt the scene against an empty kernel, leaving the document blank even though the load routine reported success.  The fix splits the reset logic so the legacy path can rebuild scene state without destroying the freshly imported geometry. 【F:src/Scene/Document.cpp†L436-L520】

## Why clearing the kernel was harmful

`reset()` is a full document wipe: it replaces the root node, clears every map, and resets ID counters.  That behavior is correct when starting a brand new document, but on legacy loads it discards the in-memory geometry that was just deserialized.  Because the kernel owns the canonical list of `GeometryObject` instances, clearing it makes it impossible for the rest of the document to reattach objects, so the UI shows an empty scene even though the load succeeded.  Preserving the kernel while rebuilding the higher level state restores the expected behavior for `.fcm` files produced by older releases. 【F:src/Scene/Document.cpp†L441-L466】【F:src/Scene/Document.cpp†L500-L520】

## How other DCC applications handle backward compatibility

* **Blender** retains imported datablocks by moving them from the legacy `Main` lists into the freshly constructed runtime `Main` instead of discarding them.  The `read_undo_move_libmain_data` helper iterates the old data and pushes each datablock into the new container, ensuring identifiers remain valid:  
  ```cpp
  ListBase *new_lb = which_libbase(new_bmain, id_type->id_code);
  BLI_movelisttolist(new_lb, lbarray[i]);
  LISTBASE_FOREACH (ID *, id_iter, new_lb) {
      BKE_main_idmap_insert_id(fd->new_idmap_uid, id_iter);
  }
  ```
  This mirrors FreeCrafter's updated strategy of preserving the geometry kernel while rebuilding higher-level indices. 【F:docs/legacy-loading-notes.md†L17-L25】
* **FreeCAD** restores legacy documents object-by-object.  After reading the feature catalog, `Document::Restore` calls `DocumentObject::Restore` on each feature without clearing the document-wide registries, so the already-loaded geometry stays intact while per-object state is refreshed.  The relevant loop looks like:  
  ```cpp
  DocumentObject* pObj = getObject(name.c_str());
  if (pObj) {
      pObj->setStatus(ObjectStatus::Restore, true);
      pObj->Restore(reader);
      pObj->setStatus(ObjectStatus::Restore, false);
  }
  ```
  Like FreeCrafter's fix, this approach rebuilds scene metadata around the persistent geometry core. 【F:docs/legacy-loading-notes.md†L26-L34】

