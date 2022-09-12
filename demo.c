/* Displays and saves NTSC filtered image. Mouse controls sharpness and gamma.
Defaults to using "test.bmp" for input and "filtered.bmp" for output. Input
image must be an uncompressed BMP. Also writes "nes.pal" RGB color file on exit.

Usage: demo [in.bmp [out.bmp]]

Space   Toggle field merging
C       Composite video quality
S       S-video quality
R       RGB video quality
M       Monochrome video quality
D       Toggle between standard and Sony decoder matrix
*/

#include "nes_ntsc.h"

#include "demo_impl.h"

/* only used to convert input image to native palette format */
static SDL_Color palette [256] = {
	{102,102,102},{  0, 42,136},{ 20, 18,168},{ 59,  0,164},
	{ 92,  0,126},{110,  0, 64},{108,  7,  0},{ 87, 29,  0},
	{ 52, 53,  0},{ 12, 73,  0},{  0, 82,  0},{  0, 79,  8},
	{  0, 64, 78},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},
	{174,174,174},{ 21, 95,218},{ 66, 64,254},{118, 39,255},
	{161, 27,205},{184, 30,124},{181, 50, 32},{153, 79,  0},
	{108,110,  0},{ 56,135,  0},{ 13,148,  0},{  0,144, 50},
	{  0,124,142},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},
	{254,254,254},{100,176,254},{147,144,254},{199,119,254},
	{243,106,254},{254,110,205},{254,130,112},{235,159, 35},
	{189,191,  0},{137,217,  0},{ 93,229, 48},{ 69,225,130},
	{ 72,206,223},{ 79, 79, 79},{  0,  0,  0},{  0,  0,  0},
	{254,254,254},{193,224,254},{212,211,254},{233,200,254},
	{251,195,254},{254,197,235},{254,205,198},{247,217,166},
	{229,230,149},{208,240,151},{190,245,171},{180,243,205},
	{181,236,243},{184,184,184},{  0,  0,  0},{  0,  0,  0}
};

