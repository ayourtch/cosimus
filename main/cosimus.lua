require 'setpaths'
require 'libreload'

-- Runtime state, gets written by the child namespace 
-- Contains the userdata pointers to the pieces of memory 
-- to use for reinitializing the stateful parts of libs
runtime_state = {}

function get_runtime_state(libname)
  return runtime_state[libname]
end

function set_runtime_state(libname, state)
  runtime_state[libname] = state
  return state
end

function pause(message)
  print(message)
  return io.stdin:read()
end


function run_child(childname, arg)
  local result, details = libreload.child(childname, arg)
  if result == "error_require" then
    print("Error in require:", details)
    pause("Fix and press Enter when ready")
  elseif result == "error_run" then
    print("Error in run:", details)
  elseif result == "return" then
    print("Normal return: ", details)
  end
  return result, details
end

keep_running = true

while keep_running do
  local result, details = run_child("child_cos", "---")
  if result == "return" then
    if details == "quit" then
      keep_running = false
    end
  end
end
