/* Next available MSG number is  66 */

/*    COLEXT.C
      Copyright (C) 1990-1992 by Autodesk, Inc.
         
      Permission to use, copy, modify, and distribute this software 
      for any purpose and without fee is hereby granted, provided 
      that the above copyright notice appears in all copies and that 
      both that copyright notice and this permission notice appear in 
      all supporting documentation.

      THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED
      WARRANTY.  ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR
      PURPOSE AND OF MERCHANTABILITY ARE HEREBY DISCLAIMED.


      DESCRIPTION:

	AutoCAD colour manipulation functions




	adcolor.c

	This file is part of radout/ddrad, modified 06. 1996 Georg Mischler.
	Please refer to the original colext.c for documentation.
	Changed some declarations to extern and undefined main/dispatch functions.
	All extern function now return int.
	Lisp callable functions have one argument struct resbuf *.

*/




#include   <stdio.h>
#include   <string.h>
#include   <ctype.h>
#include   <math.h>

#include "cadcompat.h"

/* get definitions for Boolean and ELEMENTS() */
#include   "adtools.h"


/* Special assertion handler for ADS applications. */
#define X_NDEBUG
#ifndef X_NDEBUG
#define assert(ex) {if (!(ex)){ads_printf( \
                    /*MSG1*/"COLEXT: Assertion (%s) failed: file \"%s\", \
                    line %d\n", /*MSG0*/" ex ",__FILE__,__LINE__); \
                    ads_abort(/*MSG2*/"Assertion failed.");}}
#else
#define assert(ex)
#endif


/*  Data types	*/

#define V	 (void)

/* Set point variable from three co-ordinates */

#define Spoint(pt, x, y, z)  pt[X] = (x);  pt[Y] = (y);  pt[Z] = (z)

/* Copy point  */

#define Cpoint(d, s)   d[X] = s[X];  d[Y] = s[Y];  d[Z] = s[Z]


/* Utility definitions */

#ifdef abs
#undef abs
#endif
#define abs(x)	    ((x)<0 ? -(x) : (x))
#ifdef min
#undef	min
#endif
#define min(a,b)    ((a)<(b) ? (a) : (b))
#ifdef max
#undef	max
#endif
#define max(a,b)    ((a)>(b) ? (a) : (b))

#ifndef M_E
#define M_E	2.7182818284590452354
#endif

#define Tbit(x)  (tok & (1L << ((int) (x))))  /* Test token bit set */
#define Tb(x)	 (1L << ((int) (x)))  /* Obtain bit to test */

/* AutoCAD standard color palette */

#define 	BLACK		0
#define 	RED		1
#define 	YELLOW		2
#define 	GREEN		3
#define 	CYAN		4
#define 	BLUE		5
#define 	MAGENTA 	6
#define 	WHITE		7

#define 	SAT		1.0

struct r_g_b {			      /* RGB colour description */
	ads_real red, green, blue;
};

/*  Colour naming system vocabulary definition.  The vocabulary
    is defined in this somewhat unusal fashion to facilitate
    translation to languages other than English. */

typedef enum {
	/* Chromatic colours */
	Red, Orange, Brown, Yellow, Green, Blue, Purple,

        /* "ish" forms of chromatic colours */
	Reddish, Orangish, Brownish, Yellowish, Greenish, Bluish, Purplish,

	/* Achromatic names */
	Gray, Black, White,

	/* Lightness specifications */
	Very, Dark, Medium, Light,

	/* Saturation specifications */
	Grayish, Moderate, Strong, Vivid,

	/* Punctuation */
	Hyphen, Period, Huh
       } colourvocab;


static struct {
	char *cname;
	colourvocab ccode;
} cvocab[] = {
        {/*MSG3*/"red", Red},
        {/*MSG4*/"orange", Orange},
        {/*MSG5*/"brown", Brown},
        {/*MSG6*/"yellow", Yellow},
        {/*MSG7*/"green", Green},
        {/*MSG8*/"blue", Blue},
        {/*MSG9*/"purple", Purple},

        {/*MSG10*/"reddish", Reddish},
        {/*MSG11*/"orangish", Orangish},
        {/*MSG12*/"brownish", Brownish},
        {/*MSG13*/"yellowish", Yellowish},
        {/*MSG14*/"greenish", Greenish},
        {/*MSG15*/"bluish", Bluish},
        {/*MSG16*/"purplish", Purplish},

        {/*MSG17*/"gray", Gray},
        {/*MSG18*/"grey", Gray},
        {/*MSG19*/"black", Black},
        {/*MSG20*/"white", White},

        {/*MSG21*/"very", Very},
        {/*MSG22*/"dark", Dark},
        {/*MSG23*/"medium", Medium},
        {/*MSG24*/"light", Light},

        {/*MSG25*/"grayish", Grayish},
        {/*MSG26*/"greyish", Grayish},
        {/*MSG27*/"moderate", Moderate},
        {/*MSG28*/"strong", Strong},
        {/*MSG29*/"vivid", Vivid}
       };


/* Table mapping generic hues to HSV hue indices. */

static struct {
	long cbit;
	int chue;
} colhue[] = {
	{Tb(Red),      0},	      /* red */
	{Tb(Orange),  30},	      /* orange */
	{Tb(Brown),  -30},	      /* brown */
	{Tb(Yellow),  60},	      /* yellow */
	{Tb(Green),  120},	      /* green */
	{Tb(Blue),   240},	      /* blue */
	{Tb(Purple), 300},	      /* purple */
	{0L,	     360}	      /* red (other incarnation) */
       };

/* Table mapping secondary hues to HSV hue indices. */

static struct {
	long cbit;
	int chue;
} ishhue[] = {
	{Tb(Reddish),	   0},	      /* reddish */
	{Tb(Orangish),	  30},	      /* orangish */
	{Tb(Brownish),	 -30},	      /* brownish */
	{Tb(Yellowish),   60},	      /* yellowish */
	{Tb(Greenish),	 120},	      /* greenish */
	{Tb(Bluish),	 240},	      /* bluish */
	{Tb(Purplish),	 300},	      /* purplish */
	{0L,		 360}	      /* reddish (other incarnation) */
       };

#define MAXTK	 10		      /* Maximum tokens in specification */
#define MAXTKS	 20		      /* Longest token in characters */

#define BROWNLIGHT  3		      /* Brown lightness:  Medium */
#define BROWNSAT    3		      /* Brown saturation: Strong */

/* Modal variables  */

static int defcnslit = 10000;	      /* Default lightness if none specified */
static int gamut = 256; 	      /* Colour gamut available */

/*  Local variables  */

static char *cnserr = NULL;	      /* Error message string */
static char cnserb[80]; 	      /* Error message edit buffer */
static char tokenb[MAXTKS];	      /* Token buffer */

/*  Forward functions  */

