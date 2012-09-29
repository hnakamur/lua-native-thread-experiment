local exports = {}

exports['nativethread.sum'] = function(test)
  local nt = require 'nativethread'

  nt.start([[
    local nt = require 'nativethread'
    while true do
      local x, y = nt.receive('channel1')
      local sum = tonumber(x) + tonumber(y)
      nt.send('channel2', tostring(sum))
    end
  ]])

  nt.send('channel1', '2', '3')
  local sum = nt.receive('channel2')
  test.equal(sum, '5')

  nt.send('channel1', '3', '5')
  sum = nt.receive('channel2')
  test.equal(sum, '8')

  test.done()
end

return exports
