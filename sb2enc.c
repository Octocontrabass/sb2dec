#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main( const int argc, const char * const * const argv )
{
    if( argc != 3 )
    {
        fprintf( stderr, "Usage: sb2enc script.csv script.sb2\n" );
        return argc > 1 ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    FILE * txtfile = NULL;
    FILE * sb2file = NULL;
    uint8_t * txt = NULL;
    txtfile = fopen( argv[1], "rb" );
    if( !txtfile )
    {
        fprintf( stderr, "Can't read file: %s\n", argv[1] );
        return EXIT_FAILURE;
    }
    sb2file = fopen( argv[2], "wb" );
    if( !sb2file )
    {
        fprintf( stderr, "Can't write file: %s\n", argv[2] );
        return EXIT_FAILURE;
    }
    fseek( txtfile, 0, SEEK_END );
    long int txtsize = ftell( txtfile );
    fseek( txtfile, 0, SEEK_SET );
    if( txtsize < 0 )
    {
        fprintf( stderr, "Can't read file: %s\n", argv[1] );
        return EXIT_FAILURE;
    }
    if( (unsigned long int)txtsize > SIZE_MAX )
    {
        fprintf( stderr, "I'm pretty sure you don't want to do that.\n" );
        return EXIT_FAILURE;
    }
    if( txtsize )
    {
        txt = malloc( txtsize );
        if( !txt )
        {
            fprintf( stderr, "Memory allocation failed!\n" );
            return EXIT_FAILURE;
        }
        if( !fread( txt, txtsize, 1, txtfile ) )
        {
            fprintf( stderr, "Can't read file: %s\n", argv[1] );
            return EXIT_FAILURE;
        }
    }
    uint_fast16_t lines = 0;
    uint_fast16_t length = 0;
    uint_least16_t * list = NULL;
    for( long int i = 0; i < txtsize; i++ )
    {
        if( txt[i] == 0x0d )
        {
            continue;
        }
        if( length == 0xffff )
        {
            fprintf( stderr, "\nLine %" PRIuFAST32 " too long!\n", ((uint_fast32_t)length) + 1 );
            return EXIT_FAILURE;
        }
        length++;
        if( txt[i] == 0x0a )
        {
            if( lines == 0xffff )
            {
                fprintf( stderr, "\nToo many lines!\n" );
                return EXIT_FAILURE;
            }
            fprintf( stdout, "%" PRIuFAST16 " ", length );
            lines++;
            list = realloc( list, lines * sizeof( uint_least16_t ) );
            if( !list )
            {
                fprintf( stderr, "\nMemory allocation failed!\n" );
                return EXIT_FAILURE;
            }
            list[lines - 1] = length;
            length = 0;
        }
    }
    if( length != 0 )
    {
        fprintf( stderr, "\nUnexpected trailing data!\n" );
        return EXIT_FAILURE;
    }
    fprintf( stdout, "\nFound %" PRIuFAST16 " lines...\n", lines );
    fputc( lines, sb2file );
    fputc( lines >> 8, sb2file );
    long int i = 0;
    for( uint_fast16_t j = 0; j < lines; j++ )
    {
        length = list[j];
        fputc( length, sb2file );
        fputc( length >> 8, sb2file );
        for( uint_fast16_t k = 0; k < length; k++ )
        {
            while( txt[i] == 0x0d )
            {
                i++;
            }
            fputc( txt[i] ^ 0x30, sb2file );
            i++;
        }
    }
    return EXIT_SUCCESS;
}
