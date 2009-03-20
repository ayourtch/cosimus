require 'libreload'
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
  a = io.stdin:read()
end
