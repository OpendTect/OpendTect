/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2004
 RCS:		$Id: uisegysip.cc,v 1.14 2008-10-17 13:06:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegysip.h"
#include "uisegyread.h"
#include "uilabel.h"
#include "uimsg.h"
#include "segyscanner.h"
#include "posinfodetector.h"
#include "cubesampling.h"
#include "ptrman.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "errh.h"


class uiSEGYSIPMgrDlg : public uiDialog
{
public:

uiSEGYSIPMgrDlg( uiParent* p, const uiDialog::Setup& su )
    : uiDialog(p,su)
{
    new uiLabel( this, "To be able to scan your data\n"
	    "You must define the specific properties of your SEG-Y file(s)" );
    uiSEGYRead::Setup srsu( uiSEGYRead::SurvSetup );
    sr_ = new uiSEGYRead( this, srsu );
    sr_->processEnded.notify( mCB(this,uiSEGYSIPMgrDlg,atEnd) );
}

void atEnd( CallBacker* )
{
    done( sr_->state() != uiSEGYRead::Cancelled ? 1 : 0 );
}

    uiSEGYRead*	sr_;

};


uiDialog* uiSEGYSurvInfoProvider::dialog( uiParent* p )
{
    uiDialog::Setup su( "Survey setup (SEG-Y)", mNoDlgTitle, mNoHelpID );
    su.oktext("").canceltext("");
    return new uiSEGYSIPMgrDlg( p, su );
}


bool uiSEGYSurvInfoProvider::getInfo( uiDialog* d, CubeSampling& cs,
				      Coord crd[3] )
{
    if ( !d ) return false;
    mDynamicCastGet(uiSEGYSIPMgrDlg*,dlg,d)
    if ( !dlg ) { pErrMsg("Huh?"); return false; }

    PtrMan<SEGY::Scanner> scanner = dlg->sr_->getScanner();
    if ( !scanner ) { pErrMsg("Huh?"); return false; }

    const char* errmsg = scanner->posInfoDetector().getSurvInfo(cs.hrg,crd);
    if ( errmsg && *errmsg )
	{ uiMSG().error( errmsg ); return false; }

    cs.zrg = scanner->zRange();
    return true;
}
