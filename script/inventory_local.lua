if not smv_state.local_inventory then
  smv_state.local_inventory = {}
end

function int_inventory_check_exists(AgentID)
  local inventory = smv_state["local_inventory"][AgentID]
  -- print("InventoryCheck:", inventory, AgentID)
  if not inventory then
    smv_state["local_inventory"][AgentID] = {}
    inventory = smv_state["local_inventory"][AgentID]
  end
  return inventory
end

function int_inventory_put_item_to(AgentID, key, item)
  local inventory = int_inventory_check_exists(AgentID)
  inventory[key] = item
  return item
end

function int_inventory_get_item_from(AgentID, key)
  -- print("Checking existence of ", AgentID)
  local inventory = int_inventory_check_exists(AgentID)
  return inventory[key]
end

function invloc_dump()
  pretty("fullinv", smv_state["local_inventory"])
end

function invloc_create_folder(AgentID, ID, parent, FolderType, FolderName, nocheck)
  local f = {}
  local uuid = ID
  local ch = nil
  if not uuid then
    uuid = fmv.uuid_create()
  end
  f.IsFolder = true
  f.ItemID = uuid
  f.ID = uuid
  f.ChildFolders = {}
  f.ChildItems = {}
  if parent then
    local par = int_inventory_get_item_from(AgentID, parent)
    print("Parent::", parent, AgentID)
    if not par and not nocheck then
      local uupar = invloc_create_folder(AgentID, parent, zero_uuid, 0, "Root Folder Autocreated", true)
      par = int_inventory_get_item_from(AgentID, uupar)
      print("invloc_create_folder: autocreated parent ", parent)
    end
    if par then
      ch = par.ChildFolders
    end
    if ch then
      table.insert(ch, uuid)
    elseif not nocheck then
      error("Parent with id ", parent, "was not found")
    end
    f.FolderID = parent
  else
    f.FolderID = zero_uuid
  end
  f.Type = FolderType
  f.Name = FolderName
  f.Version = 1

  int_inventory_put_item_to(AgentID, uuid, f)
  return uuid
end


function invloc_create_skeleton(AgentID)
  function add_folder(...)
    return invloc_create_folder(AgentID, fmv.uuid_create(), ...)
  end

  local root_id = 
    add_folder(nil, 8, "My Inventory")
  add_folder(root_id, 20, "Animations")
  add_folder(root_id, 13, "Body Parts")
  add_folder(root_id, 2, "Calling Cards") 
  add_folder(root_id, 5, "Clothing")
  add_folder(root_id, 21, "Gestures")
  add_folder(root_id, 3, "Landmarks")
  add_folder(root_id, 16, "Lost And Found")
  add_folder(root_id, 7, "Notecards") 
  add_folder(root_id, 6, "Objects")
  add_folder(root_id, 15, "Photo Album")
  add_folder(root_id, 10, "Scripts")
  add_folder(root_id, 1, "Sounds")
  add_folder(root_id, 12, "Textures")
  add_folder(root_id, 14, "Trash")
  return root_id
end

function invloc_retrieve_child_elements(AgentID, RootID, FieldName)
  local folders = {}
  local root = int_inventory_get_item_from(AgentID, RootID)
  if root then
    folders[1+#folders] = root
    for i, uuid in ipairs(root[FieldName]) do
      local f = int_inventory_get_item_from(AgentID, uuid)
      folders[1+#folders] = f
    end
  end
  return folders
end

function invloc_retrieve_child_folders(AgentID, RootID)
  local ch = invloc_retrieve_child_elements(AgentID, RootID, 'ChildFolders')
  if #ch == 0 then
    ch = invloc_retrieve_child_elements("library", RootID, 'ChildFolders')
  end
  return ch
end

function invloc_retrieve_child_items(AgentID, RootID)
  local ch = invloc_retrieve_child_elements(AgentID, RootID, 'ChildItems')
  if #ch == 0 then
    ch = invloc_retrieve_child_elements("library", RootID, 'ChildItems')
  end
  return ch
end


function invloc_retrieve_skeleton(AgentID, RootID)
  return invloc_retrieve_child_folders(AgentID, RootID)
end

function invloc_create_inventory_item_x(AgentID, FolderID, arg)
  local BaseMask = 0x3fffffff 
  local Flags = 0x3fffffff

  local i = {}
  local uuid = arg.ItemID or fmv.uuid_create()
  i.AssetID = arg.AssetID
  i.ItemID = uuid
  i.Type = arg.Type
  i.InvType = arg.InvType
  i.WearableType = arg.WearableType
  i.Name = arg.Name
  i.Description = arg.Description
  if FolderID then
    local par = int_inventory_get_item_from(AgentID, FolderID)
    if not par then
      print('Parent not found for', FolderID)
      return nil
      --[[
      local uupar = invloc_create_folder(AgentID, FolderID, zero_uuid, -1, "Root Folder Autocreated from invloc_create_inventory_item")
      par = int_inventory_get_item_from(AgentID, uupar)
      print("invloc_create_inventory_item: autocreated parent ", FolderID)
      --]]
    end
    local ch = par.ChildItems
    ch[1+#ch] = uuid
    i.FolderID = FolderID
  else
    i.FolderID = zero_uuid
  end

  -- Default settings: FIXME to get from config ?

  i.CreatorID = AgentID
  i.OwnerID = AgentID
  i.GroupID = zero_uuid
  i.BaseMask = BaseMask
  i.OwnerMask = BaseMask
  i.GroupMask = BaseMask
  i.EveryoneMask = BaseMask
  i.NextOwnerMask = BaseMask
  i.GroupOwned = false
  i.Flags = Flags
  i.SaleType = 0
  i.SalePrice = 123
  i.CreationDate = 100000 -- get the timestamp

  int_inventory_put_item_to(AgentID, uuid, i)
  return i
end

function invloc_create_inventory_item(AgentID, FolderID, TransactionID, AssetID, Type, InvType, WearableType, Name, Description, MaybeItemID)
  local arg = {}
  arg.ItemID = MaybeItemID
  arg.TransactionID = TransactionID
  arg.AssetID = AssetID
  arg.Type = Type
  arg.InvType = InvType
  arg.WearableType = WearableType
  arg.Name = Name
  arg.Description = Description 
  local i = nil
  i = invloc_create_inventory_item_x(AgentID, FolderID, arg)
  if i then
    return i.ItemID
  else
    return zero_uuid
  end
end

function invloc_retrieve_inventory_item(AgentID, uuid)
  local item = int_inventory_get_item_from(AgentID, uuid)
  if not item then
    item = int_inventory_get_item_from("library", uuid)
    print("Failed search for ", uuid)
  end
  pretty("item_lib", item)
  return item
end

function invloc_set_inventory_item(AgentID, uuid, item)
  local item = int_inventory_put_item_to(AgentID, uuid, item)
  return item
end

function invloc_update_inventory_item(AgentID, uuid, update_item)
  print("updating item", AgentID, uuid)
  pretty("update_item", update_item)
  local item = int_inventory_get_item_from(AgentID, uuid)
  if item then
    for k,v in pairs(update_item) do
      item[k] = update_item[k]
    end
  else
    item = update_item
  end
  int_inventory_put_item_to(AgentID, uuid, item)
  return item
end
