#include "cgilib.h"
#include "upld.h"
#include "md5.h"
#include <string.h>
#include <ctype.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

static int param;
static unsigned long g_id;
static int method;
static const char *remote_user;
static char lock_dir[512];
static char list_file[512];
static char list_tmp[512];
static unsigned char xor_shuffle[48];
static unsigned long long shuffle_counter;
static md5_context md5_ctx;

static void s_upload_handler(const pcgiform pform, char* buf, int buflen);
static void s_make_xor_shuffle(void);
static void shuffle_data(char *data, int len);

int main(void)
{
	FILE *pfc;
	remote_user = getenv_value("REMOTE_USER");
	s_make_xor_shuffle();
	set_upload_handler(s_upload_handler);
	settempdir(UPLD_TMPDIR);
	if(str_icomp(getenv_value("REQUEST_METHOD"),"GET")){
		getformdata(UPLD_MAXSENDSIZE, CGI_MULTIPART);
		method = 1;
	}else{
		getformdata(UPLD_MAXSENDSIZE, CGI_URLENCGET);
		method = 0;
	}
	sprintf(lock_dir, UPLD_LOCKDIR, remote_user);
	sprintf(list_file, UPLD_LISTFILE, remote_user);
	sprintf(list_tmp, UPLD_LISTTEMP, remote_user);
	if((pfc=fopen(list_file,UPLD_F_APPEND)) != NULL) fclose(pfc);
	param = atoi(request("param"));
	g_id = strtoul(request("id"),NULL,10);
	switch(param){
		case UPLD_PARA_UPLOAD:
			upload_file();
			printf("Location: %s\x0d\x0a\x0d\x0a", UPLD_CGINAME);
		break;
		case UPLD_PARA_DOWNLOAD:
			download_file();
		break;
		case UPLD_PARA_PREVIEW:
			preview_file();
		break;
		case UPLD_PARA_DELETE:
			delete_file();
			printf("Location: %s\x0d\x0a\x0d\x0a", UPLD_CGINAME);
		break;
		default:
			base_screen();
		break;
	}
	return 0;
}

