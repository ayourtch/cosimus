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

function HeaderArrayFieldStr(field, byref)
  local out = ""
  if field.type == "Fixed" then
    out = "u8t *" .. field.name .. ", unsigned int " .. 
                     field.name .. "_sz_" .. field.len 
  else -- if field.type == "Variable" then
    if field.len == 1 then
      out = "u8t *" .. field.name .. ", unsigned int " ..
	 field.name .. "_szV1"
    else -- if field.len == 2 then
      out = "u8t *" .. field.name .. ", unsigned int " ..
		 field.name .. "_szV2"
    end
  end
  return out
end

function HeaderVectorFieldStr(field, ref, ctype, fourth_field)
  local ox = ctype .. " " .. ref .. "x" .. field.name 
  local oy = ctype .. " " .. ref .. "y" .. field.name 
  local oz = ctype .. " " .. ref .. "z" .. field.name 
  local o4 = ""
  if fourth_field then
    o4 = ", " .. ctype .. " " .. ref .. fourth_field .. field.name 
  end
  local out = ox .. ", " .. oy .. ", " .. oz .. o4
  return out
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

function CodeFieldStr(field)
  local mtype = map_pkt_types[field.type]
  local out 
  if mtype then
    out = field.name
  elseif field.type == "LLVector3" then
    out = HeaderVectorFieldStr(field, "", "", nil)
  elseif field.type == "LLQuaternion" then
    out = HeaderVectorFieldStr(field, "", "", "w")
  elseif field.type == "LLVector4" then
    out = HeaderVectorFieldStr(field, "", "", "s")
  elseif field.type == "LLVector3d" then
    out = HeaderVectorFieldStr(field, "", "", nil)
  else -- fixed and variable array are represented by ptr+len
    out = CodeArrayFieldStr(field, byref)
  end
  return out
end

-- pass the size/max size of the buffer too
common_args = "u8t *data, unsigned int dsize"

function HeaderBlockPrototype(otab, block, prefix, suffix, byref)
  otab.p(prefix .. block.fullname .. suffix .. "(" .. common_args)
  if not (block.count == 1) then
    otab.p(", unsigned int index");
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

function CodePacketHeader(otab, packet, prototype_only)
  if NeedCode(otab, prototype_only) then
    local flags = "0|RELIABLE"
    if packet.encoded then
      flags = flags .. "|ZEROCODED"
    end
    otab.p("  Header_UDP(data, " .. (ToNumber(packet.id) % 65536) .. ", " .. 
              packet.freq .. ", " .. flags .. ");\n")
    EndCode(otab)
  end
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
  otab.p("  " .. FieldTypeName(field) .. "_UDP(" .. CodeFieldStr(field) .. ", data, &n);\n")
end

function CodeFieldFromUdp(otab, field)
  local ftype = field.type
  otab.p("  UDP_" .. FieldTypeName(field) .. "(" .. CodeFieldStr(field) .. ", data, &n);\n")
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
    otab.p("  n += " .. prevblock.fullname .. "_Length(data, dsize);\n")
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
        otab.p("      n += 1 + data[n];\n")
      else
        otab.p("      n += 2 + data[n] + 256 * data[n+1];\n")
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
    CodeOff_only(otab, block, "index")
  end
end

