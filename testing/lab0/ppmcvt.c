#include "pbm.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"

// I'm not sure if you're meant to abstract the getopt() away from main,
// but I saw the pattern online so I figured it was best practice
typedef struct {
	char mode;
	char *arg;
	char *infile;
	char *outfile;
} Option;

Option parse_command(int argc, char *argv[]) {
        Option options = {0};
        options.mode = 'b';
        int option;
        // Make sure only one transform specified
        int transform_count = 0;
        // loop through arguments and case switch?
        while ((option = getopt(argc, argv, "bg:i:r:smt:n:o:")) != -1) {
                switch (option) {
                        case 'b': options.mode = 'b'; transform_count++; break;
                        case 'g': options.mode = 'g'; options.arg = optarg; transform_count++; break;
                        case 'i': options.mode = 'i'; options.arg = optarg; transform_count++; break;
                        case 'r': options.mode = 'r'; options.arg = optarg; transform_count++; break;

                        case 's': options.mode = 's'; transform_count++; break;
                        case 'm': options.mode = 'm'; transform_count++; break;

                        case 't': options.mode = 't'; options.arg = optarg; transform_count++; break;

                        case 'n': options.mode = 'n'; options.arg = optarg; transform_count++; break;
                        case 'o': options.outfile = optarg; break;
                        default:
                                fprintf(stderr, "Usage: ppmcvt [-bgirsmtno] [FILE]\n");
                                exit(1);
        		}
		}
        if (transform_count > 1) {
                fprintf(stderr, "Error: Multiple transformations specified\n");
		exit(1);
        }

        if (optind < argc) {
                options.infile = argv[optind];
        } else {
                fprintf(stderr, "Error: No input file specified\n");
                exit(1);
        }

        if (options.outfile == NULL) {
                fprintf(stderr, "Error: No output file specified\n");
                exit(1);
        }
        return options;
}


// Transformations
void convertToBitmap(char *infile, char *outfile) {
        PPMImage *ppm = read_ppmfile(infile);
        PBMImage *pbm = new_pbmimage(ppm->width, ppm->height);

        // Initialize new map
        for (unsigned int row = 0; row < ppm->height; row++) {
                for (unsigned int col = 0; col < ppm->width; col++) {
                        pbm->pixmap[row][col] = 0;
                }
        }

        // Sum values
        for (int c = 0; c < 3; c++) {
                for (unsigned int row = 0; row < ppm->height; row++) {
                        for (unsigned int col = 0; col < ppm->width; col++) {
                                unsigned int val = ppm->pixmap[c][row][col];
                                pbm->pixmap[row][col] += val;
                        }
                }
        }

        // Convert to black/white
        for (unsigned int row = 0; row < ppm->height; row++) {
                for (unsigned int col = 0; col < ppm->width; col++) {
                        unsigned int sum = pbm->pixmap[row][col];
                        float average = sum / 3.0;

                        if (average < ppm->max / 2.0) {
                                pbm->pixmap[row][col] = 1;
                        } else {
                                pbm->pixmap[row][col] = 0;
                        }
                }
        }

        write_pbmfile(pbm, outfile);
        del_pbmimage(pbm);
        del_ppmimage(ppm);
}
void grayscale(char *infile, char*outfile, unsigned int pgmMax) {
	PPMImage *ppm = read_ppmfile(infile);
	PGMImage *pgm = new_pgmimage(ppm->width, ppm->height, pgmMax);

	// Initialize new map
        for (unsigned int row = 0; row < ppm->height; row++) {
                for (unsigned int col = 0; col < ppm->width; col++) {
                        pgm->pixmap[row][col] = 0;
                }
        }

        // Sum values
        for (int c = 0; c < 3; c++) {
                for (unsigned int row = 0; row < ppm->height; row++) {
                        for (unsigned int col = 0; col < ppm->width; col++) {
                                unsigned int val = ppm->pixmap[c][row][col];
                                pgm->pixmap[row][col] += val;
                        }
                }
        }

	// Grayscale values
	for (unsigned int row = 0; row < ppm->height; row++) {
    		for (unsigned int col = 0; col < ppm->width; col++) {
        		unsigned int sum = pgm->pixmap[row][col];
        		float average = sum / 3.0;
       	 		pgm->pixmap[row][col] = (unsigned int)(average * pgm->max / ppm->max);  // ✅ Use the grayscale value!
	   	 }
	}
	write_pgmfile(pgm, outfile);
	del_pgmimage(pgm);
	del_ppmimage(ppm);
}


