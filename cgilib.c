#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
#include "cgilib.h"

#define CGILIB_TMPDEL(pftmp) {if(pftmp!=NULL){fclose(pftmp);pftmp=NULL;}}

const char* const cgi_code_string[] = {"Shift_JIS","EUC-JP","UTF-8","ISO-8859-1"};
const char* const cgi_uri_prefix[] = {"http://","https://","ftp://",NULL};
const char base64chars[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static cgilib_code cgi_code = CGI_SJIS;
static char cgi_tempdir[CGILIB_MAXTDIRNAME+1] = ".";
static int cgi_putheader_flg = 0;
static pcgiform get_formdata = NULL;
static pcgiform post_formdata = NULL;
static pcgiform cur_formdata = NULL;
char *get_buffer = NULL;
char *post_buffer = NULL;
static int exit_handle_flg = 0;
static int socket_init_flg = 0;
#if defined(_WIN32) || defined(_WIN64)
static WSADATA s_wsadata;
#endif

static const char des_ip[] = {
	58,50,42,34,26,18,10, 2,
	60,52,44,36,28,20,12, 4,
	62,54,46,38,30,22,14, 6,
	64,56,48,40,32,24,16, 8,
	57,49,41,33,25,17, 9, 1,
	59,51,43,35,27,19,11, 3,
	61,53,45,37,29,21,13, 5,
	63,55,47,39,31,23,15, 7,
};
static const char des_fp[] = {
	40, 8,48,16,56,24,64,32,
	39, 7,47,15,55,23,63,31,
	38, 6,46,14,54,22,62,30,
	37, 5,45,13,53,21,61,29,
	36, 4,44,12,52,20,60,28,
	35, 3,43,11,51,19,59,27,
	34, 2,42,10,50,18,58,26,
	33, 1,41, 9,49,17,57,25,
};
static const char des_pc1_c[] = {
	57,49,41,33,25,17, 9,
	 1,58,50,42,34,26,18,
	10, 2,59,51,43,35,27,
	19,11, 3,60,52,44,36,
};
static const char des_pc1_d[] = {
	63,55,47,39,31,23,15,
	 7,62,54,46,38,30,22,
	14, 6,61,53,45,37,29,
	21,13, 5,28,20,12, 4,
};
static const char des_shifts[] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1
};
static const char des_pc2_c[] = {
	14,17,11,24, 1, 5,
	 3,28,15, 6,21,10,
	23,19,12, 4,26, 8,
	16, 7,27,20,13, 2,
};
static const char des_pc2_d[] = {
	41,52,31,37,47,55,
	30,40,51,45,33,48,
	44,49,39,56,34,53,
	46,42,50,36,29,32,
};
static char des_c[28];
static char des_d[28];
static char des_ks[16][48];
static char des_e[48];
static const char des_e2[] = {
	32, 1, 2, 3, 4, 5,
	 4, 5, 6, 7, 8, 9,
	 8, 9,10,11,12,13,
	12,13,14,15,16,17,
	16,17,18,19,20,21,
	20,21,22,23,24,25,
	24,25,26,27,28,29,
	28,29,30,31,32, 1,
};
static const char des_s[8][64] = {
	14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
	 0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
	 4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
	15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13,

	15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
	 3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
	 0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
	13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9,

	10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
	13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
	13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
	 1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12,

	 7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
	13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
	10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
	 3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14,

	 2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
	14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
	 4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
	11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3,

	12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
	10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
	 9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
	 4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13,

	 4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
	13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
	 1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
	 6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12,

	13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
	 1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
	 7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
	 2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11,
};
static const char des_p[] = {
	16, 7,20,21,
	29,12,28,17,
	 1,15,23,26,
	 5,18,31,10,
	 2, 8,24,14,
	32,27, 3, 9,
	19,13,30, 6,
	22,11, 4,25,
};
static char des_l[32], des_r[32];
static char des_templ[32];
static char des_f[32];
static char des_pre_s[48];

static void s_des_setkey(const char *key);
static void s_des_encrypt(char *block /* , int edflg */);
static void s_insert_formdata(pcgiform node);
static void s_clear_formdata(void);
static void s_parse_urlenc_get(unsigned long long maxsize);
static void s_parse_urlenc_post(unsigned long long maxsize);
static void s_parse_multipart_post(unsigned long long maxsize);
static int s_getline_file(char *buf, size_t size, FILE *file);
static void s_write_formvalue(pcgiform pform, char *rbuf, int rs, FILE *pftmp);
static size_t s_geturllen(const char *buf);

static void s_cgi_badalloc(void);
static void s_free_form_exit(void);

static void(*cgi_badalloc_func)(void) = s_cgi_badalloc;

static void(*cgi_upload_func)(const pcgiform pform, char *, int) = NULL;

cgilib_code setcgicode(cgilib_code cd)
{
	cgilib_code r = cgi_code;
	cgi_code = cd;
	return r;
}

cgilib_code getcgicode(void)
{
	return cgi_code;
}

int settempdir(const char *tempdir)
{
	size_t tdirlen;
	tdirlen = strlen(tempdir);
	strncpy(cgi_tempdir, tempdir, CGILIB_MAXTDIRNAME);
	cgi_tempdir[CGILIB_MAXTDIRNAME] = '\0';
	if(tdirlen>CGILIB_MAXTDIRNAME) return -1;
	if(cgi_tempdir[tdirlen>0?(tdirlen-1):0] == '/') cgi_tempdir[tdirlen>0?(tdirlen-1):0] = '\0';
	return 0;
}

int gettempdir(char *tempdir, int len)
{
	if(len < 1) return -1;
	strncpy(tempdir, cgi_tempdir, len);
	tempdir[len-1] = '\0';
	if((size_t)len <= strlen(cgi_tempdir)) return -1;
	return 0;
}

void(*set_upload_handler(void(*handler)
	(const pcgiform pform, char *buf, int bufsize)))
		(const pcgiform pform, char *buf, int bufsize)
{
	void(*ret_func)(const pcgiform, char*, int);
	
	ret_func = cgi_upload_func;
	cgi_upload_func = handler;
	return ret_func;
}

void(*set_badalloc(void(*handler)(void)))(void)
{
	void(*ret_func)(void);
	
	ret_func = cgi_badalloc_func;
	cgi_badalloc_func = handler;
	return ret_func;
}

void *cgi_alloc(size_t size, int flag)
{
	void *ret;
	if(flag && (ret=malloc(size)) == NULL){
		(*cgi_badalloc_func)();
		exit(3);
	}
	return ret;
}

void set_headerflg(int flag)
{
	cgi_putheader_flg = flag;
}

int get_headerflg(void)
{
	return cgi_putheader_flg;
}

void put_http_header(const char *head_str)
{
	if(!cgi_putheader_flg){
		printf(head_str);
		cgi_putheader_flg = 1;
	}
}

