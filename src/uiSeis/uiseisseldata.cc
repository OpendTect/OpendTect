/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		March 2019
________________________________________________________________________

-*/


#include "uiseisseldata.h"

#include "seis2ddata.h"
#include "seisioobjinfo.h"
#include "seisprovider.h"
#include "seispolyseldata.h"
#include "seisrangeseldata.h"
#include "seistableseldata.h"
#include "cubesubsel.h"
#include "linesubsel.h"

#include "uidialog.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiposprovgroupstd.h"
#include "uiselsurvranges.h"
#include "uistrings.h"


uiSeisSelDataDlg::uiSeisSelDataDlg( uiParent* p, const DBKey& dbky,
				    const SelData* sd, bool onlyrg )
    : uiDialog( p, uiDialog::Setup(tr("Seismic selection"),
				   mNoDlgTitle,mTODOHelpKey) )
    , ioobjinf_(new SeisIOObjInfo(dbky))
{
    if ( !ioobjinf_->isOK() )
	{ new uiLabel( this, tr("Invalid input selection") ); return; }
    else if ( ioobjinf_->is2D() )
	make2D();

    init( sd, onlyrg );
}


uiSeisSelDataDlg::uiSeisSelDataDlg( uiParent* p, bool is2d, const SelData* sd,
				    bool onlyrg )
    : uiDialog( p, uiDialog::Setup(tr("Seismic selection"),
				   mNoDlgTitle,mTODOHelpKey) )
{
    if ( is2d )
	make2D();

    init( sd, onlyrg );
}


uiSeisSelDataDlg::~uiSeisSelDataDlg()
{
    delete ioobjinf_;
    delete lsss_;
    delete geomids_;
}


void uiSeisSelDataDlg::make2D()
{
    lsss_ = new LineSubSelSet;
    geomids_ = new GeomIDSet;
}


void uiSeisSelDataDlg::init( const SelData* sd, bool onlyrg )
{
    if ( sd && sd->is2D() != is2D() )
	sd = nullptr;

    uiStringSet choices( toUiString("-"), uiStrings::sRange() );
    if ( !is2D() && !onlyrg )
	choices.add( uiStrings::sPolygon() ).add( uiStrings::sTable() );
    auto* lcb = new uiLabeledComboBox( this, choices, uiStrings::sType() );
    typfld_ = lcb->box();
    if ( !sd )
	typfld_->setCurrentItem( 0 );

    if ( is2D() )
	create2DFlds( sd, lcb );
    else
	create3DFlds( sd, onlyrg, lcb );

    mAttachCB( postFinalise(), uiSeisSelDataDlg::initWin );
}


void uiSeisSelDataDlg::create2DFlds( const SelData* sd, uiGroup* attgrp )
{
    const auto* rsd = sd ? sd->asRange() : 0;
    if ( rsd )
	typfld_->setCurrentItem( 1 );

    if ( !ioobjinf_ )
    {
	trcnrsel_ = new uiSelNrRange( this, uiSelNrRange::Gen, true );
	trcnrsel_->setLabelText( uiStrings::sTraceRange() );
	trcnrsel_->attach( alignedBelow, attgrp );
	zsel_ = new uiSelZRange( this, true );
	zsel_->attach( alignedBelow, trcnrsel_ );
	return;
    }

    const Seis2DDataSet ds2d( *ioobjinf_->ioObj() );
    const auto nrlines = ds2d.nrLines();

    *lsss_ = rsd ? rsd->fullSubSel().lineSubSelSet() : LineSubSelSet();
    if ( lsss_->isEmpty() )
    {
	// No previous selection means we select all
	for ( int iln=0; iln<nrlines; iln++ )
	    lsss_->add( new LineSubSel(ds2d.geomID(iln)) );
    }

    if ( nrlines > 1 )
    {
	uiListBox::Setup lbsu( OD::ChooseAtLeastOne );
	linesel_ = new uiListBox( this, lbsu );
	BufferStringSet lnms;
	for ( int iln=0; iln<nrlines; iln++ )
	{
	    lnms.add( ds2d.lineName(iln) );
	    geomids_->add( ds2d.geomID(iln) );
	}
	auto* idxs = lnms.getSortIndexes();
	lnms.useIndexes( idxs );
	geomids_->useIndexes( idxs );
	delete [] idxs;
	for ( int iln=0; iln<nrlines; iln++ )
	{
	    linesel_->addItem( ds2d.lineName(iln) );
	    if ( lsss_->find(geomids_->get(iln)) )
		linesel_->setChosen( iln );
	}
	linesel_->attach( alignedBelow, attgrp );
	linesel_->setCurrentItem( prevlidx_ );
    }

    trcnrsel_ = new uiSelNrRange( this, StepInterval<int>(0,0,1), true,
				  "Trace Range" );
    trcnrsel_->setLabelText( uiStrings::sTraceRange() );
    if ( linesel_ )
	trcnrsel_->attach( alignedBelow, linesel_ );
    else
	trcnrsel_->attach( alignedBelow, attgrp );

    zsel_ = new uiSelZRange( this, StepInterval<float>(0.f,0.f,1.f), true );
    zsel_->attach( alignedBelow, trcnrsel_ );
}


