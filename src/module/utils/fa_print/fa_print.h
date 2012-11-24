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


  filename: fa_print.h 
  version : v1.0.0
  time    : 2012/07/08 16:33 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#ifndef _FA_PRINT_H
#define _FA_PRINT_H


#ifdef __cplusplus 
extern "C"
{ 
#endif  



#include <stdio.h>  
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#ifdef __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef WIN32
#include <io.h>
#include <direct.h>
#define W_OK 02
#endif

#ifndef bool
#define bool int
#endif

#ifdef WIN32
#undef  stderr
#define stderr stdout
#endif

#define FA_PRINT_MAX_LEN     1024


int fa_print_init(bool printenable, 
                  bool fileneed, bool stdoutneed, bool stderrneed, 
                  bool pidneed, 
                  const char *path, const char *fileprefix, const char *printprefix, int thr_day);
int fa_print_uninit();

/*
    when you input print msg, below tag for your reference
    [NOTE, SUCC, FAIL, WARN, TEST]
        NOTE: for normal infomation, just remind you whan happened
        SUCC: means some critical action finish successfully
        FAIL: means error happened
        WARN: warning, not fail, but maybe something abnormal
        TEST: use this tag when you debug
    e.g:   fa_print("SUCC: global init succesfully\n");
           fa_print("TEST: debuging, test init\n");
           fa_print_err("FAIL: init failed\n");
*/
int fa_print(char *fmtstr, ...);
int fa_print_err(char *fmtstr, ...);

#define FA_PRINT_ENABLE                1
#define FA_PRINT_FILE_ENABLE           1
#define FA_PRINT_STDOUT_ENABLE         1
#define FA_PRINT_STDERR_ENABLE         1
#define FA_PRINT_PID_ENABLE            1

#define FA_PRINT_DISABLE               0 
#define FA_PRINT_FILE_DISABLE          0 
#define FA_PRINT_STDOUT_DISABLE        0 
#define FA_PRINT_STDERR_DISABLE        0  
#define FA_PRINT_PID_DISABLE           0 


#define USE_FA_PRINT         

#ifdef USE_FA_PRINT
    #ifdef __DEBUG__
    #define FA_PRINT_INIT       fa_print_init
    #define FA_PRINT_UNINIT     fa_print_uninit
    #define FA_PRINT            fa_print
    #define FA_PRINT_ERR        fa_print_err
    #define FA_PRINT_DBG        fa_print
    #else
    #define FA_PRINT_INIT       fa_print_init
    #define FA_PRINT_UNINIT     fa_print_uninit
    #define FA_PRINT            fa_print
    #define FA_PRINT_ERR        fa_print_err
    #ifdef __GNUC__
    #define FA_PRINT_DBG(...)   
    #else 
    #define FA_PRINT_DBG   
    #endif
    #endif
#else
    #ifdef __DEBUG__
    #ifdef __GNUC__
    #define FA_PRINT_INIT(...)      
    #define FA_PRINT_UNINIT(...)     
    #else 
    #define FA_PRINT_INIT      
    #define FA_PRINT_UNINIT     
    #endif
    #define FA_PRINT            printf 
    #define FA_PRINT_ERR        printf 
    #define FA_PRINT_DBG        printf 
    #else
    #ifdef __GNUC__
    #define FA_PRINT_INIT(...)      
    #define FA_PRINT_UNINIT(...)     
    #else 
    #define FA_PRINT_INIT      
    #define FA_PRINT_UNINIT     
    #endif
    #define FA_PRINT            printf 
    #define FA_PRINT_ERR        printf 
    #ifdef __GNUC__
    #define FA_PRINT_DBG(...)   
    #else 
    #define FA_PRINT_DBG   
    #endif
    #endif
#endif




#ifdef __cplusplus 
}
#endif  




#endif
