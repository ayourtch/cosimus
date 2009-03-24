require 'libfmv'
require 'libsupp'
require 'libpktsmv'
require 'serialize'

smv_state = {}
smv_state.sessions = {}
smv_state.sess_id_by_remote = {}

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
  local p = fmv.packet_new()
  fmv.PacketAckHeader(p)
  fmv.PacketAck_PacketsBlockSize(p, 1)
  fmv.PacketAck_PacketsBlock(p, 0, fmv.GetSequenceNumber(d))

  smv_send_then_unlock(sess, p)
end

-- send the new packet d towards the session sess user
function smv_send_then_unlock(sess, p)
  local p1
  smv_next_seq(sess, p)
  fmv.FinalizePacketLength(p)
  p1 = fmv.MaybeZeroEncodePacket(p)
  -- todo - checking against stale sessions
  su.sock_send_data(sess.idx, p1)
  -- su.print_dbuf(0, 0, p)
  fmv.packet_unlock(p)
  fmv.packet_unlock(p1)
end

function smv_send_region_handshake(sess)
  local p = fmv.packet_new()
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


function smv_send_parcel_overlay(sess)
  local LAND_BLOCKS_PER_PACKET = 1024
  local sequence = 0
  local parceldata = ''
  for y = 1, 64 do
    for x = 1, 64 do
      -- fill the data about the land somewhere
      parceldata = parceldata .. '\0'
      if (#parceldata >= LAND_BLOCKS_PER_PACKET) then
        local p = fmv.packet_new()
	fmv.ParcelOverlayHeader(p)
	fmv.ParcelOverlay_ParcelData(p, sequence, parceldata)
	sequence = sequence + 1
	smv_send_then_unlock(sess, p)
	parceldata = ''
      end
    end
  end
end

function smv_get_region_handle()
  return fmv.GetRegionHandle(1000, 1000)
end

function smv_ping_check_reply(sess, ping)
 su.dlock(ping)
 smv_send_then_unlock(sess, ping)
end

function smv_send_agent_movement_complete(sess)
  local p = fmv.packet_new()
  fmv.AgentMovementCompleteHeader(p)
  fmv.AgentMovementComplete_AgentData(p, sess.AgentID, sess.SessionID)
  fmv.AgentMovementComplete_Data(p, 
     30, 30, 30, -- Position
     0,0,0, -- LookAt
     smv_get_region_handle(), -- RegionHandle
     0 -- Timestamp
     )
  smv_send_then_unlock(sess, p)
end

function smv_send_money_balance(sess, d)
  local p = fmv.packet_new()
  local TransactionID = fmv.Get_MoneyBalanceRequest_MoneyData(d)
  fmv.MoneyBalanceReplyHeader(p)
  fmv.MoneyBalanceReply_MoneyData(p, sess.AgentID, TransactionID, 
     1, -- TransactionSuccess
     1000.0, -- MoneyBalance
     0, -- SquareMetersCredit
     0, -- SquareMetersCommitted
     "" -- Description
     )
  smv_send_then_unlock(sess, p)
end

function smv_logout_session(sess)
  local session_id = sess.SessionID
  print("Logging out session ", session_id)
  smv_state.sess_id_by_remote[sess.remote_str] = nil
  smv_state.sessions[session_id] = nil
end

function smv_agent_width_height(sess, d)
  local GenCounter, Height, Width = fmv.Get_AgentHeightWidth_HeightWidthBlock(d)
  print ("Agent gencounter: ", GenCounter, " width/height: ", Width, Height)
end

function smv_chat_from_viewer(sess, d)
  local AgentID, SessionID = fmv.Get_ChatFromViewer_AgentData(d)
  local Message, Type, Channel = fmv.Get_ChatFromViewer_ChatData(d)
  print("Chat from viewer type ", Type, " channel ", Channel, " message ", Message)
end

function smv_parcel_properties_request(sess, d)
  local AgentID, SessionID = fmv.Get_ParcelPropertiesRequest_AgentData(d)
  local SequenceID, West, South, East, North, SnapSelection = fmv.Get_ParcelPropertiesRequest_ParcelData(d)
  local bitmap = 
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255" ..
    "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255"
  local p = fmv.packet_new()
  fmv.ParcelPropertiesHeader(p)
  print("Length: ", #bitmap)
  
  fmv.ParcelProperties_ParcelData(p, 
     1, -- RequestResult
     SequenceID, -- SequenceID
     SnapSelection, -- SnapSelection
     1, -- SelfCount
     0, -- OtherCount
     1, -- PublicCount
     12345, -- LocalID (of what?)
     AgentID, -- OwnerID, temporarily set to user id for test
     0, -- IsGroupOwned
     0, -- AuctionID
     10000, -- ClaimDate
     0, -- ClaimPrice
     0, -- RentPrice
     0.0, 0.0, 0.0, -- AABBMin
     256.0, 256.0, 256.0, -- AABBMax
     bitmap, -- Bitmap
     65536, -- Area
     0, -- Status
     12345, -- SimWideMaxPrims
     33, -- SimWideTotalPrims
     250, -- MaxPrims
     200, -- TotalPrims
     150, -- OwnerPrims
     10, -- GroupPrims
     40, -- OtherPrims
     0, -- SelectedPrims
     0.0, -- ParcelPrimBonus
     0, -- OtherCleanTime
     0, -- ParcelFlags
     0, -- SalePrice
     "Dalien's parcel test\0", -- Name
     "Description of dalien's parcel here\0", -- Desc
     "http://127.0.0.1/test\0", -- MusicURL
     "http://127.0.0.1/test2\0", -- MediaURL
     0, -- MediaID
     1, -- MediaAutoScale
     zero_uuid, -- GroupID
     0, -- PassPrice
     0, -- PassHours
     0, -- Category
     zero_uuid, -- AuthBuyerID
     zero_uuid, -- SnapshotID
     0.0, 0.0, 0.0, -- UserLocation
     0.0, 0.0, 0.0, -- UserLookAt
     0, -- LandingType
     0, -- RegionPushOverride
     0, -- RegionDenyAnonymous
     0, -- RegionDenyIdentified
     0 -- RegionDenyTransacted
     )
  fmv.ParcelProperties_AgeVerificationBlock(p, 
     0 -- RegionDenyAgeUnverified
     )
  smv_send_then_unlock(sess, p)

end

function smv_packet(idx, d)
  local gid = fmv.global_id_str(d)
  local remote_addr, remote_port = su.cdata_get_remote4(idx)
  local remote_str = remote_addr .. ':' .. tostring(remote_port)
  if gid == "UseCircuitCode" then
    local circuit_code, session_id, user_id = fmv.Get_UseCircuitCode_CircuitCode(d)
    print("Circuit code: " .. tostring(circuit_code))
    print("session_id: " .. session_id)
    print("user_id: " .. user_id)
    if(smv_state.sessions[session_id]) then
      print("Duplicate usecircuitcode!\n")
    else
      local sess = {}
      smv_state.sessions[session_id] = sess
      smv_state.sess_id_by_remote[remote_str] = session_id

      sess.idx = idx
      sess.circuit_code = circuit_code
      sess.SessionID = session_id
      sess.AgentID = user_id
      sess.remote_addr = remote_addr
      sess.remote_port = remote_port
      sess.remote_str = remote_str
      sess.seq = 0

      smv_ack_immed(sess, d)
      smv_send_region_handshake(sess)
    end
  else 
    local sess = smv_state.sessions[smv_state.sess_id_by_remote[remote_str]]
    if sess then
      if gid == "PacketAck" then
      elseif fmv.IsReliable(d) then
        print("Got a reliable packet!\n")
        smv_ack_immed(sess, d)
      end
      if gid == "PacketAck" then
        -- do nothing
      elseif gid == "CompleteAgentMovement" then
        smv_send_agent_movement_complete(sess)
	smv_send_parcel_overlay(sess)
	smv.SendLayerData(sess)
	smv_parcel_properties_request(sess, d)

      elseif gid == "StartPingCheck" then
        smv_ping_check_reply(sess, d)
      elseif gid == "CompletePingCheck" then
        smv_ping_check_reply(sess, d)
      elseif gid == "AgentUpdate" then
        -- frequent agent updates go here
      elseif gid == "AgentHeightWidth" then
        smv_agent_width_height(sess, d)
      elseif gid == "MoneyBalanceRequest" then
        smv_send_money_balance(sess, d)
      elseif gid == "LogoutRequest" then
        smv_logout_session(sess)
      elseif gid == "ChatFromViewer" then
        smv_chat_from_viewer(sess, d)
      elseif gid == "ParcelPropertiesRequest" then
        smv_parcel_properties_request(sess, d)
      elseif gid == "RequestImage" then
        local bs = fmv.Get_RequestImage_RequestImageBlockSize(d)
        print ("Image request blocks: " .. tostring(bs))
      else
        print("Packet received on index " .. tostring(idx) .. " - " .. gid .. "\n")
      end
    else
      print("Could not find session for remote " .. remote_str)
    end
  end
end

smv.coldstart = function()
  smv.start_listener("0.0.0.0", 9000)
  print("Lua SMV startup complete!\n")
end

smv.serialize = function()
  local s = serialize("smv_state", smv_state)
  print "Serialized Lua state: "
  print(s)
  return s
end
