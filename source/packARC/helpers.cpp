#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include "helpers.h"

#if defined(_WIN32) // define manually if mkdir is not found
	#include <dir.h>
	#define MKDIR( dir ) mkdir( dir )
#else
	#include <unistd.h>
	#include <sys/stat.h>
	#define MKDIR( dir ) mkdir( dir, 0711 )
#endif

#define DIR_SEPERATOR	'/' // should be '/'
#define PATH_BUFFER		128 // must be >= 1 !


/* -----------------------------------------------
	clones a string
	----------------------------------------------- */
char* clone_string( char* string ) {
	int len = strlen( string ) + 1;	
	char* string_copy = (char*) calloc( len, sizeof( char ) );	
	
	// copy string
	if ( string_copy != NULL ) strcpy( string_copy, string );
	
	return string_copy;
}

/* -----------------------------------------------
	convert a string to lowercase
	----------------------------------------------- */
void to_lowercase( char* string ) {
	for( ; (*string) != '\0'; string++ ) {
		if ( ( (*string) >= (unsigned char) 'A' ) && ( (*string) <= (unsigned char) 'Z' ) )
			(*string) += ( (unsigned char) 'a' - (unsigned char) 'A' ); 
	}
}

/* -----------------------------------------------
	case-independent string compare
	----------------------------------------------- */
int strcmp_ci( const char* str1, const char* str2 ) {
	// special letters such as german umlauts are still case dependent!
	int diff;
	
	do {
		diff = (*str1) - (*str2);
		if ( (*str1) == '\0' ) break;
		if ( diff == ( 'a' - 'A' ) ) {
			if ( ( (*str1) >= 'a' ) && ( (*str1) <= 'z' ) ) diff = 0;
		} else if ( diff == ( 'A' - 'a' ) ) {
			if ( ( (*str1) >= 'A' ) && ( (*str1) <= 'Z' ) ) diff = 0;
		}
		str1++; str2++;
	} while ( diff == 0 );
	
	return diff;
}

/* -----------------------------------------------
	creates filename, callocs memory for it
	----------------------------------------------- */
char* create_filename( const char* base, const char* extension )
{
	int len = strlen( base ) + ( ( extension == NULL ) ? 0 : strlen( extension ) + 1 ) + 1;	
	char* filename = (char*) calloc( len, sizeof( char ) );	
	
	// create a filename from base & extension
	strcpy( filename, base );
	set_extension( filename, extension );
	
	return filename;
}

/* -----------------------------------------------
	creates filename, callocs memory for it
	----------------------------------------------- */
char* unique_filename( const char* base, const char* extension ) {
	int len = strlen( base ) + ( ( extension == NULL ) ? 0 : strlen( extension ) + 1 ) + 1;	
	char* filename = (char*) calloc( len, sizeof( char ) );	
	
	// create a unique filename using underscores
	strcpy( filename, base );
	set_extension( filename, extension );
	while ( file_exists( filename ) ) {
		len += sizeof( char );
		filename = (char*) realloc( filename, len );
		add_underscore( filename );
	}
	
	return filename;
}

/* -----------------------------------------------
	creates filename, callocs memory for it
	----------------------------------------------- */
char* unique_tempname( const char* base, const char* extension ) {
	char* nbase = ( char* ) calloc ( strlen( base ) + 2, sizeof( char ) );
	char* filename;
	
	// build new base: insert dot before filename
	strcpy( nbase, base );
	filename = get_filename( nbase );
	for ( char* fnptr = filename + strlen( filename ); fnptr >= filename; fnptr-- )
		fnptr[1] = fnptr[0];
	filename[0] = '.';
	
	// build unique filename, discard base
	filename = unique_filename( nbase, extension );
	free( nbase );
	
	return filename;
}

/* -----------------------------------------------
	adds underscore after filename
	----------------------------------------------- */
void add_underscore( char* filename ) {
	char* tmpname = (char*) calloc( strlen( filename ) + 1, sizeof( char ) );
	char* extstr;
	
	// copy filename to tmpname
	strcpy( tmpname, filename );
	// search extension in filename
	extstr = get_extension( filename );
	
	// add underscore before extension
	(*extstr++) = '_';
	strcpy( extstr, get_extension( tmpname ) );
		
	// free memory
	free( tmpname );
}

/* -----------------------------------------------
	returns extension of filename
	----------------------------------------------- */
