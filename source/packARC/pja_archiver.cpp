#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pja_archiver.h"
#include "packANYlib.h"

#if defined(SFX_STUB)
	#define EXTRACT_ONLY 1
#elif !defined(EXTRACT_ONLY)
	#include "sfxstub.h"
#endif

#define INTERN static

// classic mode means compression only for known file types and
// outputting verbose library error messages (not recommended)
// #define CLASSIC_MODE
// use packARI library for compression
#define USE_PACKARI
// use packMP3 library for compression
#define USE_PACKMP3
// use packPNM library for compression
#define USE_PACKPNM

#if defined (CLASSIC_MODE)
	#define LIBRARY_OUTPUT pja_library_status
#else
	#define LIBRARY_OUTPUT NULL
#endif

#define BUF_LEN					16 * 1024 * 1024 // size of buffer
#define CRC32_SEED				0xFFFFFFFF // crc32 start value
#define CRC32_POLY				0xEDB88320 // crc32 polynomial
#define CRC32_CALC(crc,byte)	( ( ( crc >> 8 ) & 0x00FFFFFF ) ^ ( crc32_table[ ( crc ^ (byte) ) & 0xFF ] ) )

#define FTYPE_VOID				-1 // file not existing
#define FTYPE_UNK				0 // file type: unknown
#define FTYPE_JPG				1 // file type: jpeg
#define FTYPE_MP3				2 // file type: mp3
#define FTYPE_PNM				3 // file type: pnm

#define CTYPE_STO				0 // compression type: none/store
#define CTYPE_PJG				1 // compression type: packJPG
#define CTYPE_PMP				2 // compression type: packMP3
#define CTYPE_ARI				3 // compression type: packARI
#define CTYPE_PPN				4 // compression type: packPNM
#define CTYPE_BROKEN			255 // special type for broken frames


// PJA file info struct
struct pja_file {
	// these need to be stored in the frame header
	char filename[ MAX_PATH_LENGTH + 1 ];
	unsigned char method_id;
	unsigned int header_crc;
	unsigned int file_crc;
	off_t file_size_original;
	off_t file_size_compressed;
	time_t last_changed;
	// these need not to be stored
	off_t file_address;
	off_t frame_size;
	off_t frame_address;
};

// PJA archive info struct
struct pja_archive {
	// these need to stored inside the EOF header
	unsigned char archive_version;
	off_t archive_offset; // position of first frame
	// no need to store these
	char* filename;
	FILE* fileptr;
	int num_files;
	pja_file** filelist;
};

// compression / decompression functions
#if !defined( EXTRACT_ONLY )
INTERN inline int identify_filetype( char* filepath );
INTERN inline bool insert_file( pja_archive* archive, pja_file* file, char* filepath );
#endif
INTERN inline bool extract_file( pja_archive* archive, pja_file* file, char* filepath );
INTERN inline bool verify_file( pja_archive* archive, pja_file* file );
// generic storage routines
INTERN inline off_t store( FILE* archive, char* filename );
INTERN inline off_t unstore( FILE* archive, char* filename, off_t filesize );

// internal functions
INTERN inline bool build_filelist( pja_archive* archive );
INTERN inline int search_filelist( pja_archive* archive, char* filename );
INTERN inline bool read_eof_header( pja_archive* archive );
INTERN inline bool read_frame_header( pja_file* file, FILE* fp );
#if !defined( EXTRACT_ONLY)
INTERN inline bool write_eof_header( pja_archive* archive );
INTERN inline bool write_frame_header( pja_file* file, FILE* fp );
INTERN inline bool copy_frames( pja_archive* source, pja_archive* destination );
INTERN inline bool copy_stub( pja_archive* source, pja_archive* destination );
#endif
INTERN inline void wipe_archive( pja_archive* archive );
INTERN inline void wipe_archive_info( pja_archive_info* archive_info );
INTERN inline void fill_external_info( pja_archive* archive, pja_archive_info* archive_info );
INTERN inline off_t copy_data( FILE* source, FILE* destination, off_t size );

// crc32 calculation
INTERN inline void build_crc32_table( void );
INTERN inline unsigned int calc_crc32_mem( unsigned int crc, unsigned char* data, off_t size );
INTERN inline unsigned int calc_crc32_mem( unsigned char* data, off_t size );
INTERN inline unsigned int calc_crc32_file( char* filename );

// byte packing
INTERN unsigned int unpack_bytes( int num_bytes, unsigned char* data );
#if !defined( EXTRACT_ONLY)
INTERN void pack_bytes( unsigned int number, int num_bytes, unsigned char* data );
#endif

// these structs hold all info about the current archive
INTERN pja_archive* pja_current_archive = NULL; // current archive is read only!
INTERN pja_archive* pja_temp_archive = NULL; // temp archive is write only!

// frontends may use this to generate a file list
INTERN pja_archive_info* pja_current_archive_info = NULL;

// version information is stored here
INTERN char pja_version_info[ 320 ] = { 0 };

// status messages are stored here:
INTERN char pja_current_status[ 320 ] = { 0 };

#if defined( CLASSIC_MODE )
// libstatus messages are stored here:
INTERN char pja_library_status[ 320 ] = { 0 };
#endif

// precalculated values for crc calculation
INTERN unsigned int crc32_table[ 256 ] = { 0 };

// options and settings
INTERN char existing_files_handling = 's';
INTERN bool check_extracted_crc = true;

// application info variables
INTERN const unsigned char appversion = 7;
INTERN const char*  subversion   = "beta18";
INTERN const char*  apptitle     = "packARC";
INTERN const char*  versiondate  = "12/17/2014";
INTERN const char*  author       = "Matthias Stirner / Se";
// INTERN const char*  appname      = "pjgarclib";
// INTERN const char*  website      = "http://www.elektronik.htw-aalen.de/packjpg/";
// INTERN const char*  copyright    = "2006-2014 HTW Aalen University & Matthias Stirner";
// INTERN const char*  email        = "packjpg (at) htw-aalen.de";
INTERN const char   pja_id[]     = { 'P', 'J', 'A'  };


/* -----------------------------------------------
	open PJA archive for processing
	----------------------------------------------- */
