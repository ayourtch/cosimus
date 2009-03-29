package.cpath = ""
package.path = "?.lua;../script/?.lua"
require "inventory_client"

config = {
  inventory_server = {
    ServerAddress = "0.0.0.0";
    ServerPort = 8004;
    DefaultArchives = {
      "opensim_inventory.zip",
      "cosimus_assets_inventory.zip"
    }
  },

  asset_server = {
    ServerAddress = "0.0.0.0";
    ServerPort = 8003;
    DefaultArchives = {
      "opensim_assets.zip",
      "cosimus_assets_inventory.zip"
    }
  },
}

config.inventory_client = {
    ServerAddress = "127.0.0.1";
    ServerPort = config.inventory_server.ServerPort;
  }


config.asset_client = {
    ServerAddress = "127.0.0.1";
    ServerPort = config.asset_server.ServerPort;
  }

function test_callback(a)
  print("In callback")  
end


folder_uuid = "00000112-000f-0000-0000-000100bba001"
arg = {}
arg.FetchFolders = true
arg.FetchItems = true
arg.FolderID = folder_uuid

inventory_client_fetch_inventory_descendents(zero_uuid, folder_uuid, arg, test_callback)