void isolate(char *infile, char *outfile, char* channel) {
	// Set all but specified channel to 0
	PPMImage *ppm = read_ppmfile(infile);
	PPMImage *output = new_ppmimage(ppm->width, ppm->height, ppm->max);

	// Determine channel
	int keepRed = strcmp(channel, "red") == 0;
	int keepGreen = strcmp(channel, "green") == 0;
	int keepBlue = strcmp(channel, "blue") == 0;

	if (!keepRed && !keepGreen && !keepBlue) {
		fprintf(stderr, "Error: Invalid channel specification: (%s); should be 'red', 'green', or 'blue'\n", channel);
		exit(1);
	}
	// Iterate and delete all other channel values
	for (unsigned int row = 0; row < ppm->height; row++) {
		for (unsigned int col = 0; col < ppm->width; col++) {
			if (keepRed) {
				output->pixmap[0][row][col] = ppm->pixmap[0][row][col];
				output->pixmap[1][row][col] = 0;
				output->pixmap[2][row][col] = 0;
			} else if (keepGreen) {
				output->pixmap[0][row][col] = 0;
				output->pixmap[1][row][col] = ppm->pixmap[1][row][col];
				output->pixmap[2][row][col] = 0;
			} else {
				output->pixmap[0][row][col] = 0;
				output->pixmap[1][row][col] = 0;
				output->pixmap[2][row][col] = ppm->pixmap[2][row][col];
			}
		}
	}
	write_ppmfile(output, outfile);
	del_ppmimage(ppm);
	del_ppmimage(output);
}

void removeColorChannel(char *infile, char *outfile, char* channel) {
	PPMImage *ppm = read_ppmfile(infile);
    	PPMImage *output = new_ppmimage(ppm->width, ppm->height, ppm->max);

    	int removeRed = strcmp(channel, "red") == 0;
    	int removeGreen = strcmp(channel, "green") == 0;
    	int removeBlue = strcmp(channel, "blue") == 0;

    	if (!removeRed && !removeGreen && !removeBlue) {
        	fprintf(stderr,
            	"Error: Invalid channel specification: (%s); should be 'red', 'green' or 'blue'\n", channel);
        	exit(1);
   	}

    	for (unsigned int row = 0; row < ppm->height; row++) {
        	for (unsigned int col = 0; col < ppm->width; col++) {
            		if (removeRed) {
                		output->pixmap[0][row][col] = 0;
                		output->pixmap[1][row][col] = ppm->pixmap[1][row][col];
                		output->pixmap[2][row][col] = ppm->pixmap[2][row][col];
    			} else if (removeGreen) {
                		output->pixmap[0][row][col] = ppm->pixmap[0][row][col];
                		output->pixmap[1][row][col] = 0;
                		output->pixmap[2][row][col] = ppm->pixmap[2][row][col];
            		} else { // removeBlue
                		output->pixmap[0][row][col] = ppm->pixmap[0][row][col];
                		output->pixmap[1][row][col] = ppm->pixmap[1][row][col];
                		output->pixmap[2][row][col] = 0;
            		}
        	}
    	}

    	write_ppmfile(output, outfile);
    	del_ppmimage(ppm);
    	del_ppmimage(output);
}

void sepia(char *infile, char *outfile) {
	PPMImage *ppm = read_ppmfile(infile);
	PPMImage *output = new_ppmimage(ppm->width, ppm->height, ppm->max);

	for (unsigned int row = 0; row < ppm->height; row++) {
		for (unsigned int col = 0; col < ppm->width; col++) {
			unsigned int oldR = ppm->pixmap[0][row][col];
			unsigned int oldG = ppm->pixmap[1][row][col];
			unsigned int oldB = ppm->pixmap[2][row][col];

			// Compute new values
			unsigned int newRed = (393*oldR + 769*oldG + 189*oldB) / 1000;
			unsigned int newGreen = (349*oldR + 686*oldG + 168*oldB) / 1000;
			unsigned int newBlue = (272*oldR + 534*oldG + 131*oldB) / 1000;

			// Ceiling
			if (newRed > output->max) {
				newRed = output->max;
			}
			if (newGreen > output->max) {
				newGreen = output->max;
			}
			if (newBlue > output->max) {
				newGreen = output->max;
			}
			// Assign new values
			output->pixmap[0][row][col] = newRed;
			output->pixmap[1][row][col] = newGreen;
			output->pixmap[2][row][col] = newBlue;
		}
	}
	write_ppmfile(output, outfile);
	del_ppmimage(ppm);
	del_ppmimage(output);
}