bool pja_open_archive( char* filename ) {
	// close last archive if it not already is
	if ( pja_current_archive != NULL ) pja_close_archive();
	
	// check if path already exists
	if ( path_exists( filename ) ) {
		sprintf( pja_current_status, "path with the same name already exists" );
		return false;
	}
	
	// build crc32 table
	build_crc32_table();
	
	// alloc memory for archive info structs
	pja_current_archive = (pja_archive*) calloc( 1, sizeof( pja_archive ) );
	pja_temp_archive = (pja_archive*) calloc( 1, sizeof( pja_archive ) );
	pja_current_archive_info = (pja_archive_info*) calloc( 1, sizeof( pja_archive_info ) );
	
	// store filename, open archive file
	pja_current_archive->filename = clone_string( filename ); // copy filename
	pja_current_archive->fileptr = fopen( filename, "rb" ); // open archive file
	// the archive file does not necessarily have to exist at this point!
	
	// read PJA EOF header
	if ( !read_eof_header( pja_current_archive ) ) { 
		pja_close_archive();
		sprintf( pja_current_status, "file is not a PJA archive" );
		return false;
	}
	
	// check archive size, compatibility for archives > 2GB (pointer must be at file end!)
	if ( pja_current_archive->fileptr != NULL ) if ( ftell( pja_current_archive->fileptr ) < 0 ) {
		pja_close_archive();
		sprintf( pja_current_status, "archives > 2GB need LFS support enabled" );
		return false; 
	}
	
	// check archive version
	if ( pja_current_archive->archive_version > appversion ) {
		// backward compatibility: works with public v0.3, dangerous with non-public releases
		pja_close_archive();
		sprintf( pja_current_status, "incompatible archive version (v%i.%i)",
			appversion / 10, appversion % 10 );
		return false;
	}
	
	// build file list
	if ( !build_filelist( pja_current_archive ) ) {
		pja_close_archive();
		sprintf( pja_current_status, "errors in archive structure" );
		return false;
	}
	
	// fill temp archive internal info
	pja_temp_archive->filename = unique_filename( filename, "tmp" );
	pja_temp_archive->archive_version = appversion;
	// offset is yet unknown - this MUST be resolved before any writing operations to the temp file occur!
	pja_temp_archive->archive_offset = -1; 
	// we won't open the temp file yet - maybe we don't need it at all
	pja_temp_archive->num_files = 0;
	pja_temp_archive->filelist = NULL;
	
	// fill current archive external info
	fill_external_info( pja_current_archive, pja_current_archive_info );
	
	return true;
}

/* -----------------------------------------------
	update PJA archive
	----------------------------------------------- */
bool pja_update_archive( void ) {
	char* filename = clone_string( pja_current_archive->filename );
	
	// updating the archive basically means closing & reopening it
	if ( !pja_close_archive() ) return false;
	if ( !pja_open_archive( filename ) ) return false;
	
	free( filename );
	
	return true;
}

/* -----------------------------------------------
	close PJA archive, clean up mem
	----------------------------------------------- */
bool pja_close_archive( void ) {
	bool success = true;
	
	// check if archive is opened
	if ( pja_current_archive == NULL ) {
		sprintf( pja_current_status, "archive not opened" );
		return false;
	}
	
	#if !defined(EXTRACT_ONLY)
	int i;
	// check for any modifications to the archive, only update if neccesary!
	for ( i = pja_current_archive->num_files - 1; i >= 0; i-- ) // check for file deletions
		if ( pja_current_archive->filelist[ i ] == NULL ) break;
	if ( ( i >= 0 ) || ( pja_temp_archive->fileptr != NULL ) ) { // check for temp file
		while ( true ) {
			// check for any errors in newly compressed files (if any)
			for ( i = pja_temp_archive->num_files - 1; i >= 0; i-- ) // check for broken archive
				if ( pja_temp_archive->filelist[ i ]->method_id == CTYPE_BROKEN ) break;
			if ( i >= 0 ) { // method id 255 means compression errors!
				sprintf( pja_current_status, "errors in temp file structure, rolling back" );
				remove( pja_temp_archive->filename );
				success = false; break;
			}
			
			// create the temp archive file if it is not already there
			if ( pja_temp_archive->fileptr == NULL ) {
				pja_temp_archive->fileptr = fopen( pja_temp_archive->filename, "wb" );
				if ( pja_temp_archive->fileptr == NULL ) { // check file pointer
					sprintf( pja_current_status, "cannot open temp file for writing" );
					success = false; break;
				}
			}
			
			// check & resolve temp archive offset
			if ( pja_temp_archive->archive_offset == -1 ) { // only if offset is yet unresolved!
				if ( pja_current_archive->archive_offset > 0 ) // copy stub from source			
					copy_stub( pja_current_archive, pja_temp_archive );
				else pja_temp_archive->archive_offset = 0;
			}
		
			// copy over frames from source archive to temp archive
			copy_frames( pja_current_archive, pja_temp_archive );
			// write eof header to temp archive
			write_eof_header( pja_temp_archive );
			
			// check for file errors
			if ( ferror( pja_temp_archive->fileptr ) ) { // usually this means that the disk is full
				sprintf( pja_current_status, "write error, possibly disk is full" );
				success = false; break;
			}
			
			// close the file pointers
			if ( pja_current_archive->fileptr != NULL ) {
				fclose( pja_current_archive->fileptr );
				pja_current_archive->fileptr = NULL;
			}
			fflush( pja_temp_archive->fileptr );
			fclose( pja_temp_archive->fileptr );
			pja_temp_archive->fileptr = NULL;
			
			// check for 2GB limit exceeded
			#if !defined( ENABLE_LFS )
			if ( ( file_size( pja_temp_archive->filename ) < 0 ) ||
				 ( ( sizeof( off_t ) > 4 ) && ( file_size( pja_temp_archive->filename ) >= ((off_t) 1) << 31 ) ) )
			#else
			if ( file_size( pja_temp_archive->filename ) < 0 )
			#endif
			{
				sprintf( pja_current_status, "archives > 2GB need LFS support enabled" );
				remove( pja_temp_archive->filename );
				success = false; break;
			}
			
			// now for the dangerous part :-) - delete the original archive
			remove( pja_current_archive->filename );
			if ( file_exists( pja_current_archive->filename ) ) { // check if it still exists
				sprintf( pja_current_status, "error updating archive, rolling back" );
				remove( pja_temp_archive->filename );
				success = false; break;
			}
			
			// an ugly fix for the problem of archive extension after conversion
			if ( pja_temp_archive->archive_offset != pja_current_archive->archive_offset ) {
				char* tmp_cptr = pja_current_archive->filename;
				if ( pja_temp_archive->archive_offset == 0 )
					 pja_current_archive->filename = unique_filename( tmp_cptr, PJA_ARC_EXT );
				else pja_current_archive->filename = unique_filename( tmp_cptr, PJA_SFX_EXT );
				free( tmp_cptr );
			}
			
			// point of no return reached - rename temp archive
			if ( rename( pja_temp_archive->filename, pja_current_archive->filename ) != 0 ) {
				sprintf( pja_current_status, "cannot update filename, archive is found in \"%s\"",
					pja_temp_archive->filename );
				success = false; break;
			}
			
			// if the routine finishes here, everything went fine!
			break;
		}
	}
	#endif
	
	// wipe external archive info
	wipe_archive_info( pja_current_archive_info );
	pja_current_archive_info = NULL;
	
	// wipe internal archive info
	wipe_archive( pja_current_archive );
	pja_current_archive = NULL;
	
	// wipe temp internal archive info
	wipe_archive( pja_temp_archive );
	pja_temp_archive = NULL;
	
	
	return success;
}

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	add one file to the archive
	----------------------------------------------- */
