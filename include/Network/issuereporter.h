#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
				IssueReporter(const char* hostname =nullptr,
					      const char* path =nullptr);
				~IssueReporter();

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

    bool			use(const char* filename,bool isbinary=true);
    bool			parseCommandLine();
    void			fillBasicReport();

protected:

    mDeprecated			("Call the one without arguments")
    void			fillBasicReport(const char* filename);

    BufferString		host_;
    BufferString		path_;
    uiString			errmsg_;
    BufferString		report_;
    BufferString		crashreportpath_;
    uiString			message_;
    bool			isbinary_ = true;
};

} // namespace System
