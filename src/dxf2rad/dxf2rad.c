/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.


The MIT License (MIT)

Copyright (c) 1999-2016 Georg Mischler

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <time.h>
#include <stdio.h>
#include <float.h>
#include <errno.h>

#include "readdxf.h"
#include "convert.h"
#include "tables.h"


FILE *infp;
FILE *outf;

char Inputfile[MAXPATH], Outputfile[MAXPATH];

#define DXF2RAD_VER "1.1.0"
/* 2016-12-21 1.1.0    reorganize sources, VC 2015, 64 bit */
/* 2014-05-28 1.1.0b07 increase string lengths, better error checking */
/* 2005-07-21 1.1.0b06 VIEWS had width and height switched */
/* 2003-06-02 1.1.0b05 circles always had thickness 1.0  */
/* 2003-05-20 1.1.0b04 more fixes converting arcs  */
/* 2003-04-30 1.1.0b03 crashed converting arcs, shorten string literals */
/* 2003-03-13 1.1.0b02 longer input lines - truncate, some compile warnings */
/*            1.1.0b01 write view files  */
/*            1.0.2    fix lwpolyline along axis */

#ifdef _WIN32
#define DEGREE_CHAR "\xf8"
#else
#define DEGREE_CHAR "\xb0"
#endif


Options_Type Options = {
	0,    /*verbose  */
	1.0,  /* scale  */
	none, /* exportmode  */
	{     /* etype  */
		0,  /* et_NONE  */
		1,  /* et_3DFACE  */
		1,  /* et_SOLID  */
		1,  /* et_TRACE  */
		1,  /* et_PLINE  */
		1,  /* et_WPLINE  */
		0,  /* et_POLYGON  */
		1,  /* et_PMESH  */
		1,  /* et_PFACE  */
		1,  /* et_LINE  */
		1,  /* et_ARC  */
		1,  /* et_CIRCLE  */
		0,  /* et_POINT  */
		-1, /* et_TEXT */
#ifdef ACIS
		1,  /* et_3DSOLID  */
		1,  /* et_BODY  */
		1,  /* et_REGION  */
#endif /* ACIS */
	},
	0.1,  /* disttol  */
	15.0 * DEG2RAD, /* angtol  */
	1,    /* skipfrozen  */
	1,    /* geom  */
	NULL, /* prefix  */
	0,    /* prefixlen */
	0,    /* views  */
	NULL, /* viewprefix  */
	0,    /* viewprefixlen */
	0,    /* smooth  */
	0,    /* ignorepolxwidth */
	0,    /* ignorethickness */
};


/* ------------------------------------------------------------------------ */

char *EntityDescrs[et_LAST] = {
	"Nothing",
	"3D FACEs",
	"Extruded and Flat 2D SOLIDs",
	"Extruded and Flat TRACEs",
	"Extruded 2D PLINEs",
	"Wide 2D PLINEs",
	"Closed 2D PLINEs as Polygons",
	"3D MESHes",
	"POLYFACEs",
	"Extruded LINEs",
	"Extruded ARCs",
	"Extruded and Flat CIRCLEs",
	"POINTs as Spheres",
	"TEXT",
#ifdef ACIS
	"ACIS 3D Solids",
	"ACIS Bodies",
	"ACIS 2D Regions",
#endif
};

/* Interface to getopt(): */
extern int optind;
extern char *optarg;
extern char optsign;
extern int dxf2rad_getopt( int argc, char *const *argv, const char *optstring );


void show_copyright(void)
{
	fprintf(stderr, "\n\
   dxf2rad " DXF2RAD_VER "\n\
   Copyright 2000-2016 Georg Mischler, Munich, Germany\n");
	fprintf(stderr, "\nThe MIT License (MIT)\n\n\
Permission is hereby granted, free of charge, to any person obtaining a copy\n\
of this software and associated documentation files (the \"Software\"), to deal\n\
in the Software without restriction, including without limitation the rights\n\
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n\
copies of the Software, and to permit persons to whom the Software is\n\
furnished to do so, subject to the following conditions:\n");
	fprintf(stderr, "\n\
The above copyright notice and this permission notice shall be included in all\n\
copies or substantial portions of the Software.\n");
	fprintf(stderr, "\n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n\
SOFTWARE.\n\n");
	exit(0);
}


