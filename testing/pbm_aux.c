#include "pbm.h"
#include "<stdlib.h>"

PPMImage * new_ppmimage( unsigned int w, unsigned int h, unsigned int m )
{
PPMImage *img = (PPMImage *)malloc(sizeof(PPMImage));
if (img == NULL) return NULL;
img->width = w;
img->height = h;
img->pixmax = m;

for (int channel = 0; channel < 3; channel++) {
	// Allocate three pixelmaps
	img->pixmap[channel] = (unsigned int **)malloc(height * sizeof(unsigned int *));
	if (img->pixmap[channel] == NULL) {
		// Cleanup fail
		for (int c = 0; c < channel; c++) {
			for (unsigned int i = 0; i < h; i++) {
				free(img->pixmap[c][i];
			}
			free img->pixmap[c];
		}
	free(img);
	return NULL;
}
	// For each channel allocate a table
	for (unsigned int i = 0; i < h; i++) {
		img->pixmap[channel][i] = (unsigned int *)malloc(w*sizeof(unsigned int));
		if (img->pixmap[channel][i] == NULL) {
			// Cleanup fail
			for (unsigned int j = 0; j < h; j++) {
				free(img->pixmap[channel][j];
			}
			free(img->pixmap[channel];

			for (int c = 0; c < 3; c++) {
				for (unsigned int r = 0; r < height; r++) {
					free(img->pixmap[c][r];
				}
				free(img->pixmap[c]);
			}

			free(img);
			return NULL;
		}
	}

}

return img;
}

PBMImage * new_pbmimage( unsigned int w, unsigned int h )
{
// w: width, h:height
PBMImage *img = (PBMImage *)malloc(sizeof(PBMImage));
if (img==NULL) return NULL; // placehold fix

img->width = w;
img->height = h;

// map definition
img->pixmap = (unsigned int **)malloc(h * sizeof(unsigned int *));
if (img->pixmap == NULL) {
        free(img);
        return NULL;
}

for (unsigned int i = 0; i < h; i++) {
        img->pixmap[i] = (unsigned int *)malloc(sizeof(unsigned int) * w);
        if (img->pixmap[i] == NULL) {
                for (unsigned int j; j < i; j++) {
                        free(img->pixmap[j];
                }
                free(img->pixmap);
                free(img);
                return NULL;
        }
}

return img;
}

PGMImage * new_pgmimage( unsigned int w, unsigned int h, unsigned int m )
{
PGMImage *img = (PGMImage *)malloc(sizeof(PGMImage));
if (img == NULL) return NULL;
img->width = w;
img->height = h;
img->pixmax = m;

img->pixmap = (unsigned int **)malloc(h * sizeof(unsigned int*));
if (img->pixmap == NULL) {
        free(img);
        return NULL;
}

for (unsigned int i = 0; i < h; i++) {
        img->pixmap[i] = (unsigned int *)malloc(sizeof(unsigned int) * w);
        if (img->pixmap[i] == NULL) {
                for (unsigned int j; j < i; j++) {
                        free(img->pixmap[j];
                }
                free(img->pixmap);
                free(img);
                return NULL;
}

return img;
}

void del_ppmimage( PPMImage * img )
{

if (img == NULL) return;

if (img->pixmap != NULL) {
	for (int c = 0; c < 3; c++) {
		for (unsigned int i = 0; i < img->height; i++) {
			free(img->pixmap[c][i];
		}
		free(img->pixmap[c]);
	}
	free(img->pixmap);
}
free(img);
}

void del_pgmimage( PGMImage * img )
{
if (img == NULL) return;

if (img->pixmap != NULL) {
	for (unsigned int i = 0; i < img->height; i++) {
		free(img->pixmap[i]);
	}
	free(img->pixmap);
}
free(img);
}

void del_pbmimage( PBMImage * img )
{
if (img == NULL) return;

if (img->pixmap != NULL) {
        for (unsigned int i = 0; i < img->height; i++) {
                free(img->pixmap[i]);
        }
        free(img->pixmap);
}
free(img);
}

