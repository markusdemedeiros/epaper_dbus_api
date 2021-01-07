COMMANDS::
Will be some type in an array

APPLY
apply type a(yv) -> ()
								
			command				Argument Type		Args
	0x00	clear				(y)					colour
	0x01	clearRectangle		(qqqqy)				x0 y0 x1 y1 colour_f
	0x02	setPixel			(qqy)				x0 y0 colour_f
	0x03	drawPoint			(qqyyy)				x0 y0 colour_f dot_pixel dot_style	
	0x04	drawLine			(qqqqyyy)			x0 y0 x1 y1 colour_f dot_pixel (line width) line_style
	0x05	drawRectangle		(qqqqyyb)			x0 y0 x1 y1 colour_f dot_pixel (line width) draw_fill
	0x06	drawCircle			(qqqyyb)			x0 y0 r colour_f line_width draw_fill
	0x07	drawString			(qqsyyy)			x0 y0 string font col_f col_b
	0x08	readBitmap			(sqq)				x0 y0 path


fill/colour are stored as bytes rather than bytes to support other fill types in the future
	(such as hatch marks etc) and since it doesn't use any extra space

Make lookup tables for all the important/expandable config stuff, represented by ubyte
	colour
	dot_pixel	(size of point or line)
	dot_style	(point fill style)
	line_style
	font

This is stored internally as

	cmd				UWORD (command)
	x0 y0 x1 y1		UWORD UWORD UWORD UWORD (x0, x0, x1/radius, y1)
	col_f			UBYTE (colour/colour_foreground)
	col_b			UBYTE (colour_background)
	dot_w			UBYTE (dot_pixel (ie line width))
	aux				UBYTE (dot_style/line_style/draw_fill/font)			// draw_fill is 0 or 1
	dat				string (string/path)	// ? use char array? sting librray? function TAKES mem addresses...
	


UBYTE	= uint8_t		1 byte							y (unsigned 8 bit int)
UWORD	= uint16_t		2 byte (word/normal int)		q (unsigned 16 bit int)
UDOUBLE	= uint32_t		

