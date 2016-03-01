#ifdef __cplusplus
    extern "C" {
#endif

extern int cnser (struct resbuf *);

extern int cmyac (struct resbuf *);
extern int ctempac (struct resbuf *);
extern int yiqac (struct resbuf *);
extern int hsvac (struct resbuf *);
extern int rgbac (struct resbuf *);
extern int hlsac (struct resbuf *);
extern int cnsac (struct resbuf *);

extern int cmyrgb (struct resbuf *);
extern int ctemprgb (struct resbuf *);
extern int yiqrgb (struct resbuf *);
extern int hsvrgb (struct resbuf *);
extern int hlsrgb (struct resbuf *);
extern int cnsrgb (struct resbuf *);
extern int torgb (struct resbuf *);

extern int tocmy (struct resbuf *);
extern int toyiq (struct resbuf *);
extern int tohsv (struct resbuf *);
extern int tohls (struct resbuf *);
extern int tocns (struct resbuf *);

extern int colset (struct resbuf *);

#ifdef __cplusplus
    }
#endif