int main(int argc, char** argv)
{
	if (argc > 1 && strcmp(argv[1], "-h") == 0)
		cmd_usage();
	else {
		image_t image;
		int sony_decoder = 0;
		int merge_fields = 1;
		int burst_phase = 0;
		nes_ntsc_setup_t setup = nes_ntsc_composite;

		nes_ntsc_t* ntsc = (nes_ntsc_t*)malloc(sizeof(nes_ntsc_t));
		if (!ntsc)
			fatal_error("Out of memory");

		load_bmp(&image, (argc > 1 ? argv[1] : "test.bmp"), palette);
		init_window(NES_NTSC_OUT_WIDTH(image.width), image.height * 2);

		if (argc > 2 && strcmp(argv[3], "-c") == 0)
		{

			// handle commandline flags
			burst_phase = 1;
			float sharpness = 0.0;
			float gamma = 0.0;
			char videomode[] = "NTSC composite";
			float matrix[6] = { 0.956f, 0.621f, -0.272f, -0.647f, -1.105f, 1.702f };
			int i;
			for (i = 0; i < argc; i++)
			{
				if (strcmp(argv[i], "-scanoff") == 0)
					scanlines = 0;

				if (strcmp(argv[i], "-mergeoff") == 0) {
					merge_fields = 0;
					burst_phase = atoll(argv[i + 1]);
				}

				if (strcmp(argv[i], "-sharpness") == 0)
					sharpness = strtof(argv[i + 1], NULL);

				if (strcmp(argv[i], "-gamma") == 0)
					gamma = strtof(argv[i + 1], NULL);

				// how do I switch case with strings
				if (strcmp(argv[i], "-vidmode") == 0) {
					if (strcmp(argv[i + 1], "svid") == 0) {
						setup = nes_ntsc_svideo;
						strcpy(videomode, "S-Video");
					}
					else if (strcmp(argv[i + 1], "rgb") == 0) {
						setup = nes_ntsc_rgb;
						strcpy(videomode, "RGB");
					}
					else if (strcmp(argv[i + 1], "mono") == 0) {
						setup = nes_ntsc_monochrome;
						strcpy(videomode, "Monochrome");
					}
					else{
						strcpy(videomode, "NTSC composite");
					}
				}

				if (strcmp(argv[i], "-sony") == 0)
					sony_decoder = 1;
				else if (strcmp(argv[i], "-decodematrix") == 0) {
					sony_decoder = 0;
					matrix[0] = strtof(argv[i + 1], NULL);
					matrix[1] = strtof(argv[i + 2], NULL);
					matrix[2] = strtof(argv[i + 3], NULL);
					matrix[3] = strtof(argv[i + 4], NULL);
					matrix[4] = strtof(argv[i + 5], NULL);
					matrix[5] = strtof(argv[i + 6], NULL);
				}
			}
			setup.merge_fields = merge_fields;
			setup.sharpness = sharpness;
			setup.gamma = gamma;
			if (sony_decoder)
			{
				/* Sony CXA2025AS US */
				matrix[0] = 1.630;
				matrix[1] = 0.317;
				matrix[2] = -0.378;
				matrix[3] = -0.466;
				matrix[4] = -1.089;
				matrix[5] = 1.677;
			}
			setup.decoder_matrix = matrix;
			nes_ntsc_init(ntsc, &setup);
			
			lock_pixels();

			if (setup.merge_fields)
				burst_phase = 0;

			nes_ntsc_blit(ntsc, image.byte_pixels, image.row_width, burst_phase,
				image.width, image.height, output_pixels, output_pitch);

			double_output_height();
			display_output();

			printf("\tscanline enabled:\t%i\n", scanlines);
			printf("\tmerge fields enabled:\t%i\n", merge_fields);
			printf("\tsharpness:\t\t%f\n", sharpness);
			printf("\tgamma:\t\t\t%f\n", gamma);
			printf("\tvideo mode:\t\t%s\n", videomode);
			printf("\tsony decoder enabled:\t%i\n", sony_decoder);
			printf("\tdecoding matrix:\t%f, %f, %f, %f, %f, %f\n", matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
		}
		else
		{
			while (read_input())
			{
				lock_pixels();

				burst_phase ^= 1;
				if (setup.merge_fields)
					burst_phase = 0;

				nes_ntsc_blit(ntsc, image.byte_pixels, image.row_width, burst_phase,
					image.width, image.height, output_pixels, output_pitch);

				double_output_height();
				display_output();

				switch (key_pressed)
				{
				case ' ': merge_fields = !merge_fields; break;
				case 'a': scanlines = !scanlines; break;
				case 'c': setup = nes_ntsc_composite; break;
				case 's': setup = nes_ntsc_svideo; break;
				case 'r': setup = nes_ntsc_rgb; break;
				case 'm': setup = nes_ntsc_monochrome; break;
				case 'd': sony_decoder = !sony_decoder; break;
				}

				if (key_pressed || mouse_moved)
				{
					setup.merge_fields = merge_fields;

					/* available parameters: hue, saturation, contrast, brightness,
					sharpness, gamma, bleed, resolution, artifacts, fringing */
					setup.sharpness = mouse_x;
					setup.gamma = mouse_y;

					setup.decoder_matrix = 0;
					if (sony_decoder)
					{
						/* Sony CXA2025AS US */
						static float matrix[6] = { 1.630, 0.317, -0.378, -0.466, -1.089, 1.677 };
						setup.decoder_matrix = matrix;
					}

					nes_ntsc_init(ntsc, &setup);
					fprintf(stdout, " sharpness: %.2f, gamma: %.2f \r", mouse_x, mouse_y);
				}
			}
		}
		save_bmp(argc > 2 ? argv[2] : "filtered.bmp");
		free(ntsc);

		/* write standard 192-byte NES palette */
		{
			FILE* out = fopen("nes.pal", "wb");
			if (out)
			{
				unsigned char palette[nes_ntsc_palette_size * 3];
				setup.palette_out = palette;
				nes_ntsc_init(0, &setup);
				fwrite(palette, 192, 1, out);
			}
		}
	}
	return 0;
}