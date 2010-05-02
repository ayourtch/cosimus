--[[

Copyright (c) 2008-2009 Andrew Yourtchenko, ayourtch@gmail.com.

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

package.cpath = ""
package.path = "?.lua;../script/?.lua"

default_ip = os.getenv("COSIMUS_DEFAULT_IP") or "127.0.0.1"

default_sim_ip_for_login_server = default_ip
default_login_server_ip = default_ip
default_login_server_port = 7777


config = {
  login_server = {
    ServerAddress = "0.0.0.0";
    ServerPort = 7777;
    DefaultSim = {
      IP = default_sim_ip_for_login_server;
      Port = "9000";
      X = 1000;
      Y = 1000;
      SimName = "DalienLand";
      AssetServerURL = "http://" .. default_sim_ip_for_login_server .. ":8003/";
      SeedCap = "http://" .. default_login_server_ip .. ":" .. tostring(default_login_server_port) .. "/CAPS/seed/"
    }
  },

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

  debug_server = {
    ServerAddress = "0.0.0.0";
    ServerPort = 8090;
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
local zInventoryRootLibFolderID = "00000112-000f-0000-0000-000100bba000"
x = loginserver_format_skeleton_reply(
         invloc_retrieve_skeleton("library", zInventoryRootLibFolderID ))
print(x)