char* get_extension( char* filename ) {
	char* extstr;
	
	// find position of extension in filename
	extstr = strrchr( get_filename( filename ), '.' );
	if ( ( extstr == NULL ) || ( extstr == get_filename( filename ) ) )
		extstr = filename + strlen( filename );
	
	// extstr = ( strrchr( filename, '.' ) == NULL ) ?
	//	filename + strlen( filename ) : strrchr( filename, '.' );
	
	return extstr;
}

/* -----------------------------------------------
	changes extension of filename
	----------------------------------------------- */
void set_extension( char* filename, const char* extension ) {
	char* extstr;
	
	// find position of extension in filename	
	extstr = get_extension( filename );
	
	// set new extension
	if ( extension != NULL ) {
		(*extstr++) = '.';
		strcpy( extstr, extension );
	}
	else
		(*extstr) = '\0';
}

/* -----------------------------------------------
	assure containing path exists
	----------------------------------------------- */
bool assure_cpath_rec( char* filepath ) {	
	char path_t[ MAX_PATH_LENGTH + 1 ];
	int len;
	
	
	// find out length of containing path...
	for ( len = strlen( filepath ) - 1; len > 0; len-- )
		if ( ( filepath[ len ] == '\\' ) || ( filepath[ len ] == '/' ) ) break;
	for ( ; len > 0; len-- )
		if ( ( filepath[ len - 1 ] != '\\' ) && ( filepath[ len - 1 ] != '/' ) ) break;
	if ( len == 0 ) return true; // zero length paths exist by definition
	// if ( len > MAX_PATH_LENGTH  ) return false; // too long paths are not allowed
	// ... and make a local copy
	strncpy( path_t, filepath, len );
	path_t[ len ] = '\0';
	
	// check if containing path exists
	if ( !path_exists( path_t ) ) { // directory is not there
		// get one level deeper...
		if ( !assure_cpath_rec( path_t ) ) return false;
		// ... then create the 'path_t' dir!
		if ( MKDIR( path_t ) == -1 ) return false;		
	}
	
	
	return true;
}

/* -----------------------------------------------
	assure containing path, open file
	----------------------------------------------- */
FILE* fopen_filepath( char* filepath, const char* mode ) {
	// check length of path
	if ( strlen( filepath ) > MAX_PATH_LENGTH  ) return NULL;
	// assure containing path
	if ( !assure_cpath_rec( filepath ) ) return NULL;
		
	return fopen( filepath, mode );
}

/* -----------------------------------------------
	expand filelist recursive helper function
	----------------------------------------------- */
static bool expand_filelist_rec ( char* filepath, char*** exp_filelist, int* n ) {
	DIR *dp;
	struct dirent* entry;
	char* newpath;
	
	
	dp = opendir( filepath );
	if ( dp != NULL ) { // this is a directory
		while ( ( entry = readdir( dp ) ) != NULL ) {
			if ( ( strcmp( entry->d_name, "." ) == 0 ) || ( strcmp( entry->d_name, ".." ) == 0 ) )
				continue; // skip . and .. entries
			newpath = ( char* )
				calloc( strlen( filepath ) + strlen( entry->d_name ) + 2, sizeof ( char ) );
			sprintf( newpath, "%s%c%s", filepath, DIR_SEPERATOR, entry->d_name );
			if ( expand_filelist_rec( newpath, exp_filelist, n ) ) free ( newpath );
		}
		closedir( dp );
		return true;
	} else { // this is not a directory - add filename to list
		if ( ( (*n) % PATH_BUFFER ) == 0 )
			(*exp_filelist) = ( char** ) realloc( (*exp_filelist), ( (*n) + PATH_BUFFER + 1 ) * sizeof( char* ) );
		(*exp_filelist)[ (*n)++ ] = filepath;
		return false;	
	}

	
	return false;
}

/* -----------------------------------------------
	collect filenames in directories
	----------------------------------------------- */
char** expand_filelist( char** filelist, int len ) {
	char** exp_filelist = ( char** ) calloc ( 1, sizeof( char* ) );
	int exp_len = 0;
	char* filepath;
	int i;
	
	
	// recursively collect filenames
	for ( i = 0; i < len; i++ ) {
		filepath = clone_string( filelist[i] );
		if ( expand_filelist_rec( filepath, &exp_filelist, &exp_len ) ) free( filepath );
	}
	exp_filelist = ( char** ) realloc( exp_filelist, ( exp_len + 1 ) * sizeof( char* ) );
	exp_filelist[ exp_len ] = NULL;
	
	
	return exp_filelist;
}

/* -----------------------------------------------
	returns filename (without path)
	----------------------------------------------- */
