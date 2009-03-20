require 'libreload'

runtime_state = {}

function get_runtime_state(libname)
  print("----- Getting runtime state for " .. libname , runtime_state[libname])
  return runtime_state[libname]
end

function set_runtime_state(libname, state)
  print("----- Setting runtime state for " .. libname , state)
  runtime_state[libname] = state
  return state
end

function runonce(state) 
 local res, result = reload.child("child", state)
 if res == "error_require" then
   print("Error in require:", result)
   print("Fix and hit enter")
   a = io.stdin:read()
 elseif res == "error_run" then
   print("Error in run:", result)
 elseif res == "return" then
   print("Normal return:", result)
 end
end

state = "cold"
while true do
  runonce(state)
  state = "warm"
  print("Press enter to start the new cycle")
  --a = io.stdin:read()
end
