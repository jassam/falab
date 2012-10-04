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


  filename: main.c 
  version : v1.0.0
  time    : 2012/07/08 00:42 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

  comment : this file is the simple template which will be used in falab,
            it will be changed according to the project.

*/


#include <stdio.h>
#include <stdlib.h>
#include "fa_parseopt.h"

int main(int argc, char *argv[])  
{  
    int ret;

    ret = fa_parseopt(argc, argv);

    if(ret)
        FA_PRINT_ERR("FAIL: now doing the abnormal things\n");
    else
        FA_PRINT("SUCC: now doing the normal thing\n");

    return 0;  
}  

