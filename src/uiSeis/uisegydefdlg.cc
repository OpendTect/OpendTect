/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegydefdlg.h"

#include "uisegydef.h"
#include "uisegyexamine.h"
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
#include "settings.h"

#define sKeySettNrTrcExamine \
    IOPar::compKey("SEG-Y",uiSEGYExamine::Setup::sKeyNrTrcs)


uiSEGYDefDlg::Setup::Setup()
    : uiDialog::Setup("SEG-Y tool","Specify basic properties","103.0.6")
    , defgeom_(Seis::Vol)
{
}


uiSEGYDefDlg::uiSEGYDefDlg( uiParent* p, const uiSEGYDefDlg::Setup& su,
			  IOPar& iop )
    : uiVarWizardDlg(p,su,iop,Start)
    , setup_(su)
    , geomfld_(0)
    , geomtype_(Seis::Vol)
    , readParsReq(this)
{
    const bool havevol = su.geoms_.indexOf( Seis::Vol ) >= 0;
    const bool havevolps = su.geoms_.indexOf( Seis::VolPS ) >= 0;
    const bool havevlineps = su.geoms_.indexOf( Seis::LinePS ) >= 0;
    uiSEGYFileSpec::Setup sgyfssu( havevol || havevolps || havevlineps );
    sgyfssu.forread(true).pars(&iop);
    sgyfssu.canbe3d( havevol || havevolps );
    filespecfld_ = new uiSEGYFileSpec( this, sgyfssu );
    filespecfld_->fileSelected.notify( mCB(this,uiSEGYDefDlg,fileSel) );

    uiGroup* lastgrp = filespecfld_;
    if ( su.geoms_.size() == 1 )
    {
	geomtype_ = su.geoms_[0];
    }
    else
    {
	if ( su.geoms_.isEmpty() )
	    uiSEGYRead::Setup::getDefaultTypes( setup_.geoms_ );
	if ( setup_.geoms_.indexOf( setup_.defgeom_ ) < 0 )
	    setup_.defgeom_ = setup_.geoms_[0];

	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "File type" );
	geomfld_ = lcb->box();
	for ( int idx=0; idx<su.geoms_.size(); idx++ )
	    geomfld_->addItem( Seis::nameOf( (Seis::GeomType)su.geoms_[idx] ) );
	geomfld_->setCurrentItem( setup_.geoms_.indexOf(setup_.defgeom_) );
	geomfld_->selectionChanged.notify( mCB(this,uiSEGYDefDlg,geomChg) );
	lcb->attach( alignedBelow, filespecfld_ );
	lastgrp = lcb;
    }

    uiSeparator* sep = new uiSeparator( this, "hor sep", true, false );
    sep->attach( stretchedBelow, lastgrp );

    int nrex = 100; Settings::common().get( sKeySettNrTrcExamine, nrex );
    nrtrcexfld_ = new uiGenInput( this, "Number of traces to examine",
			      IntInpSpec(nrex).setName("Traces to Examine") );
    nrtrcexfld_->attach( alignedBelow, lastgrp );
    nrtrcexfld_->attach( ensureBelow, sep );
    savenrtrcsbox_ = new uiCheckBox( this, "Save as default" );
    savenrtrcsbox_->attach( rightOf, nrtrcexfld_ );
    fileparsfld_ = new uiSEGYFilePars( this, true, &iop );
    fileparsfld_->attach( alignedBelow, nrtrcexfld_ );
    fileparsfld_->readParsReq.notify( mCB(this,uiSEGYDefDlg,readParsCB) );

    postFinalise().notify( mCB(this,uiSEGYDefDlg,initFlds) );
    	// Need this to get zero padding right
}


void uiSEGYDefDlg::initFlds( CallBacker* )
{
    usePar( pars_ );
    geomChg( 0 );
}


Seis::GeomType uiSEGYDefDlg::geomType() const
{
    if ( !geomfld_ )
	return geomtype_;

    return Seis::geomTypeOf( geomfld_->textOfItem( geomfld_->currentItem() ) );
}


int uiSEGYDefDlg::nrTrcExamine() const
{
    const int nr = nrtrcexfld_->getIntValue();
    return nr < 0 || mIsUdf(nr) ? 0 : nr;
}


void uiSEGYDefDlg::use( const IOObj* ioobj, bool force )
{
    filespecfld_->use( ioobj, force );
    SeisIOObjInfo oinf( ioobj );
    if ( oinf.isOK() )
    {
	if ( geomfld_ )
	{
	    geomfld_->setCurrentItem( Seis::nameOf(oinf.geomType()) );
	    geomChg( 0 );
	}
	useSpecificPars( ioobj->pars() );
    }
}


void uiSEGYDefDlg::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    filespecfld_->fillPar( iop );
    fileparsfld_->fillPar( iop );
    iop.set( uiSEGYExamine::Setup::sKeyNrTrcs, nrTrcExamine() );
    iop.set( sKey::Geometry, Seis::nameOf(geomType()) );
}


void uiSEGYDefDlg::usePar( const IOPar& iop )
{
    if ( &iop != &pars_ )
    {
	SEGY::FileReadOpts::shallowClear( pars_ );
	pars_.merge( iop );
    }
    filespecfld_->usePar( pars_ );
    fileparsfld_->usePar( pars_ );
    useSpecificPars( iop );
}


void uiSEGYDefDlg::useSpecificPars( const IOPar& iop )
{
    int nrex = nrTrcExamine();
    iop.get( uiSEGYExamine::Setup::sKeyNrTrcs, nrex );
    nrtrcexfld_->setValue( nrex );   
    const char* res = iop.find( sKey::Geometry );
    if ( res && *res && geomfld_ )
    {
	geomfld_->setCurrentItem( res );
	geomChg( 0 );
    }
}


void uiSEGYDefDlg::fileSel( CallBacker* )
{
    fileparsfld_->setBytesSwapped( filespecfld_->isProbablySwapped() );
}


void uiSEGYDefDlg::readParsCB( CallBacker* )
{
    readParsReq.trigger();
}


void uiSEGYDefDlg::geomChg( CallBacker* )
{
    filespecfld_->setInp2D( Seis::is2D(geomType()) );
}


bool uiSEGYDefDlg::acceptOK( CallBacker* )
{
    if ( savenrtrcsbox_->isChecked() )
    {
	const int nrex = nrTrcExamine();
	Settings::common().set( sKeySettNrTrcExamine, nrex );
	Settings::common().write();
    }

    IOPar tmp;
    if ( !filespecfld_->fillPar(tmp) || !fileparsfld_->fillPar(tmp) )
	return false;

    fillPar( pars_ );
    return true;
}
