package.path = "?.lua;../script/?.lua"
package.cpath = ""

require 'startup'
require 'zip_common'

pretty("a", assets_zip_scan("opensim_inventory.zip"))

os.exit(1)

