/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydefdlg.cc,v 1.1 2008-09-19 14:28:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegydefdlg.h"

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


uiSEGYBasic::Setup::Setup( bool fr )
    : uiDialog::Setup("SEG-Y tool","Specify basic properties",mTODOHelpID)
    , forread_(fr)
{
}


uiSEGYBasic::uiSEGYBasic( uiParent* p, const uiSEGYBasic::Setup& su,
			  IOPar& iop )
    : uiDialog( p, su )
    , setup_(su)
    , pars_(iop)
    , geomfld_(0)
    , geomtype_(Seis::Vol)
{
    filespecfld_ = new uiSEGYFileSpec( this, setup_.forread_, &iop );
    uiGroup* lastgrp = filespecfld_;
    if ( su.geoms_.size() == 1 )
	    geomtype_ = su.geoms_[0];
    else
    {
	if ( su.geoms_.isEmpty() )
	    uiSEGYIO::Setup::getDefaultTypes( setup_.geoms_, setup_.forread_ );

	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "File type" );
	geomfld_ = lcb->box();
	for ( int idx=0; idx<su.geoms_.size(); idx++ )
	    geomfld_->addItem( Seis::nameOf( (Seis::GeomType)su.geoms_[idx] ) );
	geomfld_->setCurrentItem( 0 );
	lcb->attach( alignedBelow, filespecfld_ );
	lastgrp = lcb;
    }

    uiSeparator* sep = new uiSeparator( this, "hor sep", true, false );
    sep->attach( stretchedBelow, lastgrp );

    nrtrcexfld_ = new uiGenInput( this, "Examine first",
			      IntInpSpec(100).setName("Traces to Examine") );
    nrtrcexfld_->attach( alignedBelow, lastgrp );
    nrtrcexfld_->attach( ensureBelow, sep );
    uiLabel* lbl = new uiLabel( this, "traces" );
    lbl->attach( rightOf, nrtrcexfld_ );
    fileparsfld_ = new uiSEGYFilePars( this, setup_.forread_, &iop );
    fileparsfld_->attach( alignedBelow, nrtrcexfld_ );

    finaliseDone.notify( mCB(this,uiSEGYBasic,initFlds) );
    	// Need this to get zero padding right
}


void uiSEGYBasic::initFlds( CallBacker* )
{
    usePar( pars_ );
}


uiSEGYBasic::~uiSEGYBasic()
{
}


Seis::GeomType uiSEGYBasic::geomType() const
{
    if ( !geomfld_ )
	return geomtype_;

    return Seis::geomTypeOf( geomfld_->textOfItem( geomfld_->currentItem() ) );
}


int uiSEGYBasic::nrTrcExamine() const
{
    return nrtrcexfld_->getIntValue();
}


void uiSEGYBasic::use( const IOObj* ioobj, bool force )
{
    filespecfld_->use( ioobj, force );
    fileparsfld_->use( ioobj, force );
    SeisIOObjInfo oinf( ioobj );
    if ( geomfld_ && oinf.isOK() )
	geomfld_->setCurrentItem( Seis::nameOf(oinf.geomType()) );
}


void uiSEGYBasic::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    filespecfld_->fillPar( iop );
    fileparsfld_->fillPar( iop );
    iop.set( uiSEGYExamine::Setup::sKeyNrTrcs, nrTrcExamine() );
    iop.set( sKey::Geometry, Seis::nameOf(geomType()) );
}


void uiSEGYBasic::usePar( const IOPar& iop )
{
    pars_.merge( iop );
    filespecfld_->usePar( pars_ );
    fileparsfld_->usePar( pars_ );
    int nrex = nrTrcExamine();
    iop.get( uiSEGYExamine::Setup::sKeyNrTrcs, nrex );
    nrtrcexfld_->setValue( nrex );   
    const char* res = iop.find( sKey::Geometry );
    if ( res && *res )
	geomfld_->setCurrentItem( res );
}


bool uiSEGYBasic::acceptOK( CallBacker* )
{
    IOPar tmp;
    if ( !filespecfld_->fillPar(tmp) || !fileparsfld_->fillPar(tmp) )
	return false;

    fillPar( pars_ );
    return true;
}


uiSEGYFileOptsDlg::uiSEGYFileOptsDlg( uiParent* p,
			const uiSEGYFileOptsDlg::Setup& su, IOPar& iop )
    : uiDialog(p,su)
    , setup_(su)
    , pars_(iop)
{
    uiSEGYFileOpts::Setup osu( setup_.geom_, setup_.operation_,
	   		       setup_.isrev1_ );
    optsfld_ = new uiSEGYFileOpts( this, osu, &iop );
}


uiSEGYFileOptsDlg::~uiSEGYFileOptsDlg()
{
}


bool uiSEGYFileOptsDlg::getFromScreen( bool permissive )
{
    return optsfld_->fillPar( pars_, permissive );
}


bool uiSEGYFileOptsDlg::rejectOK( CallBacker* )
{
    getFromScreen( true );
    return true;
}


bool uiSEGYFileOptsDlg::acceptOK( CallBacker* )
{
    return getFromScreen( false );
}