void put_html_header(void)
{
	int code;
	if(cgi_code < 0 || cgi_code > CGI_UTF8) code = CGI_UTF8 + 1;
	else code = cgi_code;
	if(!cgi_putheader_flg){
		printf("Content-type: text/html; charset=%s\x0d\x0a\x0d\x0a",
				cgi_code_string[code]);
		cgi_putheader_flg = 1;
	}
}

void put_text_header(void)
{
	int code;
	if(cgi_code < 0 || cgi_code > CGI_UTF8) code = CGI_UTF8 + 1;
	else code = cgi_code;
	if(!cgi_putheader_flg){
		printf("Content-type: text/plane; charset=%s\x0d\x0a\x0d\x0a",
				cgi_code_string[code]);
		cgi_putheader_flg = 1;
	}
}

char *des_crypt(const char *pw, const char *salt, char* buf, int buflen)
{
	register int i, j, c;
	int     temp;
	static char block[66], iobuf[16];
	char *pr;
	
	for(i=0; i < 66; i++) block[i] = 0;
	for(i=0; *pw && i < 64; pw++){
		c = *pw;
		for(j=0; j < 7; j++, i++) block[i] = (c>>(6-j)) & 01;
		i++;
	}
	s_des_setkey(block);
 	for(i=0; i < 66; i++) block[i] = 0;
	for(i=0; i < 2; i++){
		c = *salt++;
		iobuf[i] = c;
		if(c > 'Z') c -= 6;
		if(c > '9') c -= 7;
		c -= '.';
		for(j=0; j < 6; j++){
			if((c>>j) & 01){
				temp = des_e[6*i+j];
				des_e[6*i+j] = des_e[6*i+j+24];
				des_e[6*i+j+24] = temp;
			}
		}
	}
	
	for(i=0; i < 25; i++) s_des_encrypt(block /* , 0 */);
	for(i=0; i < 11; i++){
		c = 0;
		for(j=0; j < 6; j++){
			c <<= 1;
			c |= block[6*i+j];
		}
		c += '.';
		if(c > '9') c += 7;
		if(c > 'Z') c += 6;
		iobuf[i+2] = c;
	}
	iobuf[i+2] = 0;
	
	if(buf != NULL && buflen > 0){
		pr = buf;
		strncpy(buf, iobuf, buflen);
		buf[buflen-1] = '\0';
	}else{
		pr = iobuf;
	}
	return pr;
}

const char *getenv_value(const char *envkey)
{
	const char *p;
	p = getenv(envkey);
	if(p == NULL) return "";
	return p;
}

int char_size(const char *src)
{
	int size = 1;
	int i;
	unsigned char uc;
	uc = (unsigned char)*src;
	if(cgi_code == CGI_SJIS){
		if((uc >= 0x81 && uc <= 0x9f) || (uc >= 0xe0 && uc <= 0xfc)) size = 2;
	}else if(cgi_code == CGI_EUC){
		if((uc >= 0xa1 && uc <= 0xf4) || uc == 0x8e) size = 2;
	}else if(cgi_code == CGI_UTF8){
		if(uc >= 0xc0 && uc <= 0xdf) size = 2;
		else if(uc >= 0xc0 && uc <= 0xdf) size = 3;
		else if(uc >= 0xe0 && uc <= 0xef) size = 3;
		else if(uc >= 0xf0 && uc <= 0xf7) size = 4;
		else if(uc >= 0xf8 && uc <= 0xfb) size = 5;
		else if(uc >= 0xfc && uc <= 0xfd) size = 6;
	}
	for(i=0; i<size; i++){
		if(src[i] == '\0'){
			size = i;
			break;
		}
	}
	return size;
}

char *char_next(const char *src)
{
	return (char*)(src + char_size(src));
}

char *char_prev(const char *sta, const char *cur)
{
	int ad;
	const char *p, *pre;
	for(pre=p=sta; p<cur; p=char_next(p)) pre = p;
	return (char*)pre;
}

char *copystring(const char *str)
{
	char *p;
	p = (char*)cgialloc(strlen(str)+1);
	strcpy(p, str);
	return p;
}

char *addstring(const char *str1, const char *str2)
{
	char *p;
	size_t s;
	s = strlen(str1) + strlen(str2) + 1;
	p = (char*)cgialloc(s);
	strcpy(p, str1);
	strcat(p, str2);
	return p;
}

char *upper_string(char *str)
{
	char *p;
	if(str == NULL) return NULL;
	for(p=str; *p; p=char_next(p)){
		if(char_size(p) == 1) *p = toupper(*p);
	}
	return str;
}

char *lower_string(char *str)
{
	char *p;
	if(str == NULL) return NULL;
	for(p=str; *p; p=char_next(p)){
		if(char_size(p) == 1) *p = tolower(*p);
	}
	return str;
}

int str_icomp(const char *str1, const char *str2)
{
	char *p1, *p2;
	int ret;
	p1 = lower_string(copystring(str1));
	p2 = lower_string(copystring(str2));
	CGI_ASSERT(p1!=NULL && p2!=NULL, メモリ確保失敗); /* メモリ少ないマシン使ってるのが悪い */
	ret = strcmp(p1,p2);
	free(p1);
	free(p2);
	return ret;
}

int str_icompn(const char *str1, const char *str2, size_t len)
{
	char *p1, *p2;
	int ret;
	p1 = lower_string(copystring(str1));
	p2 = lower_string(copystring(str2));
	CGI_ASSERT(p1!=NULL && p2!=NULL, メモリ確保失敗); /* メモリ少ないマシン使ってるのが悪い */
	ret = strncmp(p1,p2,len);
	free(p1);
	free(p2);
	return ret;
}

size_t str_count(const char *str)
{
	const char *p;
	size_t cnt = 0;
	for(p=str; *p; p=char_next(p)) ++cnt;
	return cnt;
}

char *str_isearch(const char *src, const char *search)
{
	const char *p;
	size_t len;
	len = strlen(search);
	if(!len) return (char*)src;
	for(p=src; *p; p=char_next(p)){
		if(tolower(*p) == tolower(*search)){
			if(!str_icompn(p,search,len)) break;
		}
	}
	if(!*p) return NULL;
	return (char*)p;
}

