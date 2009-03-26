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

  local ii = function(itemid, assetid)
    local i = {}
    i.ItemID = itemid
    i.AssetID = assetid
    return i
  end
  print("Inventory server request for ", uri)

  if uri == "/" then
    -- dstrcat(dh, "Refresh: 0; /\n")
  elseif string.match(uri, "^/wearables/") then
    local uuid = string.gsub(uri, "^/wearables/", "")
    local inv = invloc_retrieve_inventory_item(uuid, "wearables")
    if not inv then
      inv = {}
      -- body. note the 1-based indexing in lua... grrr.
      -- inv[1] = ii("5c86b033-b9cc-11dc-95ff-0800200c9a66", "66c41e39-38f9-f75a-024e-585989bfab73")
      inv[1] = ii("d5e46210-b9d1-11dc-95ff-0800200c9a66", "66c41e39-38f9-f75a-024e-585989bfab73")
      -- skin
      inv[2] = ii("5c86b030-b9cc-11dc-95ff-0800200c9a66", "77c41e39-38f9-f75a-024e-585989bbabbb")
      inv[3] = ii("d342e6c1-b9d2-11dc-95ff-0800200c9a66", "d342e6c0-b9d2-11dc-95ff-0800200c9a66")
      inv[4] = ii(zero_uuid, zero_uuid)
      -- shirt
      inv[5] = ii("d5e46210-b9d1-11dc-95ff-0800200c9a66", "00000000-38f9-1111-024e-222222111110")
      -- Pants
      inv[6] = ii("d5e46211-b9d1-11dc-95ff-0800200c9a66", "00000000-38f9-1111-024e-222222111120")
      for i=7,13 do
        inv[i] = ii(zero_uuid, zero_uuid)
      end
    end
    su.dstrcat(dh, "Content-Type: text/json\r\n")
    su.dstrcat(dd, Json.Encode(inv))
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
  inventory_zip_scan("opensim_inventory.zip")
  su.http_start_listener(ic.ServerAddress, ic.ServerPort, "inventory_server_http")
end

