/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.cc,v 1.1 2008-09-24 19:48:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyimpdlg.h"

#include "uisegydef.h"
#include "uisegyexamine.h"
#include "uitoolbar.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiseparator.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "segytr.h"
#include "seisioobjinfo.h"


uiSEGYImpDlg::uiSEGYImpDlg( uiParent* p,
			const uiSEGYImpDlg::Setup& su, IOPar& iop )
    : uiDialog(p,su)
    , setup_(su)
    , pars_(iop)
{
    uiSEGYFileOpts::Setup osu( setup_.geom_, uiSEGYRead::Import,
	    			setup_.isrev1_ );
    optsfld_ = new uiSEGYFileOpts( this, osu, &iop );

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


uiSEGYImpDlg::~uiSEGYImpDlg()
{
}


bool uiSEGYImpDlg::getFromScreen( bool permissive )
{
    return optsfld_->fillPar( pars_, permissive );
}


bool uiSEGYImpDlg::rejectOK( CallBacker* )
{
    getFromScreen( true );
    return true;
}


bool uiSEGYImpDlg::acceptOK( CallBacker* )
{
    return getFromScreen( false );
}