char* get_filename( char* file_string ) {
	char* filename = ( strrchr( file_string, '\\' ) == NULL ) ? 
		strrchr( file_string, '/' ) : strrchr( file_string, '\\' );
	
	return ( filename == NULL )	? file_string : filename + 1;
}

/* -----------------------------------------------
	get common path for a list of files
	----------------------------------------------- */
char* get_common_path( char** filelist, int len ) {
	char* path;
	int plen;
	
	
	// make sure the filelist is not empty
	if ( len == 0 ) return NULL;
		
	// get path and basepath length for first entry
	path = clone_string( filelist[0] );
	for ( plen = strlen( path ) + 1; plen > 0; plen-- ) 
		if ( ( path[ plen - 1 ] == '\\' ) || ( path[ plen - 1 ] == '/' ) ) break;
	path[ plen ] = '\0';
	
	// iterate through all entries of the filelist
	for ( int i = 1; i < len; i++ ) {
		while ( ( plen > 0 ) && ( strncmp( filelist[i], path, plen ) != 0 ) ) {
			for ( ; plen > 0; plen-- )
				if ( ( path[ plen - 1 ] != '\\' ) && ( path[ plen - 1 ] != '/' ) ) break;
			for ( ; plen > 0; plen-- ) 
				if ( ( path[ plen - 1 ] == '\\' ) || ( path[ plen - 1 ] == '/' ) ) break;
			path[ plen ] = '\0';
		}
	}
	
	// don't waste precious memory
	path = ( char* ) realloc( path, ( strlen( path ) + 1 ) * sizeof( char ) );
	
	
	return path;
}

/* -----------------------------------------------
	create a standardized version of the filepath
	----------------------------------------------- */
void normalize_filepath( char* filepath ) {
	char* in_ptr = filepath;
	char* out_ptr = filepath;
	
	
	// double slashes at path begin are allowed (but nowhere else)
	if ( (*in_ptr) == DIR_SEPERATOR ) {
		in_ptr++;
		out_ptr++;
	}
	
	// replace all '\\' and multiple slashes with '/'
	do {
		if ( ( (*in_ptr) == '\\' ) || ( (*in_ptr) == '/' ) ) {
			in_ptr++;
			while ( (*in_ptr == '\\' ) || ( (*in_ptr) == '/' ) ) in_ptr++;
			(*out_ptr++) = DIR_SEPERATOR;
		}
		(*out_ptr++) = (*in_ptr);
	} while ( (*in_ptr++) != '\0' ); 
	
	
	return;
}

/* -----------------------------------------------
	create a standardized version of the filepath
	----------------------------------------------- */
bool set_containing_path( char* filepath )
{
	char path[ MAX_PATH_LENGTH + 1 ];
	int len;
	
	// find out length of containing path...
	for ( len = strlen( filepath ) - 1; len > 0; len-- )
		if ( ( filepath[ len ] == '\\' ) || ( filepath[ len ] == '/' ) ) break;
	for ( ; len > 0; len-- )
		if ( ( filepath[ len - 1 ] != '\\' ) && ( filepath[ len - 1 ] != '/' ) ) break;
	if ( len > MAX_PATH_LENGTH ) return false;
	
	// copy containing path
	strncpy( path, filepath, len );
	path[ len ] = '\0';
	
	// set path and return
	return ( chdir( path ) != -1 );
}

/* -----------------------------------------------
	checks if a file exists
	----------------------------------------------- */
bool file_exists( const char* filename ) {
	// needed for both, executable and library
	FILE* fp = fopen( filename, "rb" );
	
	if ( fp == NULL ) return false;
	else {
		fclose( fp );
		return true;
	}
}

/* -----------------------------------------------
	checks if a path exists
	----------------------------------------------- */
bool path_exists( const char* filename ) {
	DIR* dp = opendir( filename );
	
	if ( dp == NULL ) return false;
	else {
		closedir( dp );
		return true;
	}
}

/* -----------------------------------------------
	returns size of file if possible
	----------------------------------------------- */
off_t file_size( const char* filename ) {
	FILE* fp;
	off_t size;
	
	fp = fopen( filename, "rb" );
	if ( fp != NULL ) {
		fseek( fp, 0, SEEK_END );
		size = ftell( fp );
		fclose( fp );
	} else size = 0;
	
	return size;
}

/* -----------------------------------------------
	realloc alternative
	----------------------------------------------- */
void* frealloc( void* ptr, size_t size ) {
	void* n_ptr = frealloc( ptr, size );
	if ( n_ptr == NULL ) free( ptr );
	return n_ptr;
}
