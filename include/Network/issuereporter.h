#ifndef issuereporter_h
#define issuereporter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		June 2012
 RCS:		$Id: issuereporter.h,v 1.1 2012-06-26 12:50:14 cvskris Exp $
________________________________________________________________________

-*/

#include "bufstring.h"

namespace System
{
    
    
/*Class that can post a crash-report to OpendTect's website */
    
mClass IssueReporter
{
public:
    				IssueReporter( const char* hostname = 0,
					       const char* path = 0 );

    bool			readReport(const char* filename);
    BufferString&		getReport() { return report_; }
    const BufferString&		getReport() const { return report_; }
    
    bool			send();
    const char*			errMsg() const { return errmsg_.str(); }
    
    bool			parseCommandLine( int, char** );

protected:

    BufferString		host_;
    BufferString		path_;
    BufferString		errmsg_;
    BufferString		report_;
};
    
    


} //Namespace

#endif
