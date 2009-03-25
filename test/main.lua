function try()
  local su = require('tstlib')
  su.test()
end

aaaa= [[
for k, v in pairs(package.loaded) do
  print(k, ":", v)
end
]]

try()

a = io.stdin:read()
print("You entered: " .. a)

local newt = {}
newt.require = _G.require
newt.print = _G.print
newt.package = _G.package
newt.io = _G.io
newt._G = _G
setfenv(1, newt)

package.loaded.tstlib = nil
package.loaded.su = nil
_G['su'] = nil
_G.collectgarbage("collect")

_G.try()

a = io.stdin:read()
print("You entered: " .. a)


