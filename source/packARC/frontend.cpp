#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #define SFX_STUB
#include "pja_archiver.h"
#include "helpers.h"

#define OUTPUT_ENGINE_INFO

#define BARLEN 36
#define MSGOUT stderr

// application info variables
const unsigned char appversion   = 7; // should be same as packARC library version
static const char*  subversion   = "beta18";
#if !defined(SFX_STUB)
static const char*  apptitle     = "packARC Frontend";
static const char*  appname      = "packARC";
#else
static const char*  apptitle     = "packARC SFX Extractor";
//static const char*  appname      = "pjxstub";
#endif
static const char*  versiondate  = "12/17/2014";
static const char*  author       = "Matthias Stirner / Se";
static const char*  website      = "http://www.elektronik.htw-aalen.de/packjpg/";
static const char*	copyright    = "2006-2014 HTW Aalen University & Matthias Stirner";
static const char*  email        = "packjpg (at) htw-aalen.de";

void process_files( char command, char* archive, int num_files, char** files );
void file_listing( void );
inline void progress_bar( int current, int last );
void show_help_exit( char* my_name );

int list_format = 0;
#if !defined(SFX_STUB)
bool make_sfx = false;
#endif
bool pause_f = true;

/* -----------------------------------------------
	main-function
	----------------------------------------------- */	
int main( int argc, char** argv )
{
	char command = '\0';
	char* my_name = NULL;
	char* archive = NULL;
	int num_files = 0;
	char** filelist = NULL;
	
	// write program info to screen
	fprintf( MSGOUT,  "\n--> %s v%i.%i%s (%s) by %s <--\n",
			apptitle, appversion / 10, appversion % 10, subversion, versiondate, author );
	
	#if defined( OUTPUT_ENGINE_INFO )
	// also output archiver library info
	fprintf( MSGOUT,  "%s\n", pja_get_version_info() );
	// ... and external libraries info
	fprintf( MSGOUT,  "%s\n", pja_get_engine_info() );
	#endif
	
	// copyright notice
	fprintf( MSGOUT, "Copyright %s\nAll rights reserved\n\n", copyright );
	
	#if !defined(SFX_STUB)
	// name of this application (for help)
	my_name = clone_string( (char*) appname );
	#else
	// name of this application (for help), archive
	my_name = create_filename( get_filename( (*argv) ), NULL );
	archive = ( file_exists( (*argv) ) ) ? clone_string( (*argv) ) :
		create_filename( (*argv), PJA_SFX_EXT );
	if ( !file_exists( archive ) ) { // safety check
		fprintf( MSGOUT, "incorrect usage of SFX executable!" );
		return 0;
	}
	#endif
	
	// check number of parameters
	argc--;	argv++;
	#if !defined(SFX_STUB)
	if ( argc == 0 ) show_help_exit( my_name );
	/*else if ( argc == 1 ) {
		// special case: (frontend)
		// if only one parameter is given, we assume the parameter
		// to be the archives filename and the command to be 'l'
		command = 'l';
		archive = argv[ 1 ];
	}*/
	else if ( ( strlen( (*argv) ) > 1 ) &&
		( ( file_exists( (*argv) ) ) || ( path_exists( (*argv) ) ) ) ) {
		// special case: (frontend)
		// if the first parameter is an existing file or path and is
		// longer than 1 char use drag and drop handling
		if ( argc == 1 ) {
			// if there is only one file, check if it is a proper
			// archive - use command 'x' if it is.
			if ( pja_open_archive( (*argv) ) ) {
				pja_close_archive();
				command = 'x';
				archive = (*argv);
				argc--;	argv++;
				// set workpath (easier than properly giving absolute paths)
				set_containing_path( archive );
			}
		}
		if ( argc > 0 ) {
			// if there is (still) more than one file, or the first file
			// is not a proper archive, create a new SFX archive
			command = 'a';
			make_sfx = true;
			archive = unique_filename( (*argv), PJA_SFX_EXT );
			// rest of parameters = filelist
			num_files = argc;
			filelist = argv;
		}
	}
	#else
	if ( argc == 0 ) {
		// special case: (sfx extractor)
		// if no parameter is given, we assume the command to be 'x'
		command = 'x';
	}
	#endif
	else { // otherwise parse the command line
		// check first parameter (must be command)
		if ( strlen( (*argv) ) == 1 )
			command = (*argv)[ 0 ];
		else show_help_exit( my_name );
		if ( // check for valid command
		#if !defined(SFX_STUB)
			( command != 'a' ) &&
			( command != 'd' ) &&
			( command != 'c' ) &&
		#endif
			( command != 'x' ) &&
			( command != 't' ) &&
			( command != 'l' ) )
			show_help_exit( my_name );
			
		// check next parameters (switches)
		while ( --argc > 0 ) {
			argv++;
			if ( strcmp( (*argv), "--" ) == 0 )
				break;
			else if ( strcmp( (*argv), "-i" ) == 0 )
				pja_set_check_extracted_crc( false );
			else if ( strcmp( (*argv), "-o" ) == 0 )
				pja_set_existing_files_handling( 'o' );
			else if ( strcmp( (*argv), "-s" ) == 0 )
				pja_set_existing_files_handling( 's' );
			else if ( strcmp( (*argv), "-r" ) == 0 )
				pja_set_existing_files_handling( 'r' );
			#if !defined(SFX_STUB)
			else if ( strcmp( (*argv), "-sfx" ) == 0 )
				make_sfx = true;
			#endif
			else if ( strcmp( (*argv), "-sl" ) == 0 )
				list_format = 1;
			else if ( strcmp( (*argv), "-csv" ) == 0 )
				list_format = 2;
			else if ( strcmp( (*argv), "-sm" ) == 0 )
				list_format = 3;
			else if ( strcmp( (*argv), "-np" ) == 0 )
				pause_f = false;
			else break;
		}
		
		#if !defined(SFX_STUB)
		// check next parameter (must be name of archive for frontend)
		if ( argc == 0 ) show_help_exit( my_name );
		archive = (*argv++); argc--;
		#endif
		
		// rest of parameters = filelist
		num_files = argc;
		filelist = argv;
	}
	
	// process command and filelist
	process_files( command, archive, num_files, filelist );
	
	// pause before exit
	if ( pause_f ) {
		fprintf( MSGOUT, "\n\n< press ENTER >\n" );
		fgetc( stdin );
	}
	
	
	return 0;
}

