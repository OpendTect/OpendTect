/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"
#include "envvars.h"
#include "odver.h"
#include "msgh.h"
#include "fixedstring.h"
#include "uimain.h"
#include <iostream>

#ifdef __mac__
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#endif

extern int ODMain(int,char**);


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    uiMain::preInitForOpenGL();
    const FixedString argv1( argv[1] );
    const bool showversiononly = argv1 == "-v" || argv1 == "--version";

    int ret = 0;
    if ( showversiononly )
	std::cerr << GetFullODVersion() << std::endl;
    else
    {
	const char* msg =
	    "OpendTect can be run under one of three licenses:"
	    " (GPL, Commercial, Academic).\n"
	    "Please consult http://opendtect.org/OpendTect_license.txt.";

	std::cerr << msg << std::endl;
	OD::SetGlobalLogFile( 0 );
	UsrMsg( msg );

#ifdef __mac__
	BufferString datfile( FilePath(GetSoftwareDir(0),
			      "Resources/license.dgb.dat").fullPath());
	if ( File::exists(datfile.buf()) )
	{
	    BufferString valstr = GetEnvVar( "LM_LICENSE_FILE" );
	    if ( !valstr.isEmpty() ) valstr += ":";
	    valstr += datfile;
	    SetEnvVar( "LM_LICENSE_FILE", valstr.buf() );
	}
#endif

	ret = ODMain( argc, argv );
    }

    return ExitProgram( ret );
}
