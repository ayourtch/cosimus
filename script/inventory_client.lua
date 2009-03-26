-- remote inventory client side


inventory_client_sockets = {}
http_request = {
  GET = 0
}

function inventory_client_callback(idx, event, d)
  local is = inventory_client_sockets

  if event == su.SOCK_EVENT_CHANNEL_READY  then
    is[idx].RecvData = su.dalloc(1024)
    local p = su.dalloc(100)
    su.dstrcat(p, is[idx].SendData)
    su.sock_write_data(idx, p)
  elseif event == su.SOCK_EVENT_READ then
    -- print("Got data:", su.dgetstr(d))
    if is[idx].ReadChunk then
      is[idx].ReadChunk(idx, d)
    else
      su.dstrcat(is[idx].RecvData, su.dgetstr(d))
    end
  elseif event == su.SOCK_EVENT_CLOSED then
    if is[idx].ReadAll then
      is[idx].ReadAll(idx)
    end
    -- total_conns = total_conns - 1
    -- su.sock_tcp_connect("127.0.0.1", 8080, "test_callback")
    su.dunlock(is[idx].RecvData)
    is[idx].RecvData = nil
  end

end

function int_inventory_client_parse_wearables(idx)
  -- we've just received wearables, parse them
  local is = inventory_client_sockets
  print("WeGotWearables!!!", su.dgetstr(is[idx].RecvData))
end

function int_inventory_client_make_http_req(method, url, uuid)
  local ic = config.inventory_client
  local is = inventory_client_sockets

  local req = 'GET ' .. url .. ' HTTP/1.0\r\n\r\n'
  local idx = su.sock_tcp_connect(ic.ServerAddress, ic.ServerPort, 
                                 "inventory_client_callback")
  if is[idx] and is[idx].RecvData then
    print("Freeing up previous recvdata for ", idx)
    su.dunlock(is[idx].RecvData)
  end
  is[idx] = {}
  is[idx].SendData = req
  is[idx].AsyncID = uuid
  is[idx].ReadChunk = nil -- just accumulate chunks into RecvData
  is[idx].ReadAll = int_inventory_client_parse_wearables
  return idx
end


function inventory_client_wearables_request(SessionID, AgentID, cb)
  local a = {}
  a.TargetAgentID = AgentID
  a.SessionID = SessionID
  a.Callback = cb
  local uuid = async_put(a)
  int_inventory_client_make_http_req(http_request.GET, 
         "/wearables/" .. AgentID, uuid)
  
  return uuid
end
