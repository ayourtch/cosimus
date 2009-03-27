package.path = "?.lua;../script/?.lua"
package.cpath = ""

config = {
  inventory_server = {
    ServerAddress = "0.0.0.0";
    ServerPort = 8004;
  },

  asset_server = {
    ServerAddress = "0.0.0.0";
    ServerPort = 8003;
    DefaultArchives = {
      "opensim_assets.zip"
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

require 'cosimus'

-- su.set_debug_level(1000, 100)