/* -----------------------------------------------
	process a list of files
	----------------------------------------------- */
void process_files( char command, char* archive, int num_files, char** files ) {
	pja_archive_info* archive_info;
	char** file_status;
	bool process_all = false;
	int error_count = 0;
	int i;
	
	// open archive...
	if ( !pja_open_archive( archive ) ) {
		fprintf( MSGOUT, "could not open archive \"%s\" (%s)!\n\n", archive, pja_get_current_status() );
		return;
	}
	
	// get pointer to archive info struct
	archive_info = pja_get_archive_info();
	
	// handle num_files == 0 (in parameters)
	if ( num_files == 0 ) {
		if( ( command == 'x' ) || ( command == 't' ) ) {
			// testing or extracting with an empty filelist actually
			// means test/extract all files
			process_all = true;
			num_files = archive_info->num_files;
			files = ( char** ) calloc( num_files, sizeof( char* ) );
			for ( i = 0; i < num_files; i++ )
				files[ i ] = clone_string( archive_info->filelist[ i ]->filename );
		}
		#if !defined(SFX_STUB)
		else if ( ( command == 'a' ) || ( command == 'd' ) ) {
			fprintf( MSGOUT, "nothing to do!\n\n" );
			pja_close_archive();
			return;
		}
		#endif
	}
	else {
		// for commands 'c' and 'l', num_files has to be 0
		// but we won't check this for now :-)
	}
	
	// handle num_file == 0 (in archive)
	if ( ( archive_info->num_files == 0 ) && ( command != 'a' ) ) {
		fprintf( MSGOUT, "archive \"%s\" does not exist!\n\n", archive_info->filename );
		return;
	}
	
	// alloc memory for file status, preset with NULL
	file_status = ( char** ) calloc( num_files, sizeof( char* ) );
	for ( i = 0; i < num_files; i++ )
		file_status[ i ] = NULL;
		
	// main processing
	switch ( command )
	{
		#if !defined(SFX_STUB)
		case 'a':
			char* common_path;
			int cp_len;
			if ( make_sfx ) {
				if ( !pja_convert_archive() ) {
					fprintf( MSGOUT, "error writing SFX stub to archive \"%s\"!\n", archive_info->filename );
					break;
				}
				fprintf( MSGOUT, "adding files to SFX archive \"%s\"...\n", archive_info->filename );
			}
			else {				
				fprintf( MSGOUT, "adding files to PJA archive \"%s\"...\n", archive_info->filename );
			}
			// expand filelist, get common path
			common_path = get_common_path( files, num_files );
			cp_len = strlen( common_path );
			files = expand_filelist( files, num_files );
			// recalculate # of files, realloc file_status array
			for ( num_files = 0; files[ num_files ] != NULL; num_files++ );
			file_status = ( char** ) realloc( file_status, num_files * sizeof( char* ) );
			for ( i = 0; i < num_files; i++ ) file_status[ i ] = NULL;
			// process files
			for ( i = 0; i < num_files; i++ ) {
				// update progress message
				fprintf( MSGOUT, "processing file %2i of %2i ", i + 1, num_files );
				progress_bar( i, num_files );
				fprintf( MSGOUT, "\r" );
				// add file to archive, store message if error
				if ( !pja_add_file_to_archive( files[ i ] + cp_len, files[ i ] ) ) {
					file_status[ i ] = clone_string( pja_get_current_status() );
					error_count++;
				}
			}
			// update progress message one last time
			fprintf( MSGOUT, "processed %2i of %2i files ", num_files, num_files );
			progress_bar( 1, 1 );
			fprintf( MSGOUT, "\n" );
			// close archive
			if ( !pja_close_archive() ) {
				fprintf( MSGOUT, "-> ERROR updating archive (%s)\n", pja_get_current_status() );
				return;
			}
			else {
				// display summary
				#if !defined(CLASSIC_MODE)
				fprintf( MSGOUT, "-> added %i of %i files to the archive\n", num_files - error_count,  num_files );
				#else
				fprintf( MSGOUT, "-> compressed %i of %i files to the archive\n", num_files - error_count,  num_files );
				#endif
			}
			break;
		
		case 'd':
			fprintf( MSGOUT, "deleting files from PJA archive \"%s\"...\n", archive_info->filename );
			for ( i = 0; i < num_files; i++ ) {
				// update progress message
				fprintf( MSGOUT, "processing file %2i of %2i ", i + 1, num_files );
				progress_bar( i, num_files );
				fprintf( MSGOUT, "\r" );
				// add file to archive, store message if error
				if ( !pja_remove_file_from_archive( files[ i ] ) ) {
					file_status[ i ] = clone_string( pja_get_current_status() );
					error_count++;
				}
			}
			// update progress message one last time
			fprintf( MSGOUT, "processed %2i of %2i files ", num_files, num_files );
			progress_bar( 1, 1 );
			fprintf( MSGOUT, "\n" );
			// close archive
			if ( !pja_close_archive() ) {
				fprintf( MSGOUT, "-> ERROR updating archive! (%s)\n", pja_get_current_status() );
				return;
			}
			else // display summary
				fprintf( MSGOUT, "-> deleted %i of %i files from the archive\n", num_files - error_count,  num_files );
			break;
		
		case 'c':
			fprintf( MSGOUT, "converting %s archive \"%s\" to %s...\n",
				( archive_info->archive_type == 0 ) ? "PJA" : "SFX",
				archive_info->filename,
				( archive_info->archive_type == 0 ) ? "SFX" : "PJA" );
			while ( true ) {
				fprintf( MSGOUT, "converting archive " );
				progress_bar( 0, 2 );
				fprintf( MSGOUT, "\r" );
				if ( !pja_convert_archive() ) {
					error_count = 1; break;
				}
				fprintf( MSGOUT, "updating archive   " );
				progress_bar( 1, 2 );
				fprintf( MSGOUT, "\r" );
				if ( !pja_close_archive() ) {
					error_count = 1; break;
				}
				fprintf( MSGOUT, "converted archive  " );
				progress_bar( 2, 2 );
				fprintf( MSGOUT, "\n" );
				break;
			}
			// display summary
			if ( error_count > 0 ) {
				fprintf( MSGOUT, "-> ERROR updating archive! (%s)\n", pja_get_current_status() );
				return;
			}
			else fprintf( MSGOUT, "-> archive successfully converted\n" );
			break;
		#endif
		
		case 'x':
			fprintf( MSGOUT, "extracting files from PJA archive \"%s\"...\n", archive_info->filename );
			for ( i = 0; i < num_files; i++ ) {
				// update progress message
				fprintf( MSGOUT, "processing file %2i of %2i ", i + 1, num_files );
				progress_bar( i, num_files );
				fprintf( MSGOUT, "\r" );
				// add file to archive, store message if error
				if ( !pja_extract_file_from_archive( files[ i ], files[ i ] ) ) {
					file_status[ i ] = clone_string( pja_get_current_status() );
					error_count++;
				}
			}
			// update progress message one last time
			fprintf( MSGOUT, "processed %2i of %2i files ", num_files, num_files );
			progress_bar( 1, 1 );
			fprintf( MSGOUT, "\n" );
			// close archive
			pja_close_archive();
			// display summary
			if ( ( process_all ) && ( error_count == 0 ) )
				fprintf( MSGOUT, "-> extracted all %i files from the archive\n", num_files );
			else fprintf( MSGOUT, "-> extracted %i of %i files from the archive\n", num_files - error_count,  num_files );
			break;
			
		case 't':
			fprintf( MSGOUT, "verifiying files in PJA archive \"%s\"...\n", archive_info->filename );
			for ( i = 0; i < num_files; i++ ) {
				// update progress message
				fprintf( MSGOUT, "processing file %2i of %2i ", i + 1, num_files );
				progress_bar( i, num_files );
				fprintf( MSGOUT, "\r" );
				// add file to archive, store message if error
				if ( !pja_test_file_in_archive( files[ i ] ) ) {
					file_status[ i ] = clone_string( pja_get_current_status() );
					error_count++;
				}
			}
			// update progress message one last time
			fprintf( MSGOUT, "processed %2i of %2i files ", num_files, num_files );
			progress_bar( 1, 1 );
			fprintf( MSGOUT, "\n" );
			// close archive
			pja_close_archive();
			// display summary
			if ( ( process_all ) && ( error_count == 0 ) )
				fprintf( MSGOUT, "-> verified all %i files from the archive\n", num_files );
			else fprintf( MSGOUT, "-> verified %i of %i files from the archive\n", num_files - error_count,  num_files );
			break;
			
		case 'l':
			// file listing, handled in the file_listing() function
			file_listing();
			pja_close_archive();
			return;
		
		default:
			fprintf( MSGOUT, "unknown command: '%c', application ERROR!\n", command );
			pja_close_archive();
			return ;
	}
	
	// list errors if any, otherwise we're done
	if ( error_count > 0 ) {
		fprintf( MSGOUT, "\n\n" );
		fprintf( MSGOUT, "following files were not processed:\n" );
		fprintf( MSGOUT, "-----------------------------------\n" );
		for ( i = 0; i < num_files; i++ ) {
			if ( file_status[ i ] != NULL )
				fprintf( MSGOUT, "%s (%s)\n", files[ i ], file_status[ i ] );
		}
	}
	
	return;
}