void exit_with_usage(int exitval)
{
	int i;
	const char *optfmt = "  %-9s %s\n";
	const char *opts[][2] = {
		{"-h",       "help (show this text and exit)"},
		{"-H",       "display copyright and license and exit"},
		{"+g/-g",    "do/don't export geometry data (default +g)"},
		{"-G prefix","geometry modifier prefix (default \"l_\")"},
	/*	{ "-l        "export by layer [excludes -c] (default)"}, */
	/*	{ "-c        "export by color [excludes -l]"},  */
	/*	{ "-f        "don't export frozen/off layers"},  */
		{"-a atol",  "angle tolerance for arc subdivision (default 15.0"
			DEGREE_CHAR ")"},
		{"-d dtol",  "distance tolerance for arc subdivision (default 0.1)"},
		{"+v/-v",    "do/don't export views (default -v)"},
		{"-V prefix","view file prefix (default \"<radfile>_\")"},
		{"-r",       "report progress (repeat for verbosity)"},
		{"-s scale", "multiply all dimensions with scale"},
		{"-ennn",    "exclude entity types"},
		{"+ennn",    "include entity types"},
		{"",         "Where each 'n' is one out of (with defaults):"},
	};

	fprintf(stderr, "\n"
			"Usage:  dxf2rad [<options>] <dxffile> [<radfile>]\n"
			"\n"
			"Where options may include:\n");
	for (i = 0; i < sizeof(opts)/sizeof(opts[0]); i++) {
		fprintf(stderr, optfmt, opts[i][0], opts[i][1]);
	}
	for(i = 1; i < et_LAST; i++) {
		if (Options.etypes[i] > -0) {
			fprintf(stderr, "            %c  %c %s\n", i + 'a' - 1,
					Options.etypes[i] ? '+' : '-', EntityDescrs[i]);
		}
	}
fprintf(stderr, "\nIf <radfile> is omitted or is the single character '-',\n");
fprintf(stderr, "then the geometry output will go to stdout, in which case\n");
fprintf(stderr, "progress reporting is suppressed.\n");
	exit(exitval);
}


void parse_entarg(void)
{
	size_t i, pos;
	size_t arglen;

	arglen = strlen(optarg);
	for(i = 0; i < arglen; i++) {
		pos = optarg[i] - 'a' + 1;
		if(pos < 1 || pos > et_LAST || Options.etypes[pos] == -1) {
			fprintf(stderr,
					"Unknown entity type code '%c' in option '%ce %s'\n",
					optarg[i], optsign, optarg);
			exit_with_usage(-1);
		}
		if(optsign == '-') Options.etypes[pos] = 0;
		else if(optsign == '+') Options.etypes[pos] = 1;
	}
}

void disallow_plus(int arg)
{
	if(optsign == '+') {
		fprintf(stderr, "Unknown option '%c%c'\n", optsign, arg);
		exit_with_usage(-1);
	}
}