static void   hsv_rgb _((ads_real,ads_real,ads_real,ads_real *,ads_real *,ads_real *));
static void   rgb_hsv _((ads_real,ads_real,ads_real,ads_real *,ads_real *,ads_real *));
static void   rgb_hls _((ads_real,ads_real,ads_real,ads_real *,ads_real *,ads_real *));
static ads_real hlsval _((ads_real, ads_real, ads_real));
static void   hls_rgb _((ads_real,ads_real,ads_real,ads_real *,ads_real *,ads_real *));
static void   rgb_yiq _((ads_real,ads_real,ads_real,ads_real *,ads_real *,ads_real *));
static void   yiq_rgb _((ads_real,ads_real,ads_real,ads_real *,ads_real *,ads_real *));
static void   rgb_cmy _((ads_real,ads_real,ads_real,ads_real *,ads_real *,ads_real *));
static void   ctemp_rgb _((ads_real, ads_real *,ads_real *,ads_real *));
#ifdef NEEDED
static void   cmy_rgb _((ads_real,ads_real,ads_real,ads_real *,ads_real *,ads_real *));
#endif
static colourvocab token _((char **));
static Boolean cns_rgb _((char *, ads_real *, ads_real *, ads_real *));
static char	*cixname _((colourvocab));
static void	rgb_cns _((ads_real, ads_real, ads_real, char *));
static int	rgbacad _((ads_real, ads_real, ads_real));
static void	retrgb _((Boolean, ads_real, ads_real, ads_real));
static Boolean triple _((struct resbuf *, ads_real *, Boolean));
  
static Boolean acadcol _((struct resbuf *, struct r_g_b *));
static void	cmy _((struct resbuf *, Boolean));
static void	cns _((struct resbuf *, Boolean));
static void	ctemp _((struct resbuf * ,Boolean));
static void	hls _((struct resbuf *, Boolean));
static void	hsv _((struct resbuf *, Boolean));
static void	rgb _((struct resbuf *, Boolean));
static void	yiq _((struct resbuf *, Boolean));

extern void	acadrgb _((int, struct r_g_b *));

extern int	cnser _((struct resbuf *));

extern int	cmyac _((struct resbuf *));
extern int	ctempac _((struct resbuf *));
extern int	yiqac _((struct resbuf *));
extern int	hsvac _((struct resbuf *));
extern int	rgbac _((struct resbuf *));
extern int	hlsac _((struct resbuf *));
extern int	cnsac _((struct resbuf *));

extern int	cmyrgb _((struct resbuf *));
extern int	ctemprgb _((struct resbuf *));
extern int	yiqrgb _((struct resbuf *));
extern int	hsvrgb _((struct resbuf *));
extern int	hlsrgb _((struct resbuf *));
extern int	cnsrgb _((struct resbuf *));
extern int	torgb _((struct resbuf *));

extern int	tocmy _((struct resbuf *));
extern int	toyiq _((struct resbuf *));
extern int	tohsv _((struct resbuf *));
extern int	tohls _((struct resbuf *));
extern int	tocns _((struct resbuf *));

extern int	colset _((struct resbuf *));


/*  Command definition and dispatch table.  */


/*	***************************************************
	**						 **
	**	 Colour Interconversion Functions	 **
	**						 **
	***************************************************
*/

/*  HSV_RGB  --  Convert HSV colour specification to RGB  intensities.
		 Hue is specified as a	real  value  from  0  to  360,
		 Saturation  and  Intensity as reals from 0 to 1.  The
		 RGB components are returned as reals from 0 to 1.	*/

static void hsv_rgb(ads_real h, ads_real s, ads_real v,
					ads_real *r, ads_real *g, ads_real *b)

{
    int i;
    ads_real f, p, q, t;

    if (s == 0) {
	*r = *g = *b = v;
    } else {
	if (h == 360.0)
	    h = 0.0;
	h /= 60.0;

	i = (int) h;
	f = h - i;
	p = v * (1.0 - s);
	q = v * (1.0 - (s * f));
	t = v * (1.0 - (s * (1.0 - f)));
	assert(i >= 0 && i <= 5);
	switch (i) {

	case 0:
	    *r = v;
	    *g = t;
	    *b = p;
	    break;

	case 1:
	    *r = q;
	    *g = v;
	    *b = p;
	    break;

	case 2:
	    *r = p;
	    *g = v;
	    *b = t;
	    break;

	case 3:
	    *r = p;
	    *g = q;
	    *b = v;
	    break;

	case 4:
	    *r = t;
	    *g = p;
	    *b = v;
	    break;

	case 5:
	    *r = v;
	    *g = p;
	    *b = q;
	    break;
	}
    }
}

/*  RGB_HSV  --  Map R, G, B intensities in the range from 0 to 1 into
		 Hue, Saturation,  and	Value:	Hue  from  0  to  360,
		 Saturation  from  0  to  1,  and  Value  from 0 to 1.
                 Special case: if Saturation is 0 (it's a  grey  scale
		 tone), Hue is undefined and is returned as -1.

		 This follows Foley & van Dam, section 17.4.4.	*/

static void rgb_hsv(ads_real r, ads_real g, ads_real b,
					ads_real *h, ads_real *s, ads_real *v)

{
    ads_real imax = max(r, max(g, b)),
	     imin = min(r, min(g, b)),
	     rc, gc, bc;

    *v = imax;
    if (imax != 0)
	*s = (imax - imin) / imax;
    else
	*s = 0.0;

    if (*s == 0) {
	*h = -1.0;
    } else {
	rc = (imax - r) / (imax - imin);
	gc = (imax - g) / (imax - imin);
	bc = (imax - b) / (imax - imin);
	if (r == imax)
	    *h = bc - gc;
	else if (g == imax)
	    *h = 2.0 + rc - bc;
	else
	    *h = 4.0 + gc - rc;
	*h *= 60.0;
	if (*h < 0.0)
	    *h += 360.0;
    }
}

/*  RGB_HLS  --  Map R, G, B intensities in the range from 0 to 1 into
		 Hue, Lightness, and Saturation: Hue from  0  to  360,
		 Lightness  from  0  to 1, and Saturation from 0 to 1.
                 Special case: if Saturation is 0 (it's a  grey  scale
		 tone), Hue is undefined and is returned as -1.

		 This follows Foley & van Dam, section 17.4.5.	*/

static void rgb_hls(ads_real r, ads_real g, ads_real b, 
					ads_real *h, ads_real *l, ads_real *s)

{
    ads_real imax = max(r, max(g, b)),
	     imin = min(r, min(g, b)),
	     rc, gc, bc;

    *l = (imax + imin) / 2;

    if (imax == imin) {
	*s = 0.0;
	*h = -1.0;
    } else {
	if (*l <= 0.5)
	    *s = (imax - imin) / (imax + imin);
	else
	    *s = (imax - imin) /
		 (2.0 - imax - imin);

	rc = (imax - r) / (imax - imin);
	gc = (imax - g) / (imax - imin);
	bc = (imax - b) / (imax - imin);
	if (r == imax)
	    *h = bc - gc;
	else if (g == imax)
	    *h = 2.0 + rc - bc;
	else
	    *h = 4.0 + gc - rc;
	*h *= 60.0;
	if (*h < 0)
	    *h += 360.0;
    }
}

