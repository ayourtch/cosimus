require 'libsupp'
require 'libreload'

function main(start)
  state_orig = reload.parent_run("get_runtime_state", "libsupp", nil)
  print("Orig state", state_orig)
  state = su.libsupp_init(state_orig)
  print("New state:", state)
  reload.parent_run("set_runtime_state", "libsupp", state)


  if state_orig == nil then
    print "coldstart!\n"
    su.http_start_listener("0.0.0.0", 2222);
  end
  print("Running some cycles")
  su.run_cycles(1)
  print("Finished running cycles")
  a = "removed"
  --a = io.stdin:read()
  print ("You entered: ", a)
  -- print(1+tonumber(a))
  return "restart needed"
end
