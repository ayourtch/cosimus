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

-- SMV state management will migrate here.

-- try to restore the persistent state which was saved upon interrupt
require 'luastate'

if not smv_state then
  smv_state = {}
end

if not smv_state.assets then
  smv_state.assets = {}
end

if not smv_state.inventory then
  smv_state.inventory = {}
end

if not smv_state.transactions then
  smv_state.transactions = {}
end

if not smv_state.sessions then
  smv_state.sessions = {}
end

if not smv_state.sess_id_by_remote then
  smv_state.sess_id_by_remote = {}
end

if not smv_state.sess_id_by_agent_id then
  smv_state.sess_id_by_agent_id = {}
end

if not smv_state.scene then
  smv_state.scene = {}
end

if not smv_state.scene.next_local_id then
  smv_state.scene.next_local_id = 1000
end