/* -----------------------------------------------
	output filelisting
	----------------------------------------------- */
void file_listing( void ) {
	pja_archive_info* archive_info = pja_get_archive_info();
	pja_file_info** filelist = archive_info->filelist;
	tm* time_info;
	char time_string[ 16 + 1 ];
	int i;
	
	switch ( list_format ) {
		case 0: // verbose (standard) list format
			time_info = localtime( &(archive_info->last_changed) );
			strftime( time_string, 16, "%d/%m/%y %H:%M", time_info );
			fprintf( MSGOUT, "%s v%i.%i ARCHIVE \"%s\" (LAST CHANGED %s)\n", // archive info (title)
				( archive_info->archive_type == 0 ) ? "PJA" : "SFX",
				archive_info->archive_version / 10,
				archive_info->archive_version % 10,
				archive_info->filename,
				time_string );
			fprintf( MSGOUT, "-----------------------------------------------------------------------------\n" );
			// collumn descriptions
			fprintf( MSGOUT, "%-24s %-15s %-4s %-8s %7s %7s %6s\n",
				"FILENAME",	"LAST_CHANGED", "TYP", "CRC32", "SIZE(O)", "SIZE(C)", "RATIO" );		
			// process files one by one
			for ( i = 0; i < archive_info->num_files; i++ ) {
				time_info = localtime( &(filelist[i]->last_changed) );
				strftime( time_string, 16, "%d/%m/%y %H:%M", time_info );
				fprintf( MSGOUT, "%-24.24s %-15s %-4s %08X %7ld %7ld %6.2f\n",
					filelist[i]->filename,
					time_string,
					filelist[i]->compression_method,
					filelist[i]->file_crc,
					(long int) ( ( filelist[i]->file_size_original + 1023 ) / 1024 ),
					(long int) ( ( filelist[i]->file_size_compressed + 1023 ) / 1024 ),
					filelist[i]->compression_ratio * 100 );
			}
			// archive info (footer)
			fprintf( MSGOUT, "-----------------------------------------------------------------------------\n" );
			fprintf( MSGOUT, "TOTAL %i FILES, %ldkb COMPRESSED TO %ldkb (%.2f%%)\n",
				archive_info->num_files,
				(long int) (archive_info->file_size_extracted / 1024),
				(long int) (archive_info->file_size_archive / 1024),
				archive_info->compression_ratio * 100 );
			break;
			
		case 1: // simple list format
			fprintf( MSGOUT, "<%s v%i.%i ARCHIVE \"%s\">\n",
				( archive_info->archive_type == 0 ) ? "PJA" : "SFX",
				archive_info->archive_version / 10,
				archive_info->archive_version % 10,
				archive_info->filename );
			for ( i = 0; i < archive_info->num_files; i++ )
				fprintf( MSGOUT, "%s\n", filelist[i]->filename );
			break;
		
		case 2: // csv list format
			time_info = localtime( &(archive_info->last_changed) );
			strftime( time_string, 16, "%d/%m/%y %H:%M", time_info );
			fprintf( MSGOUT, "%s v%i.%i ARCHIVE \"%s\" (LAST CHANGED %s)\n", // archive info (title)
				( archive_info->archive_type == 0 ) ? "PJA" : "SFX",
				archive_info->archive_version / 10,
				archive_info->archive_version % 10,
				archive_info->filename,
				time_string );
			// column titles
			fprintf( MSGOUT, "%s;%s;%s;%s;%s;%s;%s\n",
				"file name",
				"last changed",
				"compression method",
				"crc32",
				"original size",
				"compressed size",
				"compression ratio" );
			// process files one by one
			for ( i = 0; i < archive_info->num_files; i++ ) {
				time_info = localtime( &(filelist[i]->last_changed) );
				strftime( time_string, 16, "%d/%m/%y %H:%M", time_info );
				fprintf( MSGOUT, "%s;%s;%s;%08X;%ld;%ld;%.2f%%\n",
					filelist[i]->filename,
					time_string,
					filelist[i]->compression_method,
					filelist[i]->file_crc,
					(long int) filelist[i]->file_size_original,
					(long int) filelist[i]->file_size_compressed,
					filelist[i]->compression_ratio * 100 );
			}
			break;
			
		case 3: // multiarc friendly list format
			fprintf( MSGOUT, "[%s v%i.%i ARCHIVE \"%s\"]\n",
				( archive_info->archive_type == 0 ) ? "PJA" : "SFX",
				archive_info->archive_version / 10,
				archive_info->archive_version % 10,
				archive_info->filename );
			// process files one by one
			for ( i = 0; i < archive_info->num_files; i++ ) {
				time_info = localtime( &(filelist[i]->last_changed) );
				strftime( time_string, 24, "%d-%m-%y %H:%M:%S", time_info );
				fprintf( MSGOUT, "%s\n %s %14ld\n",
					filelist[i]->filename,
					time_string,
					(long int) filelist[i]->file_size_original );
			}
			break;
	}
}

