-- remote inventory client side
require 'httpc_common'


inventory_client_sockets = {}

function int_inventory_client_parse_wearables(idx, d)
  -- we've just received wearables, parse them
  print ("DDD: ", d)
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
  local ic = config.inventory_client
  local a = {}
  a.TargetAgentID = AgentID
  a.SessionID = SessionID
  a.Callback = cb
  local uuid = async_put(a)
  http_client_make_req(ic.ServerAddress, ic.ServerPort, http_request.GET,   
           "/wearables/" .. AgentID, 
	   nil, uuid, nil, int_inventory_client_parse_wearables)
  
  return uuid
end