bool pja_add_file_to_archive( char* filename_int, char* filename_ext ) {
	pja_file* current_file = NULL;
	char fn[ MAX_PATH_LENGTH + 1 ];
	off_t filesize;
	int i;
	
	// check if archive is opened
	if ( pja_current_archive == NULL ) {
		sprintf( pja_current_status, "archive not opened" );
		return false;
	}
	
	// check length of internal filepath
	if ( strlen( filename_int ) > 260 ) {
		sprintf( pja_current_status, "paths above %i chars are not supported", MAX_PATH_LENGTH );
		return false;
	}
	
	// check if internal filepath is relative
	if ( ( filename_int[0] == '/' ) || ( filename_int[0] == '\\' ) || ( strchr( filename_int, ':' ) != NULL ) ) {
		sprintf( pja_current_status, "absolute paths not allowed in archive" );
		return false;
	}
	
	// check if file exists
	if ( !file_exists( filename_ext ) ) {
		sprintf( pja_current_status, "file does not exist" );
		return false;
	}
	
	// check size of file (> 2GB safety check)
	filesize = file_size( filename_ext );
	if ( ( filesize < 0 ) || ( ( sizeof( off_t ) > 4 ) && ( filesize >= ((off_t) 1) << 31 ) ) ) {
		sprintf( pja_current_status, "file is > 2GB, not supported" );
		return false;
	}
	
	// copy internal filename, normalize path
	strcpy( fn, filename_int );
	normalize_filepath( fn );
	
	// search for filename in temp archive file list
	if ( search_filelist( pja_temp_archive, fn ) >= 0 ) {
		sprintf( pja_current_status, "duplicate filenames and/or paths are not supported" );
		return false;
	}
	
	// search filename in current archive file list
	i = search_filelist( pja_current_archive, fn );
	if ( i >= 0 ) {
		// handle filename according to setting
		switch ( existing_files_handling ) {
			case 's': // skip file
				sprintf( pja_current_status, "file exists in archive, skipped" );
				return false; break;
				
			case 'o': // overwrite file
				free( pja_current_archive->filelist[ i ] );
				pja_current_archive->filelist[ i ] = NULL;
				break;
				
			case 'r': // rename file
				do {
					if ( strlen( fn ) > MAX_PATH_LENGTH ) {
						sprintf( pja_current_status, "trying to rename failed, skipped" );
						return false;
					}
					add_underscore( fn );
				} while( search_filelist( pja_current_archive, fn ) >= 0 );	
				break;
			
			default: // unknown (skipping for now)
				sprintf( pja_current_status, "file exists in archive, skipped" );
				return false; break;
		}
	}	
	
	// create the temp archive file if it is not already there
	if ( pja_temp_archive->fileptr == NULL ) {
		pja_temp_archive->fileptr = fopen( pja_temp_archive->filename, "wb" );
		if ( pja_temp_archive->fileptr == NULL ) { // check file pointer
			sprintf( pja_current_status, "cannot open temp file for writing" );
			return false;
		}
	}
	
	// check & resolve temp archive offset
	if ( pja_temp_archive->archive_offset == -1 ) { // only if offset is yet unresolved!
		if ( pja_current_archive->archive_offset > 0 ) // copy stub from source			
			copy_stub( pja_current_archive, pja_temp_archive );
		else pja_temp_archive->archive_offset = 0;
	}
	
	// build file info, fill in filename (might be different from original filename)
	current_file = ( pja_file* ) calloc( 1, sizeof( pja_file ) );
	strcpy( current_file->filename, fn );
	// other info will be added in the insert_file() function!
	
	// seek till temp archive end, do >2GB safety check
	fseek( pja_temp_archive->fileptr, 0, SEEK_END );
	if ( ftell( pja_temp_archive->fileptr ) < 0 ) {
		sprintf( pja_current_status, "2GB archive size limit exceeded" );
		return false;
	}
	
	// insert the file at the end of the temp archive file
	if ( !insert_file( pja_temp_archive, current_file, filename_ext ) ) {
		sprintf( pja_current_status, "fatal error, close archive to roll back!" );			
		return false;
	}
	
	// check for file errors
	if ( ferror( pja_temp_archive->fileptr ) ) { // usually this means that the disk is full
		sprintf( pja_current_status, "write error, possibly disk is full" );
		return false;
	}
	
	// handling of failed compression routines
	if ( current_file->method_id == CTYPE_BROKEN ) {
		sprintf( pja_current_status, "error compressing file" );
		return false;
	}
	
	// classic mode: verbose output for stored files
	#if defined( CLASSIC_MODE )
	if ( current_file->method_id == CTYPE_STO ) {
		strcpy( pja_current_status, ( identify_filetype( filename_ext ) == FTYPE_UNK ) ?
			"unknown file" : pja_library_status );
		return false;
	}
	#endif
	
	return true;
}
#endif

/* -----------------------------------------------
	extract one file from the archive
	----------------------------------------------- */
bool pja_extract_file_from_archive( char* filename_int, char* filename_ext ) {
	pja_file* current_file = NULL;
	char fn[ MAX_PATH_LENGTH + 1 ];
	int i;
	
	// check if archive is opened
	if ( pja_current_archive == NULL ) {
		sprintf( pja_current_status, "archive not opened" );
		return false;
	}
	
	// filename length checking not needed!
	
	// copy output filename
	strcpy( fn, filename_ext );
	
	// search for filename in current archive
	i = search_filelist( pja_current_archive, filename_int );
	if ( i >= 0 ) current_file = pja_current_archive->filelist[ i ];
	else {
		sprintf( pja_current_status, "file not found in archive" );
		return false;
	}
	
	// check if output filename already exists
	if ( file_exists( fn ) ) {
		// handle filename according to setting
		switch ( existing_files_handling ) {
			case 's': // skip file
				sprintf( pja_current_status, "file already exists" );
				return false; break;
				
			case 'o': // overwrite file
				remove( fn ); // remove the existing file
				if ( file_exists( fn ) ) { // check if it still exists
					sprintf( pja_current_status, "cannot overwrite file, skipped" );
					return false;
				}
				break; // nothing to do
				
			case 'r': // rename file
				do {
					if ( strlen( fn ) > MAX_PATH_LENGTH ) {
						sprintf( pja_current_status, "trying to rename failed, skipped"	);
						return false;
					}
					add_underscore( fn );
				} while( file_exists( fn ) );	
				break;
			
			default: // unknown (skipping for now)
				sprintf( pja_current_status, "file already exists, skipped" );
				return false; break;
		}
	} else { // assure the containing path
		if ( !assure_cpath_rec( fn ) ) {
			sprintf( pja_current_status, "cannot create containing path" );
			return false;
		}
	}
	
	// extract the file from the archive
	if ( !extract_file( pja_current_archive, current_file, fn ) ) {
		sprintf( pja_current_status, "error extracting file" );
		return false;
	}	
	
	return true;
}

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	delete one file from the archive
	----------------------------------------------- */
