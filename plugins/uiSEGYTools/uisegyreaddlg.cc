/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/

#include "uisegyscandlg.h"

#include "uisegydef.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "ioobj.h"
#include "od_helpids.h"


uiSEGYReadDlg::Setup::Setup( Seis::GeomType gt )
    : uiDialog::Setup(tr("SEG-Y Scan"),mNoDlgTitle,
                      mODHelpKey(mSEGYReadDlgHelpID) )
    , geom_(gt)
    , rev_(uiSEGYRead::Rev0)
{
}


uiSEGYReadDlg::uiSEGYReadDlg( uiParent* p,
			const uiSEGYReadDlg::Setup& su, IOPar& iop,
			bool forsurvsetup )
    : uiVarWizardDlg(p,su,iop,DoWork)
    , setup_(su)
    , optsfld_(0)
    , readParsReq(this)
    , writeParsReq(this)
    , preScanReq(this)
{
    if ( setup_.geom_ != Seis::Vol || setup_.rev_ != uiSEGYRead::Rev1 )
    {
	uiSEGYFileOpts::Setup osu( setup_.geom_, uiSEGYRead::Import,
				   setup_.rev_ );
	if ( forsurvsetup )
	    osu.purpose( uiSEGYRead::SurvSetup );
	optsfld_ = new uiSEGYFileOpts( this, osu, &iop );
	optsfld_->readParsReq.notify( mCB(this,uiSEGYReadDlg,readParsCB) );
	optsfld_->writeParsReq.notify( mCB(this,uiSEGYReadDlg,writeParsCB) );
	optsfld_->preScanReq.notify( mCB(this,uiSEGYReadDlg,preScanCB) );
    }

    postFinalize().notify( mCB(this,uiSEGYReadDlg,initWin) );
}


void uiSEGYReadDlg::readParsCB( CallBacker* )
{
    readParsReq.trigger();
}


void uiSEGYReadDlg::writeParsCB( CallBacker* )
{
    writeParsReq.trigger();
}


void uiSEGYReadDlg::preScanCB( CallBacker* )
{
    preScanReq.trigger();
}


void uiSEGYReadDlg::initWin( CallBacker* )
{
    if ( optsfld_ )
	optsfld_->usePar( pars_ );
}


void uiSEGYReadDlg::use( const IOObj* ioobj, bool force )
{
    if ( optsfld_ )
	optsfld_->use( ioobj, force );
}


bool uiSEGYReadDlg::getParsFromScreen( bool permissive )
{
    return optsfld_ ? optsfld_->fillPar( pars_, permissive ) : true;
}


bool uiSEGYReadDlg::rejectOK( CallBacker* cb )
{
    getParsFromScreen( true );
    return uiVarWizardDlg::rejectOK( cb );
}



bool uiSEGYReadDlg::acceptOK( CallBacker* )
{
    if ( !getParsFromScreen(false) )
	return false;

    SEGY::FileSpec fs; fs.usePar( pars_ );
    PtrMan<IOObj> inioobj = fs.getIOObj( true );
    if ( !inioobj )
    {
	uiMSG().error( tr("Internal: cannot create SEG-Y object") );
	return false;
    }
    inioobj->pars() = pars_;

    return doWork( *inioobj );
}