void mirror(char *infile, char *outfile) {
	PPMImage *ppm = read_ppmfile(infile);
	PPMImage *output = new_ppmimage(ppm->width, ppm->height, ppm->max);

	// left half stays the same and then just copy the values?
	for (unsigned int row = 0; row < ppm->height; row++) {
		for (unsigned int col = 0; col < ppm->width / 2; col++) {
			output->pixmap[0][row][col] = ppm->pixmap[0][row][col];
			output->pixmap[1][row][col] = ppm->pixmap[1][row][col];
			output->pixmap[2][row][col] = ppm->pixmap[2][row][col];
		}
	}

	for (unsigned int row = 0; row < ppm->height; row++) {
		for (unsigned int col = ppm->width/2; col < ppm->width; col++) {
			unsigned int mirror = ppm->width - col - 1;
			output->pixmap[0][row][col] = output->pixmap[0][row][mirror];
			output->pixmap[1][row][col] = output->pixmap[1][row][mirror];
			output->pixmap[2][row][col] = output->pixmap[2][row][mirror];
		}
	}
        write_ppmfile(output, outfile);
        del_ppmimage(ppm);
        del_ppmimage(output);
}

void thumbnail(char *infile, char *outfile, int n) {
	PPMImage *ppm = read_ppmfile(infile);
	int scaledHeight = ppm->height/n;
	int scaledWidth = ppm->width/n;
	PPMImage *output = new_ppmimage(scaledWidth, scaledHeight, ppm->max);

	for (unsigned int row = 0; row < scaledHeight; row++) {
		for (unsigned int col = 0; col < scaledWidth; col++) {
			output->pixmap[0][row][col] = ppm->pixmap[0][row*n][col*n];
			output->pixmap[1][row][col] = ppm->pixmap[1][row*n][col*n];
			output->pixmap[2][row][col] = ppm->pixmap[2][row*n][col*n];
		}
	}
	write_ppmfile(output, outfile);
	del_ppmimage(ppm);
	del_ppmimage(output);}

void nup(char *infile, char *outfile, int n) {
	PPMImage *ppm = read_ppmfile(infile);
	PPMImage *output = new_ppmimage(ppm->width, ppm->height, ppm->max);

	// if 12x12 and we now have 3x3, n = 4
	unsigned int scaleWidth = ppm->width/n; // 3
	unsigned int scaleHeight = ppm->height/n; // 3

	// This is kind of like that sudoku leetcode problem but with an arbitrary scale
	for (unsigned int row = 0; row < ppm->height; row++) {
		for (unsigned int col = 0; col < ppm->width; col++) {
			// At this point we know which grid we are meant to be in, and we
			// physically get there by scaling the tileRow and tileCol
			unsigned int inRow = (row % scaleHeight) * n;
			unsigned int inCol = (col % scaleWidth) * n;

			output->pixmap[0][row][col] = ppm->pixmap[0][inRow][inCol];
			output->pixmap[1][row][col] = ppm->pixmap[1][inRow][inCol];
			output->pixmap[2][row][col] = ppm->pixmap[2][inRow][inCol];
		}
	}

	write_ppmfile(output, outfile);
	del_ppmimage(ppm);
	del_ppmimage(output);
}




int main( int argc, char *argv[] ) 
{
	Option options = parse_command(argc, argv);
	switch (options.mode) {
		case 'b': convertToBitmap(options.infile, options.outfile); break;
		case 'g': {
			long pgmMax = strtol(options.arg, NULL, 10);
			if (pgmMax < 1 || pgmMax > 65535) {
    				fprintf(stderr, "Error: Invalid max grayscale pixel value: %s; must be less than 65,536\n", options.arg);
    				exit(1);
			}
			grayscale(options.infile, options.outfile,(unsigned int)pgmMax);
			break;
		}
		case 'i': isolate(options.infile, options.outfile, options.arg); break;
		case 'r': removeColorChannel(options.infile, options.outfile, options.arg); break;
		case 's': sepia(options.infile, options.outfile); break;
		case 'm': mirror(options.infile, options.outfile); break;
		case 't': {
			long scale = strtol(options.arg, NULL, 10);
			if (scale < 1 || scale > 8) {
				fprintf(stderr, "Error: Invalid scale factor: %d; must be 1-8\n", (int)scale);
				exit(1);
			}
			thumbnail(options.infile, options.outfile, (int)scale);
			break;
		}
		case 'n': {
			long scale = strtol(options.arg, NULL, 10);
			if (scale < 1 || scale > 8) {
				fprintf(stderr, "Error: Invalid scale factor: %d; must be 1-8\n", (int)scale);
				exit(1);
			}
			nup(options.infile, options.outfile, (int)scale);
			break;
		}
	}
	return 0;
}