/*  HLS_RGB  --  Convert HLS colour specification to RGB  intensities.
		 Hue  is  specified  as  a  real  value from 0 to 360;
		 Lightness and Saturation as reals from 0 to  1.   The
		 RGB components are returned as reals from 0 to 1.	*/

static ads_real hlsval(ads_real n1, ads_real n2, ads_real hue)

{
    if (hue > 360.0)
	hue -= 360.0;
    else if (hue < 0.0)
	hue += 360.0;
    if (hue < 60.0) {
	return n1 + ((n2 - n1) * hue) / 60.0;
    } else if (hue < 180.0) {
	return n2;
    } else if (hue < 240.0) {
	return n1 + ((n2 - n1) * (240.0 - hue)) / 60.0;
    } else {
	return n1;
    }
}

static void hls_rgb(ads_real h, ads_real l, ads_real s, 
					ads_real *r, ads_real *g, ads_real *b)

{
    ads_real m1, m2;

    if (l <= 0.5)
	m2 = l * (1.0 + s);
    else
	m2 = l + s - (l * s);
    m1 = 2 * l - m2;

    if (s == 0) {
	*r = *g = *b = l;
    } else {
	*r = hlsval(m1, m2, h + 120.0);
	*g = hlsval(m1, m2, h);
	*b = hlsval(m1, m2, h - 120.0);
    }
}

/*  RGB_YIQ  --  Convert RGB colour specification, R, G, B ranging
		 from 0 to 1, to Y, I, Q colour specification.

		 |Y|   |0.30  0.59  0.11|   |R|
		 |I| = |0.60 -0.28 -0.32| . |G|
		 |Q|   |0.21 -0.52  0.31|   |B|
*/

static void rgb_yiq(ads_real r, ads_real g, ads_real b, 
					ads_real *y, ads_real *i, ads_real *q)

{
    ads_real ay = (r * 0.30 + g *  0.59 + b *  0.11),
	     ai = (r * 0.60 + g * -0.28 + b * -0.32),
	     aq = (r * 0.21 + g * -0.52 + b *  0.31);

    *y = ay;
    if (ay == 1.0) {		      /* Prevent round-off on grey scale */
	ai = aq = 0.0;
    }
    *i = ai;
    *q = aq;
}

/*  YIQ_RGB  --  Convert YIQ colour specification, Y, I,  Q  given  as
		 reals,  Y  from  0  to  1, I from -0.6 to 0.6, Q from
		 -0.52 to 0.52, to R, G, B intensities	in  the  range
		 from  0 to 1.	The matrix below is the inverse of the
		 RGB_YIQ matrix above.

		 |R|   |1.00  0.948  0.624|   |Y|
		 |G| = |1.00 -0.276 -0.640| . |I|
		 |B|   |1.00 -1.105  1.730|   |Q|
*/

static void yiq_rgb(ads_real y, ads_real i, ads_real q,
					ads_real *r, ads_real *g, ads_real *b)

{
    ads_real ar = (y + i *   0.948 + q *  0.624),
	     ag = (y + i *  -0.276 + q * -0.640),
	     ab = (y + i *  -1.105 + q *  1.730);

    *r = max(0.0, min(1.0, ar));
    *g = max(0.0, min(1.0, ag));
    *b = max(0.0, min(1.0, ab));
}

/*  RGB_CMY  --  Convert RGB colour specification,  R,	G,  B  ranging
		 from  0  to  1, to C, M, Y colour specification, also
		 ranging from 0 to 1.

		 |C|   |1|   |R|
		 |M| = |1| - |G|
		 |Y|   |1|   |B|
*/

static void rgb_cmy(ads_real r, ads_real g, ads_real b,
					ads_real *c, ads_real *m, ads_real *y)

{
    *c = 1.0 - r;
    *m = 1.0 - g;
    *y = 1.0 - b;
}

#ifdef NEEDED

/*  CMY_RGB  --  Convert CMY colour specification,  C,	M,  Y  ranging
		 from  0  to  1, to R, G, B colour specification, also
		 ranging from 0 to 1.

		 |R|   |1|   |C|
		 |G| = |1| - |M|
		 |B|   |1|   |Y|
*/

static void cmy_rgb(ads_real c, ads_real m, ads_real y,
					ads_real *r, ads_real *g, ads_real *b)

{
    *r = 1.0 - c;
    *g = 1.0 - m;
    *b = 1.0 - y;
}
#endif

/*  CTEMP_RGB  --  Calculate the relative R, G, and B components for a
		   black body emitting light at a  given  temperature.
		   The	Planck	radiation  equation is solved directly
		   for the R, G, and B wavelengths defined for the CIE
		   1931  Standard  Colorimetric  Observer.  The colour
		   temperature is specified in degrees Kelvin. */

static void ctemp_rgb(double temp, double *r, double *g, double *b)

{
    double c1 = 3.74183e10,
	   c2 = 14388.0,
	   er, eg, eb, es;

/* Lambda is the wavelength in microns: 5500 angstroms is 0.55 microns. */

#define Planck(lambda)  ((c1 * pow((double) lambda, -5.0)) /  \
			 (pow(M_E, c2 / (lambda * temp)) - 1))

    er = Planck(0.7000);
    eg = Planck(0.5461);
    eb = Planck(0.4358);
#undef Planck

    es = 1.0 / max(er, max(eg, eb));

    *r = er * es;
    *g = eg * es;
    *b = eb * es;
}

/*  TOKEN  --  Scan next token from the CNS string and update the
	       scan pointer.  */

static colourvocab token(char **icp)

{
    char ch;
    char *cp = *icp, *tch;
    int i, t = 0;

    /* Ignore leading space */
#pragma warning( disable : 4127 ) /* constant condition  */
    while (True) {
	ch = *cp++;
	if (!isspace(ch))
	    break;
    }
#pragma warning( default : 4127 ) /* reset constant condition  */

    if (ch == EOS)
	return Period;

    if (ch == '-') {
	*icp = cp;
	return Hyphen;
    }

    tch = cp - 1;		      /* Start of token pointer */
    if (!isalpha(ch)) {
	*cp = EOS;
	*icp = tch;
	return Huh;
    }

    while (isalpha(ch)) {
	if (isupper(ch))
	    ch = (char)tolower(ch);
	if (t < ((sizeof tokenb) - 2))
	    tokenb[t++] = ch;
	ch = *cp++;
    }
    tokenb[t] = EOS;
    *icp = cp - 1;

    for (i = 0; i < ELEMENTS(cvocab); i++) {
	if (strcmp(tokenb, cvocab[i].cname) == 0) {
	    return cvocab[i].ccode;
	}
    }
    **icp = EOS;
    *icp = tch;
    return Huh;
}

/*  CNS_RGB  --  Convert a CNS string to RGB intensities scaled from 0
		 to 1.	If an invalid specification is made,  0
		 is returned and an error message is pointed to by the
		 global character pointer  cnserr.   Otherwise,  1  is
		 returned.  */

static Boolean cns_rgb(char *cns, ads_real *r, ads_real *g, ads_real *b)

