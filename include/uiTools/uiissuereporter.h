#ifndef uiissuereporter_h
#define uiissuereporter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          21/9/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

#include "issuereporter.h"

class uiTextEdit;
class uiGenInput;

/*! \brief reports issues to opendtect.org. Usually crash reports. */

mExpClass(uiTools) uiIssueReporterDlg : public uiDialog
{ 	
public:
    
				uiIssueReporterDlg( uiParent* );
    
    System::IssueReporter&	reporter() { return reporter_; }
    const char*			errMsg() const;

    static FixedString		sKeyAllowSending()
    				{ return "Allow sending of issue-reports"; }

protected:
    bool			allowSending() const;
    void			viewReportCB(CallBacker*);
    void			viewReport(const uiString& caption);
    void			copyToClipBoardCB(CallBacker*);
    void			proxySetCB(CallBacker*);
    
    bool			acceptOK(CallBacker*);
    void			setButSensitive(bool);
    void			getReport(BufferString&) const;

    static uiString		sSendReport() { return "Send report"; }
    static uiString		sDontSendReport() { return "Do not send"; }
    
    uiTextEdit*			commentfld_;
    uiGenInput*			emailfld_;
    
    System::IssueReporter	reporter_;
    BufferString		filename_;
};


#endif

