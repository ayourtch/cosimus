
--[[

Copyright © 2008-2009 Andrew Yourtchenko, ayourtch@gmail.com.

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom 
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
OR OTHER DEALINGS IN THE SOFTWARE. 

--]]

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
	    local folderpath = ""
	    if folderlist[relpath] and 
	                           folderlist[relpath].FolderPath then
	      folderpath = dirname(folderlist[relpath].FolderPath)
	    else
	      folderpath = dirname(relpath)
	    end
            item.ItemFileName = folderpath .. item.fileName
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

function parse_inventory_xml(zf, zfilename, relpath, items)
  local f = zf:open(relpath)
  local xml = parse_xml(f:read("*a"))
  if xml[1].label == "Nini" then
    local x = xml[1]
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
          -- folderlist[other_relpath] = item
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
              table.insert(items.folders, item)
	    else
              table.insert(items.items, item)
	    end
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

  local items_filelist = {}
  local items = {}
  items.folders = {}
  items.items = {}

  for fn in zf:files() do
    if string.match(fn.filename, 'Items.xml$') then
      table.insert(items_filelist, fn)
    elseif string.match(fn.filename, '.xml$') then
      -- print(fn.filename)
      parse_inventory_xml(zf, zfname, fn.filename, items)
    end
  end
  for i, fn in ipairs(items_filelist) do
    parse_inventory_xml(zf, zfname, fn.filename, items)
  end
  return items
end


function get_zip_content(zfname, relpath)
  local zf = zip.open(zfname)
  print('Getting content from inside ZIP "' .. zfname .. '", file "'
         .. relpath ..'"')
  local f = zf:open(relpath)
  local data = f:read("*a")
  f:close()
  zf:close()
  return data
end
