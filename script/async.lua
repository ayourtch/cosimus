
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