function CodeSetFixedBlock(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN(otab, block);
    BlockFieldsForeach(otab, block, CodeFieldToUdp)
    EndCode(otab)
  end
end

function CodeGetFixedBlock(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN(otab, block);
    BlockFieldsForeach(otab, block, CodeFieldFromUdp)
    EndCode(otab)
  end
end

function CodeSetVariableBlockSize(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN_only(otab, block);
    otab.p("  data[n] = (u8t) length;\n")
    EndCode(otab)
  end
end

function CodeSetVariableBlock(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN(otab, block);
    BlockFieldsForeach(otab, block, CodeFieldToUdp)
    EndCode(otab)
  end
end

function CodeGetVariableBlockSize(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN_only(otab, block);
    otab.p("  return (unsigned int)data[n];\n")
    EndCode(otab)
  end
end

function CodeGetVariableBlock(otab, block, prototype_only)
  if NeedCode(otab, prototype_only) then
    CodeN(otab, block);
    BlockFieldsForeach(otab, block, CodeFieldFromUdp)
    EndCode(otab)
  end
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
        otab.p("  sx = data[n];\n");
      end
      CodeOff_only(otab, block, "sx");      
      otab.p("  return (n-sn);\n");
    end
    EndCode(otab)
  end
end

function CodeGetPacketLength(otab, packet, prototype_only)
  if NeedCode(otab, prototype_only) then
    otab.p("  int n = " .. size_of(packet.freq) .. ";\n")
    for i, block in ipairs(packet.blocks) do
      otab.p("  n += " .. block.fullname .. "_Length(data, dsize);\n")
    end
    otab.p("  return n;\n")
    EndCode(otab)
  end
end


---- master function for header and code
function HeaderCodePacket(otab, packet, prototype_only)
  -- Packet header (fixed flags, etc)
  HeaderPacketPrototype(otab, packet, "void\n", "Header")
  CodePacketHeader(otab, packet, prototype_only)

  for i, block in ipairs(packet.blocks) do
    otab.p(" // --- size: " .. block.count .. "\n")
    if block.count > 0 then
      -- set a fixed block
      HeaderBlockPrototype(otab, block, "void\n", "", false)
      CodeSetFixedBlock(otab, block, prototype_only)
      -- get from a fixed block
      HeaderBlockPrototype(otab, block, "void\nGet_", "", true)
      CodeGetFixedBlock(otab, block, prototype_only)
    else -- variable block size
      otab.p("void\n" .. block.fullname .. "BlockSize(" .. 
                        common_args .. ", unsigned int length)")
      CodeSetVariableBlockSize(otab, block, prototype_only)
      HeaderBlockPrototype(otab, block, "void\n", "Block", false)
      CodeSetVariableBlock(otab, block, prototype_only)
      otab.p("unsigned int\nGet_" .. block.fullname .. "BlockSize(" .. 
                        common_args .. ")")
      CodeGetVariableBlockSize(otab, block, prototype_only)
      HeaderBlockPrototype(otab, block, "void\nGet_", "Block", true)
      CodeGetVariableBlock(otab, block, prototype_only)
    end
    otab.p("int\n" .. block.fullname .. "_Length(" .. common_args .. ")")
    CodeGetBlockLength(otab, block, prototype_only)
  end
  HeaderPacketPrototype(otab, packet, "int\n", "Length")
  CodeGetPacketLength(otab, packet, prototype_only)
end


function HeaderIncludes(otab)
  otab.p("#include <stdint.h>\n")
  otab.p("#include <strings.h>\n")
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

  otab.p("\n\n")

  for i, packet in ipairs(packets) do
    HeaderCodePacket(otab, packet, false)
  end
  otab.p("\n\n")
  otab.p("char *global_id_str(u32t global_id) {\n");
  otab.p("  switch(global_id) {\n");
  for i, packet in ipairs(packets) do
    otab.p("    case " .. packet.enum .. ": return \"" .. packet.enum .. "\";\n")
  end
  otab.p("    default: return \"unknown\";\n")

  otab.p("  }\n}\n\n")

  print (otab.result())
end

------------------------ LUA code -----------------------
--- B H L f d s c

pointer_size = 4

vwp_field = {
           S8  = { size =  1, pack = 'b' },
           S16 = { size =  2, pack = 'h' },
           S32 = { size =  4, pack = 'l' },
           U8  = { size =  1, pack = 'B' },
           U16 = { size =  2, pack = 'H' },
           U32 = { size =  4, pack = 'L' },
           U64 = { size =  8, pack = 'LL'},
           F32 = { size =  4, pack = 'f' },
           F64 = { size =  8, pack = 'd' },
        IPADDR = { size =  4, pack = 'L' },
        IPPORT = { size =  2, pack = 'H' },
        LLUUID = { size = 16, pack = 'c16' },
          BOOL = { size =  1, pack = 'B' },
     LLVector3 = { size = 12, pack = 'fff' }, 
  LLQuaternion = { size = 16, pack = 'ffff' },
     LLVector4 = { size = 16, pack = 'ffff' },
    LLVector3s = { size = 24, pack = 'ddd' },
      Variable = { size = pointer_size, pack = 'XXX' },
}

function LuaCodeField(otab, field)
  local ft = field.type
  local fn = field.name
  local fl = field.len
  otab.p("    " .. fn .. " = { ")
  if vwp_field[ft] then
    otab.p("'" .. vwp_field[ft].pack .. "'")
    otab.p(", " .. tostring(otab.field_offset))
    otab.p(", " .. tostring(vwp_field[ft].size))
    otab.field_offset = otab.field_offset + vwp_field[ft].size
  end
  otab.p(" };");
  otab.p(" ---- " .. fn .. " : " .. ft .. "[" .. tostring(fl) .. "]\n")
end

function LuaCodeBlock(otab, block)  
  local start_offset = otab.field_offset 
  local block_size = 0
  otab.p("  " .. block.name .. " = {\n")
  otab.p("    __count = " .. tostring(block.count) .. ";\n")
  if block.count == -1 then
    otab.p("    __offs = " .. tostring(start_offset) .. ";\n")
    otab.field_offset = 0
  end
  for i, field in ipairs(block.fields) do
    LuaCodeField(otab, field)
  end
  if block.count == -1 then
    block_size = otab.field_offset
    otab.field_offset = start_offset + pointer_size
  else
    block_size = otab.field_offset - start_offset
  end
  otab.p("    __size = " .. tostring(block_size) .. ";\n")
  otab.p("  };\n")
  if block.count > 1 then
    otab.field_offset = start_offset + (block_size * block.count)
  end
end


function LuaCodePacket(otab, packet)
  -- print(packet.enum)
  otab.field_offset = 0
  otab.p("\n\nvwp." .. packet.enum .." = " .. packet.global_id .. "\n")
  otab.p("vwp[" .. packet.global_id .."] = '" .. packet.enum .. "'\n\n")
  otab.p("vwp.decode." .. packet.enum .. " = {\n")
  for i, block in ipairs(packet.blocks) do
    LuaCodeBlock(otab, block) 
  end
  otab.p("}\n")
  otab.p("vwp.decode[" .. packet.global_id .. "] = vwp.decode." .. packet.enum)
  otab.p("\n")
end

function GenLuaCode(packets)
  local otab = StringAccumulator()

  --CodeIncludes(otab)

  otab.p("\n\n")
  otab.p("vwp = {}\n")
  otab.p("vwp.mt = {}\n")
  otab.p("vwp.decode = {}\n")

  -- per-message (local) code


  for i, packet in ipairs(packets) do
    if i > -1 then
      LuaCodePacket(otab, packet)
    end
  end
  otab.p("\n\n")

  -- common stuff (exported)
  --FunctionStatements(otab, true, packets)

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
  elseif action == 'lua-code' then
    GenLuaCode(packets)
  else 
    print "Unknown action!"
    assert(nil)
  end
else 
  print "Usage: parse-template.lua <header|code|lua-code> filename-with-message-template"
end
