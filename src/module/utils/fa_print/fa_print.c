/*
  falab - free algorithm lab 
  Copyright (C) 2012 luolongzhi 罗龙智 (Chengdu, China)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.


  filename: fa_print.c 
  version : v1.0.0
  time    : 2012/07/08 16:33 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h> 
#include <strings.h> 
#include <assert.h>
#include <stdarg.h>
#ifdef __GNUC__
#include <pthread.h>
#endif

#include "fa_print.h"

#define LOGFILE_NAME_LEN    512
#define LOGPREFIX_LEN       128 

typedef struct _logfile_t{

	FILE* fp_logfile;				        //the file pointer which point the log file

	char str_file[LOGFILE_NAME_LEN];	    //the logfile name

	char str_prefix[LOGPREFIX_LEN];	        //the prefix of the logfile name

    char str_printprefix[LOGPREFIX_LEN];    //log info prefix

	char log_dir[LOGFILE_NAME_LEN];	        //log file directory

}logfile_t;


static void open_logfile(logfile_t *logfile, const char *str_prefix, const char *str_printprefix);

static int  set_logdir(logfile_t *logfile, const char* str_logdir);

static void close_logfile(logfile_t *logfile);

static int  write_log(logfile_t *logfile, const char* str_logmsg, bool pidneed, bool fileneed, FILE *stdfile);


static bool         _printenable = 1;
static bool         _fileneed    = 0;
static bool         _stdoutneed  = 1;
static bool         _stderrneed  = 1;
static bool         _pidneed     = 1;
static logfile_t    _logfile;

static pthread_mutex_t  print_mutex;

static int _fa_vprintf(char *fmtstr, va_list valist);
static int _fa_err_vprintf(char *fmtstr, va_list valist);

int fa_print_init(bool printenable, 
                  bool fileneed, bool stdoutneed, bool stderrneed, 
                  bool pidneed, 
                  const char *path, const char *fileprefix, const char *printprefix)
{
    _printenable  = printenable;
    _fileneed     = fileneed;
    _stdoutneed   = stdoutneed;
    _stderrneed   = stderrneed;
    _pidneed      = pidneed;

    open_logfile(&_logfile, fileprefix, printprefix);
    if(_fileneed) {
        set_logdir(&_logfile, path);
    }

    pthread_mutex_init(&print_mutex, NULL);

    return 0;
}

int fa_print_uninit()
{
    if(_fileneed)
        close_logfile(&_logfile);

    return 0;
}

static int _fa_vprintf(char *fmtstr, va_list valist)
{
    char logmsg[FA_PRINT_MAX_LEN];
    int ret;

    if(!_printenable) return 0;

    vsprintf(logmsg, fmtstr, valist);
    if(_stdoutneed)
        ret = write_log(&_logfile, logmsg, _pidneed, _fileneed, stdout);
    else
        ret = write_log(&_logfile, logmsg, _pidneed, _fileneed, NULL);
    if (ret<0) return -1;

    return 0;
}


static int _fa_err_vprintf(char *fmtstr, va_list valist)
{
    char logmsg[FA_PRINT_MAX_LEN];
    int ret;

    if(!_printenable) return 0;

    vsprintf(logmsg, fmtstr, valist);
    if(_stderrneed)
        ret = write_log(&_logfile, logmsg, _pidneed, _fileneed, stderr);
    else
        ret = write_log(&_logfile, logmsg, _pidneed, _fileneed, NULL);

    if (ret<0) return -1;

   return 0;
}

int fa_print(char *fmtstr, ...)
{   
    va_list valist; 

    pthread_mutex_lock(&print_mutex);

    va_start(valist, fmtstr ); 
    _fa_vprintf(fmtstr, valist);
    va_end(valist); 

    pthread_mutex_unlock(&print_mutex);

    return 0;
}

int fa_print_err(char *fmtstr, ...)
{   
    va_list valist; 

    pthread_mutex_lock(&print_mutex);

    va_start(valist, fmtstr ); 
    _fa_vprintf(fmtstr, valist);
    va_end(valist); 

    pthread_mutex_unlock(&print_mutex);

    return 0;
}


static void open_logfile(logfile_t *logfile, const char *str_prefix, const char *str_printprefix)
{
	logfile->fp_logfile = NULL;
	if(str_prefix)
		strcpy(logfile->str_prefix,str_prefix);
	else 
		memset(logfile->str_prefix,0,sizeof(logfile->str_prefix));

    if(str_printprefix)
        strcpy(logfile->str_printprefix, str_printprefix);
    else
        memset(logfile->str_printprefix, 0, sizeof(logfile->str_printprefix));

	memset(logfile->str_file,0,sizeof(logfile->str_file));
	memset(logfile->log_dir,0,sizeof(logfile->log_dir));

}

static int set_logdir(logfile_t *logfile, const char* str_logdir)
{
#ifdef __GNUC__
	if(access(str_logdir,W_OK)<0) {
		if(mkdir(str_logdir,0744)<0)
			return -1;
	}
#endif

#ifdef WIN32
	if(_access(str_logdir,W_OK)<0) {
		if(_mkdir(str_logdir)<0)
            return -1;
	}
#endif

	strcpy(logfile->log_dir,str_logdir);
	if(logfile->log_dir[strlen(logfile->log_dir)-1]!='/')
		strcat(logfile->log_dir,"/");
	return 0;
}

static void close_logfile(logfile_t *logfile)
{
    if(logfile->fp_logfile) {
        fclose(logfile->fp_logfile);
        logfile->fp_logfile = NULL;
    }
}

static int write_log(logfile_t *logfile, const char* str_logmsg, bool pidneed, bool fileneed, FILE *stdfile)
{
	char strTime[50];
	char strYear[5], strMonth[3], strDay[3],strDate[9];
	char strFile[LOGFILE_NAME_LEN];
	time_t ctime;
	struct tm *ptm;

    if(fileneed) {
        //if no assigned directory ,using ./log/
        if(strlen(logfile->log_dir)==0) {
            strcpy(logfile->log_dir,"./defaultlog/");
            #ifdef __GNUC__
            if(access(logfile->log_dir,W_OK)<0) {
                if(mkdir(logfile->log_dir,0744)<0)
                    return -1;
            }
            #endif

            #ifdef WIN32
            if(_access(logfile->log_dir,W_OK)<0) {
                if(_mkdir(logfile->log_dir)<0)
                    return -1;

            }
            #endif
        }
    }

	//get the system time
	time(&ctime);
	ptm = localtime( &ctime );
	strftime( strYear, 5, "%Y", ptm );
	strftime( strMonth, 3, "%m", ptm );
	strftime( strDay, 3, "%d", ptm );
	sprintf(strDate, "%s%s%s", strYear, strMonth, strDay);

    if(fileneed) {
        //generate the file name
        #ifdef __GNUC__
	    snprintf(strFile,sizeof(strFile), "%s%s-localtime-%s.log",logfile->log_dir,logfile->str_prefix,strDate);
        #endif

        #ifdef WIN32
    	_snprintf(strFile,sizeof(strFile),"%s%s-localtime-%s.log",logfile->log_dir,logfile->str_prefix,strDate);
        #endif

        //compare the filelen
        if((strlen(logfile->str_file)==0) || (strcmp(logfile->str_file,strFile))) {
            strcpy(logfile->str_file,strFile);
            logfile->fp_logfile = fopen(strFile,"a");
            if(!logfile->fp_logfile)return -1;
        }else {
            logfile->fp_logfile = fopen(strFile,"a");
            if(!logfile->fp_logfile)return -1;
        }

    }
   
	//wirte the filename using the date
    sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d LOCAL", 
            ptm->tm_year+1900,
            ptm->tm_mon+1,
            ptm->tm_mday,
            ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
    if(pidneed) {
        if(stdfile)
            fprintf(stdfile, "#[%s][%s][P%d]#\t", strTime, logfile->str_printprefix, getpid());
        if(fileneed)
            fprintf( logfile->fp_logfile, "#[%s][%s][P%d]#\t", strTime, logfile->str_printprefix, getpid());
    }else {
        if(stdfile)
            fprintf(stdfile, "#[%s][%s]#\t", strTime, logfile->str_printprefix);
        if(fileneed)
            fprintf( logfile->fp_logfile, "#[%s][%s]#\t", strTime, logfile->str_printprefix );
    }

	//write msg
    if(stdfile)
        fprintf(stdfile, "%s", str_logmsg );

    if(fileneed) {
        fprintf( logfile->fp_logfile, "%s", str_logmsg );
        fflush( logfile->fp_logfile);
        if(logfile->fp_logfile) {
            fclose(logfile->fp_logfile);
            logfile->fp_logfile = NULL;
        }
    }

	return 0;

}




