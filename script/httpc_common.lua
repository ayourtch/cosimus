
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

-- common HTTP client stuff

http_request = {
  GET = 0
}

function http_client_callback(idx, event, d)
  local is = inventory_client_sockets
  local res = 1

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
      local s = su.dgetstr(d)
      su.dstrcat(is[idx].RecvData, s)
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
  return res
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
  return tonumber(http_code), body, http_message
end

function http_client_make_req(addr, port, method, url, postdata, uuid, chunk_cb, all_cb)
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
  if(postdata) then
    pp('Content-Length: ' .. tonumber(#postdata) .. "\r\n")
  end
  pp('\r\n')
  if postdata then
    pp(postdata)
  end
  pp('\r\n')

  local idx = su.sock_tcp_connect(addr, port, 
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

