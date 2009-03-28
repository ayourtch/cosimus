
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

function parse_xmlrpc_param(param)
  local p = {}
  -- pretty("param", param)
  if param.label == "value" then
    local pval = param[1]
    if pval.label == "struct" then
      for i,v in ipairs(pval) do
        if v.label == "member" then
	  local name
	  local value
	  for j, inner in ipairs(v) do
	    if inner.label == "name" then
	      name = inner[1]
	    elseif inner.label == "value" then
	      value = parse_xmlrpc_param(inner)
	    end
	  end
	  p[name] = value
	end
      end
    elseif pval.label == "array" then
      local arr = {}
      -- print(pretty("arr", pval))
      for i,v in ipairs(pval[1]) do
        if v.label == "value" then
	  value = parse_xmlrpc_param(v)
	  table.insert(arr, value)
	end
      end
      p = arr
    elseif pval.label == "int" then
      p = tonumber(pval[1])
    elseif pval.label == "string" then
      if pval.empty then
        p = "" 
      else
        p = pval[1]
      end
    end
  end
  -- print(pretty("param", p))
  return p
end

function parse_xmlrpc_params(params)
  local p = {}
  for i,v in ipairs(params) do
    if v.label == "param" then
      local res = parse_xmlrpc_param(v[1])
      -- print(pretty("xparam", res))
      table.insert(p, res)
    end
  end
  return p
end

function parse_xmlrpc_req(data)
  local xml = parse_xml(data)
  local x
  local req = {}

  if string.match(xml[1], "<?xml ") then
    x = xml[2]
  else
    x = xml[1]
  end

  if x.label == "methodCall" then
    for i,v in ipairs(x) do
      if v.label == "methodName" then
        req.method = v[1]
      elseif v.label == "params" then
        req.params = parse_xmlrpc_params(v)
      end
    end
  end
  return req
end

function dprint_field(pp, field)
  pp("<value>")
  if type(field) == "table" then
    if #field > 0 or field[0] == "array" then
      pp("<array>")
      pp("<data>")
      for i,v in ipairs(field) do
        dprint_field(pp, v)
      end
      pp("</data>")
      pp("</array>")
    else
      pp("<struct>")
      for n,v in pairs(field) do
        pp("<member>")
        pp("<name>")
	pp(n)
        pp("</name>")
	dprint_field(pp, v)
        pp("</member>")
      end
      pp("</struct>")
    end
  elseif type(field) == "string" then
    pp("<string>".. field .. "</string>")
  elseif type(field) == "number" then
    pp("<i4>".. field .. "</i4>")
  else
    pp("<error>" .. field .. "</error>")
  end
  pp("</value>")
end

function dprint_xml_response(resp, dd)
  local str = StringAccumulator()
  local pp = function(...)
    su.dstrcat(dd, ...)
  end
  pp('<?xml version="1.0" ?><methodResponse><params><param>')
  dprint_field(pp, resp)
  pp("</param></params></methodResponse>\n")
end

function parse_llsd_piece(p, indent)
  local res = nil
  indent = indent or 1
  pretty(string.rep(" ", indent) .. "req_val", p)
  
  if not p then
    return nil
  end
  if p.label == "map" then
    res = {}
    local key
    for i, el in ipairs(p) do
      if el.label == "key" then
        key = el[1]
        print(string.rep(" ", indent) .. "Set key to " .. key)
      else 
        res[key] = parse_llsd_piece(el, indent+1)
      end
    end 
  elseif p.label == "array" then
    res = {}
    for i, el in ipairs(p) do
      table.insert(res, parse_llsd_piece(el))
    end
  elseif p.label == "string" then
    res = p[1]
  elseif p.label == "boolean" then
    res = (p[1] == "1")
  end
  -- pretty(string.rep(" ", indent) .. "return_res", res)
  return res
end

function parse_llsd(data)
  local x = parse_xml(data)
  -- pretty("llsd", x)
  local res = {}
  if x[1].label == "llsd" then
    for i, el in ipairs(x[1]) do
      table.insert(res, parse_llsd_piece(el)) 
    end
  end
  return res
end

