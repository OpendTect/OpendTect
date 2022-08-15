#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		June 2012
________________________________________________________________________

-*/

#include "networkmod.h"
#include "bufstring.h"
#include "uistring.h"

namespace System
{


/*Class that can post a crash-report to OpendTect's website */

mExpClass(Network) IssueReporter
{ mODTextTranslationClass(IssueReporter);
public:
				IssueReporter( const char* hostname = 0,
					       const char* path = 0 );

    bool			readReport(const char* filename);
    bool			setDumpFileName(const char* filename);
    BufferString&		getReport()	    { return report_; }
    const OD::String&		getReport() const   { return report_; }
    bool			isBinary() const    { return isbinary_; }

    bool			send();
    const char*			filePath() const    { return crashreportpath_; }
    uiString			errMsg() const	    { return errmsg_; }

    uiString			getMessage() const	{ return message_; }
				//!<Message coming back from website (if any).

    bool			parseCommandLine();

protected:

    void			fillBasicReport(const char* filename);

    BufferString		host_;
    BufferString		path_;
    uiString			errmsg_;
    BufferString		report_;
    BufferString		crashreportpath_;
    uiString			message_;
    bool			isbinary_;
};

} // namespace System
