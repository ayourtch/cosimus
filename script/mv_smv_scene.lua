
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

function smv_scene_send_avatar_data_to_session(SessionID, AgentID)
  local p = fmv.packet_new()
  local av = scene_get_avatar(AgentID)
  local sess = smv_get_session(SessionID)
  fmv.ObjectUpdateHeader(p)
  fmv.ObjectUpdate_RegionData(p, smv_get_region_handle(), 32766);
  fmv.ObjectUpdate_ObjectDataBlockSize(p, 1)
  -- fmv.ObjectDataBlock(p, )
  smv_create_avatar_data(sess, p, av.X, av.Y, av.Z)
  smv_send_then_unlock(sess, p)
end

function smv_scene_ev_update_move(TargetAv, MovedAv)
  smv_scene_send_avatar_data_to_session(smv_get_session_id_agent_id(TargetAv.AgentID), MovedAv.AgentID)
end

function smv_scene_add_avatar(SessionID, AgentID, blob_av, label, x, y, z) 
  local blob = {}
  local handlers = {
    ev_update_move = smv_scene_ev_update_move
  }
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
  ev_update_move = smv_scene_ev_update_move
}

mv_scene_register_handlers("SMV", smv_scene_handlers)
