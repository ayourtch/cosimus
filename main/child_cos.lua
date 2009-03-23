require 'setpaths'
require 'libsupp'
require 'libreload'
require 'smv'

function main(start)
  state_orig = libreload.parent_run("get_runtime_state", "libsupp", nil)
  state = su.libsupp_init(state_orig)
  libreload.parent_run("set_runtime_state", "libsupp", state)
  -- If the parent has no state stored - then it is the first start of the 
  -- main application - so need to do the full init.
  if state_orig == nil then
    print "coldstart!\n"
    su.http_start_listener("0.0.0.0", 2222);
    smv.coldstart()
  end
  while(true) do
    print("Running some cycles")
    su.run_cycles(1)
    print("Finished running cycles")
  end
  return "restart"
end

