packARC v0.7RC18 (12/17/2014)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

packARC is an archiver program specially designed for lossless further 
compression of media files in the JPEG, MP3, BMP and PNM formats. It 
contains all of my latest developments, the packJPG, packMP3, packPNM 
and packARI algorithms with an easy to use command line interface. 
Typical further compression for JPEG is ~20%, for MP3 it is ~16% and for 
PNM, BMP and other file types it is highly dependent on the contents of 
the file. 

packARC creates .pja archives, which may contain several files of 
arbitrary file type. In general packARC is not as efficient as ZIP, RAR, 
or 7z when compressing generic files, but it performs very good for JPG, 
MP3, BMP and PNM-files. 

Besides the archiver program, packARC, this package also contains the 
source code for packJPG, packMP3, packPNM, and packARI. packJPG, 
packMP3, packPNM and packARI do not create archives, but compress JPG, 
MP3, PNM or, in the case of packARI, generic files on a file by file 
basis. 


LGPL v3 license and special permissions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All programs in this package are free software; you can redistribute 
them and/or modify them under the terms of the GNU Lesser General Public 
License as published by the Free Software Foundation; either version 3 
of the License, or (at your option) any later version. 

The package is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser 
General Public License for more details at 
http://www.gnu.org/copyleft/lgpl.html. 

If the LGPL v3 license is not compatible with your software project you 
might contact us and ask for a special permission to use packARC or the 
packANY library under different conditions. In any case, usage of the 
packARC algorithm under the LGPL v3 or above is highly advised and 
special permissions will only be given where necessary on a case by case 
basis. This offer is aimed mainly at closed source freeware developers 
seeking to add packARC support to their software projects. 

Copyright 2006...2016 by HTW Aalen University and Matthias Stirner.


Compression algorithms and file types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

packARC contains four unique compression algorithms, packJPG, packMP3, 
packPNM and packARI in one easy to use command line interface. Files 
will be compressed into either the regular PJA (packJPG Archive) or SFX 
(self extracting archive) formats. SFX archives are executable files 
that extract their own contents upon execution. 

packARC will handle any type of file. Folders specified as file name are 
recursed and all contained files added to the archive while preserving 
relative folder structure. For regular files, the compression algorithm 
to use is decided on a per file base, automatically analyzing each file 
before adding it to the archive. 

The packJPG algorithm handles image files in the JPEG file format. 
Basically all types of JPEG files, baseline, extended sequential, 
progressive, CMYK and YCbCr are supported by packJPG. 

The packMP3 algorithm handles audio files in the MPEG-1 Audio Layer III 
format. Files in MPEG-2/2.5 Audio Layer III format are not supported. 
You may distinguish these from MPEG-1 Audio Layer III by their sample 
rates. Only MPEG-1 Audio Layer III supports sample rates of 32kHz and 
above. 

The packPNM algorithm handles image files in the BMP format and the PPM 
8bit/16bit, PGM 8bit/16bit and PBM binary formats (the respective ASCII 
based formats are not supported). Unless the image file has structural 
errors it will always be compressed using the packPNM algorithm.

All other and unrecognized files are handled by the packARI algorithm, 
which consists of arithmetic coding with a simple variable order 
statistical model. 


Usage of packARC
~~~~~~~~~~~~~~~~

The following paragraphs explain the general usage of packARC. In these 
praragraphs, [] is used for required paramters, <> is used for optional 
parameters. 

You may create a new PJA archive or add files to an existing archive 
using this command: 

 "packARC a <switches> [name_of_archive] [file(s)_to_add]"

When extracting, specifying files to extract is optional as indicated by 
the <>. If you leave it blank, all files inside the archive are 
extracted. To extract files from an existing archive use this command: 

 "packARC x <switches> [name_of_archive] <file(s)_to_extract>"
 
packARC can extract archive contents to memory and compare extracted 
CRC32s with original CRCs, making sure that all files inside the archive 
are ok. To test archive integrity, use this command: 

 "packARC t [name_of_archive]"
 
To convert an existing regular archive to SFX (self extracting archive) 
format or to convert an SFX archive to regular archive format use the 
following command. packARC will automatically choose the direction of 
the conversion based on the format of the input archive. 

 "packARC c [name_of_archive]"
 
You may delete unwanted files from an archive using the delete command. 
Use this syntax: 

 "packARC d [name_of_archive] [file(s)_to_delete]"
 
