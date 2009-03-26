package.path = "?.lua;../script/?.lua"
package.cpath = ""
smv_state = {}
require 'startup'
require 'zip_common'

-- pretty("a", assets_zip_scan("opensim_assets.zip"))
pretty("a", inventory_zip_scan("opensim_inventory.zip"))

os.exit(1)

