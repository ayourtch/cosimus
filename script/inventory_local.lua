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
  local inventory = int_inventory_check_exists(AgentID)
  return inventory[key]
end

function invloc_create_folder(AgentID, ID, parent, FolderType, FolderName)
  local f = {}
  local uuid = ID
  if not uuid then
    uuid = fmv.uuid_create()
  end
  f.IsFolder = true
  f.ID = uuid
  f.ChildFolders = {}
  f.ChildItems = {}
  if parent then
    local par = int_inventory_get_item_from(AgentID, parent)
    local ch = par.ChildFolders
    ch[1+#ch] = uuid
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
  return invloc_retrieve_child_elements(AgentID, RootID, 'ChildFolders')
end

function invloc_retrieve_child_items(AgentID, RootID)
  return invloc_retrieve_child_elements(AgentID, RootID, 'ChildItems')
end


function invloc_retrieve_skeleton(AgentID, RootID)
  return invloc_retrieve_child_folders(AgentID, RootID)
end

function invloc_create_inventory_item(AgentID, FolderID, TransactionID, AssetID, Type, InvType, WearableType, Name, Description)
  local i = {}
  local uuid = fmv.uuid_create()
  i.AssetID = AssetID
  i.ID = uuid
  i.Type = Type
  i.InvType = InvType
  i.WearableType = WearableType
  i.Name = Name
  i.Description = Description
  if FolderID then
    local par = int_inventory_get_item_from(AgentID, FolderID)
    local ch = par.ChildItems
    ch[1+#ch] = uuid
    i.FolderID = FolderID
  else
    i.FolderID = zero_uuid
  end
  int_inventory_put_item_to(AgentID, uuid, i)
  return uuid
end

function invloc_retrieve_inventory_item(AgentID, uuid)
  local item = int_inventory_get_item_from(AgentID, uuid)
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
