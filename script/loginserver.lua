require "startup"
require "httpd_common"
require "inventory_local"

loginserver = {}

cfg = {}
cfg.IP = "192.168.1.2"
cfg.IP = "127.0.0.1"
cfg.Name = "DalienLand"
cfg.Port = "9000"
cfg.X = 1000
cfg.Y = 1000
cfg.AssetServerURL = "http://" .. cfg.IP .. ":8003/"
cfg.UserServerURL = "http://" .. cfg.IP .. ":8003/"
cfg.SeedCap = "http://" .. cfg.IP .. ":7777/CAPS/seed/"

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
    f.name = folder.Name
    f.type_default = folder.Type
    f.version = folder.Version
    out[1+#out] = f
  end
  pretty("OUT", out)
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

   responseData["initial-outfit"] = {
        { folder_name = "Clothing", gender = male }
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

print("Login server!")

function http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")
  local ctype = su.get_http_data(appdata, "content-type")

  local pp = function(...) 
    su.dstrcat(dd, ...)
  end

  if uri == "/" then
    print(hexstr(dgetstr(appdata, 0, 30)))
    -- dstrcat(dh, "Refresh: 0; /\n")
    pp('<html><body><h1>Test form</h1><form action="/foo" method="post"><textarea name="xxx"></textarea><input type="submit"></form></body></html>')
  elseif uri == "/" then 

    pp("<html><body>\n")
    pp("<h1>header test for uri:", uri, "</h1>")
    pp("<p>post len: " , su.get_http_data(appdata, "post_data_length") , "</p>")
    pp("<p>Post data: <pre>", pdata, "</pre></p>")
    pp(tostring(os.time()))
    pp("<p><b>query: ", su.get_http_data(appdata, "querystring"), "</b></p>\n")
    pp("</body></html>")
    if pdata then
      print("PostData:", pdata)
    end

    local file = io.open("tmp/loginserver.txt","a")
    file:write("---- data ----\n")
    file:write(pdata)
    file:write("\n")
    file:close()
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
  su.http_start_listener("0.0.0.0", 7777, "http")
  print("Lua HTTP Login server startup complete!\n")
end


x1 = parse_xmlrpc_req[[
<?xml version="1.0"?><methodCall><methodName>login_to_simulator</methodName><params><param><value><struct><member><name>first</name><value><string>Dalien</string></value></member><member><name>last</name><value><string>Talbot</string></value></member><member><name>passwd</name><value><string>somehash</string></value></member><member><name>start</name><value><string>home</string></value></member><member><name>version</name><value><string>Second Life Release 1.21.6.99587</string></value></member><member><name>channel</name><value><string>Second Life Release</string></value></member><member><name>platform</name><value><string>Win</string></value></member><member><name>mac</name><value><string>aaaaaaaaaaaaaaaaa</string></value></member><member><name>id0</name><value><string/></value></member><member><name>last_exec_event</name><value><int>0</int></value></member><member><name>options</name><value><array><data><value><string>inventory-root</string></value><value><string>inventory-skeleton</string></value><value><string>inventory-lib-root</string></value><value><string>inventory-lib-owner</string></value><value><string>inventory-skel-lib</string></value><value><string>initial-outfit</string></value><value><string>gestures</string></value><value><string>event_categories</string></value><value><string>event_notifications</string></value><value><string>classified_categories</string></value><value><string>buddy-list</string></value><value><string>ui-config</string></value><value><string>tutorial_setting</string></value><value><string>login-flags</string></value><value><string>global-textures</string></value></data></array></value></member></struct></value></param></params></methodCall>
]]

-- print(pretty("arg", x1.params[1]))
-- handle_xmlrpc_login(x1, nil, nil, nil)

pretty_req = [[
print(pretty("req", x1))


req = {}
req["method"] = "login_to_simulator"
req["params"] = {}
req["params"][1] = {}
req["params"][1]["mac"] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
req["params"][1]["platform"] = "Win"
req["params"][1]["options"] = {}
req["params"][1]["options"][1] = "inventory-root"
req["params"][1]["options"][2] = "inventory-skeleton"
req["params"][1]["options"][3] = "inventory-lib-root"
req["params"][1]["options"][4] = "inventory-lib-owner"
req["params"][1]["options"][5] = "inventory-skel-lib"
req["params"][1]["options"][6] = "initial-outfit"
req["params"][1]["options"][7] = "gestures"
req["params"][1]["options"][8] = "event_categories"
req["params"][1]["options"][9] = "event_notifications"
req["params"][1]["options"][10] = "classified_categories"
req["params"][1]["options"][11] = "buddy-list"
req["params"][1]["options"][12] = "ui-config"
req["params"][1]["options"][13] = "tutorial_setting"
req["params"][1]["options"][14] = "login-flags"
req["params"][1]["options"][15] = "global-textures"
req["params"][1]["first"] = "Dalien"
req["params"][1]["passwd"] = "$1$aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
req["params"][1]["version"] = "Second Life Release 1.21.6.99587"
req["params"][1]["channel"] = "Second Life Release"
req["params"][1]["start"] = "home"
req["params"][1]["last_exec_event"] = 0
req["params"][1]["id0"] = ""
req["params"][1]["last"] = "Talbot"
]]
