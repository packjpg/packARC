// packPNMdll.h - function import declarations for the packPNM DLL
#define IMPORT __declspec( dllimport )

/* -----------------------------------------------
	function declarations: library only functions
	----------------------------------------------- */
	
IMPORT bool ppnlib_convert_stream2stream( char* msg );
IMPORT bool ppnlib_convert_file2file( char* in, char* out, char* msg );
IMPORT bool ppnlib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg );
IMPORT void ppnlib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type );
IMPORT const char* ppnlib_version_info( void );
IMPORT const char* ppnlib_short_name( void );

/* a short reminder about input/output stream types
   for the ppnlib_init_streams() function
	
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
	
	vice versa for output streams! */
