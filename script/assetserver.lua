require "startup"
require "httpd_common"

assetserver = {}

print("Asset server!")

function asset_find_tag(xml, tag)
  for i, el in ipairs(xml) do
    if el.label == tag then
      return i
    end
  end
  return nil
end

function parse_asset(s)
  local xml = parse_xml(s)
  local x
  local out = {}
  local val = {}
  if string.match(xml[1], "<?xml ") then
    x = xml[2]
  else
    x = xml[1]
  end
  out.ObjectType = x.label
  out.Value = val
  pretty("x", x)
  for i, el in ipairs(x) do
    local el_val = el[1]
    local lbl = el.label
    if type(el_val) == "table" then
      val[lbl] = el_val[1]
    elseif not (type(el_val) == "string") then
      if el.empty then
        val[lbl] = ""
      else
        print("Error - type ", type(el_val), "for element #", i)
      end
    elseif string.match(el_val, "^true$") then
      val[lbl] = true
    elseif string.match(el_val, "^false$") then
      val[lbl] = false
    elseif string.match(el_val,"^[0-9.]+$") then -- fixme - what about negative ?
      val[lbl] = tonumber(string.match(el_val,"^[0-9.]+$"))
    else 
      val[lbl] = el_val
    end
  end
  return out
end

function unparse_asset(d, asset)
  local pp = function(...) 
    su.dstrcat(d, ...)
  end

  -- pp(string.char(0xEF, 0xBB, 0xBF))
  pp('<?xml version="1.0" encoding="utf-8"?>\n')
  pp('<AssetBase xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">')
    for key, val in pairs(asset) do
      pp('<' .. key .. '>')
      if key == 'FullID' then
        pp('<Guid>')
	pp(tostring(val))
        pp('</Guid>')
      else
	pp(tostring(val))
      end
      pp('</' .. key .. '>')
    end
  pp('</AssetBase>')
end

function get_asset_list(d)
  local pp = function(...) 
    su.dstrcat(d, ...)
  end

  pp('<html><head><title>Asset List</title></head><body>\n')
  pp('<ul>\n')
  for key, val in pairs(smv_state.all_assets) do
    pp('<li><a href="' .. key ..'">' .. key .. '</a></li>\n')
  end
  pp('</ul></body></html>\n')

end

if not smv_state.all_assets then
  smv_state.all_assets = {}
end

function store_asset(asset)
  print("Storing asset:")
  pretty("asset", asset)
  smv_state.all_assets[asset.ID] = asset
end

function get_asset(ID)
  return smv_state.all_assets[ID]
end

function asset_server_http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")

  local pp = function(...) 
    su.dstrcat(dd, ...)
  end

  print("assetserver request for ", uri)

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
  elseif string.match(uri, "^/assets/") then
    -- would rather test the reqtype though...
    if pdata then
      local res = parse_asset(pdata)
      if res.ObjectType == "AssetBase" then
        -- ok, store it
	store_asset(res.Value)
      else
        print("Error - object type is ", res.ObjectType)
      end
    else
      local uuid = string.gsub(uri, '^/assets/', '')
      if uuid then
        local asset = get_asset(uuid)
	if asset then
	  unparse_asset(dd, asset)
	elseif uuid == "list" then
	  su.dstrcat(dh, "Content-Type: text/html\r\n")
	  get_asset_list(dd)
	else
	  su.dstrcat(dh, "HTTP/1.0 404 Not found\r\n\r\n")
	end
      end
    end

  else 
    su.dstrcat(dh, "Refresh: 0; /\n")
  end
  -- these two will be appended to header and body respectively
  -- however, the performance of the lua string function is abysmal
  -- so to concat the data, better use dstrcat directly

  -- return "", ""
end

assetserver.coldstart = function()
  su.http_start_listener("0.0.0.0", 8003, "asset_server_http")
  print("Lua HTTP Asset server startup complete!\n")
end