bool pja_remove_file_from_archive( char* filename ) {
	int i;
	
	// check if archive is opened
	if ( pja_current_archive == NULL ) {
		sprintf( pja_current_status, "archive not opened" );
		return false;
	}
	
	// search file list for filename
	i = search_filelist( pja_current_archive, filename );
	if ( i >= 0 ) { // file was found, remove it from the list
		free( pja_current_archive->filelist[ i ] );
		pja_current_archive->filelist[ i ] = NULL;
		// all the hard work is actually done by pja_close_archive()!
	}
	else {
		sprintf( pja_current_status, "file not found in archive" );
		return false;
	}
	
	return true;
}
#endif

/* -----------------------------------------------
	test integrity of one file in the archive
	----------------------------------------------- */
bool pja_test_file_in_archive( char* filename ) {
	pja_file* current_file = NULL;
	int i;
	
	// check if archive is opened
	if ( pja_current_archive == NULL ) {
		sprintf( pja_current_status, "archive not opened" );
		return false;
	}
	
	// search for filename in current archive
	i = search_filelist( pja_current_archive, filename );
	if ( i >= 0 ) current_file = pja_current_archive->filelist[ i ];
	else {
		sprintf( pja_current_status, "file not found in archive" );
		return false;
	}
	
	// check file integrity
	if ( !verify_file( pja_current_archive, current_file ) ) {
		sprintf( pja_current_status, "CRC32 mismatch, file broken" );
		return false;
	}
	
	return true;
}

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	convert archive to SFX and vice versa
	----------------------------------------------- */
bool pja_convert_archive( void ) {
	// check if archive is opened
	if ( pja_current_archive == NULL ) {
		sprintf( pja_current_status, "archive not opened" );
		return false;
	}
	
	// check if temp archive offset already resolved
	if ( pja_temp_archive->archive_offset != -1 ) {
		sprintf( pja_current_status, "archive offset already resolved, no conversion possible" );
		return false;
	}	
	
	// create the temp archive file if it not exists
	if ( pja_temp_archive->fileptr == NULL ) {
		pja_temp_archive->fileptr = fopen( pja_temp_archive->filename, "wb" );
		if ( pja_temp_archive->fileptr == NULL ) { // check file pointer
			sprintf( pja_current_status, "cannot open temp file for writing" );
			return false;
		}
	}	
	
	if ( pja_current_archive->archive_offset > 0 ) {
		// if archive offset > 0 convert to plain PJA
		pja_temp_archive->archive_offset = 0;		
	}
	else { // otherwise convert to PJX
		// write PJX stub to archive file
		fseek( pja_temp_archive->fileptr, 0, SEEK_SET );
		fwrite( pjxstub, 1, pjxstub_size, pja_temp_archive->fileptr );
		
		// store archive offset
		pja_temp_archive->archive_offset = pjxstub_size;
		
		// check for file errors
		if ( ferror( pja_temp_archive->fileptr ) ) { // usually this means that the disk is full
			sprintf( pja_current_status, "write error, possibly disk is full" );
			return false;
		}	
	}
	
	return true;
}
#endif

/* -----------------------------------------------
	sets the handling for existing files
	----------------------------------------------- */
void pja_set_existing_files_handling( char setting ) {
	existing_files_handling = setting;
}

/* -----------------------------------------------
	check extracted files crc on/off
	----------------------------------------------- */
void pja_set_check_extracted_crc( bool setting ) {
	check_extracted_crc = setting;
}

/* -----------------------------------------------
	returns a pointer to the archive info struct
	----------------------------------------------- */
pja_archive_info* pja_get_archive_info( void ) {
	return pja_current_archive_info;
}

/* -----------------------------------------------
	returns a pointer to statuts string
	----------------------------------------------- */
char* pja_get_current_status( void ) {
	return pja_current_status;
}

/* -----------------------------------------------
	archiver version information
	----------------------------------------------- */
char* pja_get_version_info( void )
{	
	// copy version info to string
	sprintf( pja_version_info, "--> %s library v%i.%i%s (%s) by %s <--",
			apptitle, appversion / 10, appversion % 10, subversion, versiondate, author );
	
	return pja_version_info;
}

/* -----------------------------------------------
	library version information
	----------------------------------------------- */
char* pja_get_engine_info( void )
{	
	// packARC (engine info begin)
	sprintf( pja_version_info, "--> contains:" );
	
	// packJPG
	sprintf( pja_version_info + strlen( pja_version_info ), " %s", pjglib_short_name() );
	
	#if defined ( USE_PACKMP3 )
	// packMP3
	sprintf( pja_version_info + strlen( pja_version_info ), ", %s", pmplib_short_name() );
	#endif
	
	#if defined ( USE_PACKPNM )
	// packPNM
	sprintf( pja_version_info + strlen( pja_version_info ), ", %s", ppnlib_short_name() );
	#endif
	
	#if defined ( USE_PACKARI )
	// packARI
	sprintf( pja_version_info + strlen( pja_version_info ), ", %s", parlib_short_name() );
	#endif
	
	// engine info finish
	sprintf( pja_version_info + strlen( pja_version_info ), " <--" );
	
	return pja_version_info;
}

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	use to identify file type
	----------------------------------------------- */
INTERN inline int identify_filetype( char* filepath ) {
	unsigned char id[ 2 ] = { 0, 0 };
	char* ext = get_extension( filepath );
	FILE* fp;
	
	
	// read file id (first 2 bytes)
	fp = fopen( filepath, "rb" );
	if ( fp == NULL ) return FTYPE_VOID;
	if ( fread( id, 1, 2, fp ) != 2 ) {
		fclose( fp );
		return FTYPE_UNK;
	}
	fclose ( fp );
	
	// check actual filetype
	if ( ( ( strcmp_ci( ext, ".jpg" ) == 0 ) || ( strcmp_ci( ext, ".jpeg" ) == 0 ) ||
		( strcmp_ci( ext, ".jfif" ) == 0 ) ) && ( id[ 0 ] == 0xFF ) && ( id[ 1 ] == 0xD8 ) ) {
		return FTYPE_JPG; // JPEG filetype
	} else if ( ( strcmp_ci( ext, ".mp3" ) == 0 ) && ( ( id[0] != 'M' ) || ( id[1] != 'S' ) ) ) {
		return FTYPE_MP3; // MP3 filetype
	} else if ( ( ( strcmp_ci( ext, ".ppm" ) == 0 ) || ( strcmp_ci( ext, ".pgm" ) == 0 ) ||
		( strcmp_ci( ext, ".pbm" ) == 0 ) ) && ( id[ 0 ] == 'P' ) ) {
		return FTYPE_PNM; // PPM/PGM/PBM filetype
	} else if ( ( strcmp_ci( ext, ".bmp" ) == 0 ) && ( id[0] == 'B' ) && ( id[1] == 'M' ) ) {
		return FTYPE_PNM; // BMP filetype
	} 
	
	
	// all failed -> unknown filetype
	return FTYPE_UNK;
}
#endif

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	handles file insertion
	----------------------------------------------- */