void uiSeisSelDataDlg::create3DFlds( const SelData* sd, bool onlyrange,
				     uiGroup* attgrp )
{
    inlsel_ = new uiSelNrRange( this, uiSelNrRange::Inl, true );
    inlsel_->attach( alignedBelow, attgrp );
    crlsel_ = new uiSelNrRange( this, uiSelNrRange::Crl, true );
    crlsel_->attach( alignedBelow, inlsel_ );
    zsel_ = new uiSelZRange( this, true );
    zsel_->attach( alignedBelow, crlsel_ );
    const auto* rsd = sd ? sd->asRange() : 0;
    if ( rsd )
    {
	typfld_->setCurrentItem( 1 );
	const auto css = rsd->fullSubSel().cubeSubSel();
	inlsel_->setRange( css.inlSubSel().outputPosRange() );
	crlsel_->setRange( css.crlSubSel().outputPosRange() );
	zsel_->setRange( css.zSubSel().outputZRange() );
    }

    if ( onlyrange )
	return;

    const uiPosProvGroup::Setup ppgsu( false, true, true );

    polysel_ = new uiPolyPosProvGroup( this, ppgsu );
    polysel_->attach( alignedBelow, attgrp );
    const auto* psd = sd ? sd->asPoly() : 0;
    if ( psd )
    {
	typfld_->setCurrentItem( 2 );
	IOPar iop;
	psd->fillPar( iop );
	polysel_->usePar( iop );
    }

    tablesel_ = new uiTablePosProvGroup( this, ppgsu );
    tablesel_->attach( alignedBelow, attgrp );
    const auto* tsd = sd ? sd->asTable() : 0;
    if ( tsd )
    {
	typfld_->setCurrentItem( 3 );
	IOPar iop;
	tsd->fillPar( iop );
	tablesel_->usePar( iop );
    }
}


void uiSeisSelDataDlg::initWin( CallBacker* )
{
    if ( linesel_ )
	linesel_->setCurrentItem( 0 );
    updUi();

    mAttachCB( typfld_->selectionChanged, uiSeisSelDataDlg::updUi );
    if ( linesel_ )
    {
	mAttachCB( linesel_->selectionChanged, uiSeisSelDataDlg::lineChgCB );
	mAttachCB( linesel_->itemChosen, uiSeisSelDataDlg::lineChosenCB );
    }
}


void uiSeisSelDataDlg::setLSS( int lidx )
{
    if ( lidx < 0 )
	return;

    const bool ischosen = !linesel_ || linesel_->isChosen( lidx );
    auto* lss = lsss_->find( geomids_->get(lidx) );
    if ( !ischosen )
    {
	if ( lss )
	    lsss_->removeSingle( lsss_->indexOf(lss) );
	return;
    }

    if ( !lss )
    {
	lss = new LineSubSel( geomids_->get(lidx) );
	lsss_->add( lss );
    }

    lss->trcNrSubSel().setOutputPosRange( trcnrsel_->getRange() );
    lss->zSubSel().setOutputZRange( zsel_->getRange() );
}


void uiSeisSelDataDlg::lineChosenCB( CallBacker* cb )
{
    if ( !linesel_ )
	{ pErrMsg("Huh"); return; }

    mCBCapsuleUnpack( int, itm, cb );
    if ( itm < 0 )
    {
	for ( int idx=0; idx<linesel_->size(); idx++ )
	{
	    const auto geomid = geomids_->get( idx );
	    const bool ischosen = linesel_->isChosen( idx );
	    const auto* lss = lsss_->find( geomid );
	    if ( ischosen && !lss )
		lsss_->add( new LineSubSel( geomid ) );
	    else if ( !ischosen && lss )
		lsss_->removeSingle( lsss_->indexOf(lss) );
	}
    }

    updLineUi( linesel_->currentItem() );
}


void uiSeisSelDataDlg::lineChgCB( CallBacker* )
{
    if ( !linesel_ )
	{ pErrMsg("Huh"); return; }

    const auto lidx = linesel_->currentItem();
    if ( lidx != prevlidx_ )
    {
	setLSS( prevlidx_ );
	prevlidx_ = lidx;
    }

    updLineUi( lidx );
}


void uiSeisSelDataDlg::updUi( CallBacker* )
{
    const auto typ = typfld_->currentItem();
    const bool isrange = typ == 1;

    zsel_->display( isrange );

    if ( is2D() )
    {
	trcnrsel_->display( isrange );
	if ( linesel_ )
	{
	    updLineUi( linesel_->currentItem() );
	    linesel_->display( isrange );
	}
    }
    else
    {
	if ( inlsel_ )
	    inlsel_->display( isrange );
	if ( crlsel_ )
	    crlsel_->display( isrange );
	if ( polysel_ )
	    polysel_->display( typ == 2 );
	if ( tablesel_ )
	    tablesel_->display( typ == 3 );
    }
}