/* -----------------------------------------------
	displays progress bar on screen
	----------------------------------------------- */
inline void progress_bar( int current, int last )
{
	int barpos = ( ( current * BARLEN ) + ( last / 2 ) ) / last;
	int i;
	
	
	// generate progress bar
	fprintf( MSGOUT, "[" );
	#if defined(_WIN32)
	for ( i = 0; i < barpos; i++ )
		fprintf( MSGOUT, "\xFE" );
	#else
	for ( i = 0; i < barpos; i++ )
		fprintf( MSGOUT, "X" );
	#endif
	for (  ; i < BARLEN; i++ )
		fprintf( MSGOUT, " " );
	fprintf( MSGOUT, "]" );
}

/* -----------------------------------------------
	show usage instructions and exit
	----------------------------------------------- */	
void show_help_exit( char* my_name ) {
	fprintf( MSGOUT, "Website: %s\n", website );
	fprintf( MSGOUT, "Email  : %s\n", email );
	fprintf( MSGOUT, "\n\n" );
	#if !defined(SFX_STUB)
	fprintf( MSGOUT, "usage: %s [command] [switches] [name of archive] [files to process]\n", my_name );
	#else
	fprintf( MSGOUT, "usage: %s [command] [switches] [files to process]\n", my_name );
	#endif
	fprintf( MSGOUT, "\n" );
	fprintf( MSGOUT, "commands:\n" );
	#if !defined(SFX_STUB)
	fprintf( MSGOUT, " a    add/replace files\n" );
	fprintf( MSGOUT, " d    delete files from archive\n" );
	fprintf( MSGOUT, " c    convert archive to/from SFX\n" );
	#endif
	fprintf( MSGOUT, " x    extract files from archive\n" );
	fprintf( MSGOUT, " t    test archive integrity\n" );
	fprintf( MSGOUT, " l    list files in archive\n" );
	fprintf( MSGOUT, "switches:\n" );
	fprintf( MSGOUT, " --   stop processing switches\n" );
	fprintf( MSGOUT, " -o   overwrite existing files\n" );
	fprintf( MSGOUT, " -s   skip existing files (default)\n" );
	fprintf( MSGOUT, " -r   rename on existing files\n" );
	fprintf( MSGOUT, " -i   (with x) ignore crc errors\n" );
	#if !defined(SFX_STUB)
	fprintf( MSGOUT, " -sfx (with a) create sfx archive\n" );
	#endif
	fprintf( MSGOUT, " -sl  (with l) simple list format\n" );
	fprintf( MSGOUT, " -sm  (with l) MultiArc list format\n" );
	fprintf( MSGOUT, " -csv (with l) CSV list format\n" );
	fprintf( MSGOUT, " -np  no pause after after processing\n" );
	fprintf( MSGOUT, "\n" );
	#if !defined(SFX_STUB)
	fprintf( MSGOUT, "examples: %s a archive.%s *.jpg\n", my_name, PJA_ARC_EXT );
	fprintf( MSGOUT, "          %s x -o archive.%s lena.jpg\n", my_name, PJA_ARC_EXT );
	fprintf( MSGOUT, "          %s x -i archive.%s\n", my_name, PJA_SFX_EXT );
	#else
	fprintf( MSGOUT, "examples: %s x lena.jpg\n", my_name );
	fprintf( MSGOUT, "          %s l\n", my_name );
	#endif
	exit( 1 );
}
