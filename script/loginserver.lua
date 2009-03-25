require "startup"
require "httpd_common"

loginserver = {}

cfg = {}
cfg.IP = "192.168.1.2"
cfg.Name = "DalienLand"
cfg.Port = "9000"
cfg.X = 1000
cfg.Y = 1000
cfg.AssetServerURL = "http://" .. cfg.IP .. ":8003/"
cfg.UserServerURL = "http://" .. cfg.IP .. ":8003/"
cfg.SeedCap = "http://" .. cfg.IP .. ":8003/CAPS/seed/"

function handle_xmlrpc_login(req, appdata, dh, dd)
  local xxSimParams = {}
  local userids = {}
  local param = req.params[1]
  userids["Test-User"] =      "6ee4df6a-0ea9-4cf5-8ac7-cccccccccccc"
  userids["test-user"] =      "6ee4df6a-0ea9-4cf5-8ac7-cccccccccccc"
  userids["Bot-User"] =       "6ee4df6a-0ea9-4cf5-8ac7-dddddddddddd"
  userids["Dalien-Talbot"] =  "6ee4df6a-0ea9-4cf5-8ac7-eeeeeeeeeeee"

  local zSessionId = "133086b6-1270-78c6-66f7-c7f64865b16c"
  local zSecureSessionId = "6ee4df6a-0ea9-4cf5-8ac7-9745acbacccc"

  local id_index  = param.first .. "-" .. param.last
  local zAgentId = "6ee4df6a-0ea9-4cf5-8ac7-cccccccccccc"
  zAgentId = userids[id_index]

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

   responseData["inventory-skeleton"] = {
          { folder_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            parent_id = "00000000-0000-0000-0000-000000000000",
            name = "My Inventory",
            type_default = 8,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000003",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Animations",
            type_default = 20,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000002",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Body Parts",
            type_default = 13,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000004",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Calling Cards",
            type_default = 2,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000001",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Clothing",
            type_default = 5,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000005",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Gestures",
            type_default = 21,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000006",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Landmarks",
            type_default = 3,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000007",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Lost And Found",
            type_default = 16,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000008",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Notecards",
            type_default = 7,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000009",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Objects",
            type_default = 6,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-00000000000a",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Photo Album",
            type_default = 15,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-00000000000b",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Scripts",
            type_default = 10,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-00000000000c",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Sounds",
            type_default = 1,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-00000000000d",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Textures",
            type_default = 12,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-00000000000e",
            parent_id = "9846e02a-f41b-4199-860e-cde46cc25649",
            name = "Trash",
            type_default = 14,
            version = 1 }
   }

   responseData["inventory-skel-lib"] = {
          { folder_id = "a846e02a-f41b-4199-860e-cde46cc25654",
            parent_id = "00000000-0000-0000-0000-000000000000",
            name = "Lib Inventory",
            type_default = 8,
            version = 1 },
	  { folder_id = "9846e02a-f41b-4199-7777-000000000103",
            parent_id = "a846e02a-f41b-4199-860e-cde46cc25654",
            name = "Male Shape &amp; Outfit",
            type_default = 5,
            version = 1 }
   } 

   responseData["inventory-root"] = { { folder_id = "9846e02a-f41b-4199-860e-cde46cc25649" } }
   responseData["inventory-lib-root"] = { { folder_id = "a846e02a-f41b-4199-860e-cde46cc25654" } }
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

   su.dstrcat(dh, "Content-type: text/xml; charset=utf-8")

   dprint_xml_response(responseData, dd)
end

print("Login server!")
junk = [[
a1 = "\255\255\255\255"
a0 = "\000\000\000\000"
a01 = "\085\085\085\085"
a10 = "\170\170\170\170"
print(binstr(a0))
print(binstr(a1))
print(binstr(a01))
print(binstr(a10))
print(hexstr(andl(a1,a0)))
print(hexstr(xorl(a1,a10 .. a10)))
]]

function http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")

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
  elseif uri == "/login" and pdata then -- and string.match(pdata, "<?xml ") == 1 then
    print "Login!"
    local req = parse_xmlrpc_req(pdata)
    handle_xmlrpc_login(req, appdata, dh, dd)
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
