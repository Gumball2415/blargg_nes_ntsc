/* Simple shell used by demos. Uses SDL multimedia library. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "SDL.h"

/* Image loader */
typedef struct image_t
{
	unsigned char const* byte_pixels;/* 8-bit pixels */
	unsigned short const* rgb_16;    /* 16-bit pixels */
	int width;
	int height;
	int row_width; /* number of pixels to get to next row (may be greater than width) */
} image_t;
/* if no palette, loads as 16-bit RGB */
void load_bmp( image_t* out, const char* path, SDL_Color palette [256] );
void save_bmp( const char* path );
void init_window( int width, int height );
int read_input( void );
void lock_pixels( void );
void double_output_height( void );
void display_output( void );
void fatal_error( const char* str );

static unsigned char* output_pixels; /* 16-bit RGB */
static long output_pitch;
static double mouse_x, mouse_y; /* -1.0 to 1.0 */
static int mouse_moved;
static int key_pressed;
static int scanlines = 1;

/* implementation */

static SDL_Window* screen;
static SDL_Surface* surface;
static SDL_Texture* texture;
static SDL_Renderer* renderer;
static unsigned long next_time;

void fatal_error( const char* str )
{
	fprintf( stderr, "Error: %s\n", str );
	exit( EXIT_FAILURE );
}

void cmd_usage(void)
{
	fprintf(stdout, "usage:\nnes_ntsc\t[input.bmp] [output.bmp]\n");
	fprintf(stdout, "nes_ntsc\tinput.bmp output.bmp -c [-scanoff] [-mergeoff <burstphase>] [-hue <value>] [-saturation <value>] [-contrast <value>] [-brightness <value>] [-sharpness <value>] [-gamma <value>] [-resolution <value>] [-artifacts <value>] [-fringing <value>] [-bleed <value>] [-vidmode cvbs|svid|rgb|mono] [-sony] [-decodematrix <ir> <qr> <ig> <qg> <ib> <qb>]\n\n");
	fprintf(stdout, "\t[input.bmp]\tInput.bmp file. \"test.bmp\" by default.\n");
	fprintf(stdout, "\t[output.bmp]\tOutput .bmp file. \"filtered.bmp\" by default.\n");
	fprintf(stdout, "\t-c\t\tUses the following commandline parameters instead of opening a window:\n");
	fprintf(stdout, "\t[-scanoff]\tTurns off scanline effects. On by default.\n");
	fprintf(stdout, "\t[-mergeoff]\tDisables merging even and odd fields together to reduce flicker. Burst phase is 1 by default.\n");
	fprintf(stdout, "\t[-hue]\t\t<value> ranges from -1.00 (-180 deg) to 1.00 (+180 deg). 0 by default.\n");
	fprintf(stdout, "\t[-saturation]\t<value> ranges from -1.00 (grayscale) to 1.00 (oversaturated). 0 by default.\n");
	fprintf(stdout, "\t[-hue]\t\t<value> ranges from -1.00 to 1.00.. 0 by default.\n");
	fprintf(stdout, "\t[-contrast]\t<value> ranges from -1.00 (dark) to 1.00 (light). 0 by default.\n");
	fprintf(stdout, "\t[-brightness]\t<value> ranges from -1.00 (dark) to 1.00 (light). 0 by default.\n");
	fprintf(stdout, "\t[-sharpness]\t<value> ranges from -1.00 to 1.00. 0 by default.\n");
	fprintf(stdout, "\t[-gamma]\t<value> ranges from -1.00 to 1.00. 0 by default.\n");
	fprintf(stdout, "\t[-resolution]\tImage resolution.\n");
	fprintf(stdout, "\t[-artifacts]\tArtifacts caused by color changes.\n");
	fprintf(stdout, "\t[-fringing]\tColor artifacts caused by brightness changes.\n");
	fprintf(stdout, "\t[-bleed]\tColor bleed (color resolution reduction).\n");
	fprintf(stdout, "\t[-vidmode]\tSelects a video mode. \"cvbs\" by default.\n");
	fprintf(stdout, "\t[-sony]\t\tTurns on Sony decoder. Off by default.\n");
	fprintf(stdout, "\t[-decodematrix]\tSpecifies a custom decoding matrix. This overrides any decoder used.\n");
}

static void init_sdl_( void )
{
	static int initialized;
	if ( !initialized )
	{
		if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
			fatal_error( "SDL initialization failed" );
		atexit( SDL_Quit );
	}
}