{
    int i, j, k = 0, lightness, saturation;
    long tok = 0L, hue;
    colourvocab t;
    static char conflite[] = /*MSG31*/"Conflicting lightness specification.";
    /* Grey scale table */
    static int greyscale[] = {50, 17, 33, 50, 67, 83};
    /* Saturation percentage table */
    static int satab[] = {10000, 2500, 5000, 7500, 10000};
    /* Chromatic lightness table */
    static int litetab[] = {5000, 1300, 2500, 5000, 7500, 10000};

    cnserr = NULL;		      /* Initially no error in CNS string */
    j = strlen(cns);
    if (j == 0) {
        cnserr = /*MSG32*/"Void specification.";
	return False;
    }

    /* Scan string and parse tokens */

#pragma warning( disable : 4127 ) /* constant condition  */
    while (True) {
	t = token(&cns);
	if (t == Huh) {
            V sprintf(cnserb, /*MSG33*/"Unrecognised symbol: `%s'.", cns);
	    cnserr = cnserb;
	    return False;
	}
#pragma warning( default : 4127 ) /* reset constant condition  */
	if (Tbit(t)) {
            V sprintf(cnserb, /*MSG34*/"Duplicate symbol: `%s'.", tokenb);
	    cnserr = cnserb;
	    return False;
	}
	if (t == Period)
	    break;
	tok |= 1L << ((int) t);
    }

    /* Try to obtain lightness from specification */

    if (tok & (Tb(Very) | Tb(Dark) | Tb(Medium) | Tb(Light))) {
	if (Tbit(Medium)) {
	    if (Tbit(Very)) {
                cnserr = /*MSG35*/"Very used with Medium.";
		return False;
	    }
	    if (Tbit(Light) || Tbit(Dark)) {
		cnserr = conflite;
		return False;
	    }
	    lightness = 3;
	} else if (Tbit(Dark)) {
	    lightness = Tbit(Very) ? 1 : 2;
	    if (Tbit(Light)) {
		cnserr = conflite;
		return False;
	    }
	} else if (Tbit(Light)) {
	    lightness = Tbit(Very) ? 5 : 4;
	} else {
            cnserr = /*MSG36*/"Very used without Light or Dark.";
	    return False;
	}
    } else {
	lightness = 0;
    }

    /* Test for achromatic colour specification. */

    i = !!(Tbit(Black)) + !!(Tbit(Gray)) + !!(Tbit(White));
    if (i > 0) {

	/* Test for conflicting specification of more than
	   one achromatic colour. */

	if (i != 1) {
            cnserr = /*MSG37*/"Conflicting black/gray/white specification.";
	    return False;
	}

	/* Test for specification of chromatic colour with
	   achromatic colour. */

	if (tok & (Tb(Red) | Tb(Orange) | Tb(Brown) | Tb(Yellow) |
		   Tb(Green) | Tb(Blue) | Tb(Purple))) {
            cnserr = /*MSG38*/"Chromatic and achromatic shade mixed.";
	    return False;
	}

	/* Test for specification of chromatic colour ish form with
	   achromatic colour. */

	if (tok & (Tb(Reddish) | Tb(Orangish) |
		   Tb(Brownish) | Tb(Yellowish) |
		   Tb(Greenish) | Tb(Bluish) | Tb(Purplish) |
		   Tb(Hyphen))) {
            cnserr = /*MSG39*/"Chromatic modifier and achromatic shade mixed.";
	    return False;
	}

	/* Test for saturation specification with achromatic shade. */

	if (tok & (Tb(Grayish) | Tb(Moderate) | Tb(Strong) | Tb(Vivid))) {
            cnserr = /*MSG40*/"Saturation specified with achromatic shade.";
	    return False;
	}

	/* Test for lightness specified with White or Black. */

	if (Tbit(White) || Tbit(Black)) {
	    if (tok & (Tb(Very) | Tb(Dark) | Tb(Medium) | Tb(Light))) {
                cnserr = /*MSG41*/"Lightness specified with black or white.";
		return False;
	    }
	    if (Tbit(White)) {
		*r = *g = *b = 1.0;   /* White */
	    } else {
		*r = *g = *b = 0.0;     /* Black */
	    }
	    return True;
	}

	/* Calculate grey scale value from lightness specification. */

	*r = *g = *b = greyscale[lightness] / 100.0;
	return True;
    }

    /* It isn't a grey scale, so it must be a chromatic colour
       specification.  Before we tear into the hue, let's try and
       determine the saturation. */

    i = (!!Tbit(Grayish)) + (!!Tbit(Moderate)) +
	(!!Tbit(Strong)) + (!!Tbit(Vivid));
    if (i > 0) {
	if (i > 1) {
            cnserr = /*MSG42*/"Conflicting saturation specification.";
	    return False;
	}
	saturation = Tbit(Grayish) ? 1 :
		     (Tbit(Moderate) ? 2 :
		      (Tbit(Strong) ? 3 : 4));
    } else {
	saturation = 0;
    }

    /* Count primary hue specifications. */

    i = (!!Tbit(Red)) + (!!Tbit(Orange)) + (!!Tbit(Brown)) +
	(!!Tbit(Yellow)) +
	(!!Tbit(Green)) + (!!Tbit(Blue)) + (!!Tbit(Purple));

    if (i == 0) {
        cnserr = /*MSG43*/"No hue specified.";
	return False;
    }
    if (i > 2) {
        cnserr = /*MSG44*/"More than two hues specified.";
	return False;
    }

    /* Count secondary hue specifications. */

    j = (!!Tbit(Reddish)) + (!!Tbit(Orangish)) + (!!Tbit(Brownish)) +
	(!!Tbit(Yellowish)) +
	(!!Tbit(Greenish)) + (!!Tbit(Bluish)) + (!!Tbit(Purplish));

    if (j > 1) {
        cnserr = /*MSG45*/"More than one secondary hue specified.";
	return False;
    }
    if (i == 2 && j > 0) {
        cnserr = /*MSG46*/"Secondary hue specified with two primary hues.";
	return False;
    }

    /* Obtain hue based on form of specification we've determined
       is being made.

       Case 1.	Pure hue specified by a single primary hue. */

    hue = -1;
    if (i == 1 && j == 0) {
	for (i = 0; i < ELEMENTS(colhue); i++) {
	    if (tok & colhue[i].cbit) {
		hue = abs(colhue[i].chue) * 100L;
                /* If it's brown, impute saturation and lightness
		   if none was explicitly specified. */
		if (colhue[i].chue < 0) {
		    if (lightness == 0)
			lightness = BROWNLIGHT;
		    if (saturation == 0)
			saturation = BROWNSAT;
		}
		break;
	    }
	}
    } else if (i == 2) {

	/* Case 2.  Halfway hue specified by composing two adjacent
		    primary hues. */

	j = k = -1;
	for (i = 0; i < ELEMENTS(colhue); i++) {
	    if (tok & colhue[i].cbit) {
		if (j < 0)
		    j = i;
		else {
		    k = i;
		    break;
		}
	    }
	}
	if ((colhue[j].chue == -colhue[k].chue) ||
	    (((j + 1) != k) &&
	     !(j == 0 && k == 2) && !(j == 1 && k == 3) &&
	     (!(j == 0 && k == (ELEMENTS(colhue) - 2))))) {
            cnserr = /*MSG47*/"Two primary hues are not adjacent.";
	    return False;
	}

	if (Tbit(Red) && Tbit(Purple))
	    j = ELEMENTS(colhue) - 1;

	hue = (abs(colhue[j].chue) + abs(colhue[k].chue)) * 50L;
	/* If either is brown, impute saturation and lightness
	   if none was explicitly specified. */
	if (colhue[j].chue < 0 || colhue[k].chue < 0) {
	    if (lightness == 0)
		lightness = BROWNLIGHT;
	    if (saturation == 0)
		saturation = BROWNSAT;
	}
    } else {

	/* Case 3.  Quarterway hue specified by one primary hue
		    and one secondary hue. */

	for (i = 0; i < ELEMENTS(colhue); i++) {
	    if (tok & colhue[i].cbit) {
		j = i;
		break;
	    }
	}
	for (i = 0; i < ELEMENTS(ishhue); i++) {
	    if (tok & ishhue[i].cbit) {
		k = i;
		break;
	    }
	}
	if ((colhue[j].chue == -colhue[k].chue) || (
	       ((j + 1) != k) && ((j - 1) != k) &&
	       !(j == 0 && k == 2) && !(j == 1 && k == 3) &&
	       !(k == 0 && j == 2) && !(k == 1 && j == 3) &&
	       (!(j == 0 && k == (ELEMENTS(ishhue) - 2))) &&
	       (!(k == 0 && j == (ELEMENTS(ishhue) - 2)))
	      )
	   ) {
            cnserr = /*MSG48*/"Primary and secondary hues are not adjacent.";
	    return False;
	}

	if (Tbit(Red) && Tbit(Purplish))
	    j = ELEMENTS(colhue) - 1;
	else if (Tbit(Purple) && Tbit(Reddish))
	    k = ELEMENTS(ishhue) - 1;

	hue = (abs(colhue[j].chue) * 3 + abs(ishhue[k].chue)) * 25L;

	/* If either is brown, impute saturation and lightness
	   if none was explicitly specified. */

	if (colhue[j].chue < 0 || ishhue[k].chue < 0) {
	    if (lightness == 0)
		lightness = BROWNLIGHT;
	    if (saturation == 0)
		saturation = BROWNSAT;
	}
    }

    if (hue < 0) {
        cnserr = /*MSG49*/"Internal error--cannot determine hue.";
	return False;
    }

    if (lightness == 0)
	k = defcnslit;
    else
	k = litetab[lightness];

    hsv_rgb(hue / 100.0, satab[saturation] / 10000.0, k / 10000.0,
	    r, g, b);
    return True;
}

