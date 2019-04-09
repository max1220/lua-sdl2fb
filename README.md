lua-sdl2fb
-----------

Use a SDL2 windows to draw on it's framebuffer using drawbuffers!
This library exports functions for interfacing with SDL2. That includes
the basic eventloop setup.



	sdlfb = sdl2fb.new(width, height, title)

Create a new sdl2 window of the specified resolution and title.


	sdlfb:draw_from_drawbuffer(db, x, y)

Copy the content of the drawbuffer to the specified coordinates.



	ev = sdlfb:pool_event()

Get events from the window. This includes keyboard and mouse events.
Each event should have a field type, that describes the event. Other
fields are only present for specific events.


	sdlfb:close()

Closes the window. Can't use afterwards.



Todo
-----

 Test
