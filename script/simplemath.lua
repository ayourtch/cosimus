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


--[[
A very simple math library for the start. 
It will be probably a bit slow, so will need to be replaced 
with something better in the future.
--]]

-- multiply a vector by a quaternion, and multi-return the result
function math_vec_mult_quat(vX, vY, vZ,     -- Vector
                        qX, qY, qZ, qW) -- Quaternion

  local rw = - qX*vX - qY*vY - qZ*vZ
  local rx =   qW*vX + qY*vZ - qZ*vY
  local ry =   qW*vY + qZ*vX - qX*vZ
  local rz =   qW*vZ + qX*vY - qY*vX

  local ox = - rw*qX + rx*qW - ry*qZ + rz*qY
  local oy = - rw*qY + ry*qW - rz*qX + rx*qZ
  local oz = - rw*qZ + rz*qW - rx*qY + ry*qX

  return ox, oy, oz
end

