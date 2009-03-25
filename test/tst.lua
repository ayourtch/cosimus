require 'tstlib'

su.set_debug_level(1000, 300);

d = su.dalloc(1000);
su.set_debug_level(1000, 0);
local c = su.dstrcat
local s = ""
for i=0, 99999 do
  -- s = s .. "n"
  -- su.dstrcat(d, "n")
  c(d, "nnnnnn")
end
su.set_debug_level(1000, 300);
su.dlock(d)
su.print_dbuf(0, 0, d)
-- print(su.dgetstr(d))
su.dunlock(d)
su.dunlock(d)

