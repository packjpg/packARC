#include "lfs_supp.h"
#define MAX_PATH_LENGTH		260

/* -----------------------------------------------
	helper functions (*: used in frontend)
	----------------------------------------------- */

// generic string manipulation
char* clone_string( char* string ); // *
void to_lowercase( char* string );
int strcmp_ci( const char* str1, const char* str2 );

// filename specific manipulation
char* create_filename( const char* base, const char* extension );
char* unique_filename( const char* base, const char* extension );
char* unique_tempname( const char* base, const char* extension );
void add_underscore( char* filename );
char* get_extension( char* filename );
void set_extension( char* filename, const char* extension );

// path name specific manipulation
char* get_filename( char* file_string ); // *
bool assure_cpath_rec( char* filepath );
FILE* fopen_filepath( char* filepath, const char* mode );
char** expand_filelist( char** filelist, int len ); // *
char* get_common_path( char** filelist, int len ); // *
void normalize_filepath( char* filepath );
bool set_containing_path( char* filepath ); // *

// file existing / size checks
bool file_exists( const char* filename ); // *
bool path_exists( const char* filename );
off_t file_size( const char* filename );

// alternative realloc
void* frealloc( void* ptr, size_t size );
