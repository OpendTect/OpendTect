/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.cc,v 1.2 2008-09-26 13:37:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyimpdlg.h"

#include "uisegydef.h"
#include "uisegyexamine.h"
#include "uiseistransf.h"
#include "uiseissel.h"
#include "uitoolbar.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiseparator.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "segytr.h"
#include "seisioobjinfo.h"
#include "ctxtioobj.h"


uiSEGYImpDlg::Setup::Setup( Seis::GeomType gt )
    : uiDialog::Setup("SEG-Y Import",0,"103.1.5")
    , geom_(gt) 
    , nrexamine_(0)     
    , rev_(uiSEGYRead::Rev0)
{
}

#define mAddButton(fnm,func,tip,toggle) \
	tb_->addButton( fnm, mCB(this,uiSEGYImpDlg,func), tip, toggle )

uiSEGYImpDlg::uiSEGYImpDlg( uiParent* p,
			const uiSEGYImpDlg::Setup& su, IOPar& iop )
    : uiDialog(p,su)
    , setup_(su)
    , pars_(iop)
    , optsfld_(0)
    , ctio_(*uiSeisSel::mkCtxtIOObj(su.geom_))
    , readParsReq(this)
{
    ctio_.ctxt.forread = false;

    uiGroup* optsgrp = 0;
    if ( setup_.rev_ != uiSEGYRead::Rev1 )
    {
	optsgrp = new uiGroup( this, "Opts group" );
	uiSEGYFileOpts::Setup osu( setup_.geom_, uiSEGYRead::Import,
				    setup_.rev_ == uiSEGYRead::WeakRev1 );
	optsfld_ = new uiSEGYFileOpts( optsgrp, osu, &iop );
	optsfld_->readParsReq.notify( mCB(this,uiSEGYImpDlg,readParsCB) );

	savesetupfld_ = new uiGenInput( optsgrp, "On OK, save setup as" );
	savesetupfld_->attach( alignedBelow, optsfld_ );
	optsgrp->setHAlignObj( savesetupfld_ );
    }

    uiSeparator* sep = optsgrp ? new uiSeparator( this, "Hor sep" ) : 0;

    uiGroup* outgrp = new uiGroup( this, "Output group" );
    transffld_ = new uiSeisTransfer( outgrp, uiSeisTransfer::Setup(setup_.geom_)
				    .withnullfill(false)
				    .fornewentry(true) );
    outgrp->setHAlignObj( transffld_ );
    if ( sep )
    {
	sep->attach( stretchedBelow, optsgrp );
	outgrp->attach( alignedBelow, optsgrp );
	outgrp->attach( ensureBelow, sep );
    }

    seissel_ = new uiSeisSel( outgrp, ctio_, uiSeisSel::Setup(setup_.geom_) );
    seissel_->attach( alignedBelow, transffld_ );

    finaliseDone.notify( mCB(this,uiSEGYImpDlg,setupWin) );
}


void uiSEGYImpDlg::setupWin( CallBacker* )
{
    if ( setup_.nrexamine_ < 1 ) return;

    uiSEGYExamine::Setup exsu( setup_.nrexamine_ );
    exsu.modal( false ); exsu.usePar( pars_ );
    uiSEGYExamine* dlg = new uiSEGYExamine( this, exsu );
    dlg->go();
}


void uiSEGYImpDlg::readParsCB( CallBacker* )
{
    readParsReq.trigger();
}


uiSEGYImpDlg::~uiSEGYImpDlg()
{
}


bool uiSEGYImpDlg::getParsFromScreen( bool permissive )
{
    return optsfld_ ? optsfld_->fillPar( pars_, permissive ) : true;
}


bool uiSEGYImpDlg::rejectOK( CallBacker* )
{
    getParsFromScreen( true );
    return true;
}


bool uiSEGYImpDlg::acceptOK( CallBacker* )
{
    if ( !getParsFromScreen(false) )
	return false;

    return true;
}
