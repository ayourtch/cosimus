require 'startup'

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

function LLSD_StrQuote(s)
  return s
  -- return string.format("%q", s)
end

function LLSD_BasicSerialize (o)
  if type(o) == "number" then
    return '<integer>' .. tostring(o) .. '</integer>'
  elseif type(o) == "boolean" then
    local x = "false"
    if o then
      x = "true"
    end
    return '<boolean>' .. x .. '</boolean>'
  else   -- assume it is a string
    return '<string>' .. LLSD_StrQuote(o) .. '</string>'
  end
end


function LLSD_FullSerialize(d, value)
  if type(value) == "number" or type(value) == "string" or type(value) == "boolean" then
    su.dstrcat(d, LLSD_BasicSerialize(value))
  elseif type(value) == "table" then
    if value[1] then
      -- hacky hack - a table can have an [1] and the named fields... but we assume it cant
      su.dstrcat(d, "<array>")
      for k, v in ipairs(value) do 
        LLSD_FullSerialize(d, v)
      end
      su.dstrcat(d, "</array>")
    else
      su.dstrcat(d, "<map>")
      -- pretty("v", value)
      for k, v in pairs(value) do
        su.dstrcat(d, "<key>" .. LLSD_StrQuote(k) .. "</key>")
        LLSD_FullSerialize(d, v)
      end
      su.dstrcat(d, "</map>")
    end
  else
    error("cannot save a " .. type(value))
  end
end


function llsd_serialize(data)
  local d = su.dalloc(8192)
  su.dstrcat(d, "<llsd>")
  LLSD_FullSerialize(d, data)
  su.dstrcat(d, "</llsd>")
  return d
end

local xxxxtest = [[
tst = {}
tst['"'] = true
tst['\''] = false
tst["\0"] = "zero"

x = serialize("xt", tst)
t = {}
-- print(su.dgetstr(x))
deserialize(x)
pretty('tst', tst)

print(su.dgetstr(llsd_serialize(tst)))

exit(1)
]]

