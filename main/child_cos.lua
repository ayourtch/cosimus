require 'setpaths'
require 'libsupp'
require 'libreload'
require 'smv'

mtimes = {}

function watch_file(fname)
  mtimes[fname] = su.get_file_mtime(fname)
end

function files_changed()
  local changed = false
  for k, v in pairs(mtimes) do
    if not (su.get_file_mtime(k) == v) then
      changed = true
    end
  end
  return changed
end

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
  else
    print "warmstart!\n"
    local smv_ser = libreload.parent_run("get_runtime_state", "smv_lua_state", nil)
    deserialize(smv_ser)
    print "deserialized\n"
    smv.register_handlers()
  end
  keep_running = true
  retcode = "restart"

  watch_file("lib/smv.lua")
  watch_file(cpath .. "libfmv.so")
  watch_file(cpath .. "libpktsmv.so")
  watch_file(cpath .. "libsupp.so")
  -- su.set_debug_level(1000, 10);

  while(keep_running) do
    -- print("Running some cycles")
    su.run_cycles(1)
    -- print("Finished running cycles")
    if files_changed() then
      keep_running = false
      local smv_ser = smv.serialize()
      print("setting parent state to: ", smv_ser)
      libreload.parent_run("set_runtime_state", "smv_lua_state", smv_ser)
      -- poor man's way to ensure the file is not half-written before updating (FIXME!)
      os.execute("sleep 1")
    end
  end
  return retcode
end

print "In child eval"
