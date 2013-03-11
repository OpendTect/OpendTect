#ifndef issuereporter_h
#define issuereporter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		June 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "networkmod.h"
#include "bufstring.h"

namespace System
{
    
    
/*Class that can post a crash-report to OpendTect's website */
    
mExpClass(Network) IssueReporter
{
public:
    				IssueReporter( const char* hostname = 0,
					       const char* path = 0 );

    bool			readReport(const char* filename);
    bool			setDumpFileName(const char* filename);
    BufferString&		getReport()	    { return report_; }
    const BufferString&		getReport() const   { return report_; }
    
    bool			send();
    const char*			errMsg() const	    { return errmsg_.str(); }

    bool			parseCommandLine();
    
protected:
    
    BufferString		host_;
    BufferString		path_;
    BufferString		errmsg_;
    BufferString		report_;
    BufferString                crashreportpath_;
};
    
    


} //Namespace

#endif

