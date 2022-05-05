fractal.sk file size: 234KB
Program reads in the pgm file supplied in the first cmd line arg and stores
the gray values into a dynamically allocated 2D byte array.
Program then parses this array performing 1D RLE across rows and generating
the relevant sketch commands before writing to a .sk file of the same name as
the supplied pgm.
Program assumes pgm metadata to always be "P5 200 200 255".
No implementation of sk to pgm conversion.
Tests implemented for function which generates sketch commands from gray values and position of pixel, no tests for IO.
