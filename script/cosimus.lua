require 'Json'
require 'smvserver'
require 'inventory_client'
require 'loginserver'
require 'assetserver'
require 'inventoryserver'

loginserver.coldstart()
smv.coldstart()
inventoryserver.coldstart()
assetserver.coldstart()

