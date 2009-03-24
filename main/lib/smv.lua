require 'libfmv'
require 'libsupp'
require 'libpktsmv'
require 'serialize'

smv_state = {}
smv_state.assets = {}
smv_state.inventory = {}
require 'persistent_state'

smv_state.transactions = {}
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
  local bitmap = string.rep('\255', 512)
  local p = fmv.packet_new()
  fmv.ParcelPropertiesHeader(p)
  -- print("Length: ", #bitmap)
  
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

function smv_estate_covenant_request(sess, d)
  local p = fmv.packet_new()
  fmv.EstateCovenantReplyHeader(p)
  fmv.EstateCovenantReply_Data(p, 
      zero_uuid, -- CovenantID
      12345, -- CovenantTimestamp
      "Test Estate Name\0", -- EstateName
      sess.AgentID -- EstateOwnerID
    )
  
  smv_send_then_unlock(sess, p)
end

function smv_parcel_dwell_request(sess, d)
  local p = fmv.packet_new()
  local AgentID, SessionID = fmv.Get_ParcelDwellRequest_AgentData(d)
  local LocalID, ParcelID = fmv.Get_ParcelDwellRequest_Data(d)
  print("Parcel dwell request: ", LocalID, ParcelID)
  fmv.ParcelDwellReplyHeader(p)
  fmv.ParcelDwellReply_AgentData(p, AgentID)
  fmv.ParcelDwellReply_Data(p, LocalID, ParcelID, 0);
  
  smv_send_then_unlock(sess, p)
end

function smv_parcel_access_list_request(sess, d)
  local p = fmv.packet_new()
  local AgentID, SessionID = fmv.Get_ParcelAccessListRequest_AgentData(d)
  local SequenceID, Flags, LocalID = fmv.Get_ParcelAccessListRequest_Data(d)
  print("Parcel ACL request: ", SequenceID, Flags, LocalID)
  fmv.ParcelAccessListReplyHeader(p)
  fmv.ParcelAccessListReply_Data(p, AgentID, SequenceID, Flags, LocalID)
  fmv.ParcelAccessListReply_ListBlockSize(p, 1)
  fmv.ParcelAccessListReply_ListBlock(p, 0, AgentID, 10000, 0)
  smv_send_then_unlock(sess, p)
end

function smv_agent_wearables_update(sess, d)
  local p = fmv.packet_new()
  local AgentID, SessionID
  if d then
    AgentID, SessionID = fmv.Get_AgentWearablesRequest_AgentData(d)
  else 
    AgentID, SessionID = sess.AgentID, sess.SessionID
  end
  local inv = smv_state.inventory[AgentID]
  local total_wearables = 0
  print("Wearables update for agent ", AgentID, SessionID)
  
  fmv.AgentWearablesUpdateHeader(p)
  fmv.AgentWearablesUpdate_AgentData(p, AgentID, SessionID, 23456)
  if inv then
    for uuid, item in pairs(inv) do
      fmv.AgentWearablesUpdate_WearableDataBlock(p, total_wearables, 
        uuid, -- ItemID
        item.AssetID, -- AssetID
        item.WearableType -- WearableType
      )
      print("Wearable#:", total_wearables, "uuid:", uuid, "asset:", item.AssetID) 
      total_wearables = total_wearables + 1
    end
    fmv.AgentWearablesUpdate_WearableDataBlockSize(p, total_wearables)
    smv_send_then_unlock(sess, p)
  else
    print("No wearables found")
    su.dunlock(p)
  end
end

function smv_transfer_request(sess, d)
  local TransferID, ChannelType, SourceType, Priority, Params = fmv.Get_TransferRequest_TransferInfo(d)
  print("Transfer request", TransferID, "ChannelType", ChannelType, "SourceType", SourceType)
  print("Priority", Priority, "Param len:", #Params)
  local p = fmv.packet_new()
  local item_id = fmv.uuid_from_bytes(Params)
  local item = smv_state.assets[item_id]
  print("Transfer req for id", item_id)
  fmv.TransferInfoHeader(p)
  if item then
    print("Item found! length:", #(item.AssetData))
    fmv.TransferInfo_TransferInfo(p, 
      TransferID, ChannelType, 
      0,  -- TargetType
      0,  -- Status
      #(item.AssetData), -- Size
      Params)
    smv_send_then_unlock(sess, p)
    p = fmv.packet_new()

    fmv.TransferPacketHeader(p)
    fmv.TransferPacket_TransferData(p, TransferID, ChannelType, 
      0, -- Packet
      1, -- Status
      item.AssetData);
  else
    fmv.TransferInfo_TransferInfo(p, 
      TransferID, ChannelType, 
      0,  -- TargetType
      1,  -- Status
      0, -- Size
      Params)
    print("Item not found")
    
  end
  smv_send_then_unlock(sess, p)
end

function smv_uuid_name_request(sess, d)
  local p = fmv.packet_new()
  local bs = fmv.Get_UUIDNameRequest_UUIDNameBlockBlockSize(d)
  fmv.UUIDNameReplyHeader(p)
  fmv.UUIDNameReply_UUIDNameBlockBlockSize(p, bs)

  for i=0,bs-1 do
    local uuid = fmv.Get_UUIDNameRequest_UUIDNameBlockBlock(d, i)
    print("Request for resolving UUID for ", uuid)
    fmv.UUIDNameReply_UUIDNameBlockBlock(p, i, uuid, "Test\0", "User | example.org\0")
  end
  smv_send_then_unlock(sess, p)
end

function smv_viewer_effect(sess, d)
  local AgentID, SessionID = fmv.Get_ViewerEffect_AgentData(d)
  local bs = fmv.Get_ViewerEffect_EffectBlockSize(d)
  -- print("Viewer effect block size:", bs)
  for i=1,bs do
    local ID, AgentID, Type, Duration, Color, TypeData = fmv.Get_ViewerEffect_EffectBlock(d, i-1)
    -- print("Effect block", ID, AgentID, Type, Duration, #Color, #TypeData)
  end
end

function smv_agent_data_update(sess, d)
  local AgentID, SessionID = fmv.Get_ViewerEffect_AgentData(d)
  local p = fmv.packet_new()
  fmv.AgentDataUpdateHeader(p)
  fmv.AgentDataUpdate_AgentData(p, 
      AgentID,
      "Dalien", -- FirstName
      "Talbot | domain.com", -- LastName
      "TestGroup Title", -- GroupTitle
      AgentID, -- ActiveGroupID
      "\0\0\0\0\0\0\0\0", -- GroupPowers
      "TestGroupName" -- GroupName
      )
  smv_send_then_unlock(sess, p)
end

function smv_create_avatar_data(sess, p, xPos, yPos, zPos)
  -- magic objectdata values.
  local objectdata = string.rep("\0", 15) .. string.char(128) .. 
                     fmv.F32_UDP(xPos+0.01) .. fmv.F32_UDP(yPos) .. fmv.F32_UDP(zPos) ..
                     -- string.rep("\0", 39) .. 
                     string.rep("\0", 28) .. 
                     string.char(128) .. string.rep("\0", 4) .. string.char(102, 140, 61, 189) ..
		     string.rep("\0", 11)
  -- print("Avatar data length:", #objectdata)
  fmv.ObjectUpdate_ObjectDataBlock(p, 0,
    569, -- agent local id
    0, -- State
    sess.AgentID, -- FullID
    0, -- CRC
    47, -- PCode, 
    4, -- Material
    0, -- ClickAction
    1.0, 1.0, 1.0, -- Scale
    objectdata, -- ObjectData[76]
    0, -- ParentID 
    276957501, -- UpdateFlags
    16, -- PathCurve
    1, -- ProfileCurve
    0, -- PathBegin
    0, -- PathEnd
    100, 100, -- PathScale X/Y
    0, 0, -- PathShearX/Y
    0, 0, -- PathTwist / PathTwistBegin
    0, -- PathRadiusOffset
    0, 0, -- PathTaperX/Y
    0, -- PathRevolutions
    0, -- PathSkew
    0, 0, -- Profile Begin/End
    0, -- ProfileHollow
    "", -- TextureEntry
    "", -- TextureAnim
    "FirstName STRING RW SV Test User\nLastName STRING RW SV | example.com\0", --NameValue
    "SomeData", -- Data
    "TestText", -- Text
    "\0\255\0\0", -- TextColor
    "\0", -- MediaURL
    "", -- PSBlock
    "", -- ExtraParams
    zero_uuid, -- Sound
    zero_uuid, -- OwnerID
    0, -- Gain
    0, -- Flags
    0, -- Radius
    0, -- JointType
    10.0, 0.0, 0.0, -- JointPivot
    0.0, 0.0, 0.0 -- JointAxisOrAnchor
  )
end

function smv_x_send_avatar_data(sess)
  local p = fmv.packet_new()
  fmv.ObjectUpdateHeader(p)
  fmv.ObjectUpdate_RegionData(p, smv_get_region_handle(), 32766);
  fmv.ObjectUpdate_ObjectDataBlockSize(p, 1)
  -- fmv.ObjectDataBlock(p, )
  smv_create_avatar_data(sess, p, 15.0, 15.0, 2.0)
  smv_send_then_unlock(sess, p)
end

function smv_agent_update_received(sess, d)
  local AgentID, SessionID, 
    xBodyRotation, yBodyRotation, zBodyRotation, wBodyRotation,
    xHeadRotation, yHeadRotation, zHeadRotation, wHeadRotation,
    State,
    xCameraCenter, yCameraCenter, zCameraCenter,
    xCameraAtAxis, yCameraAtAxis, zCameraAtAxis,
    xCameraLeftAxis, yCameraLeftAxis, zCameraLeftAxis,
    xCameraUpAxis, yCameraUpAxis, zCameraUpAxis,
    Far,
    ControlFlags,
    Flags = fmv.Get_AgentUpdate_AgentData(d)

  -- print("Agent update camera is at:", xCameraCenter, yCameraCenter, zCameraCenter)

end

function smv_asset_upload_request(sess, d)
  -- local comment = [[
  local p = fmv.packet_new()
  local newasset = {}
  local uuid = fmv.uuid_create()
  local TransactionID, Type, Tempfile, StoreLocal, AssetData = fmv.Get_AssetUploadRequest_AssetBlock(d)
  print("Asset upload request: ", TransactionID, Type, Tempfile, StoreLocal, #AssetData)
  print("Asset Data:", AssetData)
  smv_state.transactions[TransactionID] = {}
  smv_state.transactions[TransactionID].AssetID = uuid
  smv_state.assets[uuid] = newasset
  newasset.Type = Type
  newasset.Tempfile = Tempfile
  newasset.StoreLocal = StoreLocal
  newasset.AssetData = AssetData
  fmv.AssetUploadCompleteHeader(p)
  fmv.AssetUploadComplete_AssetBlock(p, uuid, Type, 1)
  smv_send_then_unlock(sess, p)
  -- ]]
end

function smv_inv_create_inventory_item(AgentID, FolderID, TransactionID, Type, InvType, WearableType, Name, Description)
  local ItemID = fmv.uuid_create()
  local inv = smv_state.inventory[AgentID]
  if inv == nil then
    inv = {}
    smv_state.inventory[AgentID] = inv
  end
  local item = {}
  inv[ItemID] = item
  item.AssetID = smv_state.transactions[TransactionID].AssetID
  item.Type = Type
  item.InvType = InvType
  item.WearableType = WearableType
  item.Name = Name
  item.Description = Description
  item.FolderID = FolderID
  return ItemID, item.AssetID
end

function smv_create_inventory_item(sess, d)
  -- local comment = [[
  local p = fmv.packet_new()
  local AgentID, SessionID = fmv.Get_CreateInventoryItem_AgentData(d)
  local CallbackID, FolderID, TransactionID, NextOwnerMask, Type, InvType, 
        WearableType, Name, Description = fmv.Get_CreateInventoryItem_InventoryBlock(d)

  local ItemID = fmv.uuid_create();
  local BaseMask = NextOwnerMask;
  local Flags = 0

  print("TransactionID for create inventory item:", TransactionID)
  print("FolderID", FolderID)
  print("CallbackID", CallbackID)
  if FolderID == "00000000-0000-0000-0000-000000000000" then
    FolderID = "9846e02a-f41b-4199-7777-000000000001"
  end
  local ItemID, AssetID = smv_inv_create_inventory_item(AgentID, FolderID, 
                        TransactionID, Type, InvType, WearableType, Name, Description)


  fmv.UpdateCreateInventoryItemHeader(p)
  fmv.UpdateCreateInventoryItem_AgentData(p, AgentID,
      true, -- SimApproved
      TransactionID -- TransactionID
    )
  fmv.UpdateCreateInventoryItem_InventoryDataBlockSize(p,1)
  fmv.UpdateCreateInventoryItem_InventoryDataBlock(p, 0, 
      ItemID, 
      FolderID, 
      CallbackID,
      AgentID, -- CreatorID
      AgentID, -- OwnerID
      zero_uuid, -- GroupID
      BaseMask, -- BaseMask
      BaseMask, -- OwnerMask
      BaseMask, -- GroupMask
      BaseMask, -- EveryoneMask
      NextOwnerMask, -- NextOwnerMask
      false, -- GroupOwned
      AssetID, -- AssetID
      Type, -- Type
      InvType, -- InvType
      Flags, -- Flags
      0, -- SaleType,
      123, -- SalePrice
      Name, -- Name
      Description, -- Description
      100000, -- CreationDate
      0 -- CRC
    )
  smv_send_then_unlock(sess, p)
  -- ]]
end

function smv_create_inventory_folder(sess, d)
  -- local p = fmv.packet_new()
  local AgentID, SessionID = fmv.Get_CreateInventoryFolder_AgentData(d)
  local FolderID, ParentID, Type, Name = fmv.Get_CreateInventoryFolder_FolderData(d)
  print("Create folder of type ", Type, " parent ", ParentID, " name: ", Name)

end

function smv_agent_cached_texture(sess, d)
  local p = fmv.packet_new()
  local AgentID, SessionID, SerialNum = fmv.Get_AgentCachedTexture_AgentData(d)
  local sz = fmv.Get_AgentCachedTexture_WearableDataBlockSize(d)
  fmv.AgentCachedTextureResponseHeader(p)
  fmv.AgentCachedTextureResponse_AgentData(p, AgentID, SessionID, SerialNum)
  fmv.AgentCachedTextureResponse_WearableDataBlockSize(p, sz)
  for i=0,sz-1 do
    local ID, TextureIndex = fmv.Get_AgentCachedTexture_WearableDataBlock(d, i)
    fmv.AgentCachedTextureResponse_WearableDataBlock(p, i,
       zero_uuid, -- TextureID
       TextureIndex, -- TextureIndex
       "") -- Hostname
  end
  smv_send_then_unlock(sess, p)
end


function smv_packet(idx, d)
  local gid = fmv.global_id_str(d)
  local remote_addr, remote_port = su.cdata_get_remote4(idx)
  local remote_str = remote_addr .. ':' .. tostring(remote_port)
  -- print("Got packet: ", gid)
  if gid == "UseCircuitCode" then
    local circuit_code, session_id, user_id = fmv.Get_UseCircuitCode_CircuitCode(d)
    print("Circuit code: " .. tostring(circuit_code))
    print("session_id: " .. session_id)
    print("user_id: " .. user_id)
    print("smv_state:", smv_state)
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
        -- print("Got a reliable packet!\n")
        smv_ack_immed(sess, d)
      end
      if gid == "PacketAck" then
        -- do nothing
      elseif gid == "CompleteAgentMovement" then
        smv_send_agent_movement_complete(sess)
	smv_send_parcel_overlay(sess)
	smv.SendLayerData(sess)
	-- smv_parcel_properties_request(sess, d)
	-- smv_x_send_avatar_data(sess)

      elseif gid == "StartPingCheck" then
        smv_ping_check_reply(sess, d)
      elseif gid == "CompletePingCheck" then
        smv_ping_check_reply(sess, d)
	smv_x_send_avatar_data(sess)
      elseif gid == "AgentDataUpdateRequest" then
        smv_agent_data_update(sess, d)
      elseif gid == "AgentUpdate" then
        smv_agent_update_received(sess, d)
        -- frequent agent updates go here
      elseif gid == "AgentHeightWidth" then
        smv_agent_width_height(sess, d)
      elseif gid == "AgentWearablesRequest" then
        smv_agent_wearables_update(sess, d)
	smv_x_send_avatar_data(sess)
      elseif gid == "AssetUploadRequest" then
        smv_asset_upload_request(sess, d)
      elseif gid == "CreateInventoryItem" then
        smv_create_inventory_item(sess, d)
      elseif gid == "CreateInventoryFolder" then
        smv_create_inventory_folder(sess, d)
      elseif gid == "UpdateInventoryItem" then
        -- FIXME!!!!
      elseif gid == "TransferRequest" then
        smv_transfer_request(sess, d)
      elseif gid == "AgentCachedTexture" then
	smv_agent_cached_texture(sess, d)
      elseif gid == "MoneyBalanceRequest" then
        smv_send_money_balance(sess, d)
      elseif gid == "LogoutRequest" then
        smv_logout_session(sess)
      elseif gid == "ChatFromViewer" then
        smv_chat_from_viewer(sess, d)
      elseif gid == "ParcelPropertiesRequest" then
        smv_parcel_properties_request(sess, d)
      elseif gid == "EstateCovenantRequest" then
        smv_estate_covenant_request(sess, d)
      elseif gid == "ParcelDwellRequest" then
        smv_parcel_dwell_request(sess, d)
      elseif gid == "ParcelAccessListRequest" then
        smv_parcel_access_list_request(sess, d)
      elseif gid == "SetAlwaysRun" then
      elseif gid == "ViewerEffect" then
        smv_viewer_effect(sess, d)
        -- FIXME: alwaysrun
      elseif gid == "UUIDNameRequest" then
        smv_uuid_name_request(sess, d)
      elseif gid == "RequestImage" then
        local bs = fmv.Get_RequestImage_RequestImageBlockSize(d)
        -- print ("Image request blocks: " .. tostring(bs))
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
  local f = io.open("luastate.lua", "w+")
  f:write(su.dgetstr(s))
  io.close(f)
  print "Serialized Lua state: "
  print(s)
  return s
end
