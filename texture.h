#include "defines.h"

struct Texture
{
	unsigned char *raw_image = NULL;

	int width = 0;
	int height = 0;

	bool loadedTex = false;

	//get the color from the texture using the normal
	glm::dvec3 getColor(glm::dvec3 Nobj)
	{
		double u = atan2(Nobj.z, Nobj.x)/(2 * PI) + 0.5; //x
		double v = 0.5 - asin(Nobj.y)/PI; //y

		int x = floor(u * width);
		int y = floor(v * height);

		glm::dvec3 color;

		color.x = ((double)(raw_image[(y*width*3)+(x*3)+0]))/255.0;
		color.y = ((double)(raw_image[(y*width*3)+(x*3)+1]))/255.0;
		color.z = ((double)(raw_image[(y*width*3)+(x*3)+2]))/255.0;

		return color;
	}

	//modified from one of the libjpeg samples
	bool readJpegFile( char *filename )
	{
		struct jpeg_decompress_struct cinfo; //info about the image
		struct jpeg_error_mgr jerr; //errors
		
		JSAMPROW row_pointer[1]; //stores the scanline of the jpeg (one line of pixels)

		FILE *infile = fopen( filename, "rb" );
		unsigned long location = 0;

		if ( !infile )
		{
		    printf("Error opening jpeg file %s\n!", filename );
		    return false;
		}

		cinfo.err = jpeg_std_error( &jerr );
		jpeg_create_decompress( &cinfo );
		jpeg_stdio_src( &cinfo, infile );
		jpeg_read_header( &cinfo, TRUE ); //get image info

		width = cinfo.image_width; //get the width of the image
		height = cinfo.image_height; //get the height of the image

		//decompress jpeg
		jpeg_start_decompress( &cinfo );

		raw_image = (unsigned char *)malloc( cinfo.output_width * cinfo.output_height*cinfo.num_components );
		row_pointer[0] = (unsigned char *)malloc( cinfo.output_width * cinfo.num_components );
		
		while( cinfo.output_scanline < cinfo.image_height )
		{
		    jpeg_read_scanlines( &cinfo, row_pointer, 1 );
		    for(int i = 0; i < cinfo.image_width * cinfo.num_components; ++i) 
		        raw_image[location++] = row_pointer[0][i];
		}

		jpeg_finish_decompress( &cinfo ); //finish decompress
		jpeg_destroy_decompress( &cinfo );

		//free mem
		free( row_pointer[0] );
		fclose( infile );

		return true;
	}
};