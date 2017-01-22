#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int checked_fgetc( FILE * fileptr )
{
    int character = fgetc( fileptr );
    if( character == EOF )
    {
        fprintf( stderr, "\nUnexpected end of file!\n" );
        exit( EXIT_FAILURE );
    }
    return character;
}

int main( const int argc, const char * const * const argv )
{
    if( argc != 3 )
    {
        fprintf( stderr, "Usage: sb2dec script.sb2 script.csv\n" );
        return argc > 1 ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    FILE * sb2file;
    FILE * txtfile;
    sb2file = fopen( argv[1], "rb" );
    if( !sb2file )
    {
        fprintf( stderr, "Can't read file: %s\n", argv[1] );
        return EXIT_FAILURE;
    }
    txtfile = fopen( argv[2], "wb" );
    if( !txtfile )
    {
        fprintf( stderr, "Can't write file: %s\n", argv[2] );
        return EXIT_FAILURE;
    }
    uint_fast16_t lines = 0;
    lines |= checked_fgetc( sb2file );
    lines |= checked_fgetc( sb2file ) << 8;
    fprintf( stdout, "Found %" PRIuFAST16 " lines...\n", lines );
    for( uint_fast16_t i = 0; i < lines; i++ )
    {
        uint_fast16_t length = 0;
        length |= checked_fgetc( sb2file );
        length |= checked_fgetc( sb2file ) << 8;
        fprintf( stdout, "%" PRIuFAST16 " ", length );
        for( uint_fast16_t j = 0; j < length; j++ )
        {
            fputc( checked_fgetc( sb2file ) ^ 0x30, txtfile );
        }
    }
    if( fgetc( sb2file ) != EOF )
    {
        fprintf( stderr, "\nUnexpected trailing data!\n" );
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
