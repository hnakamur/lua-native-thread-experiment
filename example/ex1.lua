local nt = require 'nativethread'

nt.start([[
  local nt = require 'nativethread'
  local string = require 'string'
  print("Hi, I'm a native thread")
  print("Check if we can use string module. char code of 'a' is " .. string.byte('a'))
  while true do
    local x, y = nt.receive('channel1')
    if x == 'EXIT' then
      break
    end
    local sum = tonumber(x) + tonumber(y)
    nt.send('channel2', tostring(sum))
  end
]])

nt.send('channel1', '2', '3')
local sum = nt.receive('channel2')
print('sum=' .. sum)

nt.send('channel1', '3', '5')
sum = nt.receive('channel2')
print('sum=' .. sum)

nt.send('channel1', 'EXIT')
