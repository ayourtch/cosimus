require 'inventory_local'

function parse_assets_xml(zf, zfilename, relpath, folderlist, assetlist)
  local f = zf:open(relpath)
  local xml = parse_xml(f:read("*a"))
  if xml[1].label == "Nini" then
    local x = xml[1]
    local items = {}
    for i, xi in ipairs(x) do
      if xi.label == "Section" then
        item = {}
        item.Name = xi.xarg.Name
        for i, field in ipairs(xi) do
          if field.label == "Key" then
            item[field.xarg.Name] = field.xarg.Value
          end
        end
        item.assetType = tonumber(item.assetType)
        if item.file then
          local other_relpath = dirname(relpath) .. item.file
          folderlist[other_relpath] = item
          item.ZipFileName = zfilename
          item.FolderPath = other_relpath
        else
	  
	  if item.folderID or item.itemsFile then
	    -- Inventory item/folder/library
	  else
            item.ZipFileName = zfilename
            item.ItemFileName = dirname(folderlist[relpath].FolderPath) .. item.fileName
            -- .. item.fileName
            table.insert(items, item)
            if assetlist then
              table.insert(assetlist, item)
            end
	  end
        end
      end
    end
    return items
  else
    return nil
  end
end

function assets_zip_scan(zfname)
  local zf = zip.open(zfname)

  local folderlist = {}
  local assetlist = {}

  for fn in zf:files() do
    if string.match(fn.filename, '.xml$') then
      -- print(fn.filename)
      parse_assets_xml(zf, zfname, fn.filename, folderlist, assetlist)
    end
  end
  return assetlist
end

function parse_inventory_xml(zf, zfilename, relpath, folderlist, assetlist)
  local f = zf:open(relpath)
  local xml = parse_xml(f:read("*a"))
  if xml[1].label == "Nini" then
    local x = xml[1]
    local items = {}
    for i, xi in ipairs(x) do
      if xi.label == "Section" then
        item = {}
        item.Name = xi.xarg.Name
        for i, field in ipairs(xi) do
          if field.label == "Key" then
            item[field.xarg.Name] = field.xarg.Value
          end
        end
        if item.file then
          local other_relpath = dirname(relpath) .. item.file
          folderlist[other_relpath] = item
          item.ZipFileName = zfilename
          item.FolderPath = other_relpath
        else
	  if item.itemsFile then
	    -- library listing, do nothing
	  elseif item.folderID then
	    -- pretty("item", item)
            item.assetType = tonumber(item.assetType)
            item.inventoryType = tonumber(item.inventoryType)
	    item.currentPermissions = tonumber(item.currentPermissions)
	    item.basePermissions = tonumber(item.basePermissions)
	    item.everyonePermissions = tonumber(item.everyonePermissions)
            item.type = tonumber(item.type)
	    if item.parentFolderID then
	      -- print("Creating Folder ", item.name,  item.folderID, item.parentFolderID)
	      invloc_create_folder("library", item.folderID, item.parentFolderID, item.type, item.name)
	    else
	      -- print("Creating Item ", item.name)
	      invloc_create_inventory_item("library", item.folderID, nil, item.assetID, item.assetType, item.wearableType, item.name, item.description)
	    end
	    -- Inventory item/folder/library
            -- item.ZipFileName = zfilename
            -- item.ItemFileName = dirname(folderlist[relpath].FolderPath) .. item.fileName
            -- .. item.fileName
            -- table.insert(items, item)
            -- if assetlist then
            --  table.insert(assetlist, item)
            --end
	  else
	  end
        end
      end
    end
    return items
  else
    return nil
  end
end

function inventory_zip_scan(zfname)
  local zf = zip.open(zfname)

  local folderlist = {}
  local assetlist = {}
  local items_filelist = {}

  for fn in zf:files() do
    if string.match(fn.filename, 'Items.xml$') then
      table.insert(items_filelist, fn)
    elseif string.match(fn.filename, '.xml$') then
      print(fn.filename)
      parse_inventory_xml(zf, zfname, fn.filename, folderlist, assetlist)
    end
  end
  for i, fn in ipairs(items_filelist) do
    parse_inventory_xml(zf, zfname, fn.filename, folderlist, assetlist)
  end
  return assetlist
end


function get_zip_content(zfname, relpath)
  local zf = zip.open(zfname)
  local f = zf:open(relpath)
  local data = f:read("*a")
  f:close()
  zf:close()
  return data
end