To list the contents of an archive, use the list command:

 "packARC l <switches> [name_of_archive]"
 
packARC handles SFX archives the same way as it handles regular 
archives. You may use all of the mentioned commands on either regular 
packARC archives or on packARC SFX archives created in any OS. packARC 
is also compatible with archives written using previous release 
versions. You can edit, extract, test, convert and add to archives 
created with older versions, too. 

packARC supports wildcards like "*.*" and drag and drop of multiple 
files. When dropping an archive file, that file is extracted in place. 
When dropping multiple files and/or non-archive files, a new SFX archive 
file is created using the name of the first file in the list and all 
files are added to it. Existing archive files won't be changed or 
overwritten in the process. 

In default mode, existing files are never overwritten, neither inside, 
nor outside the archive. packARC skips these files instead. You may 
configure this behaviour to your liking using command line switches 
"-o", "-s" and "-r" (see below). 

Usage examples:

Add all files matching "*.jpg" to archive "image.pja":
 "packARC a images.pja *.jpg"

Extract all files from archive "media.pja":
 "packARC x media.pja"
 
Delete file "unneeded.txt" from archive "SFX.exe":
 "packARC d SFX.exe unneeded.txt"
 
Convert archive "archive.pja" to SFX format:
 "packARC c archive.pja"
 
List contents of archive "myfiles.exe":
 "packARC l myfiles.exe"


Command line switches
~~~~~~~~~~~~~~~~~~~~~

 -o    overwrite existing files
 -s    skip existing files (default)
 -r    rename on existing files
 -i    (with x) ignore crc errors
 -sfx  (with a) create sfx archive
 -sl   (with l) simple list format
 -csv  (with l) CSV list format
 -np   no pause after processing.
 
 
When adding or extracting, existing files are skipped by default. If you 
want packARC to overwrite files, use "-o". If the command line parameter 
"-r" is used and a filename is already in use, packARC will create a new 
filename by adding underscores to the original filename instead. As an 
example, if "lena.jpg" already existed, the new file would be called 
"lena_.jpg when "-r" is used. 

By default, packARC checks extracted files using CRC32. If the check 
fails, packARC does not extract the file in question and moves on to the 
next file instead. In theory the check should never fail, but it may 
happen with corrupted archives. If you want to disable the CRC32 check 
for any reason use the parameter "-i". 

If you want to create a new SFX archive, you don't need to create a 
regular one first using "a", then convert it it using "c". Just use the 
switch "-sfx" in conjunction with "a" to create a SFX archive right 
away. 

You can change the format of the file listing with "l". For a simpler 
list format use switch "sl". For CSV compatible output use switch "csv". 

Usage examples:

Add all files matching "newfile??.bin", overwriting existing files:
 "packARC a -o archive.pja newfile??.bin"
 
Create new SFX archive adding all files matching "*.mp3":
 "packARC a -sfx music.exe *.mp3"
 
Extract archive, skipping CRC32 check and renaming on existing files:
 "packARC x -r -i photos.pja"
 
List contents of archive using the CSV list format:
 "packARC l -csv compressed.exe"


Known Limitations 
~~~~~~~~~~~~~~~~~ 

Although packARC can handle any type of file, it was mainly designed 
with media files MP3 and JPEG in mind. It will reasonably compress the 
occasional TXT, PDF, DOC or any other file, but it is not intended to be 
used for large files such as ISOs. If the files that you want to archive 
don't consist mainly of JPEGs or MP3s you might want to consider using 
something else, such as Matt Mahoney's excellent PAQ8 series of 
archivers from http://www.mattmahoney.net/dc/.

There is low error tolerance for JPEG and MP3 files. Even if a file can 
be perfectly processed by image viewer or audio player software, it 
might not be standard compliant. If a file is not 100% standard 
compliant, it won't be processed with the appropriate media compression 
algorithm. The generic algorithm is used instead, thus generating output 
of bigger size than you might expect. You may check the archive file 
listing to see which algorithm was used for the file included in an 
archive. 

If you try to drag and drop too many files in Microsoft Windows at once, 
there might be a windowed error message about missing privileges. In 
that case you can try it again with less files or consider using the 
command prompt. packARC has been tested to work properly with thousands 
of files from the command line. This issue also happens with drag and 
drop in other applications, so it might not be a limitation of packARC 
but a limitation of Windows. 