/*  CIXNAME  --  Find name of colour vocabulary word from its index.  */

static char *cixname(colourvocab cx)

{
    int i;

    for (i = 0; i < ELEMENTS(cvocab); i++)
	if (cvocab[i].ccode == cx)
	    break;
    return cvocab[i].cname;
}

/*  RGB_CNS  --  Find best CNS description for RGB colour expressed
		 in R, G, and B, from 0 to 1.  */

static void rgb_cns(ads_real r, ads_real g, ads_real b, char *cnstr)

{
    int i, j = 0, k, d, s, v;
    long lh, ld, hd;
    ads_real rh, rs, rv;

#define C(x)  ((char) (x))
#define CV(x) ((colourvocab) (x))

    /* Grey scale name table */

    static struct {
       int intens;
       char gname[3];
    } gtab[] = {
       {0,     {C(Black),		   0}},
       {1700,  {C(Very),  C(Dark), C(Gray)  }},
       {3300,  {C(Dark),  C(Gray),	   0}},
       {5000,  {C(Gray),		   0}},
       {6700,  {C(Light), C(Gray),	   0}},
       {8300,  {C(Very),  C(Light), C(Gray) }},
       {10000, {C(White),		   0}}
      };

    /* Hue name table */

    static struct {
       long huecode;
       char purename,
	    ishname;
    } huetab[] = {
       {0L,	C(Red),    C(Reddish)},
       {3000L,	C(Orange), C(Orangish)},
       {6000L,	C(Yellow), C(Yellowish)},
       {12000L, C(Green),  C(Greenish)},
       {24000L, C(Blue),   C(Bluish)},
       {30000L, C(Purple), C(Purplish)},
       {36000L, C(Red),    C(Reddish)}
      };

    /* Chromatic lightness table */

    static struct {
       int intens;
       char lname[2];
    } ltab[] = {
       {1250,  {C(Very), C(Dark)  }},
       {2500,  {C(Dark),	 0}},
       {5000,  {C(Medium),	 0}},
       {7500,  {C(Light),	 0}},
       {10000, {C(Very), C(Light) }}
      };

    /* Chromatic saturation table */

    static struct {
       int satper;
       char sname;
    } stab[] = {
       {2500,  C(Grayish)  },
       {5000,  C(Moderate) },
       {7500,  C(Strong)   },
       {10000, C(Vivid)    }
      };

    cnstr[0] = EOS;

    rgb_hsv(r, g, b, &rh, &rs, &rv);
    lh = (long)rh * 100L;
    s = (int)rs * 10000;
    v = (int)rv * 10000;

    if (s == 0) {

	/* Achromatic */

	d = 20000;
	for (i = 0; i < ELEMENTS(gtab); i++) {
	    if (abs(gtab[i].intens - v) < d) {
		d = abs(gtab[i].intens - v);
		j = i;
	    }
	}
	for (i = 0; i < 3; i++) {
	    if (gtab[j].gname[i] == 0)
		break;
	    if (strlen(cnstr) > 0)
                V strcat(cnstr, " ");
	    V strcat(cnstr, cixname(CV(gtab[j].gname[i])));
	}
    } else {

	/* Chromatic.  */

	/* Locate intensity.   If  the	closest  intensity  is	the
           default  intensity  in  DEFCNSLIT,  we  don't  edit  any
	   intensity.  You can disable this by setting DEFCNSLIT to
	   -1.	*/

	d = 20000;
	for (i = 0; i < ELEMENTS(ltab); i++) {
	    if (abs(ltab[i].intens - v) < d) {
		d = abs(ltab[i].intens - v);
		j = i;
	    }
	}
	if (ltab[j].intens != defcnslit) {
	    for (i = 0; i < 2; i++) {
		if (ltab[j].lname[i] == 0)
		    break;
		if (strlen(cnstr) > 0)
                    V strcat(cnstr, " ");
		V strcat(cnstr, cixname(CV(ltab[j].lname[i])));
	    }
	}

	/* Locate saturation.  If the saturation is vivid, nothing
	   is edited. */

	d = 20000;
	for (i = 0; i < ELEMENTS(stab); i++) {
	    if (abs(stab[i].satper - s) <= d) {
		d = abs(stab[i].satper - s);
		j = i;
	    }
	}
	if (stab[j].satper != 10000) {
	    if (strlen(cnstr) > 0)
                V strcat(cnstr, " ");
	    V strcat(cnstr, cixname(CV(stab[j].sname)));
	}

	if (strlen(cnstr) > 0)
            V strcat(cnstr, " ");

	/* Find closest hue name. */

	ld = 100000L;
	if (lh == 36000L)
	    lh = 0;
	for (i = 0; i < ELEMENTS(huetab); i++) {
	    if (abs(huetab[i].huecode - lh) < ld) {
		ld = abs(huetab[i].huecode - lh);
		j = i;
	    }
	}

        /* Now we'll find the next hue in the direction of the
	   actual hue from the specified hue. */

	if (lh > huetab[j].huecode) {
	    if (j == (ELEMENTS(huetab) - 1))
		k = 1;
	    else
		k = j + 1;
	} else {
	    if (j == 0)
		k = ELEMENTS(huetab) - 2;
	    else
		k = j - 1;
	}

	/* Next, compute the distance between the hue and the next
           neighbour in the hue's direction.  */

	hd = abs(huetab[j].huecode - huetab[k].huecode);

	/* The	form of the hue then  depends upon the relationship
	   between the actual distance, D, and the total  distance,
	   HD,	from the closest pure hue, J, and the next pure hue
	   in the direction of the hue supplied,  K.   We  generate
	   the following based upon the relationship:

		 D / HD 	 Name
	      ------------     --------
	      0     - 0.125	  J
	      0.125 - 0.375	Kish J
	      0.375 - 0.5	 J-K
	*/

	hd = (ld * 10000L) / hd;
	if (hd < 1250L) {
	    V strcat(cnstr, cixname(CV(huetab[j].purename)));
	} else if (hd < 3750L) {
	    V strcat(cnstr, cixname(CV(huetab[k].ishname)));
            V strcat(cnstr, " ");
	    V strcat(cnstr, cixname(CV(huetab[j].purename)));
	} else {
	    V strcat(cnstr, cixname(CV(huetab[j].purename)));
            V strcat(cnstr, "-");
	    V strcat(cnstr, cixname(CV(huetab[k].purename)));
	}
    }
}

