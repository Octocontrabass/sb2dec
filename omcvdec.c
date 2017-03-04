#include <inttypes.h>
#include <png.h>
#include <stdlib.h>
#include <string.h>

uint_fast8_t checked_fget8( FILE * fileptr )
{
    int character = fgetc( fileptr );
    if( character == EOF )
    {
        fprintf( stderr, "Unexpected end of file!\n" );
        exit( EXIT_FAILURE );
    }
    return character & 0xff;
}

uint_fast16_t checked_fget16( FILE * fileptr )
{
    uint_fast16_t val = checked_fget8( fileptr ) << 8;
    val |= checked_fget8( fileptr );
    return val;
}

uint_fast32_t checked_fget32( FILE * fileptr )
{
    uint_fast32_t val = checked_fget16( fileptr ) << 16;
    val |= checked_fget16( fileptr );
    return val;
}

int main( const int argc, const char * const * const argv )
{
    if( argc != 3 )
    {
        fprintf( stderr, "Usage: omcvdec image image.png\n" );
        return argc > 1 ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    FILE * omtfile = NULL;
    FILE * pngfile = NULL;
    uint8_t fourcc[5] = {0};
    omtfile = fopen( argv[1], "rb" );
    if( !omtfile )
    {
        fprintf( stderr, "Can't read file: %s\n", argv[1] );
        return EXIT_FAILURE;
    }
    pngfile = fopen( argv[2], "wb" );
    if( !pngfile )
    {
        fprintf( stderr, "Can't write file: %s\n", argv[2] );
        return EXIT_FAILURE;
    }
    for( size_t i = 0; i < 4; i++ )
    {
        fourcc[i] = checked_fget8( omtfile );
    }
    if( memcmp( fourcc, "OmCv", 4 ) )
    {
        fprintf( stderr, "Unrecognized file header: %s\n", fourcc );
        return EXIT_FAILURE;
    }
    fprintf( stdout, "%14s: 0x%08" PRIxFAST32 "\n", "Unknown", checked_fget32( omtfile ) );
    for( size_t i = 0; i < 4; i++ )
    {
        fourcc[i] = checked_fget8( omtfile );
    }
    if( memcmp( fourcc, "OmGW", 4 ) )
    {
        fprintf( stderr, "Unrecognized image type: %s\n", fourcc );
        return EXIT_FAILURE;
    }
    uint_fast16_t width = checked_fget16( omtfile );
    fprintf( stdout, "%14s: %" PRIuFAST16 "\n", "Width", width );
    uint_fast16_t height = checked_fget16( omtfile );
    fprintf( stdout, "%14s: %" PRIuFAST16 "\n", "Height", height );
    uint_fast16_t omtbpp = checked_fget16( omtfile );
    fprintf( stdout, "%14s: %" PRIuFAST16 "\n", "Bits per pixel", omtbpp );
    uint_fast8_t compression = checked_fget8( omtfile );
    fprintf( stdout, "%14s: %" PRIuFAST8 "\n", "Compression", compression );
    uint_fast8_t version = checked_fget8( omtfile );
    fprintf( stdout, "%14s: %" PRIuFAST8 "\n", "Version", version );
    uint_fast16_t stride = checked_fget16( omtfile );
    fprintf( stdout, "%14s: %" PRIuFAST16 "\n", "Stride", stride );
    uint_fast8_t transparency = checked_fget8( omtfile );
    fprintf( stdout, "%14s: %s\n", "Transparent", transparency ? "Yes" : "No" );
    if( version == 0 || transparency )
    {
        fprintf( stderr, "Unsupported format!\n" );
        return EXIT_FAILURE;
    }
    uint_fast8_t paletted = checked_fget8( omtfile );
    fprintf( stdout, "%14s: %s\n", "Palette", paletted ? "Yes" : "No" );
    if( paletted )
    {
        fprintf( stderr, "Unsupported format!\n" );
        return EXIT_FAILURE;
    }
    
    
    png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    if( !png_ptr )
    {
        fprintf( stderr, "Failed to allocate PNG structure.\n" );
        return EXIT_FAILURE;
    }
    png_infop info_ptr = png_create_info_struct( png_ptr );
    if( !info_ptr )
    {
        fprintf( stderr, "Failed to allocate PNG header.\n" );
        return EXIT_FAILURE;
    }
    if( setjmp( png_jmpbuf( png_ptr ) ) )
    {
        fprintf( stderr, "Unspecified libpng error.\n" );
        return EXIT_FAILURE;
    }
    png_init_io( png_ptr, pngfile );
    png_set_IHDR( png_ptr, info_ptr, 1, 1, 8, PNG_COLOR_TYPE_GRAY,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT );
    png_write_info( png_ptr, info_ptr );
    png_write_row( png_ptr, (png_byte[]){0} );
    png_write_end( png_ptr, NULL );
    fclose( pngfile );
    return EXIT_SUCCESS;
}