Please note that, although being well-tested, packARC is still in an 
early stage and shouldn't be used for backup of important data! You 
should at least verify an archive before deleting your copies of the 
files inside. 
 

Developer Info / How To Compile
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The packARC and packANY library sources are provided "as is". Except for 
these notes and the inline comments you are on your own compiling and 
using the source code. Note that due to SFX support and the separate 
packANY library, packARC cannot be build by GCC "make" alone. 

That being said, compiling packARC is really not all too difficult, and 
there also several Makefiles for different OSes and use cases included 
as well as a Windows .BAT file - kindly provided by Prof. Dr. Gerhard 
Seelmann - to handle the whole process under Microsoft Windows. 

If you want full packARC support for your own software project, I 
suggest to take a look at the "pja_archiver.cpp" and "pja_archiver.h" 
source files first. These two include all the interfaces you will need. 

If you are only interested in specific compression functionality (f.e. 
packPNM), note that the respective sources can also be compiled 
standalone. 


History
~~~~~~~

v0.7beta18 (12/17/2014) (public)
 - fixed MultiArc friendly file list format
 
v0.7beta17 (11/21/2014) (public)
 - updated packMP3 library to new version (now licensed under LGPL v3)
 - packARC is now open source under the terms of the LGPL v3
 - added MultiArc friendly file list format
 - some minor bugfixes
 
v0.7beta16 (02/25/2014) (public)
 - improved/fixed temp file handling in Linux
 - fixed some more possible file name creation bugs
 
v0.7beta15 (02/24/2014) (public)
 - improved Linux compilation script (actually working now)
 - added Linux 32bit executable ('packARC.lxe')
 - updated all compression libraries to their newest versions
 - fixed some Linux specific stuff 
 - fixed a bug in SFX file name creation
 
v0.7beta13 (01/13/2014) (public)
 - updated packJPG and packPNM libraries
 - fixed segmentation fault error
 - SFX archives may now have any extension
 - improved batch compilation script for MSDOS (thanks to Se)
 - added compilation script for Linux (thanks to Se) 
 
v0.7beta11 (12/07/2013) (public)
 - updated packJPG, packPNM and packMP3 libraries
 - packARC is now completely open source under the GPL v3
 
v0.7beta (04/06/2013) (public)
 - packMP3 algorithm added
 - packPNM algorithm added (updated to v1.6)
 - packARI algorithm added
 - support for folder recursion
 - support for archives >2GB
 - backwards compatibility preserved
 - new icons for archiver, SFX and PJA files
 - several bugfixes and improvements

v0.3 (11/11/2011) (public)
 - first release alongside packJPG v2.5
 - contains support for compressing JPEGs using the packJPG library
 - contains support for storing other files
 - all basic archiver functionality included
 - no support for directory recursion yet


Acknowledgements
~~~~~~~~~~~~~~~~

packJPG, packMP3, packPNM, packARI and packARC are the result of 
countless hours of research and development. packJPG is part of my final 
year project for HTW Aalen University, packMP3 is part of my master 
thesis for Ratisbon University. 

Prof. Dr. Gerhard Seelmann from HTW Aalen University introduced me to 
the field of data compression when I was still studying in Aalen. He 
supported my development of packJPG and various other compression 
program with his extensive knowledge in the field of data compression. 
Without him, neither of my compression programs would have been 
possible. 

Prof. Dr. Christian Wolff supervised my master thesis at Ratisbon 
University. Thanks go to him for giving me such an interesting topic to 
work on and for all the helpful advice during my time working on the 
thesis. 

Thanks goes to Stephan Busch of SqueezeChart.com fame for spending many, 
many hours beta-testing packMP3 and several other of my compression 
programs. He's the one to thank for my software running smoothly and not 
causing you any trouble. 

The official homepage of packJPG, packMP3, packPNM, packARI and packARC 
is currently maintained by HTW Aalen University staff.

packARC uses UPX compression for smaller executable sizes. UPX is freely 
available from http://upx.sourceforge.net/. 

Original packJPG logos and icons are designed by Michael Kaufmann. 


Contact
~~~~~~~

Matthias Stirner's official homepage:
 http://www.matthias.stirner.com
 
The official home of packARC and packJPG:
 http://packjpg.encode.ru
 
For questions and bug reports:
 packjpg (at) matthiasstirner.com


____________________________________
packMP3 by Matthias Stirner, 01/2016
