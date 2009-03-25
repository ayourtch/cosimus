function parse_xmlrpc_param(param)
  local p = {}
  -- print(pretty("param", param))
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

