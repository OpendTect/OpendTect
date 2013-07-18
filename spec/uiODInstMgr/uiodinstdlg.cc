/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uiodinstdlg.cc 7574 2013-03-26 13:56:34Z kristofer.tingdahl@dgbes.com $";

#include "uiodinstdlg.h"
#include "odinstlogger.h"
#include "oddlsite.h"
#include "uilabel.h"
#include "uigroup.h"
#include "uihandledlsitefail.h"
#include "uimsg.h"
#include "pixmap.h"
#include "odlogo32x32.xpm"


uiODInstDLFailHndlr::uiODInstDLFailHndlr( uiODInstDlg* p )
    : par_(p)
{
    isfatal_ = false;
}


bool uiODInstDLFailHndlr::handle( ODDLSite& dlsite, BufferString& newsite,
				    float& newtmout )
{
    if ( dlsite.errMsg() && *dlsite.errMsg() )
    {
	mODInstToLog2( "Download fail:", dlsite.errMsg() );
	BufferString errmsg = dlsite.errMsg();
	/*if ( errmsg == ". Operation aborted by the user" )
	{
	    mODInstToLog2( "Download stopped:", errmsg );
	    return false;
	}*/
    }
    else
	mODInstToLog( "Download stopped" );
    uiHandleDLSiteFail dlg( par_, dlsite, isfatal_,
	    			&par_->dlHandler().availableSites() );
    if ( !dlg.go() )
	return false;

    newsite = dlg.site(); newtmout = dlg.timeout();
    mODInstLog() << "Next try, using: " << newsite <<
		    " with timeout " << newtmout << std::endl;
    return true;
}


uiODInstDlg::uiODInstDlg( uiParent* p, const uiDialog::Setup& su,
			  ODInst::DLHandler* dlh )
    : uiDialog(p,uiDialog::Setup(su).okcancelrev(true))
    , dlhndlr_(dlh)
    , dlfailhndlr_(this)
{
    if ( dlhndlr_ )
	dlhndlr_->setFailHandler( &dlfailhndlr_ );
    setOkText( "Proceed >&>" );
    setCancelText( "&<< Back" );
    preFinalise().notify( mCB(this,uiODInstDlg,addStuff) );
}


uiODInstDlg::~uiODInstDlg()
{
    // dlhndlr_ must not be deleted
}


void uiODInstDlg::addStuff( CallBacker* )
{
    addPreFinaliseStuff();
}


void uiODInstDlg::addPreFinaliseStuff()
{
    uiLabel* lbl = new uiLabel( topGroup(), "Installation Manager" );
    lbl->attach( rightBorder );
    uiLabel* pmlbl = new uiLabel( topGroup(), 0 );
    pmlbl->setPrefHeight( 40 );
    pmlbl->setPrefWidth( 40 );
    pmlbl->setPixmap( ioPixmap(od_logo_32x32) );
    pmlbl->attach( leftOf, lbl );
}


void uiODInstDlg::reportError( const char* errmsg, const char* detail,
			       bool addsupportmsg ) const
{
    if ( (!detail || !*detail) && !addsupportmsg )
    {
	uiMSG().error( errmsg );
	return;
    }

    BufferString detailstr( detail );
    if ( addsupportmsg )
    {
	detailstr += "\nFor more help write to support@opendtect.org with ";
	detailstr += mODInstLogger().logfnm_;
	detailstr += " as attachment";
    }

    BufferStringSet bufset;
    bufset.add( detailstr );
    uiMSG().errorWithDetails( bufset, errmsg );
}
