/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* libmain - flex run-time support library "main" function */

/* /usr/local/epicsmgr/cvsroot/epics/base/src/toolsComm/flex/libmain.c,v 1.2.2.1 2009/04/30 20:45:31 anj Exp */

extern int yylex();

int main(int argc, char *argv[])
{
    return yylex();
}