/*  ACADRGB  --  Takes	an  AutoCAD  colour  number in hsv and returns
		 red, green, and blue intensities in rgp in the  range
		 0.0 to 1.0 */

extern void acadrgb(int  hsv, struct r_g_b *rgp)

{
    static ads_real brightfac[5] = {  /* Brightness levels */
       1.0, 0.65, 0.5, 0.3, 0.15
      }, halfsat = .5;		      /* Halfway saturation */
    int ih, vs;
    ads_real h, s, f, value;

    assert(hsv > 0 || hsv < 256);

    switch (hsv) {
    case BLACK:
	rgp->red   = 0.0;
	rgp->blue  = 0.0;
	rgp->green = 0.0;
	value = 0.0;
	break;

    case RED:
	rgp->red   = SAT;
	rgp->green = 0.0;
	rgp->blue  = 0.0;
	value = 1.0;
	break;

    case YELLOW:
	rgp->red   = SAT;
	rgp->green = SAT;
	rgp->blue  = 0.0;
	value = 1.0;
	break;

    case GREEN:
	rgp->red   = 0.0;
	rgp->green = SAT;
	rgp->blue  = 0.0;
	value = 1.0;
	break;

    case CYAN:
	rgp->red   = 0.0;
	rgp->green = SAT;
	rgp->blue  = SAT;
	value = 1.0;
	break;

    case BLUE:
	rgp->red   = 0.0;
	rgp->green = 0.0;
	rgp->blue  = SAT;
	value = 1.0;
	break;

    case MAGENTA:
	rgp->red   = SAT;
	rgp->green = 0.0;
	rgp->blue  = SAT;
	value = 1.0;
	break;

    case WHITE:
    case 8:
    case 9:
	rgp->red   = SAT;
	rgp->green = SAT;
	rgp->blue  = SAT;
	value = 1.0;
	break;

    default:

	/*  The chromatic colors.  The	hue  resulting	from  an
	    AutoCAD color 10-249 will be determined by its first
	    two digits, and the saturation and	value  from  the
	    last digit, as follows:

	    Hues:

	     10 -- Red
	     50 -- Yellow
	     90 -- Green
	    130 -- Cyan
	    170 -- Blue
	    210 -- Magenta

	    Between  each  of these are three intermediate hues,
	    e.g., between red and yellow, we have:

	     20 -- reddish orange
	     30 -- orange
	     40 -- yellowish orange

	    To each hue number, 0, 2, 4, 6, or 8 can be added to
            give a different "value", or brightness, with 0  the
	    brightest  and  8  the  weakest.   Finally, 1 can be
            added to  produce  a  "half-saturated",  or  pastel,
	    color.  For example, color 18 is the dimmest red and
	    10 the brightest red.  19 is the dimmest pink and 11
	    the brightest pink.
	*/

	if (hsv > 9 && hsv < 250) {

	    /* Apply the algorithm from Foley & van Dam to turn
	       HSV into RGB values */

	    ih = (hsv - 10) / 10;     /* Integer hue value. */
	    if (ih >= 24)	      /* Range is 0-23. */
		ih -= 24;
	    vs = hsv % 10;	      /* Encoded value and saturation */
	    h = ih / 4.;	      /* Map into range [0.0,6.0) */
	    ih = (int) h;		      /* The integer part. */
	    f = h - ih; 	      /* Fractional part. */
	    value = brightfac[vs >> 1]; /* Value in [0,1] */
	    s = vs & 1 ? halfsat : 1.0; /* Saturation */

	    switch (ih) {
	    case 0:
		rgp->red   = 1.0;
		rgp->green = (ads_real) (1.0 - s * (1.0 - f));
		rgp->blue  = (ads_real) (1.0 - s);
		break;

	    case 1:
		rgp->red   = (ads_real) (1.0 - s * f);
		rgp->green = 1.0;
		rgp->blue  = (ads_real) (1 - s);
		break;

	    case 2:
		rgp->red   = (ads_real) (1.0 - s);
		rgp->green = 1.0;
		rgp->blue  = (ads_real) (1.0 - s *(1.0 - f));
		break;

	    case 3:
		rgp->red   = (ads_real) (1.0 - s);
		rgp->green = (ads_real) (1.0 - s * f);
		rgp->blue  = 1.0;
		break;

	    case 4:
		rgp->red   = (ads_real) (1.0 - s * (1.0 - f));
		rgp->green = (ads_real) (1.0 - s);
		rgp->blue  = 1.0;
		break;

	    case 5:
		rgp->red   = 1.0;
		rgp->green = (ads_real) (1.0 - s);
		rgp->blue  = (ads_real) (1.0 - s * f);
		break;
	    }
	} else {
	    /* Define some extra colours from dark grey to white
	       in the 250 to 255 slots */
	    value = 0.33 + (hsv - 250) * 0.134;
	    rgp->red   = 1.0;
	    rgp->green = 1.0;
	    rgp->blue  = 1.0;
	}
	break;			      /* Default */
    }
    rgp->red   *= value;	      /* Apply lightness scale factor */
    rgp->green *= value;	      /* to components resulting from */
    rgp->blue  *= value;	      /* hue and saturation. */
}

