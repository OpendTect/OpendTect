/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "issuereporter.h"

#include "fixedstring.h"

#include "prog.h"
#include <iostream>

#include "QCoreApplication"

int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );
    System::IssueReporter reporter;
    if ( !reporter.parseCommandLine( argc, argv ) )
    {
	
	std::cerr << reporter.errMsg() << '\n';
	ExitProgram( 1 );	
    }
    
    QCoreApplication app( argc, argv );
    
    if ( !reporter.send() )
    {
	std::cerr << reporter.errMsg() << '\n';
	return 1;
    }
        
    std::cerr << "Report submitted.\n";
    
    return 0;
}
