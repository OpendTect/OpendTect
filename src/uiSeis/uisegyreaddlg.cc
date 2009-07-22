/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegyreaddlg.cc,v 1.9 2009-07-22 16:01:41 cvsbert Exp $";

#include "uisegyscandlg.h"

#include "uisegydef.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "ioobj.h"


uiSEGYReadDlg::Setup::Setup( Seis::GeomType gt )
    : uiDialog::Setup("SEG-Y Scan",mNoDlgTitle,mTODOHelpID)
    , geom_(gt) 
    , rev_(uiSEGYRead::Rev0)
{
}


uiSEGYReadDlg::uiSEGYReadDlg( uiParent* p,
			const uiSEGYReadDlg::Setup& su, IOPar& iop,
       			bool forsurvsetup )
    : uiDialog(p,su)
    , setup_(su)
    , pars_(iop)
    , optsgrp_(0)
    , optsfld_(0)
    , savesetupfld_(0)
    , readParsReq(this)
    , writeParsReq(this)
    , preScanReq(this)
{
    if ( setup_.geom_ != Seis::Vol || setup_.rev_ != uiSEGYRead::Rev1 )
    {
	optsgrp_ = new uiGroup( this, "Opts group" );
	uiSEGYFileOpts::Setup osu( setup_.geom_, uiSEGYRead::Import,
				   setup_.rev_ );
	if ( forsurvsetup )
	    osu.purpose( uiSEGYRead::SurvSetup );
	optsfld_ = new uiSEGYFileOpts( optsgrp_, osu, &iop );
	optsfld_->readParsReq.notify( mCB(this,uiSEGYReadDlg,readParsCB) );
	optsfld_->preScanReq.notify( mCB(this,uiSEGYReadDlg,preScanCB) );

	savesetupfld_ = new uiGenInput( optsgrp_, "On OK, save setup as" );
	savesetupfld_->attach( alignedBelow, optsfld_ );
	optsgrp_->setHAlignObj( savesetupfld_ );
	uiLabel* lbl = new uiLabel( optsgrp_, "(optional)" );
	lbl->attach( rightOf, savesetupfld_ );
    }
}



void uiSEGYReadDlg::readParsCB( CallBacker* )
{
    readParsReq.trigger();
}


void uiSEGYReadDlg::preScanCB( CallBacker* )
{
    preScanReq.trigger();
}


uiSEGYReadDlg::~uiSEGYReadDlg()
{
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


const char* uiSEGYReadDlg::saveObjName() const
{
    return savesetupfld_ ? savesetupfld_->text() : "";
}


bool uiSEGYReadDlg::displayWarnings( const BufferStringSet& warns,
				     bool withstop )
{
    if ( warns.isEmpty() ) return true;

    BufferString msg( "Warning" ); if ( warns.size() > 1 ) msg += "s";
    msg += " during read:";
    for ( int idx=0; idx<warns.size(); idx++ )
    {
	msg += "\n\n";
	msg += warns.get( idx );
    }

    if ( !withstop )
	{ uiMSG().warning( msg ); return true; }

    msg += "\n\nContinue?";
    return uiMSG().askContinue( msg );
}


bool uiSEGYReadDlg::rejectOK( CallBacker* )
{
    getParsFromScreen( true );
    return true;
}



bool uiSEGYReadDlg::acceptOK( CallBacker* )
{
    if ( !getParsFromScreen(false) )
	return false;
    if ( *saveObjName() )
	writeParsReq.trigger();

    SEGY::FileSpec fs; fs.usePar( pars_ );
    PtrMan<IOObj> inioobj = fs.getIOObj();
    if ( !inioobj )
    {
	uiMSG().error( "Internal: cannot create SEG-Y object" );
	return false;
    }
    inioobj->pars() = pars_;
    SEGY::FileSpec::ensureWellDefined( *inioobj );

    return doWork( *inioobj );
}
