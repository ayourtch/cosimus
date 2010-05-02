
--[[

Copyright © 2008-2010 Andrew Yourtchenko, ayourtch@gmail.com.

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

require "startup"
require "httpd_common"

debugserver = {}


-- a few functions from cgilua for parsing the query

function unescape (str)
        str = string.gsub (str, "+", " ")
        str = string.gsub (str, "%%(%x%x)", function(h) return string.char(tonumber(h,16)) end)
        str = string.gsub (str, "\r\n", "\n")
        return str
end

function escape (str)
        str = string.gsub (str, "\n", "\r\n")
        str = string.gsub (str, "([^0-9a-zA-Z ])", -- locale independent
                function (c) return string.format ("%%%02X", string.byte(c)) end)
        str = string.gsub (str, " ", "+")
        return str
end


function insertfield (args, name, value)
        if not args[name] then
                args[name] = value
        else
                local t = type (args[name])
                if t == "string" then
                        args[name] = {
                                args[name],
                                value,
                        }
                elseif t == "table" then
                        table.insert (args[name], value)
                else
                        error ("CGILua fatal error (invalid args table)!")
                end
        end
end

function parsequery (query, args)
        if type(query) == "string" then
                local insertfield, unescape = insertfield, unescape
                string.gsub (query, "([^&=]+)=([^&=]*)&?",
                        function (key, val)
                                insertfield (args, unescape(key), unescape(val))
                        end)
        end
end


function debug_server_http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")
  local ctype = su.get_http_data(appdata, "content-type")

  local pp = function(...) 
    su.dstrcat(dd, ...)
  end

  local tbl = smv_state;

  local path = split("/" .. uri, "[/]")
  local tmp = ""
  local args = {}
  su.dstrcat(dd, "<html><body>")

  if pdata then
    parsequery(pdata, args)
    assert(loadstring(args.eval, "web-input"))()
  else

    for i, el in ipairs(path) do
      tmp = tmp .. el 
      if tmp ~= "/" then
	tmp = tmp .. "/"
      end
      su.dstrcat(dd, '<a href="' .. tmp .. '">' .. el .. '/</a> ')
      if tbl[el] then
	tbl = tbl[el] 
      else
	print("Could not fetch key '" .. el .. "'\n")
      end
    end
    pp('<hr size=1 noshade>')
    pp("<ul>\n")

    for k, v in pairs(tbl) do
      pp("<li>")
      if (type(v) == "table") then
	pp("<a href='" .. tostring(k) .. "/'>" .. tostring(k) .. "</a>")
      else
	pp(tostring(k) .. " : " .. tostring(v))
      end
      pp("</li>\n")
    end 
    pp("</ul>")
  end
  pp("<hr size=1 noshade>")
  pp("<form method='post'>")
  pp("<input type='submit'><br>")
  pp("<textarea name='eval' rows=25 cols=80>")
  if args.eval then
    pp(string.gsub(args.eval, "<", "&lt;"))
  end
  pp("</textarea>")
  pp("</form>")
  
  su.dstrcat(dd, "</body></html>")

  su.dstrcat(dh, "Content-Type: text/html\n")
  -- these two will be appended to header and body respectively
  -- however, the performance of the lua string function is abysmal
  -- so to concat the data, better use dstrcat directly

  -- return "", ""
end

debugserver.coldstart = function()
  local ci = config.debug_server
  su.http_start_listener(ci.ServerAddress, ci.ServerPort, "debug_server_http")
  print("Lua HTTP debug server startup complete!\n")
end

