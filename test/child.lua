require 'libsupp'

function main(start)
  if start == "cold" then
    print "coldstart!\n"
    su.http_start_listener("0.0.0.0", 2222);
  end
  print("Running some cycles")
  su.run_cycles(1)
  print("Finished running cycles")
  a = io.stdin:read()
  print ("You entered: ", a)
  print(1+tonumber(a))
  return "restart needed"
end