void init_window( int width, int height )
{
	init_sdl_();

	screen = SDL_CreateWindow("NTSC Filter Demo",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height,
		0);
	renderer = SDL_CreateRenderer(screen, -1, 0);
	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // debug
	surface = SDL_CreateRGBSurface(0, width, height, 16, 0, 0, 0, 0);
	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGB565,
		SDL_TEXTUREACCESS_STREAMING,
		width, height);
	;
	if ( screen == NULL || renderer == NULL || surface == NULL || texture == NULL )
		fatal_error( "SDL initialization failed" );
}

int read_input( void )
{
	SDL_Event e;
	
	/* limit to 60 calls per second */
	unsigned long start = SDL_GetTicks();
	if ( start < next_time && next_time - start > 10 )
		SDL_Delay( next_time - start );
	while ( SDL_GetTicks() < next_time ) { }
	next_time = start + 1000 / 60;
	
	mouse_moved = 0;
	key_pressed = 0;
	
	while ( SDL_PollEvent( &e ) )
	{
		if ( e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_QUIT )
			return 0;
		
		if ( e.type == SDL_KEYDOWN )
		{
			if ( e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q )
				return 0;
			key_pressed = e.key.keysym.sym;
		}
		
		if ( e.type == SDL_MOUSEMOTION )
		{
			int x, y;
			SDL_GetMouseState( &x, &y );
			mouse_moved = 1;
			mouse_x = x / (double) (SDL_GetWindowSurface(screen)->w - 1) * 2 - 1;
			mouse_y = (1 - y / (double) (SDL_GetWindowSurface(screen)->h - 1)) * 2 - 1;
		}
	}
	return 1;
}

void lock_pixels( void )
{
	if ( SDL_LockSurface( surface ) < 0 )
		fatal_error( "Couldn't lock surface" );
	SDL_FillRect( surface, 0, 0 );
	output_pitch = surface->pitch;
	output_pixels = (unsigned char*) surface->pixels;
}

void double_output_height( void )
{
	int y;
	for ( y = surface->h / 2; --y >= 0; )
	{
		unsigned char const* in = output_pixels + y * output_pitch;
		unsigned char* out = output_pixels + y * 2 * output_pitch;
		int n;
		for ( n = surface->w; n; --n )
		{
			unsigned prev = *(unsigned short*) in;
			unsigned next = *(unsigned short*) (in + output_pitch);
			/* mix 16-bit rgb without losing low bits */
			unsigned mixed = prev + next + ((prev ^ next) & 0x0821);
			/* darken by 12% */
			*(unsigned short*) out = prev;
			if (scanlines)
				*(unsigned short*) (out + output_pitch) = (mixed >> 1) - (mixed >> 4 & 0x18E3);
			else
				*(unsigned short*)(out + output_pitch) = prev;
			in += 2;
			out += 2;
		}
	}
}

void display_output( void )
{
	SDL_UnlockSurface( surface );
	if (SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch) < 0
		|| SDL_RenderClear(renderer) < 0
		|| SDL_RenderCopy(renderer, texture, NULL, NULL) < 0)
		fatal_error("SDL renderer failed");
	SDL_RenderPresent(renderer);

}

void load_bmp( image_t* out, const char* path, SDL_Color palette [256] )
{	
	SDL_PixelFormat fmt = { 0 }; /* clear fields */
	SDL_Palette pal = { 0 };
	SDL_Surface* bmp;
	SDL_Surface* conv;
	
	init_sdl_();
	bmp = SDL_LoadBMP( path );
	if ( !bmp )
		fatal_error( "Couldn't load BMP" );
	
	fmt.BitsPerPixel  = 16;
	fmt.BytesPerPixel = 2;
	if ( palette )
	{
		pal.ncolors = 256;
		pal.colors = palette;
		fmt.palette = &pal;
		fmt.BitsPerPixel  = 8;
		fmt.BytesPerPixel = 1;
	}
	conv = SDL_ConvertSurface( bmp, &fmt, SDL_SWSURFACE );
	if ( !conv )
		fatal_error( "Couldn't convert BMP" );
	else {
		SDL_FreeSurface(bmp);

		if (SDL_LockSurface(conv) < 0)
			fatal_error("Couldn't lock surface");

		out->byte_pixels = (unsigned char*)conv->pixels;
		out->rgb_16 = (unsigned short*)conv->pixels;
		out->width = conv->w;
		out->height = conv->h;
		out->row_width = conv->pitch / fmt.BytesPerPixel;
	}
}

void save_bmp( const char* path )
{
	if ( SDL_SaveBMP( surface, path ) )
		fatal_error( "Couldn't save BMP" );
}
