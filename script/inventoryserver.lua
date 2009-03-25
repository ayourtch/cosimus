require "startup"
require "httpd_common"

inventoryserver = {}

print("Inventory server!")

function inventory_server_http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")

  local pp = function(...) 
    su.dstrcat(dd, ...)
  end
  print("Inventory server request for ", uri)

  if uri == "/" then
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
      local file = io.open("tmp/loginserver.txt","a")
      file:write("---- data ----\n")
      file:write(pdata)
      file:write("\n")
      file:close()
    end
  else 
    su.dstrcat(dh, "Refresh: 0; /\n")
  end
  -- these two will be appended to header and body respectively
  -- however, the performance of the lua string function is abysmal
  -- so to concat the data, better use dstrcat directly

  -- return "", ""
end

inventoryserver.coldstart = function()
  su.http_start_listener("0.0.0.0", 8004, "inventory_server_http")
  print("Lua HTTP Inventory server startup complete!")
end

