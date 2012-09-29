local exports = {}

exports['nativethread.use_string_module'] = function(test)
  local nt = require 'nativethread'

  nt.start([[
    local nt = require 'nativethread'
    local string = require 'string'
    while true do
      local c = nt.receive('channel1')
      if c == 'EXIT' then
        break
      end
      local b = string.byte(c)
      nt.send('channel2', tostring(b))
    end
  ]])

  nt.send('channel1', 'a')
  local b = nt.receive('channel2')
  test.equal(b, '97')

  nt.send('channel1', 'A')
  local b = nt.receive('channel2')
  test.equal(b, '65')

  nt.send('channel1', 'EXIT')
  test.done()
end

return exports
