#!/usr/bin/env luajit
local db = require("ldb")
local sdl2fb = require("sdl2fb")

local test_db = db.new(100, 100)
test_db:clear(0x00, 0x00, 0x00, 0xFF)
for i=0, 99 do
	test_db:set_pixel(i,0, 0xFA,0,0,0xFF)
	test_db:set_pixel(0,i, 0,0xFB,0,0xFF)
	test_db:set_pixel(i,i, 0,0,0xFC,0xFF)
end


local sdlfb = sdl2fb.new(800, 600, "Hello World!")

while true do
	sdlfb:draw_from_drawbuffer(test_db,200,200)
	local ev = sdlfb:pool_event()
	if ev then
		print("event", ev)
		for k,v in pairs(ev) do
			print("",k,v)
		end
		if ev.type == "quit" then
			sdlfb:close()
			break
		end
	end
end
