/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyscandlg.cc,v 1.5 2008-11-17 12:26:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyscandlg.h"

#include "uisegydef.h"
#include "uiseissel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "pixmap.h"

#include "segyfiledef.h"
#include "segyscanner.h"


uiSEGYScanDlg::uiSEGYScanDlg( uiParent* p, const uiSEGYReadDlg::Setup& su,
				IOPar& iop, bool ss )
    : uiSEGYReadDlg(p,su,iop)
    , scanner_(0)
    , forsurvsetup_(ss)
    , ctio_(*uiSeisSel::mkCtxtIOObj(su.geom_))
{
    uiObject* attobj = 0;
    if ( setup_.dlgtitle_.isEmpty() )
    {
	BufferString ttl( "Parameters for scan of " );
	ttl += Seis::nameOf( setup_.geom_ );
	SEGY::FileSpec fs; fs.usePar( iop );
	ttl += " '"; ttl += fs.fname_; ttl += "'";
	setTitleText( ttl );
    }

    if ( forsurvsetup_ )
    {
	if ( !optsgrp_ )
	    attobj = new uiLabel( this,
		    		  "Press OK or hit enter to start SEG-Y scan" );
    }
    else
    {
	IOObjContext& ctxt = ctio_.ctxt;
	ctxt.forread = false;
	ctxt.deftransl = ctxt.trglobexpr = "DA-SEG-Y";
	uiSeisSel::Setup sssu( setup_.geom_ ); sssu.selattr( false );
	outfld_ = new uiSeisSel( this, ctio_, sssu );
	attobj = outfld_->attachObj();
	if ( optsgrp_ )
	    outfld_->attach( alignedBelow, optsgrp_ );
    }

    if ( attobj )
    {
	uiToolButton* tb = new uiToolButton( this, "Pre-scan",
			   ioPixmap("prescan.png"),
			   mCB(this,uiSEGYScanDlg,preScanCB) );
	tb->attach( rightTo, attobj ); tb->attach( rightBorder );
	tb->setToolTip( "Limited Pre-scan" );
    }
}


uiSEGYScanDlg::~uiSEGYScanDlg()
{
    delete ctio_.ioobj;
    delete scanner_;
    delete &ctio_;
}


SEGY::Scanner* uiSEGYScanDlg::getScanner()
{
    SEGY::Scanner* ret = scanner_;
    scanner_ = 0;
    return ret;
}


bool uiSEGYScanDlg::doWork( const IOObj& ioobj )
{
    SEGY::FileSpec fs; fs.usePar( pars_ );
    delete scanner_; scanner_ = new SEGY::Scanner( fs, setup_.geom_, pars_ );
    if ( setup_.rev_ == uiSEGYRead::Rev0 )
	scanner_->setForceRev0( true );
    uiTaskRunner tr( parent_ );
    bool rv = tr.execute(*scanner_);
    if ( !rv ) return false;

    if ( !displayWarnings(scanner_->warnings(),outfld_) )
	return false;
    if ( !outfld_ ) return true;

    return true;
}
