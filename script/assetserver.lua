require "startup"
require "httpd_common"
require 'zip_common'
require 'asset_common'

assetserver = {}

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

if not smv_state.file_assets then
  smv_state.file_assets = {}
end

function load_zip_assets(fname)
  local assets = assets_zip_scan(fname)
  for i, asset in ipairs(assets) do
    a = {}
    a.Type = asset.assetType
    a.ID = asset.assetID
    a.Description = asset.description
    a.Name = asset.Name
    a.ZipFileName = asset.ZipFileName
    a.ItemFileName = asset.ItemFileName
    smv_state.file_assets[a.ID] = a
  end
  print("load_zip_assets: loaded " .. tostring(#assets) .. " assets from file " .. fname)
end

function store_asset(asset)
  print("Storing asset:")
  pretty("asset", asset)
  smv_state.all_assets[asset.ID] = asset
end

function get_asset(ID)
  local a = smv_state.all_assets[ID]
  if not a then
    local fa = smv_state.file_assets[ID]
    if fa then
      a = {}
      a.Type = fa.Type
      a.ID = fa.ID
      a.FullID = fa.FullID
      a.Description = fa.Description
      a.Name = fa.Name
      a.Temporary = false
      a.Local = false
      a.Data = base64.encode(get_zip_content(fa.ZipFileName, fa.ItemFileName))
      print("Got file offset " .. a.Name, a.ID)
    end
  end
  return a
end

function asset_server_http(uri, appdata, dh, dd)
  local qstring = su.get_http_data(appdata, "querystring")
  local pdata = su.get_http_data(appdata, "post_data")

  local pp = function(...) 
    su.dstrcat(dd, ...)
  end

  print("assetserver request for ", uri)

  if uri == "/" then
    su.dstrcat(dh, "HTTP/1.0 404 Not Found\r\n")
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
    su.dstrcat(dh, "HTTP/1.0 404 Not Found\r\n")
  end
  -- these two will be appended to header and body respectively
  -- however, the performance of the lua string function is abysmal
  -- so to concat the data, better use dstrcat directly

  -- return "", ""
end

assetserver.coldstart = function()
  local ic = config.asset_server
  print("Starting HTTP Asset server on", ic.ServerAddress, ic.ServerPort)
  for i, zfile in ipairs(ic.DefaultArchives) do
    load_zip_assets(zfile)
  end
  su.http_start_listener(ic.ServerAddress, ic.ServerPort, "asset_server_http")
  print("Lua HTTP Asset server startup complete!\n")
end