int str_split(const char *base, const char *sep, char quot, int col, char ***cols)
{
	int colcnt, qflg, len, i, j, sp, cl, seplen;
	const char *p, *bp;
	char **pp;
	
	if(!*sep) return 0;
	seplen = strlen(sep);
	for(qflg=0,colcnt=1,p=base; *p; p=char_next(p)){
		if(col > 0 && colcnt >= col) break;
		if(quot && !qflg && *p == quot){
			qflg = 1;
			continue;
		}else if(quot && qflg && *p == quot){
			qflg = 0;
			continue;
		}
		if(!qflg && !strncmp(p,sep,seplen)){
			p += strlen(sep) - 1;
			++colcnt;
		}
	}
	pp = (char**)cgialloc(sizeof(char*)*colcnt);
	for(i=0,bp=base; *bp; bp=p+1){
		if(colcnt > 0 && i >= colcnt) break;
		for(len=qflg=0,p=bp; *p && (strncmp(p,sep,seplen) || qflg); p+=cl){
			cl = char_size(p);
			len += cl;
			if(cl > 1) continue;
			if(quot && !qflg && *p == quot){
				qflg = 1;
				continue;
			}else if(quot && qflg && *p == quot){
				qflg = 0;
				continue;
			}
		}
		pp[i] = (char*)cgialloc(len+1);
		for(sp=qflg=0,p=bp; *p && strncmp(p,sep,seplen) || qflg; p+=cl){
			cl = char_size(p);
			if(cl == 1){
				if(quot && !qflg && *p == quot){
					qflg = 1;
					continue;
				}else if(quot && qflg && *p == quot){
					qflg = 0;
					if(*(p+1) != quot) continue;
				}
			}
			for(j=0;j<cl;++j,++sp){
				pp[i][sp] = *(p+j);
			}
		}
		if(!strncmp(p,sep,seplen)) p = p + seplen - 1;
		pp[i][sp] = '\0';
		++i;
	}
	if(!*bp && colcnt > i){
		pp[i] = (char*)cgialloc(1);
		pp[i][0] = '\0';
	}
	*cols = pp;
	
	return colcnt;
}

void split_free(char **cols, int colsize)
{
	int i;
	for(i=0; i<colsize; ++i){
		free(cols[i]);
	}
	free(cols);
}

char *str_rep(const char *str, const char *before, const char *after, int count)
{
	int cnt;
	size_t blen, alen, newsize;
	const char *p, *bp;
	char *buf, *wp;
	
	if(!(*before) || !strcmp(before,after)) return copystring(str);
	
	blen = strlen(before);
	alen = strlen(after);
	
	if(blen == alen) newsize = strlen(str);
	else{
		/* 置換対象文字列の個数 */
		for(cnt=0,p=strstr(str,before); p!=NULL; p=strstr(p+blen,before)) ++cnt;
		if(count > 0 && cnt > count) cnt = count;
		newsize = strlen(str) + ((alen - blen) * cnt);
	}
	
	buf = (char*)cgialloc(newsize+1);
	for(*buf='\0',cnt=0,wp=buf,bp=str,p=strstr(bp,before); p!= NULL; p=strstr(bp,before)){
		++cnt;
		if(count > 0 && cnt > count) break;
		strncpy(wp, bp, p - bp);
		wp += p - bp;
		strcpy(wp, after);
		wp += alen;
		bp = p + blen;
	}
	strcpy(wp, bp);
	
	return buf;
}

char *str_irep(const char *str, const char *before, const char *after, int count)
{
	int cnt;
	size_t blen, alen, newsize;
	const char *p, *bp;
	char *buf, *wp;
	
	if(!(*before) || !strcmp(before,after)) return copystring(str);
	
	blen = strlen(before);
	alen = strlen(after);
	
	if(blen == alen) newsize = strlen(str);
	else{
		for(cnt=0,p=str_isearch(str,before); p!=NULL; p=str_isearch(p+blen,before)) ++cnt;
		if(count > 0 && cnt > count) cnt = count;
		newsize = strlen(str) + ((alen - blen) * cnt);
	}
	
	buf = (char*)cgialloc(newsize+1);
	for(*buf='\0',cnt=0,wp=buf,bp=str,p=str_isearch(bp,before); p!= NULL; p=str_isearch(bp,before)){
		++cnt;
		if(count > 0 && cnt > count) break;
		strncpy(wp, bp, p - bp);
		wp += p - bp;
		strcpy(wp, after);
		wp += alen;
		bp = p + blen;
	}
	strcpy(wp, bp);
	
	return buf;
}

char *substring(const char *str, size_t start, size_t count)
{
	const char *sp, *ep;
	char *buf;
	size_t cnt, bufsize;
	
	for(sp=str,cnt=0; cnt<start && *sp; ++cnt) sp = char_next(sp);
	if(count){
		for(ep=sp,cnt=0; cnt<count && *ep; ++cnt) ep = char_next(ep);
		bufsize = ep - sp;
	}else{
		bufsize = strlen(sp);
	}
	buf = (char*)cgialloc(bufsize+1);
	strncpy(buf, sp, bufsize);
	buf[bufsize] = '\0';
	return buf;
}

char *bsubstring(const char *str, size_t start, size_t count)
{
	const char *sp, *ep;
	char *buf;
	size_t cnt, bufsize;
	
	for(sp=str,cnt=0; cnt<start && *sp; ++cnt) ++sp;
	if(count){
		for(ep=sp,cnt=0; cnt<count && *ep; ++cnt) ++ep;
		bufsize = ep - sp;
	}else{
		bufsize = strlen(sp);
	}
	buf = (char*)cgialloc(bufsize+1);
	strncpy(buf, sp, bufsize);
	buf[bufsize] = '\0';
	return buf;
}

char *urlenc(const char *str)
{
	size_t newsize = 0;
	const char *p;
	char *buf, *ps;
	for(p=str; *p; ++p){
		if(ISURLCHAR(*p)) continue;
		if(*p == ' ') continue;
		newsize += 2;
	}
	newsize += strlen(str);
	buf = (char*)cgialloc(newsize+1);
	for(ps=buf,p=str; *p; ++p){
		if(ISURLCHAR(*p)){
			*ps++ = *p;
		}else if(*p == ' '){
			*ps++ = '+';
		}else{
			sprintf(ps, "%%%02X", (unsigned char)*p);
			ps += 3;
		}
	}
	buf[newsize] = '\0';
	return buf;
}

char *urldec(char *str)
{
	char *p, *sp;
	for(p=sp=str; *p; ++p){
		if(*p == '%'){
			if(isxdigit(*(p+1)) && isxdigit(*(p+2))){
				*(sp++) = (HEX2DEC(*(p+1)) * 16) + HEX2DEC(*(p+2));
				p += 2;
			}else{
				cgi_errendex(__LINE__, __FILE__, "URL decode error!");
			}
		}else if(*p == '+'){
			*(sp++) = ' ';
		}else{
			*(sp++) = *p;
		}
	}
	*sp = '\0';
	return str;
}

const pcgiform getfirstnode_get(void)
{
	return get_formdata;
}

const pcgiform getfirstnode_post(void)
{
	return post_formdata;
}

const pcgiform search_form(const char *key, const pcgiform pstart)
{
	pcgiform ps;
	if(pstart == NULL) ps = get_formdata;
	else ps = (pcgiform)pstart->next;
	
	for(; ps!=NULL; ps=ps->next){
		if(!strcmp(ps->name,key)) return ps;
	}
	
	if(pstart == NULL || pstart->request_type == CGI_GET) ps = post_formdata;
	for(; ps!=NULL; ps=ps->next){
		if(!strcmp(ps->name,key)) return ps;
	}
	return ps;
}

const char *request(const char *key)
{
	cur_formdata = search_form(key, NULL);
	if(cur_formdata == NULL || cur_formdata->value == NULL) return "";
	return cur_formdata->value;
}

