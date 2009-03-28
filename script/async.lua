
--[

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

--]
async_requests_store = {}
async_testrequests_store = {}

function int_async_put(item, place)
  local uuid = fmv.uuid_create()
  place[uuid] = item
  return uuid
end

function int_async_getnext(place)
  local f, t = pairs(place)
  local key, val = f(t)
  if key then
    place[key] = nil
  end
  return val
end

function int_async_get(uuid, place)
  local a = place[uuid]
  -- print("Async get for ", uuid, a)
  return a
end


function async_put(item)
  return int_async_put(item, async_requests_store)
end

function async_get(uuid)
  return int_async_get(uuid, async_requests_store)
end

function async_test_put(item)
  return int_async_put(item, async_testrequests_store)
end

function async_test_next()
  return int_async_getnext(async_testrequests_store)
end

function async_test_get(uuid)
  return int_async_get(uuid, async_testrequests_store)
end


