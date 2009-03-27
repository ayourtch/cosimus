--package.cpath="../../lib/?.so"
--require 'libsupp'



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

function FullSerialize(name, value, d, saved, depth)
  saved = saved or {}       -- initial value
  depth = depth or 1
  local shortname = string.format("t[%d]", depth)
  if depth == 1 then
    su.dstrcat(d, "local t = {}\n")
  end
  if type(value) == "number" or type(value) == "string" or type(value) == "boolean" then
    su.dstrcat(d, name .. " = ")
    su.dstrcat(d, basicSerialize(value) .. "\n")
  elseif type(value) == "table" then
    if saved[value] then    -- value already saved?
      su.dstrcat(d, name .. " = ")
      su.dstrcat(d, saved[value] .. "\n")  -- use its previous name
    else
      saved[value] = name   -- save name for next time
      su.dstrcat(d, shortname .. " = ")
      su.dstrcat(d, "{}\n")     -- create a new table
      su.dstrcat(d, name .. " = " .. shortname .. "\n")
      for k,v in pairs(value) do      -- save its fields
        local fieldname = string.format("%s[%s]", shortname,
                                          basicSerialize(k))
        FullSerialize(fieldname, v, d, saved, depth+1)
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
  return d
end
function deserialize(dbuf)
  print("Deserializing from: ", dbuf)
  assert(loadstring(su.dgetstr(dbuf)))()
end


function serialize_llsd(data)
  local d = su.dalloc(8192)
   
end

local xxxxtest = [[
t = {}
t['"'] = true
t['\''] = false
t["\0"] = "zero"

x = serialize("t", t)
t = {}
deserialize(x)

print(su.dgetstr(serialize("t", t)))


]]

