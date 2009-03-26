package.path = "?.lua;../script/?.lua"
package.cpath = ""

config = {}


config.inventory_server = {
  ServerAddress = "127.0.0.1";
  ServerPort = 8004;
}

config.inventory_client = {
  ServerAddress = config.inventory_server.ServerAddress;
  ServerPort = config.inventory_server.ServerPort;
}

require 'cosimus'

su.set_debug_level(1000, 100)

