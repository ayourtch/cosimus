require 'libfmv'
require 'libsupp'
require 'pkt-smv'

smv_sessions = {}
zero_uuid = "00000000-0000-0000-0000-000000000000"

function cr()
  print("\n")
end


function smv_next_seq(sess, p)
  sess.seq = sess.seq + 1
  fmv.SetSequenceNumber(p, sess.seq)
end

-- send an immediate ack for packet d arrived on session sess
function smv_ack_immed(sess, d)
  local p = fmv.new_packet()
  fmv.PacketAckHeader(p)
  fmv.PacketAck_PacketsBlockSize(p, 1)
  fmv.PacketAck_PacketsBlock(p, 0, fmv.GetSequenceNumber(d))

  smv_send_then_unlock(sess, p)
end

-- send the new packet d towards the session sess user
function smv_send_then_unlock(sess, p)
  smv_next_seq(sess, p)
  -- todo - checking against stale sessions
  su.sock_send_data(sess.idx, p)
  fmv.unlock_packet(p)
end

function smv_send_region_handshake(sess)
  local p = fmv.new_packet()
  fmv.RegionHandshakeHeader(p)
  fmv.RegionHandshake_RegionInfo(p, 
      72458694, -- flags
      13, -- SimAccess
      "BlackStar sim\0", -- SimName
      zero_uuid, -- SimOwner (todo)
      0, -- IsEstateManager
      0, -- WaterHeight
      0, -- BillableFactor
      zero_uuid, -- CacheID
      zero_uuid, -- TerrainBase0
      zero_uuid, -- TerrainBase1
      zero_uuid, -- TerrainBase2
      zero_uuid, -- TerrainBase3
      zero_uuid, -- TerrainDetail0
      zero_uuid, -- TerrainDetail1
      zero_uuid, -- TerrainDetail2
      zero_uuid, -- TerrainDetail3
      0.0, -- TerrainStartHeight00
      0.3, -- TerrainStartHeight01
      0.5, -- TerrainStartHeight10
      1.0, -- TerrainStartHeight11
      0.3, -- TerrainHeightRange00
      0.2, -- TerrainHeightRange01
      0.5, -- TerrainHeightRange10
      1000.0 -- TerrainHeightRange11
     ) 
  fmv.RegionHandshake_RegionInfo2(p, 
     zero_uuid -- RegionID
     )
  smv_send_then_unlock(sess, p)
end

function smv_packet(idx, d)
  local gid = fmv.global_id_str(d)
  local remote_addr, remote_port = su.cdata_get_remote4(idx)
  print("Packet received on index " .. tostring(idx) .. " - " .. gid .. "\n")
  if gid == "UseCircuitCode" then
    local circuit_code, session_id, user_id = fmv.Get_UseCircuitCode_CircuitCode(d)
    print("Circuit code: " .. tostring(circuit_code))
    print("session_id: " .. session_id)
    print("user_id: " .. user_id)
    if(smv_sessions[session_id]) then
      print("Duplicate usecircuitcode!\n")
    else
      local sess = {}
      smv_sessions[session_id] = sess
      sess.idx = idx
      sess.circuit_code = circuit_code
      sess.session_id = session_id
      sess.user_id = user_id
      sess.remote_addr = remote_addr
      sess.remote_port = remote_port

      smv_ack_immed(sess, d)
      smv_send_region_handshake(sess)
    end
  elseif gid == "RequestImage" then
    local bs = fmv.Get_RequestImage_RequestImageBlockSize(d)
    print ("Image request blocks: " .. tostring(bs))
  end
end

smv.start_listener("0.0.0.0", 9000)
print("Lua startup complete!\n")
local ncycles = 1
while true do
  print("Running some cycles:", ncycles)
  ncycles = 1 + smv.run_cycles(ncycles)
end