const char *nextrequest(const char *key)
{
	cur_formdata = search_form(key, cur_formdata);
	if(cur_formdata == NULL || cur_formdata->value == NULL) return "";
	return cur_formdata->value;
}

void getformdata(unsigned long long maxsize, cgilib_enctype enctype)
{
	static unsigned getform_callflg = 0;
	if(!exit_handle_flg){
		if(atexit(s_free_form_exit)) cgi_errendex(__LINE__, __FILE__, "atexit error!");
		exit_handle_flg = 1;
	}
	switch(enctype){
		case CGI_URLENCGET:
			if(!(getform_callflg & 0x1)){
				s_parse_urlenc_get(maxsize);
				getform_callflg |= 0x1;
			}
		break;
		case CGI_URLENCPOST:
			if(!(getform_callflg & 0x2)){
				s_parse_urlenc_post(maxsize);
				getform_callflg |= 0x2;
			}
		break;
		case CGI_MULTIPART:
			if(!(getform_callflg & 0x4)){
				s_parse_multipart_post(maxsize);
				getform_callflg |= 0x4;
			}
		break;
		default:
			cgi_errendex(__LINE__, __FILE__, "encode type error!");
		break;
	}
}

char *autolink(const char *str)
{
	size_t addsize, urlsize, totalsize;
	const char *p;
	char *url, *buf, *ps;
	int i;
	
	addsize = strlen("<a href=\"\" target=\"_blank\"></a>");
	for(i=0,totalsize=0; cgi_uri_prefix[i]!=NULL; ++i){
		for(p=strstr(str,cgi_uri_prefix[i]); p!=NULL; p=strstr(p,cgi_uri_prefix[i])){
			urlsize = s_geturllen(p);
			if(urlsize > strlen(cgi_uri_prefix[i])){
				totalsize += (urlsize + addsize);
			}
			p += urlsize;
		}
	}
	totalsize += strlen(str);
	buf = cgialloc(totalsize+1);
	for(ps=buf,p=str; *p; ++p){
		for(i=0;cgi_uri_prefix[i]!=NULL;++i){
			if(!strncmp(p,cgi_uri_prefix[i],strlen(cgi_uri_prefix[i]))){
				urlsize = s_geturllen(p);
				if(urlsize == strlen(cgi_uri_prefix[i])){
					strncpy(ps, p, urlsize);
					ps += urlsize;
					p += (urlsize-1);
					break;
				}
				url = bsubstring(p, 0, urlsize);
				sprintf(ps, "<a href=\"%s\" target=\"_blank\">%s</a>", url, url);
				ps += (urlsize*2+addsize);
				p += (urlsize-1);
				free(url);
				break;
			}
		}
		if(cgi_uri_prefix[i] == NULL) *(ps++) = *p;
	}
	
	buf[totalsize] = '\0';
	return buf;
}

char *base64encode(const char *data, size_t len)
{
	size_t size, i;
	char *p, *buf;
	unsigned const char *ucdata;
	unsigned char wp[4];
	
	if(len < 1) return NULL;
	ucdata = (unsigned char*)data;
	size = (len + 2) / 3;
	size *= 4;
	
	buf = (char*)cgialloc(size+1);
	memset(wp, 0, sizeof(wp));
	memset(buf, 0, size+1);
	for(i=0,p=buf; i<len; i++) {
		if(i%3 == 0 && i > 0) {
			*(p++) = base64chars[wp[0]];
			*(p++) = base64chars[wp[1]];
			*(p++) = base64chars[wp[2]];
			*(p++) = base64chars[wp[3]];
			memset(wp, 0, sizeof(wp));
		}
		switch(i%3) {
			case 0:
				wp[0] =(unsigned char) ucdata[i] >> 2;
				wp[1] =(unsigned char) ucdata[i] << 6;
				wp[1] >>= 2;
			break;
			case 1:
				wp[1] |=(unsigned char) ucdata[i] >> 4;
				wp[2] =(unsigned char) ucdata[i] << 4;
				wp[2] >>= 2;
			break;
			case 2:
				wp[2] |=(unsigned char) ucdata[i] >> 6;
				wp[3] =(unsigned char) ucdata[i] & 0x3F;
			break;
		}
	}
	switch(len%3) {
		case 0:
			*(p++) = base64chars[wp[0]];
			*(p++) = base64chars[wp[1]];
			*(p++) = base64chars[wp[2]];
			*p = base64chars[wp[3]];
		break;
		case 1:
			*(p++) = base64chars[wp[0]];
			*(p++) = base64chars[wp[1]];
			*(p++) = '=';
			*p = '=';
		break;
		case 2:
			*(p++) = base64chars[wp[0]];
			*(p++) = base64chars[wp[1]];
			*(p++) = base64chars[wp[2]];
			*p = '=';
		break;
	}
	return buf;
}

/* base64デコードを行う(バグがあった気がするが治したか覚えてない)  */
size_t base64decode(const char *b64str, char **data)
{
	size_t i, size, len, no = 0;
	char *p, *buf, *cp;
	unsigned char wp[3];
	
	len = strlen(b64str);
	if(len < 1) return 0;
	
	p = (char*)b64str + len - 1;
	while(*p == '=' && p > b64str) {
		p--;
		no++;
	}
	size = ((len - no) * 6) / 8;
	
	buf = cgialloc(size+no+1);
	memset(buf, 0, size+no+1);
	memset(wp, 0, sizeof(wp));
	for(i=0,p=buf; i<len; i++) {
		if(i%4 == 0 && i > 0) {
			*(p++) = wp[0];
			*(p++) = wp[1];
			*(p++) = wp[2];
			memset(wp, 0, sizeof(wp));
		}
		if((cp=(char*)strchr(base64chars,b64str[i])) == NULL) continue;
		switch(i%4) {
			case 0:
				wp[0] = (unsigned char) (cp - base64chars) << 2;
			break;
			case 1:
				wp[0] |= (unsigned char) ((cp - base64chars) >> 4);
				wp[1] = (unsigned char) (cp - base64chars) << 4;
			break;
			case 2:
				wp[1] |= (unsigned char) ((cp - base64chars) >> 2);
				wp[2] = (unsigned char) (cp - base64chars) << 6;
			break;
			case 3:
				wp[2] |= (unsigned char) (cp - base64chars);
			break;
		}
	}
	switch(len%4) {
		case 0:
			*(p++) = wp[0];
			*(p++) = wp[1];
			*p = wp[2];
		break;
		case 1:
			*p = wp[0];
		break;
		case 2:
			*(p++) = wp[0];
			*p = wp[1];
		break;
	}
	*data = buf;
	return size;
}

