require 'smvserver'
require 'loginserver'
require 'assetserver'
require 'inventoryserver'

loginserver.coldstart()
smv.coldstart()
inventoryserver.coldstart()
assetserver.coldstart()

