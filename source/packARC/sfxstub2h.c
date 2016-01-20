/*	"sfxstub2h.c"
    Converts "sfxstub.exe" to .h-file, which can be included in "packarc.c"
    11-12-20  Se  (after "file2array.c" by Matthias Stirner)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROGNAME	"sfxstub2h"

#if defined WIN32
#define INFILE		"sfxstub.exe"
#define PLATFORM	"WIN32"
#else
#define INFILE		"sfxstub"
#define PLATFORM	"unknown"
#endif
#define OUTFILE		"sfxstub.h"

#define ARRAYNAME	"pjxstub"
#define CHARS_PER_LINE 16


int main( int argc, char** argv ) {

    FILE *fip, *fop;
    unsigned char cbyte;
    int i = 0;

    /* check input, open files */
    if ( argc != 1 ) {
        fprintf(stderr, "\nUsage: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fip = fopen( INFILE, "rb" );
    if ( fip == NULL ) {
        fprintf(stderr, "\n%s: Error opening input file \"%s\"\n", PROGNAME, INFILE);
        perror(PROGNAME);
        exit(EXIT_FAILURE);
    }

    fop = fopen( OUTFILE, "wt" );
    if ( fop == NULL ) {
        fprintf(stderr, "\n%s: Error opening output file \"%s\"\n", PROGNAME, OUTFILE);
        perror(PROGNAME);
        exit(EXIT_FAILURE);
    }

    /* write to output file */
    fprintf( fop, "/* '%s' converted from '%s' by '%s' */\n", OUTFILE, INFILE, PROGNAME );
    fprintf( fop, "/* for platform: '%s' on %s */\n\n", PLATFORM, __DATE__ );
    fprintf( fop, "unsigned char %s[] = {", ARRAYNAME );
    while ( fread( &cbyte, 1, 1, fip ) == 1 ) {
        if ( i++ % CHARS_PER_LINE == 0 ) fprintf( fop, "\n\t\"" );
        fprintf( fop, "\\x%02X", cbyte );
        if ( i   % CHARS_PER_LINE == 0 ) fprintf( fop, "\"" );
    }
    if ( i   % CHARS_PER_LINE != 0 ) fprintf( fop, "\"" );
    fprintf( fop, "\n};\n\n" );
    fprintf( fop, "unsigned int %s_size = sizeof(%s);\n", ARRAYNAME, ARRAYNAME );

    /* close files */
    fclose( fip );
    fclose( fop );

    fprintf(stderr, "%s-info: %d characters read and converted.", PROGNAME, i);

    exit(EXIT_SUCCESS);
}
