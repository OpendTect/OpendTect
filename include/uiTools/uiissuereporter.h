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
class uiPushButton;

/*! \brief 
 

*/


mClass(uiTools) uiIssueReporterDlg : public uiDialog
{ 	
public:
    
				uiIssueReporterDlg( uiParent* );
    
    System::IssueReporter&	reporter() { return reporter_; }
    const char*			errMsg() const;
    

protected:
    
    void			viewReportCB(CallBacker*);
    
    bool			acceptOK(CallBacker*);
    void			getReport(BufferString&) const;
    
    uiTextEdit*			commentfld_;
    uiPushButton*		viewreportbut_;
    
    System::IssueReporter	reporter_;
    BufferString		filename_;
};


#endif