INTERN inline bool insert_file( pja_archive* archive, pja_file* file, char* filepath ) {
	FILE* fp = archive->fileptr;
	char* temp_fn = unique_tempname( archive->filename, "a.tmp" );
	int filetype;
	int i;
	
	// fill in file size, file crc, date of last change
	file->last_changed = time( NULL );
	file->file_size_original = file_size( filepath );
	file->file_crc = calc_crc32_file( filepath );
	
	// compress file to temp file
	while ( true ) {
		// identify file
		filetype = identify_filetype( filepath );
		if ( filetype == FTYPE_VOID ) { // file somehow vanished...
			file->file_size_compressed = 0; // prevent frame size errors
			file->method_id = CTYPE_BROKEN; // very, very improbable, though
			break;
		} 
		
		// handling of known filetypes
		if ( filetype == FTYPE_JPG ) {
			// packJPG compression for JPG
			if ( pjglib_convert_file2file( filepath, temp_fn, LIBRARY_OUTPUT ) ) {
				file->method_id = CTYPE_PJG;
				break;
			}
		}
		#if defined( USE_PACKMP3 )
		else if ( filetype == FTYPE_MP3 ) {
			// packMP3 compression for MP3
			if ( pmplib_convert_file2file( filepath, temp_fn, LIBRARY_OUTPUT ) ) {
				file->method_id = CTYPE_PMP;
				break;
			}
		}
		#endif
		#if defined( USE_PACKPNM )
		else if ( filetype == FTYPE_PNM ) {
			// packPNM compression for PPM/PGM/PBM
			if ( ppnlib_convert_file2file( filepath, temp_fn, LIBRARY_OUTPUT ) ) {
				file->method_id = CTYPE_PPN;
				break;
			}
		}
		#endif
		
		// packJPG/packMP3/packPNM all failed - either compress (ari) or store
		#if defined( USE_PACKARI ) && !defined( CLASSIC_MODE )
		// arithmetic compression (*has* to work)
		if ( parlib_convert_file2file( filepath, temp_fn, LIBRARY_OUTPUT ) ) {
			file->method_id = CTYPE_ARI;
			break;
		}
		#endif
		
		// store file (this will work 100%)
		file->method_id = CTYPE_STO;
		break;
	}
	
	// store and check size of compressed file
	if ( ( file->method_id != CTYPE_STO ) && ( file->method_id != CTYPE_BROKEN ) ) {
		file->file_size_compressed = file_size( temp_fn );
		// this also makes sure that the compressed file is <= 2GB
		if ( file->file_size_compressed >= file->file_size_original ) {
			file->method_id = CTYPE_STO;
			file->file_size_compressed = file->file_size_original;
		} else if ( file->file_size_compressed == 0 )
			file->method_id = CTYPE_BROKEN;
	} else if ( file->method_id == CTYPE_STO ) {
		file->file_size_compressed = file->file_size_original;
	} else file->file_size_compressed = 0;
	
	// calculate size of frame
	file->frame_size = file->file_size_compressed + ( file->file_address - file->frame_address );
	
	// store start-of-frame
	file->frame_address = ftell( fp );	
	// write frame header to archive
	write_frame_header( file, fp );	
	// store start-of-file
	file->file_address = ftell( fp );	
	
	// store (compressed) file in archive
	if ( file->file_size_compressed !=
		store( fp, ( file->method_id != CTYPE_STO ) ? temp_fn : filepath ) )
		// possibly does not match physical method_id in header (!)
		file->method_id = CTYPE_BROKEN;
		
	// remove temp file, clear memory
	if ( file_exists( temp_fn ) ) remove( temp_fn );
	free( temp_fn );
	
	// insert file info into file list
	i = archive->num_files++;
	if ( archive->filelist == NULL ) archive->filelist =
		( pja_file** ) calloc( 1, sizeof( pja_file* ) );
	else archive->filelist =
		( pja_file** ) realloc( archive->filelist, (i+1) * sizeof( pja_file* ) );
	archive->filelist[ i ] = file;
	
	
	return true;
}
#endif

/* -----------------------------------------------
	handles file extraction
	----------------------------------------------- */
INTERN inline bool extract_file( pja_archive* archive, pja_file* file, char* filepath ) {
	FILE* fp = archive->fileptr;
	char* temp_fn = unique_tempname( archive->filename, "x.tmp" );
	
	
	// unstore compressed file to temp file
	fseek( fp, file->file_address, SEEK_SET );
	unstore( fp, temp_fn, file->file_size_compressed );
	
	// further process temp file
	switch ( file->method_id ) {
		case CTYPE_STO:
			rename( temp_fn, filepath );
			break;
		
		case CTYPE_PJG:
			pjglib_convert_file2file( temp_fn, filepath, LIBRARY_OUTPUT );
			break;			
		#if defined( USE_PACKMP3 )
		case CTYPE_PMP:
			pmplib_convert_file2file( temp_fn, filepath, LIBRARY_OUTPUT );
			break;
		#endif		
		#if defined( USE_PACKARI )
		case CTYPE_ARI:
			parlib_convert_file2file( temp_fn, filepath, LIBRARY_OUTPUT );
			break;
		#endif		
		#if defined( USE_PACKPNM )
		case CTYPE_PPN:
			ppnlib_convert_file2file( temp_fn, filepath, LIBRARY_OUTPUT );
			break;
		#endif
			
		default:
			return false;
			break;
	}
	
	// remove temp file, clear memory
	if ( file_exists( temp_fn ) ) remove( temp_fn );
	free( temp_fn );
	
	// check extracted size & crc
	if ( check_extracted_crc ) { // only do this test if the option is set
		if ( file_size( filepath ) != file->file_size_original )
			return false;
		if ( file->file_crc != calc_crc32_file( filepath ) )
			return false;
	}
	
	return true;
}

