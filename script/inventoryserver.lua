require "startup"
require "httpd_common"

inventoryserver = {}

print("Inventory server!")

function inventory_server_http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")

  local pp = function(...) 
    su.dstrcat(dd, ...)
  end
  print("Inventory server request for ", uri)

  if uri == "/" then
    -- dstrcat(dh, "Refresh: 0; /\n")
  elseif string.match(uri, "^/wearables/") then
    local uuid = string.gsub(uri, "^/wearables/", "")
    local inv = invloc_retrieve_inventory_item(uuid, "wearables")
    if not inv then
      inv = {}
      inv[1] = "19053066-19fc-11de-a6fe-33b680117112"
      inv[2] = "1efd535e-19fc-11de-a6fe-33b680117112"
      inv[3] = "3a6cd682-19fc-11de-a6fe-33b680117112"
      inv[4] = "40c5a6ee-19fc-11de-a6fe-33b680117112"

    end
    local db = serialize("local wearables", inv)
    su.dstrcat(dh, "Content-Type: text/x-lua-script\r\n")
    su.dstrcat(dd, su.dgetstr(db))
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
  su.http_start_listener(ic.ServerAddress, ic.ServerPort, "inventory_server_http")
end

