function string.gather(str, pat)
  local tab = {}
  for match in str:gmatch(pat) do
    table.insert(tab, match)
  end
  return tab
end

-- fast string accumulator, from 
-- http://www.lua.org/pil/11.6.html

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
  

TemplateReader = function(filename)
  local worker = function(fname)
    for line in io.lines(fname) do
      -- get rid of comments, [rl]trim, remove trailing and multispaces
      local line2 = line:gsub("//.*$",""):gsub("^%s*","")
                        :gsub("%s+$",""):gsub("%s+"," ")
      local tokens = line2:gather("([^%s]+)")
      if(#tokens > 0) then
        coroutine.yield(tokens)
      end
    end
  end

  local me = {}
  me.fname = filename

  me.coworker = coroutine.create(worker)
  me.tokens = function(me)
    local ok, tok = coroutine.resume(me.coworker, me.fname)
    return tok
  end

  return me
end


PacketFieldRead = function(tr)
  local toks = tr:tokens()
  local field = nil
  if not (toks[1] == "}") then
    field = {}
    field.name = toks[2]
    field.type = toks[3]
    field.len = tonumber(toks[4])
     -- print("Got field name: " .. field.name .. " type " .. field.type)
  end
  return field
end

PacketBlockTemplateRead = function(tr, packet)
  local b = {}
  local toks = tr:tokens()

  b.name = toks[1]
  if toks[2] == "Single" then
    b.count = 1
  elseif toks[2] == "Multiple" then
    b.count = tonumber(toks[3])
  elseif toks[2] == "Variable" then
    b.count = -1
  else
    print("Unknown block count " .. toks[2]
          .. " for block " .. toks[1])
    -- FIXME: need an exception ?
  end

  -- c type for this block
  b.c_type = packet.name .. "_" .. b.name .. "_t"
  b.fullname = packet.name .. "_" .. b.name

  b.fields = {}
  b.packet = packet
  for field in PacketFieldRead, tr do
    table.insert(b.fields, field)
  end
  return b
end

function ToNumber(s)
  local out 
  if(string.match(s, "0x")) then
    out = tonumber(s, 16)
  else
    out = tonumber(s,10)
  end
  return out
end

function GetPacketGlobalID(pt)
  local part_id = ToNumber(pt.id) % 65536 -- same as "& 0xFFFF"
  local part_freq = 0;

  if pt.freq == "Fixed" then
    part_freq = 0x10000
  elseif pt.freq == "Low" then
    part_freq = 0x10000
  elseif pt.freq == "Medium" then
    part_freq = 0x20000
  elseif pt.freq == "High" then
    part_freq = 0x30000
  else
    print("Unexpected freq:", pt.freq)
    assert(nil)
  end
  -- we can replace the logical or with addition
  -- because the two values are not intersecting
  return part_id + part_freq;
end


PacketTemplateRead = function(tr)
  local start = tr:tokens()
  local pt = nil
  local finished = false

  if (start and #start == 1 and start[1] == "{") then
    local pkt_info = tr:tokens()
    local prevblock = nil
    pt = {}
    pt.blocks = {}
    pt.name = pkt_info[1]
    if pkt_info[2] == "Fixed" then
      pt.freq = "Low" 
    else 
      pt.freq = pkt_info[2]
    end
    pt.id = pkt_info[3]
    pt.trusted = (pkt_info[4] == "Trusted")
    pt.encoded = (pkt_info[5] == "Zerocoded")
    -- set the calculated fields
    -- used to refer to the unique part of the message
    pt.c_type = pt.name .. "_t"
    -- used to refer to the entire structure including common parts
    pt.full_c_type = "Pack" .. pt.name .. "_t"
    pt.global_id = GetPacketGlobalID(pt)
    -- add frequency above FIXME
    -- The name for the enum
    pt.enum = pt.name
    -- fetch all the blocks that make up this packet
    repeat
      local tok = tr:tokens()
      if tok[1] == "{" then
        local blk = PacketBlockTemplateRead(tr, pt)
	blk.prevblock = prevblock
	prevblock = blk
        table.insert(pt.blocks, blk)
      else
        finished = true
      end
    until finished
  elseif start then
    print("start[1] is: " .. start[1])
    print("foobar!")
  end
  return pt
end

--------------------------- end of reading functions, now generating -----------------

map_pkt_types = {
  S8  = 's8t',
  S16 = 's16t',
  S32 = 's32t',
  U8  = 'u8t',
  U16 = 'u16t',
  U32 = 'u32t',
  U64 = 'u64t',
  F32  = 'f32t',
  F64  = 'f64t',
  IPADDR = 'u32t',
  IPPORT = 'u16t',
  LLUUID = 'uuid_t',
  BOOL   = 'int'
}

map_lua_int = {
  S8  = true,
  S16 = true,
  S32 = true,
  U8  = true,
  U16 = true,
  U32 = true,
  -- U64 = true,
  -- F32  = 'f32t',
  -- F64  = 'f64t',
  -- IPADDR = 'u32t',
  -- IPPORT = 'u16t',
  -- LLUUID = 'uuid_t',
  BOOL   = true
}

map_pkt_type_sz = {
  -- packet headers
  Low = 10,
  Fixed = 10,
  Medium = 8,
  High = 7,
  -- special values
  Variable = -1,
  -- normal types
  S8  = 1,
  S16 = 2,
  S32 = 4,
  U8  = 1,
  U16 = 2,
  U32 = 4,
  U64 = 8,
  F32  = 4,
  F64  = 8,
  IPADDR = 4,
  IPPORT = 2,
  LLUUID = 16,
  BOOL   = 1,
  LLVector3 = 12,
  LLQuaternion = 12,
  LLVector4 = 16,
  LLVector3d = 24
}

blah = {
  LLVector3 = 'vw_vector3_t',
  LLQuaternion = 'vw_quaternion_t',
  LLVector4 = 'vw_vector4_t',
  LLVector3d = 'vw_vector3d_t'
}

function size_of(something)
  local out = map_pkt_type_sz[something]
  return out
end

function wrap_header(name, innercontent)
  local hname = "H_" .. name .. "_H"
  local output = 
    "#ifndef " .. hname .. "\n" ..
    "#define " .. hname .. "\n\n\n" ..
    "/**** Auto-generated, do not edit! ****/\n\n" ..
    innercontent ..
    "#endif /* " .. hname .. " */\n\n"
  return output 
end

function CodeArrayFieldStr(field)
  local out
  if field.type == "Fixed" then
    out = field.name .. ", " .. field.name .. "_sz_" .. field.len
  else -- if field.type == "Variable" then
    if field.len == 1 then
      out = field.name .. ", " .. field.name .. "_szV1"
    else
      out = field.name .. ", " .. field.name .. "_szV2"
    end
  end
  return out
end

function HeaderArrayFieldStr(field, byref, spacer)
  local out = ""
  local sp = ", "
  if spacer then
    sp = spacer
  end
  if field.type == "Fixed" then
    out = "u8t *" .. field.name .. sp .. "unsigned int " .. 
                     field.name .. "_sz_" .. field.len 
  else -- if field.type == "Variable" then
    if field.len == 1 then
      out = "u8t *" .. field.name .. sp .. "unsigned int " ..
	 field.name .. "_szV1"
    else -- if field.len == 2 then
      out = "u8t *" .. field.name .. sp .. "unsigned int " ..
		 field.name .. "_szV2"
    end
  end
  return out
end

function HeaderVectorFieldStr(field, ref, ctype, fourth_field, spacer, epilogue)
  local ep = ""
  if epilogue then
    ep = epilogue
  end
  local ox = ctype .. " " .. ref .. "x" .. field.name .. ep
  local oy = ctype .. " " .. ref .. "y" .. field.name .. ep
  local oz = ctype .. " " .. ref .. "z" .. field.name .. ep
  local o4 = ""
  local sp = ", "
  if spacer then
    sp = spacer
  end
  if fourth_field then
    o4 = sp .. ctype .. " " .. ref .. fourth_field .. field.name .. ep
  end
  local out = ox .. sp .. oy .. sp .. oz .. o4
  return out
end

function LuaPopVectorFieldStr(field, fourth_field)
  return HeaderVectorFieldStr(field, "", "", fourth_field, '; ', ' = luaL_checknumber(L, lua_argn++)')
end

function LuaPushVectorFieldStr(field, fourth_field)
  return HeaderVectorFieldStr(field, "", "lua_pushnumber(L,", fourth_field, '; ', '); lua_argn++')
end

function HeaderFieldStr(field, byref)
  -- return a field as its C definition (no semicolon)
  local mtype = map_pkt_types[field.type]
  local out = ""
  local ref = ""
  if byref then 
    ref = "*"
  end
  if mtype then
    out = mtype .. " " .. ref .. field.name
  elseif field.type == "LLVector3" then
    out = HeaderVectorFieldStr(field, ref, "f32t", nil)
  elseif field.type == "LLQuaternion" then
    out = HeaderVectorFieldStr(field, ref, "f32t", "w")
  elseif field.type == "LLVector4" then
    out = HeaderVectorFieldStr(field, ref, "f32t", "s")
  elseif field.type == "LLVector3d" then
    out = HeaderVectorFieldStr(field, ref, "f64t", nil)
  else -- fixed and variable array are represented by ptr+len
    out = HeaderArrayFieldStr(field, byref)
  end
  return "\n                " .. out;
end

function CodeFieldStr(field, byref)
  local mtype = map_pkt_types[field.type]
  local out 
  local ref = ''
  if byref then
    ref = '&'
  end
  if mtype then
    out = ref .. field.name
  elseif field.type == "LLVector3" then
    out = HeaderVectorFieldStr(field, "", ref, nil)
  elseif field.type == "LLQuaternion" then
    out = HeaderVectorFieldStr(field, "", ref, "w")
  elseif field.type == "LLVector4" then
    out = HeaderVectorFieldStr(field, "", ref, "s")
  elseif field.type == "LLVector3d" then
    out = HeaderVectorFieldStr(field, "", ref, nil)
  else -- fixed and variable array are represented by ptr+len
    out = CodeArrayFieldStr(field, byref)
  end
  return out
end

function LuaDefineFieldStr(field)
  local mtype = map_pkt_types[field.type]
  local out 
  if mtype then
    out = mtype .. " " .. field.name
  elseif field.type == "LLVector3" then
    out = HeaderVectorFieldStr(field, "", "f32t", nil, "; ")
  elseif field.type == "LLQuaternion" then
    out = HeaderVectorFieldStr(field, "", "f32t", "w", "; ")
  elseif field.type == "LLVector4" then
    out = HeaderVectorFieldStr(field, "", "f32t", "s", "; ")
  elseif field.type == "LLVector3d" then
    out = HeaderVectorFieldStr(field, "", "f64t", nil, "; ")
  else -- fixed and variable array are represented by ptr+len
    out = HeaderArrayFieldStr(field, byref, "; ")
  end
  return out
end


function LuaPopFieldStr(field)
  local m_int = map_lua_int[field.type]
  local out
  if m_int then
    out = field.name .. ' = luaL_checkint(L, lua_argn++)'
  elseif field.type == "LLVector3" then
    out = LuaPopVectorFieldStr(field, nil)
  elseif field.type == "LLQuaternion" then
    out = LuaPopVectorFieldStr(field, 'w')
  elseif field.type == "LLVector4" then
    out = LuaPopVectorFieldStr(field, 's')
  elseif field.type == "LLVector3d" then
    out = LuaPopVectorFieldStr(field, nil)
  else 
    out = '/* fixme LuaPopFieldStr: ' .. field.name .. '*/'
  end
  return out .. ';'
end

function LuaPushFieldStr(field)
  local m_int = map_lua_int[field.type]
  local out
  if m_int then
    out = 'lua_pushnumber(L, ' .. field.name .. '); lua_argn++'
  elseif field.type == "LLVector3" then
    out = LuaPushVectorFieldStr(field, nil)
  elseif field.type == "LLQuaternion" then
    out = LuaPushVectorFieldStr(field, 'w')
  elseif field.type == "LLVector4" then
    out = LuaPushVectorFieldStr(field, 's')
  elseif field.type == "LLVector3d" then
    out = LuaPushVectorFieldStr(field, nil)
  else 
    out = '/* fixme LuaPushFieldStr: ' .. field.name .. '*/'
  end
  return out .. ';'
end

-- pass the size/max size of the buffer too
common_args = "dbuf_t *d"
lua_functions = {}

function LuaAddFunction(name)
  lua_functions[#lua_functions + 1] = name
end

function LuaStartFunc(otab, name)
  LuaAddFunction(name)
  otab.p('static int\nlua_fn_' .. name .. '(lua_State *L)\n{\n')
  otab.p('  int lua_argn = 1;\n')
  otab.p('  dbuf_t *d = luaL_checkuserdata(L, lua_argn++);\n')
end

function LuaEndFunc(otab, count)
  otab.p('  return ' .. tostring(count) .. ';\n}\n\n')
end

function HeaderBlockPrototype(otab, block, prefix, suffix, byref)
  otab.p(prefix .. block.fullname .. suffix .. "(" .. common_args)
  if not (block.count == 1) then
    otab.p(", unsigned int idx");
  end
  for i, field in ipairs(block.fields) do
    otab.p(", " .. HeaderFieldStr(field, byref))
  end
  otab.p(")")
end

function HeaderPacketPrototype(otab, packet, prefix, suffix)
  otab.p(prefix .. packet.name .. suffix .. "(" .. common_args .. ")")
end

--- code for outputting the functional code stuff
function NeedCode(otab, prototype_only)
  if prototype_only then
    otab.p(";\n")
    return false
  else
    otab.p(" {\n")
    return true
  end
end

function EndCode(otab)
  otab.p("}\n\n")
end

function CoreCodePacketHeader(otab, packet)
  local flags = "0|RELIABLE"
  if packet.encoded then
    flags = flags .. "|ZEROCODED"
  end
  otab.p("  Header_UDP(d->buf, " .. (ToNumber(packet.id) % 65536) .. ", " .. 
            packet.freq .. ", " .. flags .. ");\n")
end

function CodePacketHeader(otab, packet, prototype_only)
  if NeedCode(otab, prototype_only) then
    CoreCodePacketHeader(otab, packet)
    EndCode(otab)
  end
end

function LuaCodePacketHeader(otab, packet)
  LuaStartFunc(otab, packet.name .. "Header")
  CoreCodePacketHeader(otab, packet)
  LuaEndFunc(otab, 0)
end


function FieldTypeName(field)
  local res
  if field.type == "Variable" then
    return field.type .. tostring(field.len)
  else
    return field.type
  end
end

function CodeFieldToUdp(otab, field)
  otab.p("  " .. FieldTypeName(field) .. "_UDP(" .. CodeFieldStr(field) .. ", d->buf, &n);\n")
end

function LuaCodeFieldDeclare(otab, field)
end

function LuaCodeFieldToUdp(otab, field)
  otab.p("  {\n")
  otab.p("    " .. LuaDefineFieldStr(field) .. ";\n")
  otab.p("    " .. LuaPopFieldStr(field) .. "\n")
  otab.p("    " .. FieldTypeName(field) .. "_UDP(" .. CodeFieldStr(field) .. ", d->buf, &n);\n")
  otab.p("  }\n")
end

function CodeFieldFromUdp(otab, field)
  local ftype = field.type
  otab.p("  UDP_" .. FieldTypeName(field) .. "(" .. CodeFieldStr(field) .. ", d->buf, &n);\n")
end

function LuaCodeFieldFromUdp(otab, field)
  otab.p("  {\n")
  otab.p("    " .. LuaDefineFieldStr(field) .. ";\n")
  otab.p("    UDP_" .. FieldTypeName(field) .. "(" .. CodeFieldStr(field, true) .. ", d->buf, &n);\n")
  otab.p("    " .. LuaPushFieldStr(field) .. "\n")
  otab.p("  }\n")
end

function BlockFieldsForeach(otab, block, func)
  for i, field in ipairs(block.fields) do
    func(otab, field)
  end
end

function CodeN_only(otab, block)
  local prevblock = block.prevblock
  otab.p("  int n = " .. size_of(block.packet.freq) .. ";\n")

  while prevblock do
    otab.p("  n += " .. prevblock.fullname .. "_Length(d);\n")
    prevblock = prevblock.prevblock
  end
end


-- position the "n" onto the start of
-- current data element in a block array

function CodeOff_only(otab, block, limit_var)
  local size = 0
  local need_prologue = true
  local flushsz = function(otab, size)
    if size > 0 then
      otab.p("      n += " .. tostring(size) .. ";\n")
    end
  end
  if block.count < 0 then
    -- variable sized block have length as first item
    otab.p("  /* variable-sized block  - skip the length */\n")
    otab.p("  n++;\n");
  end
  
  for i, field in ipairs(block.fields) do
    local sz = size_of(field.type)
    if sz and sz > 0 then
      size = size + sz
    else
      -- encountered a variable field, so will
      -- need to have a cycle to position the offset
      -- size till now has a size before the variable-sized element
      if need_prologue then
        otab.p("  { int j; \n    for(j=0; j<" .. limit_var .. "; j++) {\n")
        need_prologue = false;
      end
      flushsz(otab, size)
      size = 0
      otab.p("      /* " .. field.name .. " : " .. field.type .. " " .. field.len .. " */\n")
      if (field.len == 1) then
        otab.p("      n += 1 + d->buf[n];\n")
      else
        otab.p("      n += 2 + d->buf[n] + 256 * d->buf[n+1];\n")
      end
    end
  end
  -- no prologue means entire size is inside the "size" var = static, can do multiply
  if need_prologue then
    if size > 0 then
      otab.p("  n += (" .. size .. " * " .. limit_var .. ");\n")
    end
  else
    flushsz(otab, size)
    otab.p("    }\n  }\n")
  end
end

function CodeN(otab, block)
  CodeN_only(otab, block)
  if not (block.count == 1) then
    CodeOff_only(otab, block, "idx")
  end
end

function LuaCodeN(otab, block)
  if not (block.count == 1) then
    otab.p("  int idx = luaL_checkint(L, lua_argn++);\n")
  end

  CodeN(otab, block)
end

function CodeSetFixedBlock(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN(otab, block);
    BlockFieldsForeach(otab, block, CodeFieldToUdp)
    EndCode(otab)
  end
end

function LuaCodeSetFixedBlock(otab, block)
  LuaStartFunc(otab, block.fullname)
  LuaCodeN(otab, block);
  BlockFieldsForeach(otab, block, LuaCodeFieldToUdp)
  LuaEndFunc(otab, 0)
end

function CodeGetFixedBlock(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN(otab, block);
    BlockFieldsForeach(otab, block, CodeFieldFromUdp)
    EndCode(otab)
  end
end

function LuaCodeGetFixedBlock(otab, block)
  LuaStartFunc(otab, 'Get_' .. block.fullname)
  LuaCodeN(otab, block);
  otab.p('  lua_argn = 0;\n')
  BlockFieldsForeach(otab, block, LuaCodeFieldFromUdp)
  LuaEndFunc(otab, 'lua_argn')
end

function CodeSetVariableBlockSize(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN_only(otab, block);
    otab.p("  d->buf[n] = (u8t) length;\n")
    EndCode(otab)
  end
end

function LuaCodeSetVariableBlockSize(otab, block)
  LuaStartFunc(otab, block.fullname .. "BlockSize")
  otab.p("  int size = luaL_checkint(L, lua_argn++);\n")
  otab.p("  " .. block.fullname .. "BlockSize(d, size);\n")
  LuaEndFunc(otab, 0)
end

function CodeSetVariableBlock(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN(otab, block);
    BlockFieldsForeach(otab, block, CodeFieldToUdp)
    EndCode(otab)
  end
end

function LuaCodeSetVariableBlock(otab, block)
  LuaStartFunc(otab, block.fullname .. "Block")
  LuaCodeN(otab, block);
  BlockFieldsForeach(otab, block, LuaCodeFieldToUdp)
  LuaEndFunc(otab, 0)
end

function CodeGetVariableBlockSize(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN_only(otab, block);
    otab.p("  return (unsigned int)d->buf[n];\n")
    EndCode(otab)
  end
end

function LuaCodeGetVariableBlockSize(otab, block)
  LuaStartFunc(otab, "Get_" .. block.fullname .. "BlockSize")
  otab.p("  int len = Get_" .. block.fullname .. "BlockSize(d);\n")
  otab.p("  lua_pushnumber(L, len);\n") 
  LuaEndFunc(otab, 1)
end

function CodeGetVariableBlock(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN(otab, block);
    BlockFieldsForeach(otab, block, CodeFieldFromUdp)
    EndCode(otab)
  end
end

function LuaCodeGetVariableBlock(otab, block)
  LuaStartFunc(otab, 'Get_' .. block.fullname)
  LuaCodeN(otab, block);
  otab.p('  lua_argn = 0;\n')
  BlockFieldsForeach(otab, block, LuaCodeFieldFromUdp)
  LuaEndFunc(otab, 'lua_argn')
end

function CodeGetBlockLength(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    if block.count == 1 then
      otab.p("  int sn;\n");
      CodeN_only(otab, block);
      otab.p("  sn = n;\n");
      CodeOff_only(otab, block, "1");      
      otab.p("  return (n-sn);\n");
    else 
      otab.p("  int sn, sx;\n");
      CodeN_only(otab, block);
      otab.p("  sn = n;\n");
      otab.p("  /* block count: " .. tostring(block.count) .. " */\n")
      if block.count > 0 then
        otab.p("  sx = " .. tostring(block.count) .. ";\n");
      else
        otab.p("  sx = d->buf[n];\n");
      end
      CodeOff_only(otab, block, "sx");      
      otab.p("  return (n-sn);\n");
    end
    EndCode(otab)
  end
end

function LuaCodeGetBlockLength(otab, block)
  LuaStartFunc(otab, block.fullname .. "_Length")
  otab.p("  int len = " .. block.fullname .. "_Length(d);\n")
  otab.p("  lua_pushnumber(L, len);\n") 
  LuaEndFunc(otab, 1)
end

function CodeGetPacketLength(otab, packet, prototype_only)
  if NeedCode(otab, prototype_only) then
    otab.p("  int n = " .. size_of(packet.freq) .. ";\n")
    for i, block in ipairs(packet.blocks) do
      otab.p("  n += " .. block.fullname .. "_Length(d);\n")
    end
    otab.p("  return n;\n")
    EndCode(otab)
  end
end

function LuaCodeGetPacketLength(otab, packet)
  LuaStartFunc(otab, packet.name .. "Length")
  otab.p("  int len = " .. packet.name .. "Length(d);\n")
  otab.p("  lua_pushnumber(L, len);\n") 
  LuaEndFunc(otab, 1)
end


---- master function for header and code
function HeaderCodePacket(otab, packet, prototype_only)
  -- Packet header (fixed flags, etc)
  HeaderPacketPrototype(otab, packet, "void\n", "Header")
  CodePacketHeader(otab, packet, prototype_only)
  if not prototype_only then
    LuaCodePacketHeader(otab, packet)
  end

  for i, block in ipairs(packet.blocks) do
    otab.p(" // --- size: " .. block.count .. "\n")
    if block.count > 0 then
      -- set a fixed block
      HeaderBlockPrototype(otab, block, "void\n", "", false)
      CodeSetFixedBlock(otab, block, prototype_only)
      if not prototype_only then
        LuaCodeSetFixedBlock(otab, block)
      end
      -- get from a fixed block
      HeaderBlockPrototype(otab, block, "void\nGet_", "", true)
      CodeGetFixedBlock(otab, block, prototype_only)
      if not prototype_only then
        LuaCodeGetFixedBlock(otab, block)
      end
    else -- variable block size
      otab.p("void\n" .. block.fullname .. "BlockSize(" .. 
                        common_args .. ", unsigned int length)")
      CodeSetVariableBlockSize(otab, block, prototype_only)
      if not prototype_only then
        LuaCodeSetVariableBlockSize(otab, block)
      end

      HeaderBlockPrototype(otab, block, "void\n", "Block", false)
      CodeSetVariableBlock(otab, block, prototype_only)
      if not prototype_only then
        LuaCodeSetVariableBlock(otab, block)
      end
      otab.p("unsigned int\nGet_" .. block.fullname .. "BlockSize(" .. 
                        common_args .. ")")
      CodeGetVariableBlockSize(otab, block, prototype_only)
      if not prototype_only then
        LuaCodeGetVariableBlockSize(otab, block)
      end

      HeaderBlockPrototype(otab, block, "void\nGet_", "Block", true)
      CodeGetVariableBlock(otab, block, prototype_only)
      if not prototype_only then
        LuaCodeGetVariableBlock(otab, block)
      end
    end
    otab.p("int\n" .. block.fullname .. "_Length(" .. common_args .. ")")
    CodeGetBlockLength(otab, block, prototype_only)
    if not prototype_only then
      LuaCodeGetBlockLength(otab, block)
    end
  end
  HeaderPacketPrototype(otab, packet, "int\n", "Length")
  CodeGetPacketLength(otab, packet, prototype_only)
  if not prototype_only then
    LuaCodeGetPacketLength(otab, packet)
  end
end


function HeaderIncludes(otab)
  otab.p("#include <stdint.h>\n")
  otab.p("#include <strings.h>\n")
  otab.p('#include "lib_dbuf.h"\n')
  otab.p('#include "lua.h"\n')
  otab.p('#include "lua_fmv.h"\n')
  otab.p('#include "sta_fmv.h"\n')

  otab.p("\n\n")
end

function Header(name, packets)
  local otab = StringAccumulator()
  local hname = "H_" .. name .. "_H"

  otab.p("#ifndef " .. hname .. "\n")
  otab.p("#define " .. hname .. "\n\n")
  otab.p("/**** Auto-generated, do not edit! ****/\n\n")
  --otab.p("#pragma pack(1)\n\n")
  HeaderIncludes(otab)

  otab.p("char *global_id_str(u32t global_id);\n");

  -- output packet-specific typedefs
  for i, packet in ipairs(packets) do
     otab.p("\n")
     otab.p("/************ BEGIN PACKET: " .. packet.name .. " */\n")
     HeaderCodePacket(otab, packet, true)
     otab.p("/************ END PACKET: " .. packet.name .. " */\n")
  end
  --xx_remove_after_production= [[
  -- output the enum for packet types
  otab.p("\n\n")
  otab.p("enum PacketType {\n") 
  for i, packet in ipairs(packets) do
    otab.p("  " .. packet.enum .. " = " .. packet.global_id .. ",\n")
  end
  otab.p("  EndOfPackEnums\n")
  otab.p("};\n\n")
  --  ]]

  -- declarations of the C functions
  -- FunctionStatements(otab, false, packets)
  
  otab.p("\n\n")
  otab.p("#endif /* " .. hname .. " */\n\n")
  return otab.result()
end

function GenHeader(packets)
  print( Header("VWL_PACKETS", packets))
end

--------------------------- C code autogeneration functions ------------------

function GenCode(packets)
  local otab = StringAccumulator()

  otab.p('#include "gen_fmv.h"\n')
  otab.p('#include <lauxlib.h>\n')
  otab.p('#include <lualib.h>\n')


  otab.p("\n\n")

  for i, packet in ipairs(packets) do
    HeaderCodePacket(otab, packet, false)
  end

  -- big enum function
  otab.p("\n\n")
  otab.p("char *global_id_str(u32t global_id) {\n");
  otab.p("  switch(global_id) {\n");
  for i, packet in ipairs(packets) do
    otab.p("    case " .. packet.enum .. ": return \"" .. packet.enum .. "\";\n")
  end
  otab.p("    default: return \"unknown\";\n")

  otab.p("  }\n}\n\n")

  -- lua registration data and function

  otab.p("\n\nstatic const luaL_reg fmvlib[] = {\n")
  for i, fname in ipairs(lua_functions) do
    otab.p('  { "' .. fname .. '", lua_fn_' .. fname .. ' },\n')
  end
  otab.p('  { NULL, NULL }\n')
  otab.p('};\n\n')

  otab.p('LUA_API int luaopen_libfmv (lua_State *L) {\n')
  otab.p('  luaL_openlib(L, "fmv", fmvlib, 0);\n')
  otab.p('  return 1;\n')
  otab.p('}\n\n\n')


  print (otab.result())
end

--------------------------- main program ----------------------

action = arg[1]
filename = arg[2]

if action and filename then
  tr = TemplateReader(filename)
  version = tr:tokens()
  -- print("version: " .. version[2])
  packets = {}
  for packet in PacketTemplateRead, tr do
    -- print("Packet obtained: " .. packet.name)
    table.insert(packets, packet)
  end
  -- print("done, total: " .. #packets)
  if action == 'header' then
    GenHeader(packets)
  elseif action == 'code' then
    GenCode(packets)
  else 
    print "Unknown action!"
    assert(nil)
  end
else 
  print "Usage: parse-template.lua <header|code|lua-code> filename-with-message-template"
end