void uiSeisSelDataDlg::updLineUi( int lidx )
{
    if ( !geomids_->validIdx(lidx) )
	return;

    const auto geomid = geomids_->get( lidx );
    const auto* lss = lsss_->find( geomid );
    PtrMan<LineSubSel> lssdestroyer;
    if ( !lss )
    {
	lssdestroyer = new LineSubSel( geomid );
	lss = lssdestroyer;
    }

    trcnrsel_->setLimitRange( lss->trcNrSubSel().inputPosRange() );
    trcnrsel_->setRange( lss->trcNrSubSel().outputPosRange() );
    zsel_->setRangeLimits( lss->zSubSel().inputZRange() );
    zsel_->setRange( lss->zSubSel().outputZRange() );

    const bool ischosen = linesel_->isChosen( lidx );
    trcnrsel_->setSensitive( ischosen );
    zsel_->setSensitive( ischosen );
}


Seis::SelData* uiSeisSelDataDlg::getSelData()
{
    if ( typfld_->currentItem() < 1 )
	return nullptr;
    return is2D() ? get2DSelData() : get3DSelData();
}


Seis::SelData* uiSeisSelDataDlg::get2DSelData()
{
    if ( geomids_->isEmpty() )
    {
	LineSubSel* lss = new LineSubSel( GeomID() );
	lss->trcNrSubSel().setInputPosRange( trcnrsel_->getRange() );
	lss->zSubSel().setInputZRange( zsel_->getRange() );
	return new Seis::RangeSelData( *lss );
    }

    return new Seis::RangeSelData( *lsss_ );
}


Seis::SelData* uiSeisSelDataDlg::get3DSelData()
{
    const auto typ = typfld_->currentItem();
    if ( typ > 1 )
    {
	uiPosProvGroup* ppg = polysel_;
	if ( typ > 2 )
	    ppg = tablesel_;
	IOPar iop;
	if ( !ppg->fillPar(iop) )
	    return nullptr;
	return SelData::get( iop );
    }

    Seis::RangeSelData* rsd = new Seis::RangeSelData;
    CubeSubSel css = rsd->fullSubSel().cubeSubSel();
    css.inlSubSel().setOutputPosRange( inlsel_->getRange() );
    css.crlSubSel().setOutputPosRange( crlsel_->getRange() );
    css.zSubSel().setOutputZRange( zsel_->getRange() );
    rsd->set( css );
    return rsd;
}


bool uiSeisSelDataDlg::acceptOK()
{
    if ( linesel_ )
	setLSS( linesel_->currentItem() );
    return true;
}



uiSeisSelData::uiSeisSelData( uiParent* p, Opt opt )
    : uiCompoundParSel(p,uiStrings::sSubSel())
    , onlyrange_(opt == OnlyRange)
    , typeChanged(this)
{
    mAttachCB( butPush, uiSeisSelData::doDlg );
    mAttachCB( postFinalise(), uiSeisSelData::initGrp );
}


uiSeisSelData::~uiSeisSelData()
{
    detachAllNotifiers();
    delete seldata_;
}


void uiSeisSelData::initGrp( CallBacker* )
{
    updUi();
}


void uiSeisSelData::doDlg( CallBacker* )
{
    uiSeisSelDataDlg dlg( this, dbky_, seldata_, onlyrange_ );
    if ( dlg.go() )
    {
	const int prevtype = seldata_ ? (int)seldata_->type() : -1;
	delete seldata_;
	seldata_ = dlg.getSelData();
	const int newtype = seldata_ ? (int)seldata_->type() : -1;
	if ( prevtype != newtype )
	    typeChanged.trigger();
    }
    updUi();
}


uiString uiSeisSelData::getSummary() const
{
    return seldata_ ? seldata_->usrSummary() : toUiString( "-" );
}


Seis::SelData* uiSeisSelData::get() const
{
    return seldata_ ? seldata_->clone() : 0;
}


void uiSeisSelData::set( const SelData* sd )
{
    delete seldata_;
    seldata_ = sd ? sd->clone() : 0;
    updUi();
}


void uiSeisSelData::setInput( const DBKey& dbky )
{
    dbky_ = dbky;
    set( SelData::get(dbky_) );
}


void uiSeisSelData::updUi()
{
    updateSummary();
    selbut_->setSensitive( dbky_.isValid() );
}


void uiSeisSelData::fillPar( IOPar& iop ) const
{
    if ( !seldata_ )
	SelData::removeFromPar( iop );
    else
	seldata_->fillPar( iop );
}


void uiSeisSelData::usePar( const IOPar& iop )
{
    set( SelData::get(iop) );
}