void base_screen(void)
{
	FILE *pf;
	filelist fl;
	const char *splcolor;
	int clflg, cnt=1, i;
	char datestr[64];
	char mime[128];
	struct tm *ptm;
	unsigned long fc;
	
	fc = get_listcount();
	pf = fopen(list_file,UPLD_F_READ);
	if(pf==NULL) cgi_errend("ファイルが開けません。");
	put_html_header();
	printf(
		"<html>\n"
		"<head>\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=Shift_JIS\">\n"
		"<script language=\"javascript\" src=\"chk.js\"></script>\n"
		"</head>\n"
		"<title>%s</title>\n"
		"<style type=\"text/css\">\n"
		"h1{font-size: 14pt; color: #000000;}\n"
		"h2,ul,dt{font-size: 12pt;color:#00000;}\n"
		"h3{font-size: 12pt; color: #000000;}\n"
		"div{font-size: 10pt;  color: #000000;}\n"
		"table,tbody,tr{font-size: 10pt; color: #000000; border-color: #3080ff; background-color: #c0e0ff; border-width: 0px 1px 1px 0px; border-style: solid;}\n"
		"td{font-size: 10pt;color: #555555; border-color: #3080ff; border-width: 1px 0px 0px 1px; border-style: solid;}\n"
		"hr{border: 0; border-bottom: 1px dashed #c0ccff; background: #3080ff;}\n"
		"body{font-size: 9pt; color:#000000; background-color: #f0f0f0f; }\n"
		"a{text-decoration: none;}\n"
		"a:visited{color: #0000ff;}\n"
		"a:link{color: #0000ff;}\n"
		"a:hover{color: #ff0000;}\n"
		"</style>\n"
		"<body>\n"
		"<div>\n"
		"<h1><a href=\"%s\">%s</a></h1>\n"
		"<b>%s</b><br>\n"
		"<hr>\n"
		"<div id=\"msg1\">ファイルのサイズは最大で<b>%llu%s</b>程度まで<img src=\"./wait.gif\" width=\"0\" height=\"0\"></div>\n",
		UPLD_TITLE,
		UPLD_CGINAME, UPLD_TITLE,
		*remote_user ? remote_user : "<font color=\"#ff0000\">shared file list</font>",
		UPLD_SIZE2(UPLD_MAXSENDSIZE-4096), UPLD_SIZEU2(UPLD_MAXSENDSIZE-4096)
	);
	printf(
		"<form name=\"upl\" action=\"%s\" method=\"POST\" enctype=\"multipart/form-data\" onsubmit=\"return UplCheck();\">\n"
		"アップロードファイル：<input type=\"file\" name=\"upfile\" size=\"50\">\n"
		"<input type=\"hidden\" name=\"param\" value=\"%d\">\n"
		"<input type=\"submit\" name=\"up\" value=\"アップロード\"></form>\n"
		"<hr><br><br>\n",
		UPLD_CGINAME,
		UPLD_PARA_UPLOAD
	);
	printf("ファイル数 : <b>%lu/%lu</b><br>\n", fc, UPLD_MAXDATACNT);
	if(fc >= UPLD_MAXDATACNT){
		printf(
			"<font color=\"#ff0000\">ファイル数の上限が<b>%lu</b>のため次のアップロードで<br>"
			"一番古いファイルが削除されます。</font>",
			UPLD_MAXDATACNT
		);
	}
	printf(
		"<form name=\"del\" action=\"%s\" method=\"POST\" enctype=\"multipart/form-data\" onsubmit=\"return DelCheck();\">\n",
		UPLD_CGINAME
	);
	
	printf(
		"<table cellspacing=\"0\" cellpadding=\"3\"><tbody>\n"
		"<tr>"
		"<td align=\"center\"><font color=\"#3080ff\"><b>No</b></font></td>"
		"<td align=\"center\"><font color=\"#3080ff\"><b>削除</b></font></td>"
		"<td align=\"center\"><font color=\"#3080ff\"><b>ファイル</b></font></td>"
		"<td align=\"center\"><font color=\"#3080ff\"><b>サイズ</b></font></td>"
		"<td align=\"center\"><font color=\"#3080ff\"><b>日付</b></font></td>"
		"<td align=\"center\"><font color=\"#3080ff\"><b>MIME</b></font></td>"
		"<td align=\"center\"><font color=\"#3080ff\"><b>MD5</b></font></td></tr>"
		"</tr>\n"
	);
	
	
	clflg = 0;
	while(fread(&fl,sizeof(fl),1,pf)==1){
		shuffle_data(fl.filename, sizeof(fl.filename));
		shuffle_data(fl.mime, sizeof(fl.mime));
		for(i=0; i<sizeof(mime)-1; ++i){
			if(!fl.mime[i] || fl.mime[i]=='/') break;
			mime[i] = tolower(fl.mime[i]);
		}
		mime[i] = '\0';
		clflg = !clflg;
		if(clflg) splcolor = "#ffffff";
		else splcolor = "#e0fff0";
		ptm = localtime((time_t*)&fl.id);
		sprintf(datestr,"%04d/%02d/%02d %02d:%02d:%02d",
				ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		printf(
			"<tr>"
			"<td align=\"right\" nowrap bgcolor=\"%s\">%d</d>"
			"<td nowrap bgcolor=\"%s\"><input type=\"checkbox\" name=\"delkey\" value=\"%lu\"></td>"
			"<td nowrap bgcolor=\"%s\"><a href=\"%s?param=%d&id=%lu\" target=\"_blank\">●</a> / <a href=\"%s?param=%d&id=%lu\">%s</a></td>"
			"<td align=\"right\" nowrap bgcolor=\"%s\">%s</td>"
			"<td align=\"center\" nowrap bgcolor=\"%s\">%s</td>"
			"<td nowrap bgcolor=\"%s\">%s</td>"
			"<td nowrap bgcolor=\"%s\">%s</td>"
			"</tr>\n",
			splcolor, cnt++,
			splcolor, fl.id,
			splcolor, UPLD_CGINAME, strncmp(mime,"image",5) ? UPLD_PARA_DOWNLOAD : UPLD_PARA_PREVIEW, fl.id, UPLD_CGINAME, UPLD_PARA_DOWNLOAD, fl.id,fl.filename,
			splcolor, screen_size(fl.size),
			splcolor, datestr,
			splcolor, fl.mime,
			splcolor, fl.filehash
		);
	}
	if(ferror(pf)) cgi_errend("入力ファイルでエラーが発生");
	fclose(pf);
	printf(
		"</tbody></table><br><br>\n"
		"<hr>\n"
		"<input type=\"hidden\" name=\"param\" value=\"%d\">\n"
		"<input type=\"submit\" name=\"dlt\" value=\"削除\">"
		"</form>\n"
		"<hr>\n",
		UPLD_PARA_DELETE
	);
	printf(
		"</div>\n"
		"</body>\n</html>\n"
	);
}

char *screen_size(unsigned long size)
{
	static char sizestr[32];
	char buf[24], *p;
	
	if(size < (1<<10)){
		sprintf(sizestr, "<b>%lu B</b>", size);
	}else{
		sprintf(buf, "%.2f", UPLD_SIZE(size));
		if((p=strchr(buf,'.'))!=NULL){
			if(!strncmp(p+1,"00",2)) *p = '\0';
		}
		sprintf(sizestr, "<b>%s %s</b>", buf, UPLD_SIZEU(size));
	}
	return sizestr;
}

unsigned long get_listcount(void)
{
	long po;
	FILE *pf;
	if((pf=fopen(list_file,UPLD_F_READ)) == NULL) cgi_errend("リストファイルが開けません");
	if(fseek(pf,0,SEEK_END)) cgi_errend("fseekエラー");
	if((po=ftell(pf)) < 0) cgi_errend("ファイルサイズ取得エラー");
	fclose(pf);
	return ((unsigned long)po/sizeof(filelist));
}

void upload_file(void)
{
	FILE *ptemp, *pfile;
	pcgiform pform;
	char tmpfile[512], upfile[512], *p, buf[32], delfile[512];
	unsigned char digest[16];
	unsigned long fileid;
	int i, len;
	filelist fl;
	srand((unsigned int)time(NULL));
	
	if(method == 0) cgi_errend("GETではファイルはうｐできませんが。");
	pform = search_form("upfile",NULL);
	if(pform == NULL) cgi_errend("パラメータエラー");
	if(!pform->valuesize) cgi_errend("ファイルサイズがゼロです。");
	if(!pform->svfile[0]) cgi_errend("ファイルではありません。");
	gettempdir(tmpfile,200);
	strcat(tmpfile,"/");
	strcat(tmpfile,pform->svfile);
	lock_file();
	fileid = time(NULL);
	while(id_check(fileid)){
		sleep(1);
		fileid = time(NULL);
	}
	sprintf(upfile, "%s/%s_%lu", UPLD_UPDIR, remote_user, fileid);
	if((ptemp=fopen(list_tmp,UPLD_F_WRITE)) == NULL){
		unlock_file();
		remove(tmpfile);
		cgi_errend("ファイルリストのオープンに失敗（書き込みできません）");
	}
	if((pfile=fopen(list_file,UPLD_F_READ)) == NULL){
		unlock_file();
		remove(tmpfile);
		cgi_errend("ファイルリストのオープンに失敗（書き込みできません）");
	}
	memset(&fl, 0, sizeof(fl));
	fl.id = fileid;
	for(p=pform->filename+strlen(pform->filename)-1;p!=pform->filename;p=char_prev(pform->filename,p)){
		if(*p == '\\' || *p == '/' || *p == ':'){
			++p;
			break;
		}
	}
	strncpy(fl.filename, p, sizeof(fl.filename));
	fl.filename[sizeof(fl.filename)-1] = '\0';
	len = strlen(fl.filename);
	memset(fl.filename+len, rand()%256, sizeof(fl.filename)-strlen(fl.filename));
	fl.filename[len] = '\0';
	shuffle_data(fl.filename, sizeof(fl.filename));
	strncpy(fl.mime, pform->mime, sizeof(fl.mime));
	fl.mime[sizeof(fl.mime)-1] = '\0';
	len = strlen(fl.mime);
	memset(fl.mime+len, rand()%256, sizeof(fl.mime)-strlen(fl.mime));
	fl.mime[len] = '\0';
	shuffle_data(fl.mime, sizeof(fl.mime));
	fl.size = pform->valuesize;
	md5_finish(&md5_ctx, digest);
	for(i=0; i<16; ++i) sprintf(fl.filehash+i*2, "%02x", digest[i]);
	fwrite(&fl,sizeof(fl),1,ptemp);
	i = 1;
	while(fread(&fl,sizeof(fl),1,pfile)==1){
		++i;
		if(ferror(ptemp)){
			unlock_file();
			remove(tmpfile);
			cgi_errend("書き込み失敗");
		}
		if(i <= UPLD_MAXDATACNT){
			fwrite(&fl,sizeof(fl),1,ptemp);
		}else{
			sprintf(delfile, "%s/%s_%lu", UPLD_UPDIR, remote_user, fl.id);
			remove(delfile);
		}
	}
	if(ferror(pfile)){
		unlock_file();
		remove(tmpfile);
		cgi_errend("読込み失敗");
	}
	fclose(pfile);
	fclose(ptemp);
	if(remove(list_file)){
		unlock_file();
		remove(tmpfile);
		cgi_errend("リスト更新できません。");
	}
	if(rename(list_tmp,list_file)){
		unlock_file();
		remove(tmpfile);
		cgi_errend("リストファイルが削除されました。");
	}
	if(rename(tmpfile,upfile)){
		unlock_file();
		remove(tmpfile);
		cgi_errend("アップロードファイルのコピー失敗");
	}
	unlock_file();
}

void lock_file(void)
{
	int i;
	for(i=0;i<10;++i){
		if(cgi_mkdir(lock_dir)) sleep(1);
		else break;
	}
}

void unlock_file(void)
{
	rmdir(lock_dir);
}

int id_check(unsigned long id)
{
	FILE *pf;
	filelist fl;
	int ret = 0;
	if((pf=fopen(list_file,UPLD_F_READ)) == NULL){
		cgi_errend("リストファイルを開けません。");
		unlock_file();
	}
	while(fread(&fl,sizeof(fl),1,pf)==1){
		if(fl.id == id){
			ret = 1;
			break;
		}
	}
	fclose(pf);
	return ret;
}

void download_file(void)
{
	FILE *pf;
	char downfile[256];
	char buf[4096];
	filelist fl;
	size_t bufsize;
	int i;
	unsigned long long shc = 0ULL;
	
	if(get_file_list(&fl,g_id)) cgi_errend("ファイルが見つかりません。");
	sprintf(downfile, "%s/%s_%lu", UPLD_UPDIR, remote_user, g_id);
#if defined(_WIN32) || defined(_WIN64)
	if(setmode(fileno(stdout),O_BINARY) == -1) cgi_errendex(__LINE__, __FILE__, "setmode error!");
#endif
	
	if((pf=fopen(downfile,UPLD_F_READ)) == NULL) cgi_errend("ファイルが開けません");
	printf("Content-Type: %s\x0d\x0a"
			"Content-Disposition: attachment; filename=\"%s\"\x0d\x0a"
			"Content-Length: %lu\x0d\x0a\x0d\x0a",
			fl.mime,
			fl.filename,
			fl.size);
	set_headerflg(1);
	while((bufsize=fread(buf,1,sizeof(buf),pf)) > 0){
		for(i=0; i<sizeof(buf); i++){
			buf[i] ^= xor_shuffle[(size_t)(shc%sizeof(xor_shuffle))];
			++shc;
		}
		fwrite(buf, bufsize, 1, stdout);
		if(ferror(pf) || ferror(stdout)) cgi_errend("ダウンロードエラー");
	}
	fclose(pf);
}

void preview_file(void)
{
	FILE *pf;
	char downfile[256];
	char buf[4096];
	filelist fl;
	size_t bufsize;
	int i;
	unsigned long long shc = 0ULL;
	
	if(get_file_list(&fl,g_id)) cgi_errend("ファイルが見つかりません。");
	sprintf(downfile, "%s/%s_%lu", UPLD_UPDIR, remote_user, g_id);
#if defined(_WIN32) || defined(_WIN64)
	if(setmode(fileno(stdout),O_BINARY) == -1) cgi_errendex(__LINE__, __FILE__, "setmode error!");
#endif
	
	if((pf=fopen(downfile,UPLD_F_READ)) == NULL) cgi_errend("ファイルが開けません");
	printf("Content-Type: %s\x0d\x0a"
			"Content-Length: %lu\x0d\x0a\x0d\x0a",
			fl.mime,
			fl.size);
	set_headerflg(1);
	while((bufsize=fread(buf,1,sizeof(buf),pf)) > 0){
		for(i=0; i<sizeof(buf); i++){
			buf[i] ^= xor_shuffle[(size_t)(shc%sizeof(xor_shuffle))];
			++shc;
		}
		fwrite(buf, bufsize, 1, stdout);
		if(ferror(pf) || ferror(stdout)) cgi_errend("ダウンロードエラー");
	}
	fclose(pf);
}

void delete_file(void)
{
	FILE *pfile, *ptemp;
	filelist fl;
	char delfile[256];
	pcgiform pform;
	int delflg;
	
	lock_file();
	if((pfile=fopen(list_file,UPLD_F_READ)) == NULL){
		unlock_file();
		cgi_errend("リストファイルオープンエラー");
	}
	if((ptemp=fopen(list_tmp,UPLD_F_WRITE)) == NULL){
		unlock_file();
		cgi_errend("tmpfileオープンエラー");
	}
	
	while(fread(&fl,sizeof(fl),1,pfile)==1){
		if(ferror(pfile) || ferror(ptemp)){
			unlock_file();
			cgi_errend("ファイルI/Oエラー");
		}
		for(delflg=0,pform=search_form("delkey",NULL);pform!=NULL;pform=search_form("delkey",pform)){
			if(strtoul(pform->value==NULL?"":pform->value,NULL,10)==fl.id){
				delflg = 1;
				break;
			}
		}
		if(delflg){
			sprintf(delfile, "%s/%s_%lu", UPLD_UPDIR, remote_user, fl.id);
			remove(delfile);
			continue;
		}
		fwrite(&fl, sizeof(fl), 1, ptemp);
	}
	
	fclose(pfile);
	fclose(ptemp);
	if(remove(list_file)){
		unlock_file();
		cgi_errend("リスト更新できません。");
	}
	if(rename(list_tmp,list_file)){
		unlock_file();
		cgi_errend("リストファイルが削除されました。");
	}
	unlock_file();
}

int get_file_list(filelist *pfl, unsigned long id)
{
	FILE *pf;
	int hit = 0;
	if((pf=fopen(list_file,UPLD_F_READ)) == NULL) cgi_errend("リストファイルオープンエラー");
	while(fread(pfl,sizeof(filelist),1,pf)==1){
		if(ferror(pf)) cgi_errend("File I/O error!");
		if(pfl->id == id){
			hit = 1;
			shuffle_data(pfl->filename, sizeof(pfl->filename));
			shuffle_data(pfl->mime, sizeof(pfl->mime));
			break;
		}
	}
	fclose(pf);
	if(!hit) memset(pfl,0,sizeof(filelist));
	return (!hit);
}

static void s_upload_handler(const pcgiform pform, char* buf, int buflen)
{
	int i;
	if(!strcmp(pform->name,"upfile")){
		if(!pform->valuesize){
			md5_starts(&md5_ctx);
			shuffle_counter = 0ULL;
		}
		md5_update(&md5_ctx, buf, buflen);
		for(i=0; i<buflen; i++){
			buf[i] ^= xor_shuffle[(size_t)(shuffle_counter%sizeof(xor_shuffle))];
			++shuffle_counter;
		}
	}
}

static void s_make_xor_shuffle(void)
{
	md5_context md5c;
	char buf[256];
	unsigned char digest[16];
	char digest_str[32+1];
	int cplen, i;
	
	strncpy(buf, remote_user, sizeof(buf));
	buf[sizeof(buf)-1] = '\0';
	cplen = sizeof(buf) - (strlen(buf) + strlen(UPLD_SALT)); 
	if(cplen > 0){
		strncat(buf, UPLD_SALT, cplen);
		buf[sizeof(buf)-1] = '\0';
	}
	
	md5_starts(&md5c);
	md5_update(&md5c, buf, strlen(buf));
	md5_finish(&md5c, digest);
	memcpy(xor_shuffle, digest, sizeof(digest));
	for(i=0; i<16; ++i) sprintf(digest_str+i*2, "%02x", digest[i]);
	memcpy(xor_shuffle+16, digest_str, 32);
}

static void shuffle_data(char *data, int len)
{
	int i;
	for(i=0; i<len; i++){
		data[i] ^= xor_shuffle[(size_t)(i%sizeof(xor_shuffle))];
	}
}