int set_cookie(const char *key, const char *value, time_t period)
{
	char *enckey, *encvalue;
	const char *month[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	const char *week[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	struct tm *ptm;
	time_t ct;
	
	if(cgi_putheader_flg) return 1;
	enckey = urlenc(key);
	encvalue = urlenc(value);
	printf("Set-Cookie: %s=%s;", enckey, encvalue);
	if(period){
		ct = time(NULL) + period;
		ptm = localtime(&ct);
		printf(" expires=%s, %02d-%s-%04d %02d:%02d:%02d GMT",
				week[ptm->tm_wday], ptm->tm_mday, month[ptm->tm_mon], ptm->tm_year+1900,
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	}
	printf("\x0d\x0a");
	free(enckey);
	free(encvalue);
	return 0;
}

int delete_cookie(const char *key)
{
	char *enckey;
	const char *month[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	const char *week[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	struct tm *ptm;
	time_t ct;
	
	if(cgi_putheader_flg) return 1;
	enckey = urlenc(key);
	printf("Set-Cookie: %s=;", enckey);
	ct = 0;
	ptm = localtime(&ct);
	printf(" expires=%s, %02d-%s-%04d %02d:%02d:%02d GMT",
			week[ptm->tm_wday], ptm->tm_mday, month[ptm->tm_mon], ptm->tm_year+1900,
			ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	printf("\x0d\x0a");
	free(enckey);
	return 0;
}

size_t get_cookie(const char *key, char *value, size_t bufsize)
{
	char **keys, *pv, *pk;
	int keycnt, i;
	size_t ret = 0;
	keycnt = str_split(getenv_value("HTTP_COOKIE"), ";", 0, 0, &keys);
	for(i=0; i<keycnt; ++i){
		if((pv=strchr(keys[i],'=')) != NULL){
			*(pv++) = '\0';
			for(pk=keys[i];*pk&&*pk==' ';++pk);
			urldec(pk);
			urldec(pv);
			if(!strcmp(key,pk)){
				if(bufsize && value != NULL){
					strncpy(value, pv, bufsize);
					value[bufsize-1] = '\0';
					ret = strlen(pv);
					if(ret >= bufsize) ret = bufsize - 1;
				}else{
					ret = strlen(pv);
				}
				break;
			}
		}
	}
	if(!ret && bufsize && value!=NULL) *value = '\0';
	split_free(keys, keycnt);
	return ret;
}

int get_jpeg_size(const unsigned char *jpeg, size_t size, unsigned *width, unsigned *height)
{
	size_t i, s;
	
	*width = 0;
	*height = 0;
	if(size <= 28) return 1;
	s = size - 8;
	for(i=2; i<s && i>0; i++){
		if(jpeg[i] == 0xFF){
			if(jpeg[i+1]>=0xC0 && jpeg[i+1]<=0xC3){
				*width = (unsigned) (jpeg[i+7]<<8) + jpeg[i+8];
				*height = (unsigned) (jpeg[i+5]<<8) + jpeg[i+6];
				return 0;
			}else{
				i = (size_t) i + ((jpeg[i+2]<<8)+jpeg[i+3]) + 1;
			}
		}
	}
	return 1;
}

int get_gif_size(const unsigned char *gif, size_t size, unsigned *width, unsigned *height)
{
	if(size<11||tolower(gif[0])!='g'||tolower(gif[1])!='i'||tolower(gif[2])!='f'){
		*width = 0;
		*height = 0;
		return 1;
	}
	*width = (unsigned) (gif[7] << 8) + gif[6];
	*height = (unsigned) (gif[9] << 8) + gif[8];
	return 0;
}

int get_png_size(const unsigned char *png, size_t size, unsigned *width, unsigned *height)
{
	if(size<24||tolower(png[12])!='i'||tolower(png[13])!='h'||tolower(png[14])!='d'||tolower(png[15])!='r'){
		*width = 0;
		*height = 0;
		return 1;
	}
	*width = png[16] << 24 | png[17] << 16 | png[18] << 8 | png[19];
	*height = png[20] << 24 | png[21] << 16 | png[22] << 8 | png[23];
	return 0;
}

size_t fileread(const char *file, char **buf)
{
	FILE *pf;
	size_t size = 0, rs;
	char rb[4096];
	char *abuf = NULL, *p;
#if defined(_WIN32) || defined(_WIN64)
	pf = fopen(file, "rb");
#else
	pf = fopen(file, "r");
#endif
	if(pf == NULL){
		*buf = abuf;
		return size;
	}
	while((rs=fread(rb,1,sizeof(rb),pf)) > 0){
		if(ferror(pf)){
			*buf = NULL;
			free(abuf);
			fclose(pf);
			return 0;
		}
		p = realloc(abuf,size+rs+1);
		if(p == NULL){
			fclose(pf);
			(*cgi_badalloc_func)();
		}
		abuf = p;
		memcpy(abuf+size, rb, rs);
		size += rs;
		abuf[size] = '\0';
		if(feof(pf)) break;
	}
	*buf = abuf;
	fclose(pf);
	return size;
}

int cgi_sock_init(void)
{
#if defined(_WIN32) || defined(_WIN64)
	int ret;
	if(socket_init_flg) return 0;
	ret = WSAStartup(2, &s_wsadata);
	if(!ret) socket_init_flg = 1;
	return ret;
#else
	socket_init_flg = 1;
	return 0;
#endif
}

int cgi_sock_end(void)
{
#if defined(_WIN32) || defined(_WIN64)
	int ret;
	if(!socket_init_flg) return 0;
	ret = WSACleanup();
	if(!ret) socket_init_flg = 0;
	return ret;
#else
	socket_init_flg = 0;
	return 0;
#endif
}

int is_ipv4addr(const char *addr)
{
	const char *p;
	int state, oc;
	for(state=oc=0,p=addr; *p;){
		switch(state){
			case 0:
				if(*p == '.'){
					state = 1;
					break;
				}
				if(*p < '0' || *p > '9') return 0;
				oc = oc * 10 + (*p-'0');
				if(oc > 255) return 0;
				++p;
			break;
			case 1:
				++p;
				state = oc = 0;
			break;
		}
	}
	return 1;
}

int ipv4_parse(const char *ipv4str, unsigned char ipv4oct[4])
{
	int i, cnt;
	char **ips;
	if(!is_ipv4addr(ipv4str)) return -1;
	cnt = str_split(ipv4str, ".", 0, 0, &ips);
	if(cnt != 4){
		split_free(ips, cnt);
		return -1;
	}
	for(i=0; i<cnt; ++i){
		ipv4oct[i] = (unsigned char)atoi(ips[i]);
	}
	split_free(ips, cnt);
	return 0;
}

extern int host2ipv4addr(const char *host, char *ipv4, size_t len)
{
	char ipv4buf[32];
	unsigned char ipv4oct[4];
	struct hostent *he;
	
	if(len < 1) return 1;
	if(ipv4_parse(host,ipv4oct)){
		if((he=gethostbyname(host)) == NULL){
			*ipv4 = '\0';
			return -1;
		}
		if(he->h_length != 4 || he->h_addr_list[0] == NULL){
			*ipv4 = '\0';
			return -1;
		}
		ipv4oct[0] = (unsigned char)*(he->h_addr_list[0]);
		ipv4oct[1] = (unsigned char)*((he->h_addr_list[0])+1);
		ipv4oct[2] = (unsigned char)*((he->h_addr_list[0])+2);
		ipv4oct[3] = (unsigned char)*((he->h_addr_list[0])+3);
	}
	sprintf(ipv4buf, "%u.%u.%u.%u", ipv4oct[0], ipv4oct[1], ipv4oct[2], ipv4oct[3]);
	strncpy(ipv4, ipv4buf, len);
	ipv4[len-1] = '\0';
	if(strlen(ipv4buf) >= len) return 1;
	return 0;
}

int ipv4addr2host(const char *ipv4, char *host, size_t len)
{
	unsigned char ipv4oct[4];
	struct hostent *he;
	unsigned int ip;
	
	if(len < 1) return 1;
	if(!is_ipv4addr(ipv4)){
		*host = '\0';
		return -1;
	}
	ip = inet_addr(ipv4);
	if((he=gethostbyaddr((const char *)&ip, 4, AF_INET)) == NULL){
		strncpy(host, ipv4, len);
		host[len-1] = '\0';
		return -1;
	}
	strncpy(host, he->h_name, len);
	host[len-1] = '\0';
	if(strlen(he->h_name) >= len) return 1;
	return 0;
}

int filecpy(const char *destfile, char *srcfile)
{
	FILE *pfi, *pfo;
	char buf[4096];
	int ret = 0;
	size_t rs;
#if defined(_WIN32) || defined(_WIN64)
	const char *rmode = "rb", *wmode = "wb";
#else
	const char *rmode = "r", *wmode = "w";
#endif
	if((pfi=fopen(srcfile,rmode)) == NULL) return 1;
	if((pfo=fopen(destfile,wmode)) == NULL){
		fclose(pfi);
		return 1;
	}
	while((rs=fread(buf,1,sizeof(buf),pfi)) > 0){
		if(fwrite(buf,rs,1,pfo) == 0){
			ret = -1;
			break;
		}
	}
	if(ferror(pfi)) ret = 1;
	if(ferror(pfo)) ret = -1;
	fclose(pfi);
	fclose(pfo);
	return ret;
}



void cgi_errend(const char *msg, ...)
{
	va_list list;
	
	put_html_header();
	va_start(list, msg);
	vprintf(msg, list);
	va_end(list);
	exit(1);
}

void cgi_errendex(int line, const char *file, const char *msg, ...)
{
	va_list list;
	
	put_html_header();
	if(line) printf("%s(%d)\n", file, line);
	va_start(list, msg);
	vprintf(msg, list);
	va_end(list);
	exit(1);
}

static void s_des_setkey(const char *key)
{
	register int i, j, k;
	int t;
	
	memset(des_l, 0, sizeof des_l);
	memset(des_r, 0, sizeof des_r);
	memset(des_templ, 0, sizeof des_templ);
	memset(des_f, 0, sizeof des_f);
	memset(des_pre_s, 0, sizeof des_pre_s);
	
	for(i=0; i < 28; i++){
		des_c[i] = key[des_pc1_c[i]-1];
		des_d[i] = key[des_pc1_d[i]-1];
	}	
	for(i=0; i < 16; i++){
		for(k=0; k < des_shifts[i]; k++){
			t = des_c[0];
			for(j=0; j < 28-1; j++) des_c[j] = des_c[j+1];
			des_c[27] = t;
			t = des_d[0];
			for(j=0; j < 28-1; j++) des_d[j] = des_d[j+1];
			des_d[27] = t;
		}
		for(j=0; j < 24; j++){
			des_ks[i][j] = des_c[des_pc2_c[j]-1];
			des_ks[i][j+24] = des_d[des_pc2_d[j]-28-1];
		}
	}
	
	for(i=0; i < 48; i++) des_e[i] = des_e2[i];
}

static void s_des_encrypt(char *block /* , int edflg */)
{
	int     i;
	register int t, j, k;

	for(j=0; j < 64; j++) des_l[j] = block[des_ip[j]-1];
	for(i=0; i < 16; i++){
		for(j=0; j < 32; j++) des_templ[j] = des_r[j];
		for(j=0; j < 48; j++) des_pre_s[j] = des_r[des_e[j]-1] ^ des_ks[i][j];
		for(j=0; j < 8; j++){
			t = 6 * j;
			k = des_s[j][(des_pre_s[t+0]<<5)+
				(des_pre_s[t+1]<<3)+
				(des_pre_s[t+2]<<2)+
				(des_pre_s[t+3]<<1)+
				(des_pre_s[t+4]<<0)+
				(des_pre_s[t+5]<<4)];
			t = 4 * j;
			des_f[t+0] = (k>>3)&01;
			des_f[t+1] = (k>>2)&01;
			des_f[t+2] = (k>>1)&01;
			des_f[t+3] = (k>>0)&01;
		}
		for(j=0; j < 32; j++) des_r[j] = des_l[j] ^ des_f[des_p[j]-1];
		for(j=0; j < 32; j++) des_l[j] = des_templ[j];
	}
	for(j=0; j < 32; j++){
		t = des_l[j];
		des_l[j] = des_r[j];
		des_r[j] = t;
	}
	for(j=0; j < 64; j++) block[j] = des_l[des_fp[j]-1];
}

static void s_insert_formdata(pcgiform node)
{
	pcgiform pf;
	if(node->request_type == CGI_GET){
		if(get_formdata == NULL){
			get_formdata = node;
			get_formdata->next = NULL;
			get_formdata->prev = NULL;
		}else{
			for(pf=get_formdata; pf->next!=NULL; pf=pf->next);
			pf->next = node;
			node->prev = pf;
			node->next = NULL;
		}
	}else{
		if(post_formdata == NULL){
			post_formdata = node;
			post_formdata->next = NULL;
			post_formdata->prev = NULL;
		}else{
			for(pf=post_formdata; pf->next!=NULL; pf=pf->next);
			pf->next = node;
			node->prev = pf;
			node->next = NULL;
		}
	}
}

static void s_clear_formdata(void)
{
	char tmpfl[CGILIB_MAXTDIRNAME+CGILIB_MAXSVFLNAME+2];
	pcgiform pf, wpf;
	for(pf=get_formdata; pf!=NULL; ){
		wpf = pf->next;
		free(pf);
		pf = wpf;
	}
	get_formdata = NULL;
	free(get_buffer);
	get_buffer = NULL;
	for(pf=post_formdata; pf!=NULL; ){
		if(pf->request_type == CGI_POSTM) free(pf->value);
		if(pf->svfile[0]){
			sprintf(tmpfl, "%s/%s", cgi_tempdir, pf->svfile);
			remove(tmpfl);
		}
		wpf = pf->next;
		free(pf);
		pf = wpf;
	}
	post_formdata = NULL;
	free(post_buffer);
	post_buffer = NULL;
}

static void s_parse_urlenc_get(unsigned long long maxsize)
{
	pcgiform pform;
	const char *query_string;
	char *p, *bp;
	unsigned long long recvsize;
	
	query_string = getenv_value("QUERY_STRING");
	recvsize = strlen(query_string);
	if(recvsize > maxsize) cgi_errendex(__LINE__, __FILE__, "data size over!");
	get_buffer = copystring(getenv_value("QUERY_STRING"));
	
	for(p=bp=get_buffer; *p; ++p){
		if(*p == '&'){
			*p = '\0';
			pform = cgialloc(sizeof(cgiform));
			if((pform->value=strchr(bp,'=')) == NULL) pform->value = p;
			else *(pform->value++) = '\0';
			pform->request_type = CGI_GET;
			pform->mime[0] = '\0';
			pform->filename[0] = '\0';
			pform->svfile[0] = '\0';
			urldec(bp);
			strncpy(pform->name, bp, CGILIB_MAXPARANAME);
			pform->name[CGILIB_MAXPARANAME] = '\0';
			urldec(pform->value);
			pform->valuesize = strlen(pform->value);
			s_insert_formdata(pform);
			bp = p + 1;
		}
	}
	if(*bp){
		pform = cgialloc(sizeof(cgiform));
		if((pform->value=strchr(bp,'=')) == NULL) pform->value = p;
		else *(pform->value++) = '\0';
		pform->request_type = CGI_GET;
		pform->mime[0] = '\0';
		pform->filename[0] = '\0';
		pform->svfile[0] = '\0';
		urldec(bp);
		strncpy(pform->name, bp, CGILIB_MAXPARANAME);
		pform->name[CGILIB_MAXPARANAME] = '\0';
		urldec(pform->value);
		pform->valuesize = strlen(pform->value);
		s_insert_formdata(pform);
	}
}

static void s_parse_urlenc_post(unsigned long long maxsize)
{
	pcgiform pform;
	char *p, *bp;
	unsigned long long recvsize;
	
	recvsize = cgi_strtoull(getenv_value("CONTENT_LENGTH"), NULL, 10);
	if(recvsize > maxsize) cgi_errendex(__LINE__, __FILE__, "data size over!");
	post_buffer = cgialloc((size_t)recvsize+1);
	if(fread(post_buffer,(size_t)recvsize,1,stdin) < 1) cgi_errendex(__LINE__, __FILE__, "stdin read error!");
	post_buffer[(size_t)recvsize] = '\0';
	for(p=bp=post_buffer; *p; ++p){
		if(*p == '&'){
			*p = '\0';
			pform = cgialloc(sizeof(cgiform));
			if((pform->value=strchr(bp,'=')) == NULL) pform->value = p;
			else *(pform->value++) = '\0';
			pform->request_type = CGI_POST;
			pform->mime[0] = '\0';
			pform->filename[0] = '\0';
			pform->svfile[0] = '\0';
			urldec(bp);
			strncpy(pform->name, bp, CGILIB_MAXPARANAME);
			pform->name[CGILIB_MAXPARANAME] = '\0';
			urldec(pform->value);
			pform->valuesize = strlen(pform->value);
			s_insert_formdata(pform);
			bp = p + 1;
		}
	}
	if(*bp){
		pform = cgialloc(sizeof(cgiform));
		if((pform->value=strchr(bp,'=')) == NULL) pform->value = p;
		else *(pform->value++) = '\0';
		pform->request_type = CGI_POST;
		pform->mime[0] = '\0';
		pform->filename[0] = '\0';
		pform->svfile[0] = '\0';
		urldec(bp);
		strncpy(pform->name, bp, CGILIB_MAXPARANAME);
		pform->name[CGILIB_MAXPARANAME] = '\0';
		urldec(pform->value);
		pform->valuesize = strlen(pform->value);
		s_insert_formdata(pform);
	}
}

static void s_parse_multipart_post(unsigned long long maxsize)
{
	pcgiform pform;
	char crlf[2];
	char rbuf[4096], tmpfile[CGILIB_MAXTDIRNAME+CGILIB_MAXSVFLNAME+1];
	char *p, *p2, *p3, *boundary, *endboundary, *name, *fname, *pvs;
	char rbc;
	int i, rflg, rs, fcnt, crf;
	unsigned long long recvsize;
	size_t crpadding;
	FILE *pftmp;
	
	recvsize = cgi_strtoull(getenv_value("CONTENT_LENGTH"), NULL, 10);
	if(recvsize > maxsize) cgi_errendex(__LINE__, __FILE__, "data size over!");
	
#if defined(_WIN32) || defined(_WIN64)
	/* winは標準入力がデフォルトでテキストモードなのでバイナリへ */
	if(setmode(fileno(stdin),O_BINARY) == -1) cgi_errendex(__LINE__, __FILE__, "setmode error!");
#endif
	
	if(s_getline_file(rbuf,sizeof(rbuf),stdin) == 0) cgi_errendex(__LINE__, __FILE__, "bad encode");
	rs = strlen(rbuf);
	if(rs < 2) cgi_errendex(__LINE__, __FILE__, "bad encode");
	if(rbuf[rs-2] != 0x0d || rbuf[rs-1] != 0x0a) cgi_errendex(__LINE__, __FILE__, "bad encode");
	if(!memcmp(rbuf,"\x0d\x0a",2)) rflg = 4;
	else rflg = 1;
	boundary = copystring(rbuf);
	rbuf[rs-2] = '\0'; 
	endboundary = addstring(rbuf, "--\x0d\x0a");
	for(rbc=0,crpadding=0,fcnt=crf=0,name=fname=NULL,pftmp=NULL;rflg!=4;){
		switch(rflg){
			case 1:
				if(s_getline_file(rbuf,sizeof(rbuf),stdin)==0){
					CGILIB_TMPDEL(pftmp);
					cgi_errendex(__LINE__,__FILE__,"bad encode");
				}
				if(rbuf[strlen(rbuf)-1] != 0x0a){
					CGILIB_TMPDEL(pftmp);
					cgi_errendex(__LINE__,__FILE__,"bad encode");
				}
				if(str_icompn(rbuf,"Content-Disposition",19)){
					CGILIB_TMPDEL(pftmp);
					cgi_errendex(__LINE__,__FILE__,"bad encode");
				}
				if((p=str_isearch(&rbuf[19],"filename=\"")) != NULL){
					p3 = p + 4;
					p2 = p + 10;
					for(p=p2; *p&&*p!='"'; ++p);
					if(*p2 == '"') fname = NULL;
					else fname = bsubstring(p2, 0, p - p2);
				}
				if((p=str_isearch(&rbuf[19],"name=\"")) != NULL){
					if(p == p3) p = str_isearch(p+1, "name=\"");
					if(p != NULL){
						p2 = p + 6;
						for(p=p2; *p&&*p!='"'; ++p);
						if(*p2 == '"') name = NULL;
						else name = bsubstring(p2, 0, p - p2);
					}else{
						CGILIB_TMPDEL(pftmp);
						cgi_errendex(__LINE__,__FILE__,"bad encode");
					}
				}
				pform = (pcgiform)cgialloc(sizeof(cgiform));
				pform->request_type = CGI_POSTM;
				if(name != NULL){
					strncpy(pform->name, name, CGILIB_MAXPARANAME);
					pform->name[CGILIB_MAXPARANAME] = '\0';
				}else pform->name[0] = '\0';
				pform->value = NULL;
				pform->mime[0] = '\0';
				if(fname != NULL){
					strncpy(pform->filename, fname, CGILIB_MAXFILENAME);
					pform->filename[CGILIB_MAXFILENAME] = '\0';
				}else pform->filename[0] = '\0';
				pform->valuesize = 0LL;
				free(name);
				free(fname);
				name = fname = NULL;
				rflg = 2;
			break;
			case 2:
				if(s_getline_file(rbuf,sizeof(rbuf),stdin)==0){
					CGILIB_TMPDEL(pftmp);
					cgi_errendex(__LINE__,__FILE__,"bad encode");
				}
				if(rbuf[strlen(rbuf)-1] != 0x0a){
					CGILIB_TMPDEL(pftmp);
					cgi_errendex(__LINE__,__FILE__,"bad encode");
				}
				if(!str_icompn(rbuf,"Content-type",12)){
					/* MIMEを取得 */
					for(p=rbuf+12; *p&&(*p==':'||*p==' '); ++p);
					strncpy(pform->mime, p, CGILIB_MAXMIMENAME);
					pform->mime[CGILIB_MAXMIMENAME] = '\0';
					for(p=pform->mime; *p&&*p!=' '&&*p!=0x0d; ++p);
					*p = '\0';
				}else if(strcmp(rbuf,"\x0d\x0a")){
					CGILIB_TMPDEL(pftmp);
					cgi_errendex(__LINE__, __FILE__, "bad encode");
				}else{
					if(*(pform->filename)){
						sprintf(pform->svfile, "%s%d_%d",
								CGILIB_TMPFILEBASE,
								getpid(), ++fcnt);
						sprintf(tmpfile, "%s/%s",
								cgi_tempdir, pform->svfile);
						if(pftmp != NULL) fclose(pftmp);
#if defined(_WIN32) || defined(_WIN64)
						pftmp = fopen(tmpfile, "wb");
#else
						pftmp = fopen(tmpfile, "w");
#endif
						if(pftmp == NULL){
							CGILIB_TMPDEL(pftmp);
							cgi_errendex(__LINE__, __FILE__, "tmpfile open error");
						}
					}else{
						pform->svfile[0] = '\0';
						if(pftmp != NULL) fclose(pftmp);
						pftmp = NULL;
					}
					rflg = 3;
				}
			break;
			case 3:
				if((rs=s_getline_file(rbuf+crpadding,sizeof(rbuf)-1-crpadding,stdin)+crpadding) == 0){
					CGILIB_TMPDEL(pftmp);
					cgi_errendex(__LINE__, __FILE__, "bad encode");
				}
				if(crpadding){
					rbuf[0] = rbc;
					crpadding = 0;
					rbc = '\0';
				}
				if(rbuf[rs-1] == 0x0d){
					rbuf[rs] = getc(stdin);
					if(feof(stdin)){
						CGILIB_TMPDEL(pftmp);
						cgi_errendex(__LINE__, __FILE__, "bad encode");
					}
					if(ferror(stdin)){
						CGILIB_TMPDEL(pftmp);
						cgi_errendex(__LINE__, __FILE__, "stdin error");
					}
					if(rbuf[rs] != 0x0a){
						rbc = rbuf[rs];
						rbuf[rs] = '\0';
						crpadding = 1;
					}else{
						++rs;
					}
				}
				
				if(!memcmp(rbuf,endboundary,strlen(endboundary))){
					if(pftmp != NULL){
						fclose(pftmp);
						pftmp = NULL;
					}
					s_insert_formdata(pform);
					rflg = 4;
				}else if(!memcmp(rbuf,boundary,strlen(boundary))){
					if(pftmp != NULL){
						fclose(pftmp);
						pftmp = NULL;
					}
					s_insert_formdata(pform);
					crf = 0;
					rflg = 1;
				}else{
					if(crf){
						memcpy(crlf, "\x0d\x0a", 2);
						s_write_formvalue(pform, crlf, 2, pftmp);
					}
					if(rs > 1 && rbuf[rs-1] == 0x0a && rbuf[rs-2] == 0x0d){
						rs -= 2;
						crf = 1;
					}else{
						crf = 0;
					}
					s_write_formvalue(pform, rbuf, rs, pftmp);
				}
			break;
		}
	}
	if(rs == 0){
		CGILIB_TMPDEL(pftmp);
		cgi_errendex(__LINE__, __FILE__, "bad eof?");
	}
	
	free(boundary);
	free(endboundary);
}

static int s_getline_file(char *buf, size_t size, FILE *file)
{
	int ret = 0, r;
	size_t i;
	memset(buf, 0, size);
	if(size < 1) return 0;
	for(i=0; i<(size-1); ++i){
		r = getc(file);
		if(feof(file)) break;
		buf[i] = (char)r;
		++ret;
		if(r == 0x0a) break;
	}
	return ret;
}

static void s_write_formvalue(pcgiform pform, char *rbuf, int rs, FILE *pftmp)
{
	char *pvs;
	
	if(rs < 1) return;
	if(pftmp != NULL){
		if(cgi_upload_func != NULL){
			cgi_upload_func(pform, rbuf, rs);
		}
		if(fwrite(rbuf,rs,1,pftmp)<1) cgi_errendex(__LINE__, __FILE__, "tmpfile write error");
		pform->valuesize += rs;
	}else{
		pvs = realloc(pform->value, (size_t)pform->valuesize + rs + 1);
		if(pvs == NULL) (*cgi_badalloc_func)();
		pform->value = pvs;
		memcpy(pform->value + (size_t)pform->valuesize, rbuf, rs);
		pform->valuesize += rs;
		pform->value[(size_t)pform->valuesize] = '\0';
	}
}

static size_t s_geturllen(const char *buf)
{
	const char *p;
	size_t ret = 0;
	for(p=buf; *p; ++p){
		if((*p >= 'a' && *p <= 'z')
		|| (*p >= '0' && *p <= '9')
		|| (*p >= 'A' && *p <= 'Z')
		|| (*p == '#' || *p == '@'
		|| *p == '+' || *p == '-'
		|| *p == '/' || *p == '_'
		|| *p == '~' || *p == '%'
		|| *p == ':' || *p == ';'
		|| *p == '.' || *p == ','
		|| *p == '?' || *p == '='
		|| *p == '&' || *p == ' ')) ++ret;
		else break;
	}
	return ret;
}


static void s_cgi_badalloc(void)
{
	put_html_header();
	puts("<b><font color=\"#ff0000\">メモリが確保できませんでした。</font></b>");
	exit(1);
}

static void s_free_form_exit(void)
{
	s_clear_formdata();
}
