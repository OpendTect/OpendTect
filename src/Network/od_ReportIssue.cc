/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: od_ReportIssue.cc,v 1.3 2012/06/27 05:17:23 cvsnageswara Exp $";

#include "issuereporter.h"

#include "fixedstring.h"

#include "prog.h"
#include <iostream>

#include "QCoreApplication"

int main( int argc, char ** argv )
{
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
