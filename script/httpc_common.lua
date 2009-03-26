-- common HTTP client stuff

http_request = {
  GET = 0
}

function http_client_callback(idx, event, d)
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
      print("ReadAll time!")
      is[idx].ReadAll(idx)
    end
    -- total_conns = total_conns - 1
    -- su.sock_tcp_connect("127.0.0.1", 8080, "test_callback")
    if (is[idx].RecvData) then
      su.dunlock(is[idx].RecvData)
    end
    is[idx].RecvData = nil
  end
end

function http_client_parse_reply(idx)
  local is = inventory_client_sockets
  local reply = su.dgetstr(is[idx].RecvData)
  local http_code, http_message = string.match(reply, "^HTTP/1.[10]%s(%d+)%s(%a+)")
  local sep_idx, sep_idx_end = string.find(reply, "\r\n\r\n")
  local body = nil
  if(sep_idx_end) then
    body = string.sub(reply, sep_idx_end + 1)
  end
  return http_code, body, http_message
end

function http_client_make_req(addr, port, method, url, postdata, uuid, chunk_cb, all_cb)
  local ic = config.inventory_client
  local is = inventory_client_sockets
  local req = {};

  local pp = function(str)
    table.insert(req, str)
  end

  if method == http_request.GET then
    pp('GET')
    postdata = nil 
  elseif method == http_request.POST then
    pp('POST')
  else 
    return nil
  end
  pp(' ' .. url .. ' HTTP/1.0\r\n')
  if postdata then
    pp(postdata)
  end
  pp('\r\n\r\n')

  local idx = su.sock_tcp_connect(ic.ServerAddress, ic.ServerPort, 
                                 "http_client_callback")
  if is[idx] and is[idx].RecvData then
    print("Freeing up previous recvdata for ", idx)
    su.dunlock(is[idx].RecvData)
  end
  is[idx] = {}
  is[idx].SendData = table.concat(req)
  is[idx].AsyncID = uuid
  is[idx].ReadChunk = chunk_cb -- if nil then just accumulate chunks into RecvData
  is[idx].ReadAll = all_cb
  return idx
end

