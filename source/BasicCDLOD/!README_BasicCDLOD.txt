BasicCDLOD - a simple example of the CDLOD technique
Copyright 2010, Filip Strugar

This folder contains StreamingCDLOD code and data examples for the "Continuous Distance-Dependent Quadtree-Based Level of Detail for Rendering Heightmaps
(CDLOD)" paper.

All provided code is distributed under Zlib license (http://en.wikipedia.org/wiki/Zlib_License), except the code by other authors, listed below, distributed under their own respective licenses.


BasicCDLOD example implements CDLOD technique in its basic form; all terrain data is kept in memory at all times.

This project has older and probably less stable code than the StreamingCDLOD project.



Instructions:

 - BasicCDLOD displays terrain datasets defined by the .ini file that contains terrain size and position information, LOD settings and links to input data.

 - Input data required (heightmap and optional overlay image data) must be in .tbmp tiled bitmap format - conversion to/from it can be performed using 'tbmpconv' tool provided in the binaries_tools_testdata.exe archive.
   Please refer to provided example data .ini file and/or binaries_tools_testdata.exe and other example dataset archives for more info.



Requirements: 

 - DirectX9 August 2009 SDK (or newer)

 - Microsoft Visual C++ 2005 (or newer)

 - Shader model 3.0 capable graphics card.



Special thanks to authors of the code used by the demo and tools:

 - Jean-loup Gailly and Mark Adler, authors of 'zlib' compression library (http://zlib.net)

 - The Independent JPEG Group for the JPEG software library (http://www.ijg.org/), used by 'tbmpconv' conversion tool

 - Authors of LibTIFF library (http://www.libtiff.org/), used by 'tbmpconv' conversion tool

 - Lucian Wischik, René Nyffenegger, authors of C++ code for creating AVI files (http://www.adp-gmbh.ch/win/programming/avi/avi.html)

 - N. Devillard, author of 'iniparser' (http://ndevilla.free.fr/iniparser/index.html)


For any feedback, bug reports or info please contact me at filip.strugar@gmail.com


