
--[[

Copyright © 2008-2009 Andrew Yourtchenko, ayourtch@gmail.com.

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom 
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
OR OTHER DEALINGS IN THE SOFTWARE. 

--]]



-- metaverse scene, SMV-specific functions. Heavily interacts with packet sending stuff
require 'mv_scene'
require 'smv_state_mgmt'

function smv_scene_allocate_local_id()
  local id = smv_state.scene.next_local_id;
  smv_state.scene.next_local_id = smv_state.scene.next_local_id + 1
  return id
end

function smv_scene_create_avatar_data(sess, p, av)
  -- magic objectdata values.
  local objectdata = string.rep("\0", 15) .. string.char(128) .. 
                     fmv.F32_UDP(av.X) .. fmv.F32_UDP(av.Y) .. fmv.F32_UDP(av.Z) ..
                     -- string.rep("\0", 39) .. 
                     string.rep("\0", 28) .. 
                     string.char(128) .. string.rep("\0", 4) .. string.char(102, 140, 61, 189) ..
		     string.rep("\0", 11)
  -- print("Avatar data length:", #objectdata)
  fmv.ObjectUpdate_ObjectDataBlock(p, 0,
    av.Blob.LocalID, -- agent local id
    0, -- State
    av.AgentID, -- FullID
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
    "FirstName STRING RW SV " .. av.Label .. "\nLastName STRING RW SV |" .. tostring(av.Blob.LocalID) .. "|\0", --NameValue
    "SomeData", -- Data
    "TestText\0", -- Text
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


function smv_scene_send_avatar_data_to_session(SessionID, AgentID)
  local p = fmv.packet_new()
  local av = scene_get_avatar(AgentID)
  local sess = smv_get_session(SessionID)
  print("Sending data about " .. AgentID .. " to " .. SessionID .. " index " .. sess.idx)
  fmv.ObjectUpdateHeader(p)
  fmv.ObjectUpdate_RegionData(p, smv_get_region_handle(), 32766);
  fmv.ObjectUpdate_ObjectDataBlockSize(p, 1)
  -- fmv.ObjectDataBlock(p, )
  smv_scene_create_avatar_data(sess, p, av)
  smv_send_then_unlock(sess, p)
end

function smv_scene_ev_update_move(TargetAv, MovedAv)
  smv_scene_send_avatar_data_to_session(smv_get_session_id_agent_id(TargetAv.AgentID), MovedAv.AgentID)
end

function smv_scene_ev_update_add(TargetAv, AddedAv)
  smv_scene_send_avatar_data_to_session(smv_get_session_id_agent_id(TargetAv.AgentID), AddedAv.AgentID)
end

function smv_scene_add_avatar(SessionID, AgentID, blob_av, label, x, y, z) 
  local blob = {}
  blob.LocalID = smv_scene_allocate_local_id()

  scene_add_avatar(AgentID, "SMV", blob, label, x, y, z)
  -- send the update to this session too.
  smv_scene_send_avatar_data_to_session(SessionID, AgentID)
end

function smv_scene_remove_avatar(AgentID)
  scene_remove_avatar(AgentID)
end

function smv_scene_move_avatar_by(AgentID, dx, dy, dz)
  -- move the avatar by some offset
  scene_move_avatar_by(AgentID, dx, dy, dz)
end

smv_scene_handlers = {
  ev_update_move = smv_scene_ev_update_move,
  ev_update_add = smv_scene_ev_update_add
}

mv_scene_register_handlers("SMV", smv_scene_handlers)
