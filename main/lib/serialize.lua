-- package.cpath="../../lib/?.so"
require 'libsupp'



function basicSerialize (o)
  if type(o) == "number" then
    return tostring(o)
  elseif type(o) == "boolean" then
    local x = "false"
    if o then
      x = "true"
    end
    return x
  else   -- assume it is a string
    return string.format("%q", o)
  end
end

function FullSerialize(name, value, d, saved)
  saved = saved or {}       -- initial value
  su.dstrcat(d, name .. " = ")
  if type(value) == "number" or type(value) == "string" or type(value) == "boolean" then
    su.dstrcat(d, basicSerialize(value) .. "\n")
  elseif type(value) == "table" then
    if saved[value] then    -- value already saved?
      su.dstrcat(d, saved[value] .. "\n")  -- use its previous name
    else
      saved[value] = name   -- save name for next time
      su.dstrcat(d, "{}\n")     -- create a new table
      for k,v in pairs(value) do      -- save its fields
        local fieldname = string.format("%s[%s]", name,
                                          basicSerialize(k))
        FullSerialize(fieldname, v, d, saved)
      end
    end
  else
    error("cannot save a " .. type(value))
  end
end

function serialize(name, value)
  local d = su.dalloc(8192)
  local s
  FullSerialize(name, value, d)
  s = su.dgetstr(d)
  su.dunlock(d)
  return s
end

local xxxxtest = [[
t = {}
t['"'] = true
t['\''] = false
t["\0"] = "zero"

print(serialize("t", t))
]]