/* -----------------------------------------------
	handles file verification
	----------------------------------------------- */
INTERN inline bool verify_file( pja_archive* archive, pja_file* file ) {
	char* temp_fn = unique_tempname( archive->filename, "t.tmp" );
	
	// simply extract, CRC check done in extract_file() 
	if ( !extract_file( archive, file, temp_fn ) ) return false;
	
	// remove temp file, clear memory
	if ( file_exists( temp_fn ) ) remove( temp_fn );
	free( temp_fn );
	
	return true;
}

/* -----------------------------------------------
	store file in archive
	----------------------------------------------- */
INTERN inline off_t store( FILE* archive, char* filename ) {
	FILE* fp;
	off_t filesize;
	
	fp = fopen( filename, "rb" );
	if ( fp == NULL ) return 0;
	filesize = copy_data( fp, archive, -1 );
	fclose( fp );
	
	return filesize;
}

/* -----------------------------------------------
	unstore file in archive
	----------------------------------------------- */
INTERN inline off_t unstore( FILE* archive, char* filename, off_t filesize ) {
	FILE* fp;
	
	fp = fopen( filename, "wb" );
	if ( fp == NULL ) return 0;
	filesize = copy_data( archive, fp, filesize );
	fflush( fp );
	fclose( fp );
	
	return filesize;
}

/* -----------------------------------------------
	reads EOF file header
	----------------------------------------------- */
INTERN inline bool read_eof_header( pja_archive* archive ) {
	FILE* fp = archive->fileptr;
	unsigned char pja_eof_header[ 4 + 4 + 3 + 1 ];
	
	// first, check if the file already exists
	if ( fp == NULL ) { // file does not yet exist (ok for adding operation)
		archive->archive_version = appversion;
		archive->archive_offset = 0;
		return true;
	}
	
	// read the EOF header
	fseek( fp, 0 - 4 - 4 - 3 - 1, SEEK_END );
	fread( pja_eof_header, 1, 4 + 4 + 3 + 1, fp );
	// check next 4 bytes (must be 0xFFFFFFFF)
	if ( pja_eof_header[  0 ] != 0xFF ) return false;
	if ( pja_eof_header[  1 ] != 0xFF ) return false;
	if ( pja_eof_header[  2 ] != 0xFF ) return false;
	if ( pja_eof_header[  3 ] != 0xFF ) return false;
	// read next 4 bytes (archive offset)
	archive->archive_offset = (off_t) unpack_bytes( 4, pja_eof_header + 4 );
	// check PJA archive id (must be "PJA", see above)
	if ( pja_eof_header[  8 ] != pja_id[ 0 ] ) return false;
	if ( pja_eof_header[  9 ] != pja_id[ 1 ] ) return false;
	if ( pja_eof_header[ 10 ] != pja_id[ 2 ] ) return false;
	// read PJA archive version
	archive->archive_version = pja_eof_header[ 11 ];
	
	return true;
}

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	writes EOF file header
	----------------------------------------------- */
INTERN inline bool write_eof_header( pja_archive* archive ) {
	FILE* fp = archive->fileptr;
	unsigned char pja_eof_header[ 4 + 4 + 3 + 1 ];
	
	if ( fp == NULL ) // file is not writeable
		return false;
	
	// EOF header must be written at the end of the archive!
	fseek( fp, 0, SEEK_END );
	// first 4 bytes must be 0xFFFFFFFF
	pja_eof_header[  0 ] = 0xFF;
	pja_eof_header[  1 ] = 0xFF;
	pja_eof_header[  2 ] = 0xFF;
	pja_eof_header[  3 ] = 0xFF;
	// write archive offset
	pack_bytes( (unsigned int) archive->archive_offset, 4, pja_eof_header + 4 );
	// write PJA archive id
	pja_eof_header[  8 ] = pja_id[ 0 ];
	pja_eof_header[  9 ] = pja_id[ 1 ];
	pja_eof_header[ 10 ] = pja_id[ 2 ];
	// write PJA archive version
	pja_eof_header[ 11 ] = archive->archive_version;
	
	// write EOF header to file
	fwrite( pja_eof_header, 1, 4 + 4 + 3 + 1, fp );
	
	return true;
}
#endif

/* -----------------------------------------------
	build list of files in archive
	----------------------------------------------- */
INTERN inline bool build_filelist( pja_archive* archive ) {
	FILE* fp = archive->fileptr;
	pja_file* current_file = NULL;
	unsigned char chkbt[ 4 ];
	int i;
	
	// first, check if the archive file already exists
	if ( fp == NULL ) { // great, we're done already!
		archive->num_files = 0;
		archive->filelist = NULL;
		return true;
	}
	else {
		archive->num_files = 0;
		archive->filelist = NULL;
		fseek( fp, archive->archive_offset, SEEK_SET ); // skip SFX stub
		for ( i = 0; ; i++ ) {
			// check for EOF header or file EOF
			if ( fread( chkbt, 1, 4, fp ) != 4 ) return false; // read 4 bytes, check for end of file
			if ( chkbt[ 0 ] == 0xFF && chkbt[ 1 ] == 0xFF && chkbt[ 2 ] == 0xFF && chkbt[ 3 ] == 0xFF )
				break; // EOF file header reached
			fseek( fp, -4, SEEK_CUR ); // go back 4 bytes
			
			// read frame header, skip to begin of next frame
			current_file = ( pja_file* ) calloc( 1, sizeof( pja_file ) );
			current_file->frame_address = ftell( fp );
			if ( !read_frame_header( current_file, fp ) ) return false;
			current_file->file_address = ftell( fp );
			fseek( fp, current_file->file_size_compressed, SEEK_CUR );
			current_file->frame_size = ftell( fp ) - current_file->frame_address;
			
			// insert file info into file list
			if ( archive->filelist == NULL ) archive->filelist =
				( pja_file** ) calloc( 1, sizeof( pja_file* ) );
			else archive->filelist =
				( pja_file** ) realloc( archive->filelist, (i+1) * sizeof( pja_file* ) );
			archive->filelist[ i ] = current_file;
		}
		archive->num_files = i;
	}
	
	return true;
}

/* -----------------------------------------------
	search for file in filelist
	----------------------------------------------- */
INTERN inline int search_filelist( pja_archive* archive, char* filename ) {
	char fn[ MAX_PATH_LENGTH + 1 ];
	int i;
	
	// check filename length
	if ( strlen( filename ) > MAX_PATH_LENGTH ) return -1;
	
	// copy filename, normalize path
	strcpy( fn, filename );
	normalize_filepath( fn );
	
	// search for filename in list, return index if found, -1 else
	for ( i = archive->num_files - 1; i >= 0; i-- ) {
		if ( archive->filelist[ i ] != NULL )
			if ( strcmp_ci( archive->filelist[ i ]->filename, fn ) == 0 ) break;
	}
	
	return i;
}

