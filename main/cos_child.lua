require 'setpaths'
require 'libsupp'
require 'libreload'

function main(start)
  state_orig = libreload.parent_run("get_runtime_state", "libsupp", nil)
  state = su.libsupp_init(state_orig)
  libreload.parent_run("set_runtime_state", "libsupp", state)

  if state_orig == nil then
    print "coldstart!\n"
    su.http_start_listener("0.0.0.0", 2222);
  end
  print("Running some cycles")
  su.run_cycles(1)
  print("Finished running cycles")
  return "restart"
end