void parseoptions(int argc, char*argv[])
{
	int c;
	double dval;
	char *endptr;

	while((c = dxf2rad_getopt(argc, argv, "HhglcfrvV:s:e:d:a:f:G:")) != EOF) {
		switch(c) {
		case 'e':
			parse_entarg();
			break;
		case 'h':
			disallow_plus(c);
			exit_with_usage(0);
			break;
		case 'H':
			disallow_plus(c);
			show_copyright();
			break;
		case 'f':
			disallow_plus(c);
			Options.skipfrozen++;
			break;
		case 'r':
			disallow_plus(c);
			Options.verbose++;
			break;
		case 'l':
			disallow_plus(c);
			if(Options.exportmode == bycolor) {
				fprintf(stderr, "Can't use both export modes together\n");
				exit_with_usage(-1);
			}
			Options.exportmode = bylayer;
			break;
		case 'c':
			disallow_plus(c);
			if(Options.exportmode == bylayer) {
				fprintf(stderr, "Can't use both export modes together\n");
				exit_with_usage(-1);
			}
			Options.exportmode = bycolor;
			break;
		case 'G':
			disallow_plus(c);
			if(Options.prefix) {
				fprintf(stderr, "Modifier prefix specified more than once\n");
				exit_with_usage(-1);
			}
			Options.prefixlen = strlen(optarg);
			if((Options.prefixlen + 32) >= MAXSTRING) {
				fprintf(stderr, "Modifier prefix too long\n");
				exit_with_usage(-1);
			}
			Options.prefix = malloc(Options.prefixlen+1);
			strncpy(Options.prefix, optarg, Options.prefixlen+1);
			break;
		case 'g':
			if(optsign == '-') Options.geom = 0;
			else Options.geom = 1;
			break;
		case 'v':
			if(optsign == '-') Options.views = 0;
			else Options.views = 1;
			break;
		case 'V':
			disallow_plus(c);
			if(Options.viewprefix) {
				fprintf(stderr, "View prefix specified more than once\n");
				exit_with_usage(-1);
			}
			Options.viewprefixlen = strlen(optarg);
			if((Options.viewprefixlen + 32) > MAXSTRING) {
				fprintf(stderr, "View prefix too long\n");
				exit_with_usage(-1);
			}
			Options.viewprefix = malloc(Options.viewprefixlen+1);
			strncpy(Options.viewprefix, optarg, Options.viewprefixlen+1);
			break;
		case 's':
			disallow_plus(c);
			dval = strtod((const char*)optarg, &endptr);
			if(dval == 0.0) {
				fprintf(stderr, "Invalid or zero scale factor: \"%s\"\n",
						optarg);
				exit_with_usage(-1);
			} else if(dval == DBL_MAX || dval == DBL_MIN) {
				fprintf(stderr, "Invalid scale factor: \"%s\"\n", optarg);
				exit_with_usage(-1);
			} else if(*endptr != '\0') {
				fprintf(stderr, "Invalid scale factor: \"%s\"\n", optarg);
				exit_with_usage(-1);
			}
			Options.scale = dval;
			break;
		case 'd':
			disallow_plus(c);
			dval = strtod((const char*)optarg, &endptr);
			if(dval == 0.0) {
				fprintf(stderr, "Invalid or zero distance tolerance: \"%s\"\n",
						optarg);
				exit_with_usage(-1);
			} else if(dval == DBL_MAX || dval == DBL_MIN) {
				fprintf(stderr, "Invalid distance tolerance: \"%s\"\n", optarg);
				exit_with_usage(-1);
			} else if(*endptr != '\0') {
				fprintf(stderr, "Invalid distance tolerance: \"%s\"\n", optarg);
				exit_with_usage(-1);
			}
			Options.disttol = dval;
			break;
		case 'a':
			disallow_plus(c);
			dval = strtod((const char*)optarg, &endptr);
			if(dval == 0.0) {
				fprintf(stderr, "Invalid or zero angle tolerance: \"%s\"\n",
						optarg);
				exit_with_usage(-1);
			} else if(dval == DBL_MAX || dval == DBL_MIN) {
				fprintf(stderr, "Invalid angle tolerance: \"%s\"\n", optarg);
				exit_with_usage(-1);
			} else if(*endptr != '\0') {
				fprintf(stderr, "Invalid angle tolerance: \"%s\"\n", optarg);
				exit_with_usage(-1);
			}
			Options.angtol = dval * DEG2RAD;
			break;
		}
	}
	if(Options.geom == 0 && Options.views == 0) {
		fprintf(stderr, "No output specified, nothing to do\n");
		exit_with_usage(-1);
	}
	if(Options.exportmode == none) {
		Options.exportmode = bylayer;
	}
	if(Options.prefix == NULL) {
		if(Options.exportmode == bylayer) {
			Options.prefix = "l_";
			Options.prefixlen = 2;
		}
		if(Options.exportmode == bycolor) {
			Options.prefix = "c_";
			Options.prefixlen = 2;
		}
	}
	/* Get file names and add extensions if necessary.  */
	if((argc - optind) > 2) {
		fprintf(stderr, "Too many arguments\n");
		exit_with_usage(-1);
	}
	if((argc - optind) < 1) {
		fprintf(stderr, "Missing input file argument\n");
		exit_with_usage(-1);
	}
	if ((strlen(argv[optind]) + 5) > MAXPATH){
		fprintf(stderr, "Input file path name too long\n");
		exit_with_usage(-1);
	}
	strncpy(Inputfile, argv[optind], MAXPATH);
	if (strcspn(Inputfile,".") == strlen(Inputfile))
		strncat(Inputfile, ".dxf", 5);
	if((argc - optind) < 2
		|| (argv[optind+1][0] == '-' && argv[optind+1][1] == '\0')) {
		if(Options.geom == 1) {
			Options.verbose = 0;
		}
		Outputfile[0] = '\0';
	} else {
		if ((strlen(argv[optind+1]) + 5) > MAXPATH){
			fprintf(stderr, "Output file path name too long\n");
			exit_with_usage(-1);
		}
		strncpy(Outputfile, argv[optind+1], MAXPATH);
		if (strcspn(Outputfile,".") == strlen(Outputfile))
			strncat(Outputfile, ".rad", 4);
	}
	if(Options.viewprefix == NULL) {
		size_t pnlen;
		char *pn;
		pnlen = strlen(Outputfile[0]?Outputfile:Inputfile);
		Options.viewprefix = malloc(pnlen+2);
		strncpy(Options.viewprefix, Outputfile[0]?Outputfile:Inputfile, pnlen+1);
		pn = strrchr(Options.viewprefix, '.');
		if(pn != NULL) *pn = '\0';
		strncat(pn, "_", 2);
		Options.viewprefixlen = strlen(Options.viewprefix);
	}
}


