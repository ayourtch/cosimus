function handle_req(uri, appdata, dh, dd)
  print("URI: ", uri)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")

  print("Query string", qstring)
end

-- su.http_start_listener("0.0.0.0", 7777, "handle_req")

total_conns = 0
HTTP_REQ = "GET / HTTP/1.0\r\n\r\n"
ev_newconn = "newconn"
ev_read = "read"
ev_closed = "closed"
hostname = "127.0.0.1"
callback = "test_callback"

function test_callback(idx, event, d)
  -- print("Event:", idx, event, d)
  if event == su.SOCK_EVENT_CHANNEL_READY  then
    local p = su.dalloc(100)
    su.dstrcat(p, HTTP_REQ)
    su.sock_write_data(idx, p)
  elseif event == su.SOCK_EVENT_READ then
    -- print("Got data:", su.dgetstr(d))
    if (total_conns < 50) then
      -- su.sock_tcp_connect("127.0.0.1", 8080, "test_callback")
      su.sock_tcp_connect(hostname, 8080, callback)
      total_conns = total_conns + 1
    end
  elseif event == su.SOCK_EVENT_CLOSED then
    total_conns = total_conns - 1
    -- su.sock_tcp_connect("127.0.0.1", 8080, "test_callback")
  end
  return 1
end

-- su.set_debug_level(1000, 100)
-- su.sock_tcp_connect("209.85.229.103", 80, "test_callback")
su.sock_tcp_connect("127.0.0.1", 8080, "test_callback")

