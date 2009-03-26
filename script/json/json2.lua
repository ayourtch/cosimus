-- Grabbed from: git://github.com/elemel/lua-json.git

-- Copyright (c) 2008 Mikael Lind
-- 
-- Permission is hereby granted, free of charge, to any person
-- obtaining a copy of this software and associated documentation
-- files (the "Software"), to deal in the Software without
-- restriction, including without limitation the rights to use,
-- copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the
-- Software is furnished to do so, subject to the following
-- conditions:
-- 
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
-- 
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
-- OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
-- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
-- HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
-- WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
-- FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
-- OTHER DEALINGS IN THE SOFTWARE.

local json = {}

json.null = {}

local impl = {}

impl.array = {}
impl.object = {}

local escapes = {["\""] = "\"", ["\\"] = "\\", ["/"] = "/", b = "\b",
                 f = "\f", n = "\n", r = "\r", t = "\t"}

function impl.is_number_char(char)
    return string.find("-0123456789.eE", char, 1, true) ~= nil
end

function impl.is_whitespace_char(char)
    return string.find(" \n\r\t", char, 1, true) ~= nil
end

function impl.read(peek_char, read_char)
    impl.skip_whitespace(peek_char, read_char)
    local value = impl.read_value(peek_char, read_char)
    impl.skip_whitespace(peek_char, read_char)
    assert(not peek_char())
    return value
end

function impl.read_value(peek_char, read_char)
    local char = peek_char()
    if char == "\"" then
        return impl.read_string(peek_char, read_char)
    elseif impl.is_number_char(char) then
        return impl.read_number(peek_char, read_char)
    elseif char == "{" then
        return impl.read_object(peek_char, read_char)
    elseif char == "[" then
        return impl.read_array(peek_char, read_char)
    elseif char == "t" then
        return impl.read_constant(peek_char, read_char, "true", true)
    elseif char == "f" then
        return impl.read_constant(peek_char, read_char, "false", false)
    elseif char == "n" then
        return impl.read_constant(peek_char, read_char, "null", json.null)
    else
        assert(false)
    end
end

function impl.skip_whitespace(peek_char, read_char)
    while true do
        local char = peek_char()
        if not char then
            break
        elseif char == "/" then
            impl.skip_comment(peek_char, read_char)
        elseif impl.is_whitespace_char(char) then
            read_char()
        else
            break
        end
    end
end

function impl.skip_comment(peek_char, read_char)
    local char = read_char()
    assert(char == "/")
    char = read_char()
    if char == "/" then
        repeat
            char = read_char()
        until not char or char == "\n"
        return nil
    elseif char == "*" then
        local star = false
        while true do
            char = read_char()
            assert(char)
            if char == "*" then
                star = true
            elseif star and char == "/" then
                return nil
            else
                star = false
            end
        end
    else
        assert(false)
    end
end

function impl.read_string(peek_char, read_char)
    local buffer = {}
    local char = read_char()
    assert(char == "\"")
    while true do
        char = read_char()
        assert(char)
        if char == "\"" then
            break
        elseif char == "\\" then
            char = read_char()
            if escapes[char] then
                table.insert(buffer, escapes[char])
            elseif char == "u" then
                -- JSON unicode escapes are not supported.
                assert(false)
            else
                assert(false)
            end
        else
            table.insert(buffer, char)
        end
    end
    return table.concat(buffer)
end

function impl.read_number(peek_char, read_char)
    local buffer = {}
    while peek_char() and impl.is_number_char(peek_char()) do
        table.insert(buffer, read_char())
    end
    return tonumber(table.concat(buffer))
end

function impl.read_object(peek_char, read_char)
    local obj = json.new_object()
    local char = read_char()
    assert(char == "{")
    while true do
        impl.skip_whitespace(peek_char, read_char)
        char = peek_char()
        assert(char)
        if char == "}" then
            read_char()
            break
        end
        local key = impl.read_value(peek_char, read_char)
        impl.skip_whitespace(peek_char, read_char)
        local char = read_char()
        assert(char == ":")
        impl.skip_whitespace(peek_char, read_char)
        local value = impl.read_value(peek_char, read_char)
        obj[key] = value
        char = read_char()
        if char == "}" then
            break
        end
        assert(char == ",")
    end
    return obj
end

function impl.read_array(peek_char, read_char)
    local arr = json.new_array()
    local char = read_char()
    assert(char == "[")
    while true do
        impl.skip_whitespace(peek_char, read_char)
        char = peek_char()
        assert(char)
        if char == "]" then
            read_char()
            break
        end
        table.insert(arr, impl.read_value(peek_char, read_char))
        impl.skip_whitespace(peek_char, read_char)
        char = read_char()
        assert(char)
        if char == "]" then
            break
        end
        assert(char == ",")
    end
    return arr
end

function impl.read_constant(peek_char, read_char, name, value)
    local i = 1
    while i <= #name do
        local char = read_char()
        assert(char == string.sub(name, i, i))
        i = i + 1
    end
    return value
end

function impl.write_value(value, write_string)
    if type(value) == "string" then
        write_string(string.format("%q", value))
    elseif type(value) == "number" then
        write_string(tostring(value))
    elseif json.is_array(value) then
        impl.write_array(value, write_string)
    elseif json.is_object(value) then
        impl.write_object(value, write_string)
    elseif type(value) == "boolean" then
        write_string(tostring(value))
    elseif json.is_null(value) then
        write_string("null")
    else
        assert(false)
    end
end

function impl.write_array(arr, write_string)
    write_string("[")
    for i, value in ipairs(arr) do
        if i >= 2 then
            write_string(", ")
        end
        impl.write_value(value, write_string)
    end
    write_string("]")
end

function impl.write_object(obj, write_string)
    write_string("{")
    local keys = {}
    for key, _ in pairs(obj) do
        table.insert(keys, key)
    end
    table.sort(keys)
    for i, key in ipairs(keys) do
        if i >= 2 then
            write_string(", ")
        end
        impl.write_value(key, write_string)
        write_string(": ")
        impl.write_value(obj[key], write_string)
    end
    write_string("}")
end

function json.new_array(arr)
    arr = arr or {}
    setmetatable(arr, impl.array)
    return arr
end

function json.new_object(obj)
    obj = obj or {}
    setmetatable(obj, impl.object)
    return obj
end

function json.is_array(value)
    local t = type(value)
    local mt = getmetatable(value)
    return t == "table" and value ~= json.null and
            (mt == impl.array or mt == nil and value[1] ~= nil)
end

function json.is_null(value)
    return value == nil or value == json.null
end

function json.is_object(value)
    local t = type(value)
    local mt = getmetatable(value)
    return t == "table" and value ~= json.null and
            (mt == impl.object or mt == nil and value[1] == nil)
end

function json.read(str)
    local pos = 1
    local function peek_char()
        return pos <= #str and string.sub(str, pos, pos) or nil
    end
    local function read_char()
        local char = peek_char()
        pos = pos + 1
        return char
    end
    local success, result = pcall(impl.read, peek_char, read_char)
    if not success then
        error("syntax error at character " .. pos)
    end
    return result
end

function json.write(val)
    local buffer = {}
    local function write_string(str)
        table.insert(buffer, str)
    end
    impl.write_value(val, write_string)
    return table.concat(buffer)
end

return json
