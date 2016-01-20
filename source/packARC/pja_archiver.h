#include <time.h>
#include "lfs_supp.h"
#include "helpers.h"

// only suggestions!
#define PJA_ARC_EXT	"pja"
#if defined(_WIN32)
	#define PJA_SFX_EXT	"exe"
#else
	#define PJA_SFX_EXT	"sfx"
#endif


struct pja_file_info {
	// compressed file information
	char filename[ MAX_PATH_LENGTH + 1 ];
	char compression_method[ 3 + 1 ];
	unsigned int file_crc;
	off_t file_size_original;
	off_t file_size_compressed;
	float compression_ratio;
	time_t last_changed;
};

struct pja_archive_info {
	// archive file information
	char* filename;
	char archive_type;
	unsigned char archive_version;
	int num_files;
	pja_file_info** filelist;
	off_t file_size_archive;
	off_t file_size_extracted;
	float compression_ratio;
	time_t last_changed;
};

// functions for use by frontend
bool pja_open_archive( char* filename );
bool pja_update_archive( void );
bool pja_close_archive( void );
bool pja_add_file_to_archive( char* filename_int, char* filename_ext );
bool pja_extract_file_from_archive( char* filename_int, char* filename_ext );
bool pja_remove_file_from_archive( char* filename );
bool pja_test_file_in_archive( char* filename );
bool pja_convert_archive( void );
void pja_set_existing_files_handling( char setting );
void pja_set_check_extracted_crc( bool setting );
pja_archive_info* pja_get_archive_info( void );
char* pja_get_current_status( void );
char* pja_get_version_info( void );
char* pja_get_engine_info( void );
