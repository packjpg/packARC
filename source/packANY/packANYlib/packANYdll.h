// packANYdll.h - function declarations and reference for the packANY DLL
#define IMPORT __declspec( dllimport )


/* suggested usage:
	1.) init streams using xxxlib_init_streams() (see below)
	2.) convert files using the xxxlib_convert_stream2stream() function
	... or ...
	1.) convert between files using xxxlib_convert_file2file()

   input/output stream types and setup for
   the xxxlib_init_streams() function:
	
	if input is file
	----------------
	in_scr -> name of input file
	in_type -> 0
	in_size -> ignore
	
	if input is memory
	------------------
	in_scr -> array containg data
	in_type -> 1
	in_size -> size of data array
	
	if input is *FILE (f.e. stdin)
	------------------------------
	in_src -> stream pointer
	in_type -> 2
	in_size -> ignore
	
	same for output streams! */


	
/* -----------------------------------------------
	function declarations: packARI library
	----------------------------------------------- */

IMPORT bool parlib_convert_stream2stream( char* msg );
IMPORT bool parlib_convert_file2file( char* in, char* out, char* msg );
IMPORT bool parlib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg );
IMPORT void parlib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type );
IMPORT void parlib_force_encoding( bool setting );
IMPORT const char* parlib_version_info( void );
IMPORT const char* parlib_short_name( void );


/* -----------------------------------------------
	function declarations: packJPG library
	----------------------------------------------- */

IMPORT bool pjglib_convert_stream2stream( char* msg );
IMPORT bool pjglib_convert_file2file( char* in, char* out, char* msg );
IMPORT bool pjglib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg );
IMPORT void pjglib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type );
IMPORT const char* pjglib_version_info( void );
IMPORT const char* pjglib_short_name( void );


/* -----------------------------------------------
	function declarations: packMP3 library
	----------------------------------------------- */

IMPORT bool pmplib_convert_stream2stream( char* msg );
IMPORT bool pmplib_convert_file2file( char* in, char* out, char* msg );
IMPORT bool pmplib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg );
IMPORT void pmplib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type );
IMPORT const char* pmplib_version_info( void );
IMPORT const char* pmplib_short_name( void );


/* -----------------------------------------------
	function declarations: packPNM library
	----------------------------------------------- */

IMPORT bool ppnlib_convert_stream2stream( char* msg );
IMPORT bool ppnlib_convert_file2file( char* in, char* out, char* msg );
IMPORT bool ppnlib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg );
IMPORT void ppnlib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type );
IMPORT const char* ppnlib_version_info( void );
IMPORT const char* ppnlib_short_name( void );
