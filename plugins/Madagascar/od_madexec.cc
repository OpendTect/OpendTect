/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	R. K. Singh
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_madexec.cc,v 1.14 2009-01-20 10:54:43 cvsraman Exp $";

#include "batchprog.h"
#include "envvars.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "madprocexec.h"
#include "madprocflow.h"
#include "madstream.h"
#include "oddirs.h"
#include "progressmeter.h"
#include "seistype.h"
#include "strmprov.h"

#include <iostream>


static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyMadagascar = "Madagascar";
static const char* sKeyInput = "Input";
static const char* sKeyOutput = "Output";
static const char* sKeyProc = "Proc";
static const char* sKeyWrite = "Write";



bool BatchProgram::go( std::ostream& strm )
{
    ODMad::ProcExec exec( pars(), strm );
    if ( !exec.init() )
    {
	strm << "Cannot initialize process..." << std::endl;
	return false;
    }

    exec.execute();
    return true;
}    

#undef mErrRet

