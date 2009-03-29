
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




-- metaverse scene. Avatars are running around here. Objects are sitting and running around here too.
if not mv_state then
  mv_state = {}
end

if not mv_state.scene then
  mv_state.scene = {}
end
if not mv_state.scene.avatars then
  mv_state.scene.avatars = {}
end
if not mv_state.scene.objects then
  mv_state.scene.objects = {}
end

if not mv_runtime then
  mv_runtime = {}
end

if not mv_runtime.avatar_scene_handlers then
  mv_runtime.avatar_scene_handlers = {} 
end


function scene_update_all_agent_add(av)
  -- send the updates to all the rest of the agents about a new avatar about to be added
end

function scene_update_all_agent_remove(av)
  -- send the updates to all the rest of the agents about a new avatar that is being removed
end

function scene_update_all_agent_move(av)
  -- send the updates to all avatars' agents about the avatar being moved
  -- for now glue the "avatar" and "agent" conceptually here.
  for ag_uuid, ag in pairs(mv_state.scene.avatars) do
    local ev_update_move = mv_runtime.avatar_scene_handlers[ag.StackType].ev_update_move
    ev_update_move(ag, av)
  end
end

function scene_add_avatar(AgentID, StackType, blob_av, label, x, y, z) 
  mv_state.scene.avatars[AgentID] = {}
  local av = mv_state.scene.avatars[AgentID]
  av.AgentID = AgentID
  av.X = x
  av.Y = y
  av.Z = z
  av.Blob = blob_av -- platform-specific data that may not be understood by all the client stacks
  av.StackType = StackType -- client stack type
  print("MV adding avatar to scene")
  pretty("AV", av)
  scene_update_all_agent_add(av)
end

function scene_get_avatar(AgentID)
  return mv_state.scene.avatars[AgentID]
end

function scene_remove_avatar(AgentID)
  -- this will send the cleanups to all the other agents
  local av = mv_state.scene.avatars[AgentID]
  mv_state.scene.avatars[AgentID] = nil
  scene_update_all_agent_remove(av)
end

function scene_move_avatar_by(AgentID, dx, dy, dz)
  -- move the avatar by some offset
  local av = scene_get_avatar(AgentID)
  av.X = av.X + dx
  av.Y = av.Y + dy
  av.Z = av.Z + dz
  scene_update_all_agent_move(av)
end

function mv_scene_register_handlers(StackType, handlers)
  mv_runtime.avatar_scene_handlers[StackType] = handlers
end


