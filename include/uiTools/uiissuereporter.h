#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		21/9/2000
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

#include "issuereporter.h"

class uiTextEdit;
class uiGenInput;

/*! \brief reports issues to opendtect.org. Usually crash reports. */

mExpClass(uiTools) uiIssueReporterDlg : public uiDialog
{  mODTextTranslationClass(uiIssueReporterDlg);
public:
    
				uiIssueReporterDlg(uiParent*,
						   System::IssueReporter&);
    
    System::IssueReporter&	reporter() { return reporter_; }
    const char*			errMsg() const;

    static StringView		sKeyAllowSending()
				{ return "Allow sending of issue-reports"; }

protected:
    bool			allowSending() const;
    void			viewReportCB(CallBacker*);
    void			viewReport(const uiString& caption);
    void			copyToClipBoardCB(CallBacker*);
    void			proxySetCB(CallBacker*);
    
    bool			acceptOK(CallBacker*) override;
    void			setButSensitive(bool);
    void			getReport(BufferString&) const;

    static uiString		sSendReport() { return tr("Send report"); }
    static uiString		sDontSendReport() { return tr("Do not send"); }
    
    uiTextEdit*			commentfld_;
    uiGenInput*			emailfld_;
    
    System::IssueReporter&	reporter_;
    BufferString		filename_;

};