/*  RGBACAD  --  Find the AutoCAD colour closest to in RGB space to a
		 specified RGB triple.	*/

static int rgbacad(ads_real r, ads_real g, ads_real b)

{
    int i, low, ccol;
    ads_real closest = 1000.0;
    struct r_g_b rc;

    assert(r >= 0.0 && r <= 1.0);
    assert(g >= 0.0 && g <= 1.0);
    assert(b >= 0.0 && b <= 1.0);

    /* If we're mapping to the 8 colour gamut, turn all grey scale
       colours into white and map the rest based on hue alone. */

    if (gamut == 8) {
	ads_real h, s, v;

	rgb_hsv(r, g, b, &h, &s, &v);
	return s == 0.0 ? WHITE :
	       (RED + ((((int) (h + 30.0)) % 360) / 60));
    }

    /* Note  that we start with  colour 1 since 0 (black) is not a
       valid user-specified colour.  If this is a grey scale tone,
       map only to AutoCAD's grey scale indices.  */

    ccol = low = (r == g && r == b) ? 250 : 1;

    for (i = low; i < 256; i++) {
	ads_real cdist;

	acadrgb(i, &rc);
	rc.red -= r;
	rc.green -= g;
	rc.blue -= b;
	cdist = rc.red * rc.red + rc.green * rc.green +
		rc.blue * rc.blue;
	if (cdist < closest) {
	    ccol = i;
	    if ((closest = cdist) == 0.0)
		break;
	}
    }
    if (ccol == 255)		      /* If synonym for white... */
	ccol = 7;		      /* ...make simple white. */
    return ccol;
}

/*  RETRGB  --	Return an RGB triple as either an RGB point or
		the closest AutoCAD standard colour.  */

static void retrgb(Boolean acad, ads_real r, ads_real g, ads_real b)

{
    if (acad) {
	ads_retint(rgbacad(r, g, b));
    } else {
	ads_point p;

	Spoint(p, r, g, b);
	ads_retpoint(p);
    }
}

/*  TRIPLE  --	Scan  a  triple  of  real  arguments  into an array of
		reals.	Integers are accepted and converted to	reals.
		True  is  returned  if	valid  arguments are obtained;
		False otherwise.  */

static Boolean triple(struct resbuf *rb1, ads_real cdesc[3], Boolean rangecheck)

{
    int nargs;
    /* struct resbuf *rb1 = ads_getargs(); */

    ads_retnil();
    for (nargs = 0; nargs < 3; nargs++) {
	if (rb1 == NULL)
	    break;
	if (rb1->restype == RTSHORT) {
	    cdesc[nargs] = (ads_real)rb1->resval.rint;
	} else if (rb1->restype == RTREAL) {
	    cdesc[nargs] = rb1->resval.rreal;
	} else if (nargs == 0 && rb1->restype == RT3DPOINT) {
	    Cpoint(cdesc, rb1->resval.rpoint);
	    nargs = 2;
	} else {
            ads_fail(/*MSG50*/"incorrect argument type");
	    return False;
	}
	rb1 = rb1->rbnext;
    }

    /* Make sure there were enough arguments. */

    if (nargs < 3) {
        ads_fail(/*MSG51*/"too few arguments");
	return False;
    }

    /* Make sure there are no more arguments. */

    if (rb1 != NULL) {
        ads_fail(/*MSG52*/"too many arguments");
	return False;
    }

    /* Range check arguments if requested. */

    if (rangecheck) {
	for (nargs = 0; nargs < 3; nargs++) {
	    if (rangecheck && (cdesc[nargs] < 0.0 || cdesc[nargs] > 1.0)) {
                ads_fail(/*MSG53*/"argument out of range");
		return False;
	    }
	}
    }

    return True;
}

/*  CMY  --  Specify colour as CMY triple.  */

static void cmy(struct resbuf *rb, Boolean acad)

{
    ads_real cdesc[3];

    if (triple(rb, cdesc, True)) {
	retrgb(acad, 1.0 - cdesc[0], 1.0 - cdesc[1],
	       1.0 - cdesc[2]);
    }
}

/*  CTEMP  --  Specify colour as a colour temperature.	*/

static void ctemp(struct resbuf *rb, Boolean acad)

{

    ads_retnil();

    if (rb == NULL) {
        ads_fail(/*MSG63*/"too few arguments");
    } else {
	ads_real degrees, ir, ig, ib;

	if (rb->restype == RTSHORT) {
	    degrees = (ads_real)rb->resval.rint;
	} else if (rb->restype == RTREAL) {
	    degrees = rb->resval.rreal;
	} else {
            ads_fail(/*MSG64*/"incorrect argument type");
	    return;
	}

	/* Make sure there are no more arguments. */

	if (rb->rbnext != NULL) {
            ads_fail(/*MSG65*/"too many arguments");
	    return;
	}

	ctemp_rgb(degrees, &ir, &ig, &ib);
	retrgb(acad, ir, ig, ib);
    }
}

/*  CNS  --  Specify colour as a CNS string.  */

static void cns(struct resbuf *rb, Boolean acad)

{

    ads_retnil();

    if (rb == NULL) {
        ads_fail(/*MSG54*/"too few arguments");
	return;
    } else {
	struct resbuf *rb1 = rb;
	ads_real ir, ig, ib;

	if (rb1->restype != RTSTR) {
            ads_fail(/*MSG55*/"incorrect argument type");
	    return;
	}

	/* Make sure there are no more arguments. */

	if (rb1->rbnext != NULL) {
            ads_fail(/*MSG56*/"too many arguments");
	    return;
	}

	if (cns_rgb(rb1->resval.rstring, &ir, &ig, &ib)) {
	    retrgb(acad, ir, ig, ib);
	}
    }
}

#pragma warning( disable : 4100 ) /* unused argument  */

/*  CNSER  --  Return string describing last CNS error, if any.  */
/* argsused */
extern int cnser(struct resbuf *rb)

{
    if (cnserr == NULL)
	ads_retnil();
    else
	ads_retstr(cnserr);
	return RTNORM;
}
#pragma warning( default : 4100 ) /* reset unused argument  */

/*  HLS  --  Specify colour as HLS triple.  */

static void hls(struct resbuf *rb, Boolean acad)

{
    ads_real cdesc[3];

    if (triple(rb, cdesc, True)) {
	ads_real ir, ig, ib;

	hls_rgb(cdesc[0] * 360.0, cdesc[1], cdesc[2], &ir, &ig, &ib);
	retrgb(acad, ir, ig, ib);
    }
}

/*  HSV  --  Specify colour as HSV triple.  */

static void hsv(struct resbuf *rb, Boolean acad)

