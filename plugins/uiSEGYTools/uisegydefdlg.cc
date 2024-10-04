/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
#include "uifileinput.h"
#include "uilabel.h"
#include "uitable.h"
#include "uimsg.h"
#include "filepath.h"
#include "keystrs.h"
#include "segydirectdef.h"
#include "segytr.h"
#include "seisioobjinfo.h"
#include "settings.h"
#include "od_helpids.h"


uiSEGYDefDlg::Setup::Setup()
    : uiDialog::Setup(tr("SEG-Y tool"),tr("Specify basic properties"),
		      mODHelpKey(mSEGYDefDlgHelpID) )
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
    , writeParsReq(this)
{
    const bool havevol = su.geoms_.isPresent( Seis::Vol );
    const bool havevolps = su.geoms_.isPresent( Seis::VolPS );
    const bool havevlineps = su.geoms_.isPresent( Seis::LinePS );
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
	if ( !setup_.geoms_.isPresent( setup_.defgeom_ ) )
	    setup_.defgeom_ = setup_.geoms_[0];

	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, tr("File type") );
	geomfld_ = lcb->box();
	for ( int idx=0; idx<su.geoms_.size(); idx++ )
	    geomfld_->addItem( Seis::nameOf( (Seis::GeomType)su.geoms_[idx] ) );
	geomfld_->setCurrentItem( setup_.geoms_.indexOf(setup_.defgeom_) );
	geomfld_->selectionChanged.notify( mCB(this,uiSEGYDefDlg,geomChg) );
	lcb->attach( alignedBelow, filespecfld_ );
	lastgrp = lcb;
    }

    uiSeparator* sep = new uiSeparator( this, "hor sep", OD::Horizontal, false);
    sep->attach( stretchedBelow, lastgrp );

    int nrex = 100; Settings::common().get( sKeySettNrTrcExamine, nrex );
    nrtrcexfld_ = new uiGenInput( this, tr("Number of traces to examine"),
			      IntInpSpec(nrex).setName("Traces to Examine") );
    nrtrcexfld_->attach( alignedBelow, lastgrp );
    nrtrcexfld_->attach( ensureBelow, sep );
    savenrtrcsbox_ = new uiCheckBox( this, uiStrings::sSaveAsDefault() );
    savenrtrcsbox_->attach( rightOf, nrtrcexfld_ );
    fileparsfld_ = new uiSEGYFilePars( this, true, &iop );
    fileparsfld_->attach( alignedBelow, nrtrcexfld_ );
    fileparsfld_->readParsReq.notify( mCB(this,uiSEGYDefDlg,readParsCB) );
    fileparsfld_->writeParsReq.notify( mCB(this,uiSEGYDefDlg,writeParsCB) );

    postFinalize().notify( mCB(this,uiSEGYDefDlg,initFlds) );
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
	fileparsfld_->usePar( ioobj->pars() );
	useSpecificPars( ioobj->pars() );
    }
}


void uiSEGYDefDlg::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    filespecfld_->fillPar( iop );
    fileparsfld_->fillPar( iop );
    iop.set( uiSEGYExamine::Setup::sKeyNrTrcs, nrTrcExamine() );
    iop.set( sKey::Geometry(), Seis::nameOf(geomType()) );
}


void uiSEGYDefDlg::usePar( const IOPar& iop )
{
    if ( &iop != &pars_ )
    {
	FileReadOpts::shallowClear( pars_ );
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
    const BufferString res = iop.find( sKey::Geometry() );
    if ( !res.isEmpty() && geomfld_ )
    {
	geomfld_->setCurrentItem( res.buf() );
	geomChg( nullptr );
    }
}


void uiSEGYDefDlg::fileSel( CallBacker* )
{
    const bool allswpd = filespecfld_->isProbablySwapped();
    const bool dataswpd = filespecfld_->isProbablySeisWare()
			&& filespecfld_->isIEEEFmt();
    fileparsfld_->setBytesSwapped( allswpd, dataswpd );
}


void uiSEGYDefDlg::readParsCB( CallBacker* )
{
    readParsReq.trigger();
}


void uiSEGYDefDlg::writeParsCB( CallBacker* )
{
    writeParsReq.trigger();
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
