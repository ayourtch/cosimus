-- Create a string accumulator. 
-- better use dstrcat if you interface with native code
-- because of ~13x speedup

-- StringAccumulator: 0.1214s
-- dstrcat:  0.0095s

function StringAccumulator()
  local otab = {""}

  otab.p = function(s)
    -- table.insert(otab, s)
    table.insert(otab, s)    -- push 's' into the the stack
    for i=table.getn(otab)-1, 1, -1 do
      if string.len(otab[i]) > string.len(otab[i+1]) then
        break
      end
      otab[i] = otab[i] .. table.remove(otab)
    end
  end

  otab.result = function(body)
    return table.concat(otab)
  end

  return otab
end

function dezeroize(s)
  if s then
    return string.gsub(s, "\000$", "")
  else
    return s
  end
end

function enzeroize(s)
  if s then
    return s .. "\0"
  else
    return s
  end
end


-- Return the XML parsed

function parse_xml(s)
  local stack = {}
  local top = {}
  table.insert(stack, top)
  local ni,c,label,xarg, empty
  local i, j = 1, 1
  local parseargs = function(s)
    local arg = {}
    string.gsub(s, "(%w+)=([\"'])(.-)%2", function (w, _, a)
      arg[w] = a
    end)
    return arg
  end

  while true do
    ni,j,c,label,xarg, empty = string.find(s, "<(%/?)(%w+)(.-)(%/?)>", i)
    if not ni then break end
    local text = string.sub(s, i, ni-1)
    if not string.find(text, "^%s*$") then
      table.insert(top, text)
    end
    if empty == "/" then  -- empty element tag
      table.insert(top, {label=label, xarg=parseargs(xarg), empty=1})
    elseif c == "" then   -- start tag
      top = {label=label, xarg=parseargs(xarg)}
      table.insert(stack, top)   -- new level
    else  -- end tag
      local toclose = table.remove(stack)  -- remove top
      top = stack[#stack]
      if #stack < 1 then
        error("nothing to close with "..label)
      end
      if toclose.label ~= label then
        error("trying to close "..toclose.label.." with "..label)
      end
      table.insert(top, toclose)
    end
    i = j+1
  end
  local text = string.sub(s, i)
  if not string.find(text, "^%s*$") then
    table.insert(stack[#stack], text)
  end
  if #stack > 1 then
    error("unclosed "..stack[stack.n].label)
  end
  return stack[1]
end

-- pretty-print the stuff

function pretty (name, value, saved)
      saved = saved or {}       -- initial value
      local basicSerialize = function(o)
        if not (type(o) == "string") then
          return tostring(o)
        else   -- assume it is a string
          return string.format("%q", o)
        end
      end

      io.write(name, " = ")
      if type(value) == "number" or type(value) == "string" or type(value) == "boolean" or type(value) == "nil" then
        io.write(basicSerialize(value), "\n")
      elseif type(value) == "table" then
        if saved[value] then    -- value already saved?
          io.write(saved[value], "\n")  -- use its previous name
        else
          saved[value] = name   -- save name for next time
          io.write("{}\n")     -- create a new table
          for k,v in pairs(value) do      -- save its fields
            local fieldname = string.format("%s[%s]", name,
                                            basicSerialize(k))
            pretty(fieldname, v, saved)
          end
        end
      else
        error("cannot save a " .. type(value))
      end
    end

--
-- Pure Lua version of dirname.
--
dirname = function(path)
	while true do
		if path == "" or
		   string.sub(path, -1) == "/" or
		   string.sub(path, -2) == "/." or
		   string.sub(path, -3) == "/.." or
		   (string.sub(path, -1) == "." and
		    string.len(path) == 1) or
		   (string.sub(path, -2) == ".." and
		    string.len(path) == 2) then
			break
		end
		path = string.sub(path, 1, -2)
	end
	if path == "" then
		path = "."
	end
	if string.sub(path, -1) ~= "/" then
		path = path .. "/"
	end

	return path
end

--
-- Pure Lua version of basename.
--
basename = function(path)
	local i = string.len(path)

	while string.sub(path, i, i) == "/" and i > 0 do
		path = string.sub(path, 1, i - 1)
		i = i - 1
	end
	while i > 0 do
		if string.sub(path, i, i) == "/" then
			break
		end
		i = i - 1
	end
	if i > 0 then
		path = string.sub(path, i + 1, -1)
	end
	if path == "" then
		path = "/"
	end

	return path
end