int main (int argc, char *argv[])
{
	int status = 0;
	static char eoferrmsg[] =
		"Unexpected end of file in %s section of file \"%s\" (line %d)\n";

	parseoptions(argc, argv);
	InitTables();
	InitConvert();

	errno = 0;
	infp = fopen(Inputfile, "r");
	if(!infp) {
		fprintf(stderr, "Can't open file '%s' for input (E%d: %s)\n",
			Inputfile, errno, strerror(errno));
		exit(-1);
	}
	/* Initialise group  */
	Group.line = 0;

	/* Read DXF file and write Radiance data.  */
	next_group(infp, &Group);
	while (!feof(infp) && strcmp(Group.value,FILEEND) != 0) {
		if(Group.code == 0) {
			if(strcmp(Group.value, SECTION) == 0) {
				next_group(infp, &Group); /* code 2 group */
				if(strcmp(Group.value,HEADER) == 0) {
					if(Options.verbose > 0) {
						fprintf(stderr, "  Reading headers\n");
					}
					HeaderSection();
					if(feof(infp)) {
						fprintf(stderr, eoferrmsg,
								"HEADER", Inputfile, Group.line);
						status = -1;
					}
				}
				else if(strcmp(Group.value,CLASSES) == 0) {
					if(Options.verbose > 0) {
						fprintf(stderr, "  Ignoring classes\n");
					}
					IgnoreSection();
					if(feof(infp)) {
						fprintf(stderr, eoferrmsg,
								"CLASSES", Inputfile, Group.line);
						status = -1;
					}
				}
				else if(strcmp(Group.value,TABLES) == 0) {
					if(Options.verbose > 0) {
						fprintf(stderr, "  Reading tables\n");
					}
					TablesSection();
					if(feof(infp)) {
						fprintf(stderr, eoferrmsg,
								"TABLES", Inputfile, Group.line);
						status = -1;
					}
				}
				else if(strcmp(Group.value,BLOCKS) == 0) {
					if(Options.verbose > 0) {
						fprintf(stderr, "  Reading blocks\n");
					}
					BlocksSection();
					if(feof(infp)) {
						fprintf(stderr, eoferrmsg,
								"BLOCKS", Inputfile, Group.line);
						status = -1;
					}
				}
				else if(strcmp(Group.value,ENTITIES) == 0) {
					if(Options.geom) {
						int i;
						time_t ltime;
						if(*Outputfile == '\0') {
							outf = stdout;
						} else {
							errno = 0;
							outf = fopen((const char*)&Outputfile, "w");
							if(outf == NULL) {
								fprintf(stderr,
										"Can't open file '%s' for output (E%d: %s)\n",
										Outputfile, errno, strerror(errno));
								exit(1);
							}
						}
						(void)time(&ltime);
						fprintf(outf, "## Radiance geometry file \"%s\"\n",
								Outputfile[0] ? Outputfile : "<stdout>");
						fprintf(outf, "## Converted by dxf2rad %s: %s##",
								DXF2RAD_VER, ctime(&ltime));
						for(i = 0; i < argc; i ++) {
							fprintf(outf, " %s", argv[i]);
						}
						fprintf(outf, "\n\n");
						if(Options.verbose > 0) {
							fprintf(stderr, "  Reading entities\n");
						}
						EntitiesSection();
						if(feof(infp)) {
							fprintf(stderr, eoferrmsg,
									"ENTITIES", Inputfile, Group.line);
							status = -1;
						}
					} else {
						if(Options.verbose > 0) {
							fprintf(stderr, "  Ignoring entities\n");
						}
						IgnoreSection();
						if(feof(infp)) {
							fprintf(stderr, eoferrmsg,
									"ENTITIES", Inputfile, Group.line);
							status = -1;
						}
					}
				}
				else if(strcmp(Group.value,OBJECTS) == 0) {
					if(Options.verbose > 0) {
						fprintf(stderr, "  Ignoring objects\n");
					}
					IgnoreSection();
					if(feof(infp)) {
						fprintf(stderr, eoferrmsg,
								"OBJECTS", Inputfile, Group.line);
						status = -1;
					}
				}
			}
		}
		next_group(infp, &Group);
	}

	if(outf) {
		if (status == 0) {
			fprintf(outf, "\n## End of Radiance geometry file \"%s\"\n\n",
					Outputfile[0] ? Outputfile : "<stdout>");
		}
		fclose(outf);
	}
	fclose(infp);
	return status;
}
