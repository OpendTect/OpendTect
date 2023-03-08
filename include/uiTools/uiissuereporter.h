#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

#include "issuereporter.h"

class uiCheckBox;
class uiLineEdit;
class uiTextBrowser;
class uiTextEdit;

/*! \brief reports issues to opendtect.org. Usually crash reports. */

mExpClass(uiTools) uiIssueReporterDlg : public uiDialog
{  mODTextTranslationClass(uiIssueReporterDlg);
public:
				uiIssueReporterDlg(uiParent*,
						   System::IssueReporter&);
				~uiIssueReporterDlg();
    
    System::IssueReporter&	reporter() { return reporter_; }
    const char*			errMsg() const;

    static StringView		sKeyAllowSending()
				{ return "Allow sending of issue-reports"; }

protected:
    bool			allowSending() const;
    void			viewReportCB(CallBacker*);
    void			proxySetCB(CallBacker*);
    void			machInfoCB(CallBacker*);
    
    bool			acceptOK(CallBacker*) override;
    void			setButSensitive(bool);
    void			getReport(BufferString&) const;

    static uiString		sSendReport() { return tr("Send report"); }
    static uiString		sDontSendReport() { return tr("Do not send"); }
    
    uiTextBrowser*		machinfofld_;
    uiCheckBox*			machinfobut_;
    uiTextEdit*			commentfld_;
    uiLineEdit*			emailfld_;
    
    System::IssueReporter&	reporter_;
    BufferString		filename_;

};
