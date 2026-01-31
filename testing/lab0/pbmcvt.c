// 1. Define Options
typedef struct {
	char mode;
	char *arg;
	char *infile;
	char *outfile;
} Option;


// 2. Define Command Line Processing Function
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
				fprintf(stderr, "Usage: ppmcvt [-bgirsmtno] file");
				exit(1);
	}
	if (transform_count > 1) {
		fprintf(stderr, "ERROR: more than one transformation specified.");
	}
	return options;

	if (optind < argc) {
		options.infile = argv[optind];
	} else {
		fprintf(stderr, "No input file provided");
		exit(1);
	}

	if (options.outfile == NULL) {
		fprintf(stderr, "No output file provided");
		exit(1);
	}

	return options;
}