/* -----------------------------------------------
	read frame header
	----------------------------------------------- */
INTERN inline bool read_frame_header( pja_file* file, FILE* fp ) {
	unsigned char frame_header[ MAX_PATH_LENGTH + 64 ]; // !!!
	unsigned char* fh_ptr = frame_header;
	int fn_len = 0;
	
	// read filename
	do {
		fread( fh_ptr, 1, 1, fp );
		if ( ++fn_len > MAX_PATH_LENGTH ) break;
	} while ( (*fh_ptr++) != '\0' );
	// check filename length
	if ( ( fn_len == 0 ) || ( fn_len > MAX_PATH_LENGTH ) )
		return false; // irrecoverable errors in header
	
	// read rest of data
	if ( fread( fh_ptr, 1, 4 + 1 + 4 + 4 + 4 + 4, fp ) != 4 + 1 + 4 + 4 + 4 + 4 )
		return false;
	
	// fill file header
	strcpy( file->filename, (char*) frame_header );
	file->last_changed = ( time_t ) unpack_bytes( 4, fh_ptr +  0 );
	file->method_id	= fh_ptr[  4 ];
	file->file_size_original = (off_t) unpack_bytes( 4, fh_ptr +  5 );
	file->file_size_compressed = (off_t) unpack_bytes( 4, fh_ptr +  9 );
	file->file_crc = unpack_bytes( 4, fh_ptr + 13 );
	file->header_crc = unpack_bytes( 4, fh_ptr + 17 );
	
	// done, check header crc
	if ( file->header_crc != calc_crc32_mem( frame_header, fn_len + 4 + 1 + 4 + 4 + 4 ) )
		return false;
	
	return true;
}

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	write frame header
	----------------------------------------------- */
INTERN inline bool write_frame_header( pja_file* file, FILE* fp ) {
	unsigned char frame_header[ MAX_PATH_LENGTH + 64 ]; // !!!
	unsigned char* fh_ptr = frame_header;
	int fn_len = strlen( file->filename ) + 1;
	
	// write filename to frame header array
	strcpy( (char*) fh_ptr, file->filename );
	fh_ptr += fn_len;
	
	// fill rest of frame header array
	pack_bytes( (unsigned int) file->last_changed, 4, fh_ptr +  0 ); // Y2K38 might occur !!!!
	fh_ptr[ 4 ] = file->method_id;
	pack_bytes( (unsigned int) file->file_size_original, 4, fh_ptr +  5 );
	pack_bytes( (unsigned int) file->file_size_compressed, 4, fh_ptr + 9 );
	pack_bytes( file->file_crc, 4, fh_ptr + 13 );
	
	file->header_crc = calc_crc32_mem( frame_header, fn_len + 4 + 1 + 4 + 4 + 4 );
	pack_bytes( file->header_crc, 4, fh_ptr + 17 );
	
	// write data to file
	fwrite( frame_header, 1, fn_len + 4 + 1 + 4 + 4 + 4 + 4, fp );
	
	return true;
}
#endif

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	copy frames from one archive to another
	----------------------------------------------- */
INTERN inline bool copy_frames( pja_archive* source, pja_archive* destination ) {
	FILE* fp_src = source->fileptr;
	FILE* fp_dst = destination->fileptr;
	int i;
	
	// copy frames from source archive to destination archive
	for ( i = 0; i < source->num_files; i++ ) {
		if ( source->filelist[ i ] != NULL ) {
			// copy data
			fseek( fp_src, source->filelist[ i ]->frame_address, SEEK_SET );
			fseek( fp_dst, 0, SEEK_END );
			copy_data( fp_src, fp_dst, source->filelist[ i ]->frame_size );
			// file info is NOT added to the destination filelist struct
			// as this is not actually needed here!
		}
	}
	
	return true;
}
#endif

#if !defined(EXTRACT_ONLY)
INTERN inline bool copy_stub( pja_archive* source, pja_archive* destination ) {
	FILE* fp_src = source->fileptr;
	FILE* fp_dst = destination->fileptr;
	
	// using this function when the destination file has
	// already been written to leads to severe errors!
	
	// copy data using copy_data()
	fseek( fp_src, 0, SEEK_SET );
	fseek( fp_dst, 0, SEEK_SET );
	if ( copy_data( fp_src, fp_dst, source->archive_offset ) != source->archive_offset )
		return false;
	// destination archive offset = source archive offset
	destination->archive_offset = source->archive_offset;
	
	return true;
}
#endif

/* -----------------------------------------------
	wipe the archive struct, free memory
	----------------------------------------------- */
INTERN inline void wipe_archive( pja_archive* archive ) {
	int i;
	
	if ( archive != NULL ) {
		// cleanup internal archive struct
		// free each entry of the file list
		for ( i = archive->num_files - 1; i >= 0; i-- ) {
			if ( archive->filelist[ i ] != NULL )
				free ( archive->filelist[ i ] );
		}
		// free filelist array
		if ( archive->filelist != NULL )
			free( archive->filelist );
		// free filename array
		if ( archive->filename != NULL )
			free( archive->filename );
		// close file pointer
		if ( archive->fileptr != NULL )
			fclose( archive->fileptr );
		// free archive itself
		free( archive );
	}
}

/* -----------------------------------------------
	wipe the archive info struct, free memory
	----------------------------------------------- */
INTERN inline void wipe_archive_info( pja_archive_info* archive_info ) {
	int i;
	
	if ( archive_info != NULL ) {
		// cleanup external archive info struct
		// free each entry of the file list
		for ( i = archive_info->num_files - 1; i >= 0; i-- ) {
			if ( archive_info->filelist[ i ] != NULL )
				free ( archive_info->filelist[ i ] );
		}
		// free filelist array
		if ( archive_info->filelist != NULL )
			free( archive_info->filelist );
		// free filename array
		if ( archive_info->filename != NULL )
			free( archive_info->filename );
		// free archive info itself
		free( archive_info );
	}
}

/* -----------------------------------------------
	fill external info struct
	----------------------------------------------- */
