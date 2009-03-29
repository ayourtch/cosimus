
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
require "inventory_local"

loginserver = {}

if not smv_state.users then
  smv_state.users = {}
end
if not smv_state.logins then
  smv_state.logins = {}
end

function loginserver_get_user_record(uid, enforced)
  local r = smv_state.users[uid]
  if (not r) and (not enforced) then
    r = {}
    smv_state.users[uid] = r
    r.FirstName = "FirstRemove"
    r.LastName = "LastMe"
  end
  return r
end

function loginserver_get_user_uuid(first, last, enforced)
  local key = first .. "-" .. last 
  local uuid = smv_state.logins[key]
  if (not uuid) and (not enforced) then
    uuid = fmv.uuid_create()
    print("Creating new login record for ", first, last)
    local r = loginserver_get_user_record(uuid, false)
    r.FirstName = first
    r.LastName = last
    smv_state.logins[key] = uuid
  end
  return uuid
end

function loginserver_get_inventory_root(uid)
  local r = loginserver_get_user_record(uid)
  if not r.InventoryRootID then
    r.InventoryRootID = invloc_create_skeleton(uid)
  end
  return r.InventoryRootID
end

function loginserver_format_skeleton_reply(folders)
  local out = {}
  print("Skeleton folders to format: ", #folders)
  for i, folder in ipairs(folders) do
    local f = {}
    f.folder_id = folder.ID
    f.parent_id = folder.FolderID
    f.name = string.gsub(folder.Name, "&", "&amp;")
    f.type_default = folder.Type
    f.version = folder.Version
    out[1+#out] = f
  end
  -- pretty("OUT", out)
  return out
end


function create_login_response(param, appdata, dh, dd)

  local xxSimParams = {}
  local userids = {}


  local zSessionId = fmv.uuid_create()
  local zSecureSessionId = fmv.uuid_create()

  --  local id_index  = param.first .. "-" .. param.last
  local zAgentId = loginserver_get_user_uuid(param.first, param.last, false)
  local zInventoryRootFolderID = loginserver_get_inventory_root(zAgentId)
  local zInventoryRootLibFolderID = "00000112-000f-0000-0000-000100bba000"
  

  local zSessionId = fmv.uuid_create()
  local zSecureSessionId = fmv.uuid_create()

  local zCircuitCode = math.random(2^24)
  local cfg = config.login_server.DefaultSim
  local zRegionX = cfg.X
  local zRegionY = cfg.Y
  
   xxSimParams["session_id"] = zSessionId:gsub("-","")
   xxSimParams["secure_session_id"] = zSecureSessionId:gsub("-","")
   xxSimParams["firstname"] = param.first
   xxSimParams["lastname"] = param.last
   xxSimParams["agent_id"] = zAgentId:gsub("-", "")
   xxSimParams["circuit_code"] = zCircuitCode
   xxSimParams["startpos_x"] = 128
   xxSimParams["startpos_y"] = 128
   xxSimParams["startpos_z"] = 30
   xxSimParams["regionhandle"] = "12345" -- regionhandle_str(zRegionX, zRegionY)

   local responseData = {}
   local xxGlobalT = {}
   xxGlobalT["sun_texture_id"] = "00000000-3333-0101-0202-000000000001";
   xxGlobalT["cloud_texture_id"] = "00000000-3333-0101-0202-000000000002";
   xxGlobalT["moon_texture_id"] = "00000000-3333-0101-0202-000000000003";

   local xxLoginFlags = {}
   xxLoginFlags["daylight_savings"] = "N"
   xxLoginFlags["stipend_since_login"] = "N"
   xxLoginFlags["gendered"] = "Y"
   xxLoginFlags["ever_logged_in"] = "Y"

   responseData["first_name"] = param["first"]
   responseData["last_name"] = param["last"]
   
   local xxUIconfig = {}
   xxUIconfig["allow_first_life"] = "Y"
   responseData["ui-config"] = { xxUIconfig }

   responseData["inventory-skeleton"] = 
      loginserver_format_skeleton_reply(
         invloc_retrieve_skeleton(zAgentId, zInventoryRootFolderID))

   responseData["inventory-skel-lib"] = 
      loginserver_format_skeleton_reply(
         invloc_retrieve_skeleton("library", zInventoryRootLibFolderID ))

   responseData["inventory-root"] = { { folder_id = zInventoryRootFolderID } }
   responseData["inventory-lib-root"] = { { folder_id = zInventoryRootLibFolderID } }
   responseData["inventory-lib-owner"] = { { agent_id = zAgentId  } }
   responseData["gestures"] = { }
   responseData["gestures"][0] = "array"

   responseData["login-flags"] = { {
      gendered = "Y",
      ever_logged_in = "N" 
      } }

   responseData["initial-outfit"] = {
        { folder_name = "Cosimus Default Outfit", gender = female }
        }
   responseData["seconds_since_epoch"] = os.time()
   responseData["start_location"] = "last";
   responseData["message"] = "Hello there!"
   responseData["circuit_code"] = zCircuitCode  -- random
   responseData["agent_id"] = zAgentId
   responseData["home"] = "\{'region_handle':[r" .. tostring(cfg.X * 256) .. ".0,r" ..tostring(cfg.Y * 256) .. ".0], 'position':[r128.0,r128.0,r30.0], 'look_at':[r0.0,r0.0,r0.0]\}"
   responseData["region_x"] = cfg.X*256
   responseData["region_y"] = cfg.Y*256
   responseData["sim_ip"] = cfg.IP
   responseData["sim_port"] = tonumber(cfg.Port)
   responseData["seed_capability"] = cfg.SeedCap
   responseData["agent_access"] = "M";
   responseData["session_id"] = zSessionId
   responseData["secure_session_id"] = zSecureSessionId
   responseData["login"] = "true"
   return responseData
end

function handle_xmlrpc_login(req, appdata, dh, dd)
  local param = req.params[1]
  local responseData = create_login_response(param, appdata, dh, dd)
  su.dstrcat(dh, "Content-type: text/xml; charset=utf-8\r\n")
  dprint_xml_response(responseData, dd)
end

function login_server_http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")
  local ctype = su.get_http_data(appdata, "content-type")

  local pp = function(...) 
    su.dstrcat(dd, ...)
  end

  if uri == "/" then
    su.dstrcat(dh, "HTTP/1.0 404 Not Found\r\n")
  elseif uri == "/login" and pdata and ctype == "application/xml+llsd" then
    print "LLSD login!"
    local req = parse_llsd(pdata)
    pretty("llsd-parsed", req)
    local param = req[1] 
    local responseData = create_login_response(param, appdata, dh, dd)
    su.dstrcat(dh, "Content-type: application/xml+llsd\r\n")
    su.dstrcat(dd, su.dgetstr(llsd_serialize(responseData)))
  elseif uri == "/login" and pdata then -- and string.match(pdata, "<?xml ") == 1 then
    print "Login!"
    local req = parse_xmlrpc_req(pdata)
    handle_xmlrpc_login(req, appdata, dh, dd)
  elseif string.match(uri, "^/CAPS/") then
    local caps = {}
    print("CAPS request")
    su.dstrcat(dh, "Content-type: application/xml+llsd\r\n")
    caps.MapLayer = "http://127.0.0.1:1/CAPS/x"
    caps.NewFileAgentInventory = "http://127.0.0.1:1/CAPS/x"
    caps.UpdateNotecardAgentInventory = "http://127.0.0.1:1/CAPS/x"
    caps.UpdateScriptAgentInventory = "http://127.0.0.1:1/CAPS/x"
    caps.UpdateScriptTaskInventory = "http://127.0.0.1:1/CAPS/x"
    caps.RequestTextureDownload = "http://127.0.0.1:1/CAPS/x"
    su.dstrcat(dd, su.dgetstr(llsd_serialize(caps)))
  else 
    su.dstrcat(dh, "Refresh: 0; /\n")
  end
  -- these two will be appended to header and body respectively
  -- however, the performance of the lua string function is abysmal
  -- so to concat the data, better use dstrcat directly

  -- return "", ""
end

loginserver.coldstart = function()
  local ci = config.login_server
  su.http_start_listener(ci.ServerAddress, ci.ServerPort, "login_server_http")
  print("Lua HTTP Login server startup complete!\n")
end

