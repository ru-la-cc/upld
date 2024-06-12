#ifndef __INC_CGILIB_H__
#define __INC_CGILIB_H__
#define		CGILIB_VERSION				0x0102
#include <stdio.h>
#include <time.h>
#if defined(_WIN32) || defined(_WIN64)
#define cgi_mkdir(d) mkdir(d)
#define cgi_strtoll(s, p, b) _strtoi64(s, p, b)
#define cgi_strtoull(s, p, b) _strtoui64(s, p, b)
#define sleep(s) Sleep((s)*1000)
#else
#define cgi_mkdir(d) mkdir(d,0755)
#define cgi_strtoll(s, p, b) strtoll(s, p, b)
#define cgi_strtoull(s, p, b) strtoull(s, p, b)
#define sleep(s) sleep(s)
#endif

#define CGILIB_MAXPARANAME		31
#define CGILIB_MAXMIMENAME		47
#define CGILIB_MAXFILENAME		255
#define CGILIB_MAXSVFLNAME		31
#define CGILIB_MAXTDIRNAME		205
#define CGILIB_TMPFILEBASE		"_cgitmp_"

#define HEX2DEC(c) (((c)>='a'&&(c)<='f')?((c)-'a'+10):((c)>='A'&&(c)<='F')?((c)-'A'+10):((c)>='0'&&(c)<='9')?((c)-'0'):(c))
#define ISURLCHAR(c)(((c)>='0'&&(c)<='9')||((c)>='A'&&(c)<='Z')||((c)>='a'&&(c)<='z')||(c)=='.'||(c)=='-'||(c)=='*'||(c)=='_')

#ifdef NDEBUG
#define CGI_ASSERT(b,m)
#define CGI_ASSERT2(b,m)
#else
#define CGI_ASSERT(b,m) {if(!(b)){printf("%s\nsource:%s\nline:%d\n",#m,__FILE__,__LINE__);fflush(stdout);exit(3);}}
#define CGI_ASSERT2(b,m) {if(!(b)){printf("%s<br />\nsource:%s<br />\nline:%d\n",#m,__FILE__,__LINE__);exit(3);}}
#endif

typedef enum{
	CGI_SJIS = 0,
	CGI_EUC,
	CGI_UTF8
} cgilib_code;

typedef enum{
	CGI_URLENCGET,
	CGI_URLENCPOST,
	CGI_MULTIPART
} cgilib_enctype;

typedef enum{
	CGI_GET = 1,
	CGI_POST,
	CGI_POSTM
} cgilib_request;

typedef struct _cgifd{
	cgilib_request request_type;
	char name[CGILIB_MAXPARANAME+1];
	char *value;
	char mime[CGILIB_MAXMIMENAME+1];
	char filename[CGILIB_MAXFILENAME+1];
	char svfile[CGILIB_MAXSVFLNAME+1];
	unsigned long long valuesize;
	struct _cgifd *next;
	struct _cgifd *prev;
} cgiform, *pcgiform;

extern const char* const cgi_code_string[];
extern const char* const cgi_uri_prefix[];

extern cgilib_code setcgicode(cgilib_code cd);
extern cgilib_code getcgicode(void);
extern int settempdir(const char *tempdir);
extern int gettempdir(char *tempdir, int len);
extern void(*set_upload_handler(void(*handler)
		(const pcgiform pform, char *buf, int bufsize)))
			(const pcgiform pform, char *buf, int bufsize);
extern void(*set_badalloc(void(*handler)(void)))(void);
extern void *cgi_alloc(size_t size, int flag);
extern void set_headerflg(int flag);
extern int get_headerflg(void);
extern void put_http_header(const char *head_str);
extern void put_html_header(void);
extern void put_text_header(void);
extern char *des_crypt(const char *pw, const char *salt, char* buf, int buflen);
extern const char *getenv_value(const char *envkey);
extern int char_size(const char *src);
extern char *char_next(const char *src);
extern char *char_prev(const char *sta, const char *cur);
extern char *copystring(const char *str);
extern char *addstring(const char *str1, const char *str2);
extern char *upper_string(char *str);
extern char *lower_string(char *str);
extern int str_icomp(const char *str1, const char *str2);
extern int str_icompn(const char *str1, const char *str2, size_t len);
extern size_t str_count(const char *str);
extern char *str_isearch(const char *src, const char *search);
extern int str_split(const char *base, const char *sep, char quot, int col, char ***cols);
extern void split_free(char **cols, int colsize);
extern char *str_rep(const char *str, const char *before, const char *after, int count);
extern char *str_irep(const char *str, const char *before, const char *after, int count);
extern char *substring(const char *str, size_t start, size_t count);
extern char *bsubstring(const char *str, size_t start, size_t count);
extern char *urlenc(const char *str);
extern char *urldec(char *str);
extern const pcgiform getfirstnode_get(void);
extern const pcgiform getfirstnode_post(void);
extern const pcgiform search_form(const char *key, const pcgiform pstart);
extern const char *request(const char *key);
extern const char *next_request(const char *key);
extern void getformdata(unsigned long long maxsize, cgilib_enctype enctype);
extern char *autolink(const char *str);
extern char *base64encode(const char *data, size_t len);
extern size_t base64decode(const char *b64str, char **data);
extern int set_cookie(const char *key, const char *value, time_t period);
extern int delete_cookie(const char *key);
extern size_t get_cookie(const char *key, char *value, size_t bufsize);
extern int get_jpeg_size(const unsigned char *jpeg, size_t size, unsigned *width, unsigned *height);
extern int get_gif_size(const unsigned char *gif, size_t size, unsigned *width, unsigned *height);
extern int get_png_size(const unsigned char *png, size_t size, unsigned *width, unsigned *height);
extern size_t fileread(const char *file, char **buf);
extern int cgi_sock_init(void);
extern int cgi_sock_end(void);
extern int is_ipv4addr(const char *addr);
extern int ipv4_parse(const char *ipv4str, unsigned char ipv4oct[4]);
extern int host2ipv4addr(const char *host, char *ipv4, size_t len);
extern int ipv4addr2host(const char *ipv4, char *host, size_t len);
extern int filecpy(const char *destfile, char *srcfile);
extern void cgi_errend(const char *msg, ...);
extern void cgi_errendex(int line, const char *file, const char *msg, ...);

#define cgialloc(s) cgi_alloc(s,1)

#endif /* __INC_CGILIB_H__ */
