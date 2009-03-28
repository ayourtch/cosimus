queue = {}

function enqueue(item)
  local uuid = fmv.uuid_create()
  queue[uuid] = item
  return uuid
end

function dequeue()
  local f, t = pairs(queue)
  local key, val = f(t)
  if key then
    queue[key] = nil
  end
  return val
end

x = "first"
enqueue(x)
x = "second" 
enqueue(x)
print(dequeue())
print(dequeue())
print(dequeue())

