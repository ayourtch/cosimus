-- remote asset client side
require 'httpc_common'
require 'asset_common'


asset_client_sockets = {}

function int_asset_client_parse_asset(idx, d)
  -- we've just received the asset, parse it
  local is = inventory_client_sockets
  local http_code, body, http_message = http_client_parse_reply(idx)
  local asset_obj = nil
  print("WeGotAssets!!!", http_code, http_message, "'" .. body .. "'")
  local asset_base = nil
  if body then
    asset_obj = parse_asset(body)
    if asset_obj and asset_obj.Value then
      asset_base = asset_obj.Value
      if asset_base.Data then
        asset_base.Data = base64.decode(asset_base.Data)
      end
    end
  end
  local a = async_get(is[idx].AsyncID)
  a.Callback(a, asset_base)
end

function asset_client_request_asset(SessionID, AssetID, arg, cb)
  local ic = config.asset_client
  local a = {}
  a.SessionID = SessionID
  a.Callback = cb
  a.AssetID = AssetID
  a.arg = arg
  local uuid = async_put(a)
  print("Asset Client making request to", ic.ServerAddress, ic.ServerPort)
  http_client_make_req(ic.ServerAddress, ic.ServerPort, http_request.GET,   
           "/assets/" .. AssetID, 
	   nil, uuid, nil, int_asset_client_parse_asset)
  return uuid
end

function asset_client_upload_asset(SessionID, Asset, cb)
  local ic = config.asset_client
  local a = {}
  local d = su.dalloc(2048)
  a.SessionID = SessionID
  a.Callback = cb
  a.AssetID = Asset.AssetID
  local uuid = async_put(a)
  local asset_base = {}
  asset_base.Type = Asset.Type
  asset_base.Tempfile = Asset.Tempfile
  asset_base.FullID = Asset.AssetID
  asset_base.ID = Asset.AssetID
  asset_base.Local = Asset.StoreLocal
  asset_base.Description = "---"
  asset_base.Data = base64.encode(Asset.AssetData)
  print("Asset Client making request to", ic.ServerAddress, ic.ServerPort)
  unparse_asset(d, asset_base)
  http_client_make_req(ic.ServerAddress, ic.ServerPort, http_request.POST,   
           "/assets/" .. a.AssetID, 
	   su.dgetstr(d), uuid, nil, int_asset_client_parse_asset)
  su.dunlock(d)
  return uuid
end
