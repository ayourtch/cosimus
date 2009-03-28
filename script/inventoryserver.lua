
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

require "startup"
require "httpd_common"
require "zip_common"

inventoryserver = {}

print("Inventory server!")

function inventory_server_http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")

  local pp = function(...) 
    su.dstrcat(dd, ...)
  end

  local ii = function(uuid, itemid)
    local i
    local inv
    if itemid == zero_uuid then
      inv = {}
      inv.ItemID = zero_uuid
      inv.AssetID = zero_uuid
    else
      inv = invloc_retrieve_inventory_item(uuid, itemid)
    end
    if inv then
      i = inv
    else
      i = {}
      i.ItemID = zero_uuid
      i.AssetID = zero_uuid
      i.Error = "Could not retrieve inventory " .. itemid
      -- invloc_dump()
      print(i.Error)
    end
    return i
  end

  local json_header = function(dh)
    su.dstrcat(dh, "Content-Type: text/json\r\n")
  end

  print("Inventory server request for ", uri)

  if uri == "/" then
    -- dstrcat(dh, "Refresh: 0; /\n")
  elseif string.match(uri, "^/wearables/") then
    local uuid = string.gsub(uri, "^/wearables/", "")
    local wearables = invloc_retrieve_inventory_item(uuid, "wearables")
    local inv = {}
    for j = 1, 13 do
      inv[j] = ii(uuid, zero_uuid)
    end
    if wearables and wearables.WearablesList then
      for i, w in ipairs(wearables.WearablesList) do
        inv[w.WearableType+1] = ii(uuid, w.ItemID)
      end
    else
      -- body. note the 1-based indexing in lua... grrr.
      -- inv[1] = ii("5c86b033-b9cc-11dc-95ff-0800200c9a66", "66c41e39-38f9-f75a-024e-585989bfab73")
      inv[1] = ii(uuid, "5c86b033-b9cc-11dc-95ff-0800200c9a66")
      -- skin
      inv[2] = ii(uuid, "5c86b030-b9cc-11dc-95ff-0800200c9a66")
      -- hair
      inv[3] = ii(uuid, "d342e6c1-b9d2-11dc-95ff-0800200c9a66")
      -- shirt
      inv[5] = ii(uuid, "d5e46210-b9d1-11dc-95ff-0800200c9a66")
      -- Pants
      inv[6] = ii(uuid, "d5e46211-b9d1-11dc-95ff-0800200c9a66")
    end
    json_header(dh)
    su.dstrcat(dd, Json.Encode(inv))
  elseif string.match(uri, "^/inventory/") then
    -- uuid of the agent 
    local uuid = string.gsub(uri, "^/inventory/", "")
    if pdata then
      print("Trying to store the inventory:", pdata)
      local req = Json.Decode(pdata)
      local res = {}
      if req.Command == "create" then
        local item = invloc_create_inventory_item_x(req.AgentID, req.FolderID, req.arg)
	res.Result = "OK"
	res.Item = item
      elseif req.Command == "fetch_descendents" then
        -- arg.FolderID, arg.OwnerID, arg.SortOrder, arg.FetchFolders, arg.FetchItems
        res.Result = "OK"
	res.Descendents = {}
	local desc = res.Descendents

	if req.arg.FetchFolders then
	  desc.Folders = invloc_retrieve_child_folders(req.AgentID, req.arg.FolderID)
	else
	  desc.Folders = {}
	end
	if req.arg.FetchItems then
	  desc.Items = invloc_retrieve_child_items(req.AgentID, req.arg.FolderID)
	else
	  desc.Items = {}
	end
      elseif req.Command == "update_wearables" then
        local w = {}
	w.WearablesList = req.arg
        res.Result = "OK"
	res.Item = invloc_update_inventory_item(req.AgentID, "wearables", w)
      elseif req.Command == "update" then
        -- group update items, arg is an array of items
	res.Result = "OK"
	res.Items = {}
	for idx, item in ipairs(req.arg) do
	  table.insert(res.Items, invloc_update_inventory_item(req.AgentID, item.ItemID, item))
	end
      elseif req.Command == "retrieve" then
        local item = invloc_retrieve_inventory_item(req.AgentID, req.ItemID)
	if item then
	  res.Result = "OK"
	  res.Item = item
	else 
	  res.Result = "ERROR"
	  res.ErrorText = "Item not found"
	end
      else
        res.Result = "ERROR"
	res.ErrorText = "Command not implemented"
      end
      json_header(dh)
      su.dstrcat(dd, Json.Encode(res))
    else
      print("Unimplemented yet: get inventory")
    end
  else 
    su.dstrcat(dh, "HTTP/1.0 404 Not Found\r\n\r\n")
  end
  -- these two will be appended to header and body respectively
  -- however, the performance of the lua string function is abysmal
  -- so to concat the data, better use dstrcat directly

  -- return "", ""
end

inventoryserver.coldstart = function()
  local ic = config.inventory_server
  print("Starting HTTP Inventory server on", ic.ServerAddress, ic.ServerPort)
  invloc_zap_library()
  inventory_zip_scan("opensim_inventory.zip")
  su.http_start_listener(ic.ServerAddress, ic.ServerPort, "inventory_server_http")
end

