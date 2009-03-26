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
	    pretty("item", item)
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

function get_zip_content(zfname, relpath)
  local zf = zip.open(zfname)
  local f = zf:open(relpath)
  local data = f:read("*a")
  f:close()
  zf:close()
  return data
end
