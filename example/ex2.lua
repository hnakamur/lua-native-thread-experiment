local nt = require 'nativethread'

nt.start([[
  local nt = require 'nativethread'
  while true do
    local x, y = nt.receive('channel1')
    if x == 'EXIT' then
      break
    end
    local sum = tonumber(x) + tonumber(y)
    nt.send('channel1', tostring(sum))
  end
]])

nt.start([[
  local nt = require 'nativethread'
  while true do
    local x, y = nt.receive('channel2')
    if x == 'EXIT' then
      break
    end
    local diff = tonumber(x) - tonumber(y)
    nt.send('channel2', tostring(diff))
  end
]])

nt.send('channel1', '2', '3')
nt.send('channel2', '2', '3')

local sum = nt.receive('channel1')
print('sum=' .. sum)

local diff = nt.receive('channel2')
print('diff=' .. diff)

nt.send('channel1', 'EXIT')
nt.send('channel2', 'EXIT')
