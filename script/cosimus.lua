require 'loginserver'
require 'smvserver'
require 'assetserver'
require 'inventoryserver'

loginserver.coldstart()
smv.coldstart()
inventoryserver.coldstart()
assetserver.coldstart()

