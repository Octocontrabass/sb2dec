#include <inttypes.h>
#include <png.h>
#include <stdlib.h>
#include <string.h>

uint_fast8_t checked_fget8( FILE * fileptr )
{
    int character = fgetc( fileptr );
    if( character == EOF )
    {
        fprintf( stderr, "\nUnexpected end of file!\n" );
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

uint_fast16_t checked_fget16le( FILE * fileptr )
{
    uint_fast16_t val = checked_fget8( fileptr );
    val |= checked_fget8( fileptr ) << 8;
    return val;
}

uint_fast32_t checked_fget32le( FILE * fileptr )
{
    uint_fast32_t val = checked_fget16le( fileptr );
    val |= checked_fget16le( fileptr ) << 16;
    return val;
}

int main( const int argc, const char * const * const argv )
{
    if( argc < 2 || argc > 3 )
    {
        fprintf( stderr, "Usage: omcvdec image [image.png]\n" );
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
    char * name;
    if( argc == 2 )
    {
        name = malloc( strlen( argv[1] ) + strlen( ".png" ) + 1 );
        if( !name )
        {
            fprintf( stderr, "Failed to allocate memory.\n" );
            return EXIT_FAILURE;
        }
        strcpy( name, argv[1] );
        strcat( name, ".png" );
    }
    else
    {
        name = (char *)argv[2];
    }
    pngfile = fopen( name, "wb" );
    if( !pngfile )
    {
        fprintf( stderr, "Can't write file: %s\n", name );
        return EXIT_FAILURE;
    }
    if( argc == 2 )
    {
        free( name );
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
    fprintf( stdout, "%17s: 0x%08" PRIxFAST32 "\n", "Unknown", checked_fget32( omtfile ) );
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
    fprintf( stdout, "%17s: %" PRIuFAST16 "\n", "Width", width );
    uint_fast16_t height = checked_fget16( omtfile );
    fprintf( stdout, "%17s: %" PRIuFAST16 "\n", "Height", height );
    uint_fast16_t bpp = checked_fget16( omtfile );
    fprintf( stdout, "%17s: %" PRIuFAST16 "\n", "Bits per pixel", bpp );
    if( bpp != 8 && bpp != 16 && bpp != 32 )
    {
        fprintf( stderr, "Unsupported format!\n" );
        return EXIT_FAILURE;
    }
    uint_fast8_t compression = checked_fget8( omtfile );
    fprintf( stdout, "%17s: %" PRIuFAST8 "\n", "Compression", compression );
    if( !(compression == 0 && bpp == 8) && !(compression == 1 && bpp == 16) && !(compression == 2 && bpp == 32) )
    {
        fprintf( stderr, "Unsupported format!\n" );
        return EXIT_FAILURE;
    }
    uint_fast8_t version = checked_fget8( omtfile );
    fprintf( stdout, "%17s: %" PRIuFAST8 "\n", "Version", version );
    if( version != 1 )
    {
        fprintf( stderr, "Unsupported format!\n" );
        return EXIT_FAILURE;
    }
    uint_fast16_t stride = checked_fget16( omtfile );
    fprintf( stdout, "%17s: %" PRIuFAST16 "\n", "Stride", stride );
    if( stride != width * (bpp >> 3) )
    {
        fprintf( stderr, "Unsupported format!\n" );
        return EXIT_FAILURE;
    }
    uint_fast16_t transparency = checked_fget16( omtfile );
    fprintf( stdout, "%17s: %s\n", "Transparent", transparency ? "Yes" : "No" );
    if( transparency > 1 )
    {
        fprintf( stderr, "Unsupported format!\n" );
        return EXIT_FAILURE;
    }
    uint_fast32_t transparent = 0;
    if( transparency )
    {
        transparent = checked_fget32( omtfile );
        fprintf( stdout, "%17s: 0x%08" PRIxFAST32 "\n", "Transparent color", transparent );
    }
    uint_fast16_t paletted = checked_fget16( omtfile );
    fprintf( stdout, "%17s: %s\n", "Palette", paletted ? "Yes" : "No" );
    if( paletted > 1 || (paletted && bpp > 8) )
    {
        fprintf( stderr, "Unsupported format!\n" );
        return EXIT_FAILURE;
    }
    uint_fast32_t palsize = 0;
    png_colorp palette = NULL;
    png_bytep tpalette = NULL;
    if( paletted )
    {
        for( size_t i = 0; i < 4; i++ )
        {
            fourcc[i] = checked_fget8( omtfile );
        }
        if( memcmp( fourcc, "OPa2", 4 ) )
        {
            fprintf( stderr, "Unrecognized palette header: %s\n", fourcc );
            return EXIT_FAILURE;
        }
        if( checked_fget8( omtfile ) )
        {
            fprintf( stderr, "Unsupported format!\n" );
            return EXIT_FAILURE;
        }
        palsize = checked_fget32( omtfile );
        fprintf( stdout, "%17s: 0x%08" PRIxFAST32 "\n", "Palette size", palsize );
        if( palsize > 256 || (transparency && transparent > palsize) )
        {
            fprintf( stderr, "Unsupported format!\n" );
            return EXIT_FAILURE;
        }
        palette = malloc( (palsize > 2 ? palsize : 2) * sizeof(png_color) );
        if( !palette )
        {
            fprintf( stderr, "Failed to allocate palette.\n" );
            return EXIT_FAILURE;
        }
        palette[0].red = 0xff;
        palette[0].green = 0xff;
        palette[0].blue = 0xff;
        palette[1].red = 0x00;
        palette[1].green = 0x00;
        palette[1].blue = 0x00;
        for( size_t i = 0; i < palsize; i++ )
        {
            palette[i].red = checked_fget16( omtfile ) >> 8;
            palette[i].green = checked_fget16( omtfile ) >> 8;
            palette[i].blue = checked_fget16( omtfile ) >> 8;
        }
        fseek( omtfile, 4096, SEEK_CUR ); // whyyyy
        if( transparency )
        {
            tpalette = malloc( (palsize > 2 ? palsize : 2) * sizeof(png_byte) );
            if( !tpalette )
            {
                fprintf( stderr, "Failed to allocate palette.\n" );
                return EXIT_FAILURE;
            }
            tpalette[0] = 0xff;
            tpalette[1] = 0xff;
            for( size_t i = 0; i < palsize; i++ )
            {
                tpalette[i] = 0xff;
            }
            tpalette[transparent] = 0x00;
        }
    }
    fprintf( stdout, "%17s: %" PRIuFAST32 "\n", "Compressed size", checked_fget32( omtfile ) );
    
    
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
    
    if( bpp == 8 )
    {
        png_set_IHDR( png_ptr, info_ptr, width, height, bpp, PNG_COLOR_TYPE_PALETTE,
                      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                      PNG_FILTER_TYPE_DEFAULT );
        png_set_PLTE( png_ptr, info_ptr, palette, palsize > 2 ? palsize : 2 );
    }
    else if( bpp >= 16 )
    {
        png_set_IHDR( png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
                      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                      PNG_FILTER_TYPE_DEFAULT );
    }
    if( transparency && bpp == 8 )
    {
        png_set_tRNS( png_ptr, info_ptr, tpalette, palsize > 2 ? palsize : 2, NULL );
    }
    else if( transparency && bpp == 16 )
    {
        png_set_tRNS( png_ptr, info_ptr, NULL, 0, (png_color_16[]){{0,
                      ((transparent >> 7) & 0xf8) | ((transparent >> 12) & 0x07),
                      ((transparent >> 2) & 0xf8) | ((transparent >> 7) & 0x07),
                      ((transparent << 3) & 0xf8) | ((transparent >> 2) & 0x07),
                      0}});
    }
    else if( transparency && bpp == 32 )
    {
        png_set_tRNS( png_ptr, info_ptr, NULL, 0, (png_color_16[]){{0,
                      (transparent >> 16) & 0xff,
                      (transparent >> 8) & 0xff,
                      transparent & 0xff,
                      0}});
    }
    png_write_info( png_ptr, info_ptr );
    if( bpp == 8 )
    {
        uint_least8_t * omtrow = malloc( stride );
        png_bytep row = malloc( width * sizeof( png_byte ) );
        if( !omtrow || !row )
        {
            fprintf( stderr, "Failed to allocate memory.\n" );
            return EXIT_FAILURE;
        }
        
        for( size_t i = 0; i < height; i++ )
        {
            fprintf( stdout, "%" PRIuFAST16 " ", checked_fget16( omtfile ) );
            size_t j = 0;
            while( j < width )
            {
                int_fast8_t run = (int8_t)checked_fget8( omtfile );
                if( run < 0 )
                {
                    uint_fast8_t temp = checked_fget8( omtfile );
                    for( size_t k = -run + 1; k > 0; k-- )
                    {
                        omtrow[j] = temp;
                        j++;
                    }
                }
                else
                {
                    for( size_t k = run + 1; k > 0; k-- )
                    {
                        omtrow[j] = checked_fget8( omtfile );
                        j++;
                    }
                }
            }
            for( j = 0; j < width; j++ )
            {
                row[j] = omtrow[j];
            }
            png_write_row( png_ptr, row );
        }
    }
    else if( bpp == 16 )
    {
        uint_least16_t * omtrow = malloc( stride );
        png_bytep row = malloc( 3 * width * sizeof( png_byte ) );
        if( !omtrow || !row )
        {
            fprintf( stderr, "Failed to allocate memory.\n" );
            return EXIT_FAILURE;
        }
        
        for( size_t i = 0; i < height; i++ )
        {
            fprintf( stdout, "%" PRIuFAST16 " ", checked_fget16( omtfile ) );
            size_t j = 0;
            while( j < width )
            {
                int_fast8_t run = (int8_t)checked_fget16le( omtfile );
                if( run < 0 )
                {
                    uint_fast16_t temp = checked_fget16( omtfile );
                    for( size_t k = -run + 1; k > 0; k-- )
                    {
                        omtrow[j] = temp;
                        j++;
                    }
                }
                else
                {
                    for( size_t k = run + 1; k > 0; k-- )
                    {
                        omtrow[j] = checked_fget16( omtfile );
                        j++;
                    }
                }
            }
            for( j = 0; j < width; j++ )
            {
                row[j * 3] = ((omtrow[j] >> 7) & 0xf8) | ((omtrow[j] >> 12) & 0x07);
                row[j * 3 + 1] = ((omtrow[j] >> 2) & 0xf8) | ((omtrow[j] >> 7) & 0x07);
                row[j * 3 + 2] = ((omtrow[j] << 3) & 0xf8) | ((omtrow[j] >> 2) & 0x07);
            }
            png_write_row( png_ptr, row );
        }
    }
    else if( bpp == 32 )
    {
        uint_least32_t * omtrow = malloc( stride );
        png_bytep row = malloc( 3 * width * sizeof( png_byte ) );
        if( !omtrow || !row )
        {
            fprintf( stderr, "Failed to allocate memory.\n" );
            return EXIT_FAILURE;
        }
        
        for( size_t i = 0; i < height; i++ )
        {
            fprintf( stdout, "%" PRIuFAST16 " ", checked_fget16( omtfile ) );
            size_t j = 0;
            while( j < width )
            {
                int_fast8_t run = (int8_t)checked_fget32le( omtfile );
                if( run < 0 )
                {
                    uint_fast32_t temp = checked_fget32( omtfile );
                    for( size_t k = -run + 1; k > 0; k-- )
                    {
                        omtrow[j] = temp;
                        j++;
                    }
                }
                else
                {
                    for( size_t k = run + 1; k > 0; k-- )
                    {
                        omtrow[j] = checked_fget32( omtfile );
                        j++;
                    }
                }
            }
            for( j = 0; j < width; j++ )
            {
                row[j * 3] = (omtrow[j] >> 16) & 0xff;
                row[j * 3 + 1] = (omtrow[j] >> 8) & 0xff;
                row[j * 3 + 2] = omtrow[j] & 0xff;
            }
            png_write_row( png_ptr, row );
        }
    }
    
    png_write_end( png_ptr, NULL );
    fclose( pngfile );
    return EXIT_SUCCESS;
}
