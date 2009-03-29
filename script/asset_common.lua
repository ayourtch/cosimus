
--[[

Copyright © 2008-2009 Andrew Yourtchenko, ayourtch@gmail.com.

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom 
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
OR OTHER DEALINGS IN THE SOFTWARE. 

--]]

function asset_find_tag(xml, tag)
  for i, el in ipairs(xml) do
    if el.label == tag then
      return i
    end
  end
  return nil
end

function parse_asset(s)
  local xml = parse_xml(s)
  local x
  local out = {}
  local val = {}
  -- pretty("xml_parse_asset", xml)
  if 0 == #xml then
    return nil
  end
  if string.match(xml[1], "<?xml ") then
    x = xml[2]
  else
    x = xml[1]
  end
  out.ObjectType = x.label
  out.Value = val
  -- pretty("x", x)
  for i, el in ipairs(x) do
    local el_val = el[1]
    local lbl = el.label
    if type(el_val) == "table" then
      val[lbl] = el_val[1]
    elseif not (type(el_val) == "string") then
      if el.empty then
        val[lbl] = ""
      else
        print("Error - type ", type(el_val), "for element #", i)
      end
    elseif string.match(el_val, "^true$") then
      val[lbl] = true
    elseif string.match(el_val, "^false$") then
      val[lbl] = false
    elseif string.match(el_val,"^[0-9.]+$") then -- fixme - what about negative ?
      val[lbl] = tonumber(string.match(el_val,"^[0-9.]+$"))
    else 
      val[lbl] = el_val
    end
  end
  return out
end

function unparse_asset(d, asset)
  local pp = function(...) 
    su.dstrcat(d, ...)
  end

  -- pp(string.char(0xEF, 0xBB, 0xBF))
  pp('<?xml version="1.0" encoding="utf-8"?>\n')
  pp('<AssetBase xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">')
    for key, val in pairs(asset) do
      pp('<' .. key .. '>')
      if key == 'FullID' then
        pp('<Guid>')
	pp(tostring(val))
        pp('</Guid>')
      else
	pp(tostring(val))
      end
      pp('</' .. key .. '>')
    end
  pp('</AssetBase>')
end