INTERN inline void fill_external_info( pja_archive* archive, pja_archive_info* archive_info ) {
	pja_file_info* current_file_info = NULL;
	pja_file* current_file = NULL;
	int i;
	
	// copy basic info from internal archive struct
	archive_info->filename = clone_string( get_filename( archive->filename ) );
	archive_info->archive_type = ( archive->archive_offset > 0 ) ? 1 : 0;
	archive_info->archive_version = archive->archive_version;
	archive_info->file_size_archive = file_size( archive->filename );
	archive_info->num_files = archive->num_files;
	
	// presets - need to process filelist to fill
	archive_info->file_size_extracted = 0;
	archive_info->last_changed = 0;
	
	// process filelist
	archive_info->filelist = ( pja_file_info** ) calloc( archive->num_files, sizeof( pja_file_info* ) );
	for ( i = archive->num_files - 1; i >= 0; i-- ) {
		current_file_info = ( pja_file_info* ) calloc( 1, sizeof( pja_file_info ) );
		current_file = archive->filelist[ i ];
		// copy info from internal file list to external file list
		strcpy( current_file_info->filename, archive->filelist[ i ]->filename );
		switch ( archive->filelist[ i ]->method_id ) {
			case CTYPE_STO:	 strcpy( current_file_info->compression_method, "STO" ); break;
			case CTYPE_PJG:	 strcpy( current_file_info->compression_method, "PJG" ); break;
			case CTYPE_PMP:	 strcpy( current_file_info->compression_method, "PMP" ); break;
			case CTYPE_ARI:	 strcpy( current_file_info->compression_method, "ARI" ); break;
			case CTYPE_PPN:	 strcpy( current_file_info->compression_method, "PPN" ); break;
			default: strcpy( current_file_info->compression_method, "???" ); break;
		}
		current_file_info->file_crc = ~(current_file->file_crc); // inverted crc, as this more common
		current_file_info->file_size_original = current_file->file_size_original;
		current_file_info->file_size_compressed = current_file->file_size_compressed;
		current_file_info->compression_ratio = ( current_file->file_size_original == 0 ) ? 0 :
			(float) current_file->file_size_compressed / (float) current_file->file_size_original;
		current_file_info->last_changed = current_file->last_changed;
		// update archive info
		archive_info->filelist[ i ] = current_file_info;
		archive_info->file_size_extracted += current_file_info->file_size_original;
		if ( current_file_info->last_changed > archive_info->last_changed )
			archive_info->last_changed = current_file_info->last_changed;
	}
	archive_info->compression_ratio = ( archive_info->file_size_extracted == 0 ) ? 0 :
		(float) archive_info->file_size_archive / (float) archive_info->file_size_extracted;
}

/* -----------------------------------------------
	copy data from one file to another
	----------------------------------------------- */
INTERN inline off_t copy_data( FILE* source, FILE* destination, off_t size ) {
	unsigned char* buffer;
	off_t count = 0;
	
	
	// set up buffer
	buffer = ( unsigned char* ) calloc( BUF_LEN, 1 );
	if ( buffer == NULL ) return -1;
	
	// copy data from source to destination
	if ( size == -1 ) { // size -1 means copy till EOF
		while( true ) {
			size = fread( buffer, 1, BUF_LEN, source );
			if ( size == 0 ) break;
			count += fwrite( buffer, 1, size, destination );
		}
	}
	else { // copy specified amount of data otherwise
		for ( ; size >= BUF_LEN; size -= BUF_LEN ) {
			fread( buffer, 1, BUF_LEN, source );
			count += fwrite( buffer, 1, BUF_LEN, destination );
		}
		if ( size > 0 ) {
			fread( buffer, 1, size, source );
			count += fwrite( buffer, 1, size, destination );
		}
	}
	
	// free buffer
	free( buffer );
	
	
	return ( ferror( destination ) ) ? -1 : (off_t) count;
}

/* -----------------------------------------------
	build crc table, only needs to run once!
	----------------------------------------------- */
INTERN inline void build_crc32_table( void ) {
	unsigned int crc;
	int i;

	// fill crc table
	for ( i = 0; i < 256; i++ ) {
		crc = i;
		crc = ( crc & 1 ) ? ( ( crc >> 1 ) ^ CRC32_POLY ) : ( crc >> 1 );
		crc = ( crc & 1 ) ? ( ( crc >> 1 ) ^ CRC32_POLY ) : ( crc >> 1 );
		crc = ( crc & 1 ) ? ( ( crc >> 1 ) ^ CRC32_POLY ) : ( crc >> 1 );
		crc = ( crc & 1 ) ? ( ( crc >> 1 ) ^ CRC32_POLY ) : ( crc >> 1 );
		crc = ( crc & 1 ) ? ( ( crc >> 1 ) ^ CRC32_POLY ) : ( crc >> 1 );
		crc = ( crc & 1 ) ? ( ( crc >> 1 ) ^ CRC32_POLY ) : ( crc >> 1 );
		crc = ( crc & 1 ) ? ( ( crc >> 1 ) ^ CRC32_POLY ) : ( crc >> 1 );
		crc = ( crc & 1 ) ? ( ( crc >> 1 ) ^ CRC32_POLY ) : ( crc >> 1 );
		crc32_table[ i ] = crc;
	}
}

/* -----------------------------------------------
	calculate crc32 for data from memory
	----------------------------------------------- */
INTERN inline unsigned int calc_crc32_mem( unsigned int crc, unsigned char* data, off_t size ) {
	for ( ; size > 0; size--, data++ )
		crc = CRC32_CALC( crc, (*data) ); 
	
	return crc;
}

/* -----------------------------------------------
	calculate crc32 for data from memory
	----------------------------------------------- */
INTERN inline unsigned int calc_crc32_mem( unsigned char* data, off_t size ) {
	return calc_crc32_mem( CRC32_SEED, data, size );
}

/* -----------------------------------------------
	calculate crc32 for data from file 
	----------------------------------------------- */
INTERN inline unsigned int calc_crc32_file( char* filename ) {
	FILE* fp;
	unsigned char* buffer;
	unsigned int crc = CRC32_SEED;
	off_t size;
	
	
	// set up buffer
	buffer = ( unsigned char* ) calloc( BUF_LEN, 1 );
	if ( buffer == NULL ) return 0;
	
	fp = fopen( filename, "rb" );
	if ( fp == NULL ) return 0; // return 0 if file can't be opened
	while( true ) {
		size = fread( buffer, 1, BUF_LEN, fp );
		if ( size == 0 ) break;
		crc = calc_crc32_mem( crc, buffer, size ); // calculate crc
	}
	fclose( fp );
	free( buffer );
	
	
	return crc;
}

/* -----------------------------------------------
	fetches numbers from data, LSB first
	----------------------------------------------- */
INTERN unsigned int unpack_bytes( int num_bytes, unsigned char* data ) {
	unsigned int number = 0;
	int i;
	
	for ( i = 0; i < num_bytes; i++ )
		number |= (*data++) << (i * 8);
	
	return number;
}

#if !defined(EXTRACT_ONLY)
/* -----------------------------------------------
	packs numbers to data, LSB first
	----------------------------------------------- */
INTERN void pack_bytes( unsigned int number, int num_bytes, unsigned char* data ) {
	while ( num_bytes-- > 0 ) {
		(*data++) = ( unsigned char ) number & 0xFF;
		number >>= 8;
	}
}
#endif
