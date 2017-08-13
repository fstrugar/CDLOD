StreamingCDLOD - an example of the CDLOD technique that supports data streaming
Copyright 2010, Filip Strugar

This folder contains StreamingCDLOD code and data examples for the "Continuous Distance-Dependent Quadtree-Based Level of Detail for Rendering Heightmaps
(CDLOD)" paper.

All provided code is distributed under Zlib license (http://en.wikipedia.org/wiki/Zlib_License), except the code by other authors, listed below, distributed under their own respective licenses.


StreamingCDLOD example implements CDLOD technique in its practical usable form with more optimal base algorithm memory use and simple data streaming.
Please refer to the CDLOD paper for more details.

This project has newer and probably cleaner and more stable code than the BasicCDLOD project.



Instructions:

 - StreamingCDLOD both creates and displays streamable terrain datasets (.scdlod).

 - To create a .scdlod file, create a .ini file with terrain dataset info and open it using the StreamingCDLOD application or by specifying "-c [input.ini] [output.sdlod]" command line parameters. 
   Input data required (heightmap and optional overlay image data) must be in .tbmp tiled bitmap format - conversion to/from it can be performed using 'tbmpconv' tool provided in the binaries_tools_testdata.exe archive.
   Please refer to provided example data .ini file and/or binaries_tools_testdata.exe and other example dataset archives for more info.



Requirements: 

 - DirectX9 August 2009 SDK (or newer)

 - Microsoft Visual C++ 2005 (or newer)

 - Shader model 3.0 capable graphics card.



Special thanks to authors of the code used by the demo and tools:

 - Jean-loup Gailly and Mark Adler, authors of 'zlib' compression library (http://zlib.net)

 - The Independent JPEG Group for the JPEG software library (http://www.ijg.org/), used by 'tbmpconv' conversion tool

 - Authors of LibTIFF library (http://www.libtiff.org/), used by 'tbmpconv' conversion tool

 - N. Devillard, author of 'iniparser' (http://ndevilla.free.fr/iniparser/index.html)



For any feedback, bug reports or info please contact me at filip.strugar@gmail.com


