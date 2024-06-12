#ifndef __INC_UPLD__
#define __INC_UPLD__
#define UPLD_CGINAME			"upld.cgi"
#define UPLD_TITLE				"アップローダー"
#define UPLD_LOCKDIR			"../../upld2/_%s.locked"
#define UPLD_MAXSENDSIZE		((1ULL<<30)+4096)
#define UPLD_MAXDATACNT			100
#define UPLD_LISTFILE			"../../upld/_%s.dat"
#define UPLD_LISTTEMP			"../../upld/_%s.tmp"
/* #define UPLD_UPDIR				"../../upld/upfiles" */
#define UPLD_UPDIR				"../../upld_files"
#define UPLD_TMPDIR				"../../upld_tmp"

#define UPLD_SALT				"&bVqN--@7"

#define UPLD_PARA_UPLOAD		10
#define UPLD_PARA_DOWNLOAD		20
#define UPLD_PARA_PREVIEW		25
#define UPLD_PARA_DELETE		30

#define UPLD_SIZE(s) ((s)>=(1ULL<<10)?((s)>=(1LL<<20)?((s)>=(1ULL<<30)?((double)(s)/(1ULL<<30)):((double)(s)/(1ULL<<20))):((double)(s)/(1ULL<<10))):(s))
#define UPLD_SIZEU(s) ((s)>=(1ULL<<10)?((s)>=(1ULL<<20)?((s)>=(1ULL<<30)?"<font color=\"#0000ff\">GB</font>":"<font color=\"#008000\">MB</font>"):"<font color=\"#ff8000\">KB</font>" ):" B")
#define UPLD_SIZE2(s) ((s)>=(1ULL<<10)?((s)>=(1ULL<<20)?((s)>=(1ULL<<30)?((s)/(1ULL<<30)):((s)/(1ULL<<20))):((s)/(1ULL<<10))):(s))
#define UPLD_SIZEU2(s) ((s)>=(1ULL<<10)?((s)>=(1ULL<<20)?((s)>=(1ULL<<30)?"GB":"MB"):"KB" ):"B")

#if defined(_WIN32) || defined(_WIN64)
#define UPLD_F_READ			"rb"
#define UPLD_F_WRITE		"wb"
#define UPLD_F_APPEND		"ab"
#else
#define UPLD_F_READ			"r"
#define UPLD_F_WRITE		"w"
#define UPLD_F_APPEND		"a"
#endif

typedef struct _upld_filelist{
	unsigned long 		id;
	char				filename[256];
	char				mime[256];
	char				filehash[64];
	char				pwdhash[64]; /* 今は使わない */
	unsigned long long	size;
} filelist;

void base_screen(void);
char *screen_size(unsigned long size);
unsigned long get_listcount(void);
void upload_file(void);
void lock_file(void);
void unlock_file(void);
int id_check(unsigned long id);
void download_file(void);
void preview_file(void);
void delete_file(void);
int get_file_list(filelist *pfl, unsigned long id);
#endif
