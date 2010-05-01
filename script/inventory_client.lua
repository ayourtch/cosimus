
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

-- remote inventory client side
require 'startup'
require 'httpc_common'
require 'async'
require 'json'


inventory_client_sockets = {}

function int_inventory_client_http_req(reqtype, uri, postdata, async_uuid, cb)
  local ic = config.inventory_client
  return http_client_make_req(ic.ServerAddress, ic.ServerPort, reqtype, uri, postdata, async_uuid, nil, cb)
end

function int_inventory_client_rcmd(AgentID, cmd, async_uuid, cb)
  return int_inventory_client_http_req(http_request.POST, "/inventory/" .. AgentID, 
          Json.Encode(cmd), async_uuid, cb)
end

function int_inventory_client_parse_wearables(idx, d)
  -- we've just received wearables, parse them
  local is = inventory_client_sockets
  local http_code, body, http_message = http_client_parse_reply(idx)
  print("Callback int_inventory_client_parse_wearables:", http_code, http_message) -- , body)
  local w = Json.Decode(body)
  -- pretty("wrbl", w)
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
  a.TransactionID = assert(arg.TransactionID)

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


function int_cb_inventory_client_update_default_wearables(idx, d)
  local http_code, body, http_message = http_client_parse_reply(idx)
  print("Got a reply after wearables update", http_code, http_message, body)
  -- there was a notification only, no callback needed.
  -- FIXME: we need to handle the errors.
end

function inventory_client_update_default_wearables(SessionID, AgentID, wearables)
  local cmd = {}
  local uuid, a = int_inventory_client_mkasync(SessionID, AgentID, cb)
  cmd.AgentID = AgentID
  cmd.Command = "update_wearables"
  cmd.arg = wearables
  int_inventory_client_rcmd(AgentID, cmd, uuid, int_cb_inventory_client_update_default_wearables)
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
  cmd.arg = {}
  cmd.arg.FolderID = FolderID
  cmd.arg.ParentID = ParentID
  cmd.arg.Type = Type
  cmd.arg.Name = Name
  int_inventory_client_rcmd(AgentID, cmd, uuid, int_cb_inventory_client_create_folder)
end

function int_cb_inventory_client_fetch_descendents(idx, d)
  local http_code, body, http_message = http_client_parse_reply(idx)
  print("Callback int_cb_inventory_client_fetch_descendents, result: ", http_code, http_message) -- , body)
  if http_code == 200 then
    local res = Json.Decode(body)
    if res.Result == "OK" then
      local a = async_get(inventory_client_sockets[idx].AsyncID)
      -- pretty("DescendentsY", res.Descendents)
      a.Callback(a, res.Descendents)
    else
      fixme_error("No error-callback... FIXME!")
    end
  else
    fixme_error("No error-callback... FIXME!")
  end
end

function inventory_client_fetch_inventory_descendents(SessionID, AgentID, arg, cb)
  local cmd = {}
  local uuid, a = int_inventory_client_mkasync(SessionID, AgentID, cb)
  cmd.Command = "fetch_descendents"
  cmd.AgentID = AgentID
  cmd.arg = arg
  a.arg = arg
  int_inventory_client_rcmd(AgentID, cmd, uuid, int_cb_inventory_client_fetch_descendents)
end