{
    ads_real cdesc[3];

    if (triple(rb, cdesc, True)) {
	ads_real ir, ig, ib;

	hsv_rgb(cdesc[0] * 360.0, cdesc[1], cdesc[2], &ir, &ig, &ib);
	retrgb(acad, ir, ig, ib);
    }
}

/*  RGB  --  Specify colour as RGB triple.  */

static void rgb(struct resbuf *rb, Boolean acad)

{
    ads_real cdesc[3];

    if (triple(rb, cdesc, True)) {
	retrgb(acad, cdesc[0], cdesc[1], cdesc[2]);
    }
}

/*  YIQ  --  Specify colour as YIQ triple.  */

static void yiq(struct resbuf *rb, Boolean acad)

{
    ads_real cdesc[3];

    if (triple(rb, cdesc, False)) {
	ads_real ir, ig, ib;

	if ((cdesc[0] < 0.0 || cdesc[0] > 1.0) &&
	    (cdesc[1] < -0.6 || cdesc[0] > 0.6) &&
	    (cdesc[2] < -0.52 || cdesc[2] > 0.52)) {
            ads_fail(/*MSG57*/"argument out of range");
	}

	yiq_rgb(cdesc[0], cdesc[1], cdesc[2], &ir, &ig, &ib);

	retrgb(acad, ir, ig, ib);
    }
}

/*  Colour system to AutoCAD colour functions. */

extern int cmyac(struct resbuf *rb )  { cmy(rb, True); return RTNORM;  }
extern int ctempac(struct resbuf *rb) { ctemp(rb, True);return RTNORM; }
extern int yiqac(struct resbuf *rb )  { yiq(rb, True); return RTNORM;  }
extern int hsvac(struct resbuf *rb )  { hsv(rb, True); return RTNORM;  }
extern int rgbac(struct resbuf *rb )  { rgb(rb, True); return RTNORM;   }
extern int hlsac(struct resbuf *rb )  { hls(rb, True); return RTNORM;   }
extern int cnsac(struct resbuf *rb )  { cns(rb, True); return RTNORM;   }

/*  Colour system to RGB functions.  */

extern int cmyrgb(struct resbuf *rb )   { cmy(rb, False);  return RTNORM;  }
extern int ctemprgb(struct resbuf *rb ) { ctemp(rb, False); return RTNORM; }
extern int yiqrgb(struct resbuf *rb )   { yiq(rb, False);  return RTNORM;  }
extern int hsvrgb(struct resbuf *rb )   { hsv(rb, False);  return RTNORM;  }
extern int hlsrgb(struct resbuf *rb )   { hls(rb, False);  return RTNORM;  }
extern int cnsrgb(struct resbuf *rb )   { cns(rb, False);  return RTNORM;  }

/*  ACADCOL  --  Obtain AutoCAD colour.  We accept any of the following:

    1.	A single integer, representing an AutoCAD standard colour index.
    2.	A triple of reals and/or integers, representing RGB intensities.
    3.	A list of three reals and/or integers, representing RGB intensities.
*/

static Boolean acadcol(struct resbuf *rb, struct r_g_b *rp)

{
    ads_real crgb[3];
    /* struct resbuf *rb = ads_getargs(); */

    ads_retnil();

    if (rb == NULL) {
        ads_fail(/*MSG58*/"too few arguments");
	return False;
    }

    if ((rb->restype == RTSHORT) && (rb->rbnext == NULL)) {
	int cindex = rb->resval.rint;

	if (cindex < 0 || cindex > 255) {
            ads_fail(/*MSG59*/"argument out of range");
	    return False;
	}
	acadrgb(cindex, rp);
	return True;
    }

    if (triple(rb, crgb, True)) {
	rp->red   = crgb[0];
	rp->green = crgb[1];
	rp->blue  = crgb[2];
    } else {
	return False;
    }

    return True;
}

/*  TORGB  --  Convert internal colour to RGB triple.  */

extern int torgb(struct resbuf *rb)

{
    struct r_g_b rc;

    if (acadcol(rb, &rc)) {
	ads_point p;

	Spoint(p, rc.red, rc.green, rc.blue);
	ads_retpoint(p);
    }
	return RTNORM;
}

/*  TOCMY  --  Convert internal colour to CMY triple.  */

extern int tocmy(struct resbuf *rb)

{
    struct r_g_b rc;

    if (acadcol(rb, &rc)) {
	ads_point p;

	rgb_cmy(rc.red, rc.green, rc.blue, &p[X], &p[Y], &p[Z]);
	ads_retpoint(p);
    }
	return RTNORM;
}

/*  TOYIQ  --  Convert internal colour to YIQ triple.  */

extern int toyiq(struct resbuf *rb)

{
    struct r_g_b rc;

    if (acadcol(rb, &rc)) {
	ads_point p;

	rgb_yiq(rc.red, rc.green, rc.blue, &p[X], &p[Y], &p[Z]);
	ads_retpoint(p);
    }
	return RTNORM;
}

/*  TOHSV  --  Convert internal colour to HSV triple.  */

extern int tohsv(struct resbuf *rb)

{
    struct r_g_b rc;

    if (acadcol(rb, &rc)) {
	ads_point p;

	rgb_hsv(rc.red, rc.green, rc.blue, &p[X], &p[Y], &p[Z]);
	p[X] = (p[X] < 0.0) ? 0.0 : (p[X] / 360.0);
	ads_retpoint(p);
    }
	return RTNORM;
}

/*  TOHLS  --  Convert internal colour to HLS triple.  */

extern int tohls(struct resbuf *rb)

{
    struct r_g_b rc;

    if (acadcol(rb, &rc)) {
	ads_point p;

	rgb_hls(rc.red, rc.green, rc.blue, &p[X], &p[Y], &p[Z]);
	p[X] = (p[X] < 0.0) ? 0.0 : (p[X] / 360.0);
	ads_retpoint(p);
    }
	return RTNORM;
}

/*  TOCNS  --  Convert internal colour to CNS string.  */

extern int tocns(struct resbuf *rb)

{
    struct r_g_b rc;

    if (acadcol(rb, &rc)) {
	char cnstr[40];

	rgb_cns(rc.red, rc.green, rc.blue, cnstr);
	ads_retstr(cnstr);
    }
	return RTNORM;
}

/*  COLSET  --	Set colour gamut available.  */

extern int colset(struct resbuf *rb)

{
    /* struct resbuf *rb = ads_getargs(); */

    ads_retnil();

    if (rb == NULL) {
	ads_retint(gamut);
	return RTNORM;
    }

    if (rb->rbnext != NULL) {
        ads_fail(/*MSG60*/"too many arguments");
	return RTNORM;
    }

    if (rb->restype == RTSHORT) {
	int colsys = rb->resval.rint;

	switch (colsys) {
	case 8:
	case 256:
	    gamut = colsys;
	    ads_retint(gamut);
	    break;

	default:
            ads_fail(/*MSG61*/"argument out of range");
	}
	return RTNORM;
    }
    ads_fail(/*MSG62*/"incorrect argument type");
	return RTNORM;
}
