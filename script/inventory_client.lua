-- remote inventory client side
require 'httpc_common'


inventory_client_sockets = {}

function int_inventory_client_http_req(reqtype, uri, postdata, async_uuid, cb)
  local ic = config.inventory_client
  return http_client_make_req(ic.ServerAddress, ic.ServerPort, reqtype, uri, postdata, uuid, nil, cb)
end

function int_inventory_client_rcmd(AgentID, cmd, async_uuid, cb)
  return int_inventory_client_http_req(http_request.POST, "/inventory/" .. AgentID, 
          Json.Encode(cmd), async_uuid, cb)
end

function int_inventory_client_parse_wearables(idx, d)
  -- we've just received wearables, parse them
  local is = inventory_client_sockets
  local http_code, body, http_message = http_client_parse_reply(idx)
  print("WeGotWearables!!!", http_code, http_message, body)
  local w = Json.Decode(body)
  pretty("wrbl", w)
  local a = async_get(is[idx].AsyncID)
  a.Wearables = w
  a.Callback(a)
end

function inventory_client_wearables_request(SessionID, AgentID, cb)
  local a = {}
  a.TargetAgentID = AgentID
  a.SessionID = SessionID
  a.Callback = cb
  local uuid = async_put(a)
  int_inventory_client_http_req(http_request.GET, "/wearables/" .. AgentID, 
          nil, uuid, int_inventory_client_parse_wearables)
  return uuid
end


function int_cb_inventory_client_create(idx, d)
  local http_code, body, http_message = http_client_parse_reply(idx)
  print("Got a reply from inventory server!!!", http_code, http_message, body)
  -- FIXME: callback and all that
  if http_code == 200 then
    local res = Json.Decode(body)
    if res.Result == "OK" then
      local a = async_get(inventory_client_sockets[idx].AsyncID)
      a.Callback(a, res.Item)
    else
      fixme_error("No error-callback... FIXME!")
    end
  else
    fixme_error("No error-callback... FIXME!")
  end
end

function inventory_client_create_item(SessionID, AgentID, arg, cb)
  local a = {}
  a.AgentID = AgentID
  a.SessionID = SessionID
  a.Callback = cb
  a.CallbackID = assert(arg.CallbackID)

  local uuid = async_put(a)
  local cmd = {}

  cmd.AgentID = AgentID
  cmd.Command = "create"
  cmd.arg = arg

  int_inventory_client_rcmd(AgentID, cmd, uuid, int_cb_inventory_client_create)
  return uuid
end

function int_cb_inventory_client_update(idx, d)
  local http_code, body, http_message = http_client_parse_reply(idx)
  print("Got a reply from inventory server after update!!!", http_code, http_message, body)
  -- FIXME: callback and all that
  if http_code == 200 then
    local res = Json.Decode(body)
    if res.Result == "OK" then
      local a = async_get(inventory_client_sockets[idx].AsyncID)
      a.Callback(a, res.Items)
    else
      fixme_error("No error-callback... FIXME!")
    end
  else
    fixme_error("No error-callback... FIXME!")
  end
end

function int_inventory_client_mkasync(SessionID, AgentID, cb)
  local a = {}
  a.AgentID = AgentID
  a.SessionID = SessionID
  a.Callback = cb
  local uuid = async_put(a)
  return uuid, a
end


function inventory_client_update_items(SessionID, AgentID, items, cb)
  local cmd = {}
  local uuid, a = int_inventory_client_mkasync(SessionID, AgentID, cb)
  a.CallbackIDs = assert(items.CallbackIDs)


  cmd.AgentID = AgentID
  cmd.Command = "update"
  cmd.arg = items.Items
  int_inventory_client_rcmd(AgentID, cmd, uuid, int_cb_inventory_client_update)
  return uuid
end

function int_cb_inventory_client_create_folder(idx, d)
  local http_code, body, http_message = http_client_parse_reply(idx)
  print("Create folder server reply!!!", http_code, http_message, body)
  if http_code == 200 then
    local res = Json.Decode(body)
    if res.Result == "OK" then
      local a = async_get(inventory_client_sockets[idx].AsyncID)
      a.Callback(a, res.Folder)
    else
      fixme_error("No error-callback... FIXME!")
    end
  else
    fixme_error("No error-callback... FIXME!")
  end
end

function inventory_client_create_folder(SessionID, AgentID, FolderID, ParentID, Type, Name, cb)
  local cmd = {}
  local uuid, a = int_inventory_client_mkasync(SessionID, AgentID, cb)
  cmd.Command = "create_folder"
  cmd.AgentID = AgentID
  cmd.arg.FolderID = FolderID
  cmd.arg.ParentID = ParentID
  cmd.arg.Type = Type
  cmd.arg.Name = Name
  int_inventory_client_rcmd(AgentID, cmd, uuid, int_cb_inventory_client_create_folder)
end

