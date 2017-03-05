/*
 *  original file: Dustin Franklin (NVIDIA)
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

void read_png_file(char* file_name)
{
        char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        fread(header, 1, 8, fp);
        if (png_sig_cmp((png_bytep)header, 0, 8))
                abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);


        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);

        fclose(fp);
}


void write_png_file(char* file_name)
{
        /* create file */
        FILE *fp = fopen(file_name, "wb");
        if (!fp)
                abort_("[write_png_file] File %s could not be opened for writing", file_name);


        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[write_png_file] png_create_write_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[write_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during init_io");

        png_init_io(png_ptr, fp);


        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing header");

        png_set_IHDR(png_ptr, info_ptr, width, height,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing bytes");

        png_write_image(png_ptr, row_pointers);


        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during end of write");

        png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
        for (y=0; y<height; y++)
                free(row_pointers[y]);
        free(row_pointers);

        fclose(fp);
}

#include <QImage>

#define FORMAT_RGB24 0
#define FORMAT_INDEXED8 1

#define FORMAT FORMAT_INDEXED8

unsigned char palette[21][3] = {
	{ 0,0,0 },
	{ 111,74,0 },
	{ 81,0,81 },
	{ 128,64,128 },
	{ 244,35,232 },
	{ 250,170,160 },
	{ 70,70,70 },
	{ 102,102,156 },
	{ 190,153,153 },
	{ 180,165,180 },
	{ 150,100,100 },
	{ 153,153,153 },
	{ 250,170,30 },
	{ 220,220,0 },
	{ 107,142,35 },
	{ 152,251,152 },
	{ 70,130,180 },
	{ 220,20,60 },
	{ 0,0,142 },
	{ 0,0,70 },
	{ 119,11,32 },
};

void process_file(char* file_name)
{
        /*if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
                abort_("[process_file] input file is PNG_COLOR_TYPE_RGB but must be PNG_COLOR_TYPE_RGBA "
                       "(lacks the alpha channel)");*/

        /*if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA)
                abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)",
                       PNG_COLOR_TYPE_RGBA, png_get_color_type(png_ptr, info_ptr));*/

		if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGB)
                abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGB (%d) (is %d)",
                       PNG_COLOR_TYPE_RGB, png_get_color_type(png_ptr, info_ptr));

	printf("width %d   height %d   bit depth %d\n", width, height, bit_depth);

       QImage img;
       QRgb colors[255];
       int colors_count = 21;
       for (int i = 0; i < colors_count; i++) {
       	colors[i] = qRgb(palette[i][0], palette[i][1], palette[i][2]);
       }

        if (FORMAT == FORMAT_RGB24) {
		img = QImage(width, height, QImage::Format_RGB32);
	} else if (FORMAT == FORMAT_INDEXED8) {
		img = QImage(width, height, QImage::Format_Indexed8);
		for (int i = 0; i < colors_count; i++) img.setColor(i,colors[i]);
        }

        for (y=0; y<height; y++) {
                png_byte* row = row_pointers[y];
                for (x=0; x<width; x++) {
                        png_byte* ptr = &(row[x*3]);


#if 0
                       // 		printf("Pixel at position [ (x,y) = (%d,%d) ] has RGB values: [%02X%02X%02X] %d - %d - %d\n",
                       //        x, y, ptr[0], ptr[1], ptr[2], ptr[0], ptr[1], ptr[2]);

					const int classIdx = ptr[1];

				    if( classIdx < 0 || classIdx > 15 )
				    {
						printf("%d %d  has invalid class ID %d\n", x, y, ptr[1]);
						continue;
				    }

					int r = 0;	// if( classIdx == 0 || classIdx == 13 || classIdx == 14 )
					int g = 0;
					int b = 0;

					if( classIdx == 1 ) { r = 128; g = 128; b = 128; }
					else if( classIdx == 2 )	{ r = 128; g = 0; b = 0; }
					else if( classIdx == 3 )	{ r = 128; g = 64; b = 128; }
					else if( classIdx == 4 )	{ r = 0; g = 0; b = 192; }
					else if( classIdx == 5 )	{ r = 64; g = 64; b = 128; }
					else if( classIdx == 6 ) { r = 128; g = 128; b = 0; }
					else if( classIdx == 7 ) { r = 192; g = 192; b = 128; }
					else if( classIdx == 8 ) { r = 64; g = 0; b = 128; }
					else if( classIdx == 9 ) { r = 192; g = 128; b = 128; }
					else if( classIdx == 10 ) { r = 64; g = 64; b = 0; }
					else if( classIdx == 11 ) { r = 0; g = 128; b = 192; }
					else if( classIdx == 12 ) { r = 0; g = 172; b = 0; }
					else if( classIdx == 15 ) { r = 0; g = 128; b = 128; }

#endif

					int r = ptr[0];
					int g = ptr[1];
					int b = ptr[2];

					ptr[0] = r;
					ptr[1] = g;
					ptr[2] = b;

                                        if (FORMAT == FORMAT_INDEXED8) {
					     int col = -1;
					     for (int i = 0; i < colors_count; i++) {
					         if (colors[i] == qRgb(r,g,b)) {
						    col = i;
						    break;
						 }
					     }
					     if (col == -1) {
					      col = colors_count;
					      colors_count++;
					      colors[col] = qRgb(r,g,b);
					      img.setColor(col,colors[col]);
					     }
					     img.setPixel(x, y, col);
					} else {
					img.setPixel(x, y, qRgb(r,g,b));
					}
                }
        }

        if (FORMAT == FORMAT_INDEXED8) {
	    printf("Colors used in total: %d\n", colors_count); 
	}

	if( !img.save(file_name) )
		printf("failed to save output %s\n", file_name);
}


int main(int argc, char **argv)
{
        if (argc != 3)
                abort_("Usage: program_name <file_in> <file_out>");

        read_png_file(argv[1]);
        process_file(argv[2]);
        //write_png_file(argv[2]);

        return 0;
}

