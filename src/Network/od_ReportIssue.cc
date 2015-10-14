/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "issuereporter.h"
#include "prog.h"
#include "applicationdata.h"


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    ApplicationData app;
    
    System::IssueReporter reporter;
    if ( !reporter.parseCommandLine() )
    {
	od_cout() << reporter.errMsg().getFullString() << '\n';
	return ExitProgram( 1 );	
    }
    
    if ( !reporter.send() )
    {
	od_cout() << reporter.errMsg().getFullString() << od_endl;
	return ExitProgram( 1 );	
    }
        
    if ( !reporter.getMessage().isEmpty() )
    {
	const BufferString msg = reporter.getMessage().getFullString();
	od_cout() << msg << "\n";
    }
    else
    {
	od_cout() << "Report submitted.\n";
    }
    return ExitProgram( 0 );
}

