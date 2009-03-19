require 'libfmv'
require 'pkt-smv'

function cr()
  print("\n")
end

function smv_packet(idx, d)
  local gid = fmv.global_id_str(d)
  print("Packet received on index " .. tostring(idx) .. " - " .. gid .. "\n")
  if gid == "UseCircuitCode" then
    local circuit_code, session_id, user_id = fmv.Get_UseCircuitCode_CircuitCode(d)
    print("Circuit code: " .. tostring(circuit_code))
    -- print("session_id: " .. session_id)
    --print("user_id: " .. user_id)
    cr()
  elseif gid == "RequestImage" then
    local bs = fmv.Get_RequestImage_RequestImageBlockSize(d)
    print ("Image request blocks: " .. tostring(bs))
    cr()
  end
end

smv.start_listener("0.0.0.0", 9000)
print("Lua startup complete!\n")
local ncycles = 1
while true do
  print("Running some cycles:", ncycles)
  ncycles = 1 + smv.run_cycles(ncycles)
end

