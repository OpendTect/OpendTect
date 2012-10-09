/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra / Bert
 Date:          March 2003 / Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiattribcrossplot.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "posprovider.h"
#include "posfilterset.h"
#include "posvecdataset.h"
#include "seisioobjinfo.h"
#include "seis2dline.h"
#include "surv2dgeom.h"

#include "mousecursor.h"
#include "uidatapointset.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiposfilterset.h"
#include "uiposprovider.h"
#include "uitaskrunner.h"
#include "uiseislinesel.h"

using namespace Attrib;

uiAttribCrossPlot::uiAttribCrossPlot( uiParent* p, const Attrib::DescSet& d )
	: uiDialog(p,uiDialog::Setup("Attribute cross-plotting",
		     "Select attributes and locations for cross-plot"
		     ,"111.1.0").modal(false))
	, ads_(*new Attrib::DescSet(d.is2D()))
    	, lnmfld_(0)
    	, curdps_(0)
    	, dpsdispmgr_(0)
    	, attrinfo_(0)
{
    uiGroup* attrgrp = new uiGroup( this, "Attribute group" );
    uiLabeledListBox* llb =
	new uiLabeledListBox( attrgrp, "Attributes", true,
			      uiLabeledListBox::AboveMid );
    llb->attach( leftBorder, 20 );
    attrsfld_ = llb->box();
    if ( !ads_.is2D() )
	attrsfld_->setMultiSelect( true );

    if ( ads_.is2D() )
    {
	attrsfld_->setItemsCheckable( true );
	attrsfld_->itemChecked.notify(
		mCB(this,uiAttribCrossPlot,attrChecked) );
	attrsfld_->selectionChanged.notify(
		mCB(this,uiAttribCrossPlot,attrChanged) );
	uiLabeledListBox* lnmlb =
	    new uiLabeledListBox( attrgrp, "Line names", true,
		    		  uiLabeledListBox::AboveMid ); 
	lnmfld_ = lnmlb->box();
	lnmlb->attach( rightTo, llb );
	lnmfld_->selectionChanged.notify(
		mCB(this,uiAttribCrossPlot,lineChecked) );
    }

    uiGroup* provgrp = new uiGroup( this, "Attribute group" );
    provgrp->attach( leftAlignedBelow, attrgrp );
    uiPosProvider::Setup psu( ads_.is2D(), true, true );
    psu.seltxt( "Select locations by" ).choicetype( uiPosProvider::Setup::All );
    posprovfld_ = new uiPosProvider( provgrp, psu );
    posprovfld_->setExtractionDefaults();

    uiPosFilterSet::Setup fsu( ads_.is2D() );
    fsu.seltxt( "Location filters" ).incprovs( true );
    posfiltfld_ = new uiPosFilterSetSel( provgrp, fsu );
    posfiltfld_->attach( alignedBelow, posprovfld_ );

    setDescSet( d );
    postFinalise().notify( mCB(this,uiAttribCrossPlot,initWin) );
}


void uiAttribCrossPlot::setDescSet( const Attrib::DescSet& newads )
{
    const_cast<Attrib::DescSet&>(ads_) = newads;
    adsChg();
}


void uiAttribCrossPlot::adsChg()
{
    attrsfld_->setEmpty();

    delete attrinfo_;
    attrinfo_ = new Attrib::SelInfo( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinfo_->attrnms_.size(); idx++ )
	attrsfld_->addItem( attrinfo_->attrnms_.get(idx), false );
    
    for ( int idx=0; idx<attrinfo_->ioobjids_.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinfo_->ioobjids_.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinfo_->ioobjnms_.get(idx).buf();
	    attrsfld_->addItem( attrinfo_->is2D(defkey)
		    ? SeisIOObjInfo::defKey2DispName(defkey,ioobjnm)
		    : SeisIOObjInfo::def3DDispName(defkey,ioobjnm) );
	}
    }

    if ( !attrsfld_->isEmpty() )
	attrsfld_->setCurrentItem( int(0) );

    attrChanged( 0 );
}


uiAttribCrossPlot::~uiAttribCrossPlot()
{
    delete const_cast<Attrib::DescSet*>(&ads_);
    deepErase( dpsset_ );
}


void uiAttribCrossPlot::initWin( CallBacker* )
{
}


MultiID uiAttribCrossPlot::getSelectedID() const
{
    if ( !attrinfo_ )
	return MultiID();

    const int curitem = attrsfld_->currentItem();
    const int attrsz = attrinfo_->attrnms_.size();
    const bool isstored = curitem >= attrsz;
    if ( isstored )
    {
	int storedidx = -1;
	for ( int idx=0; idx<attrinfo_->ioobjids_.size(); idx++ )
	{
	    BufferStringSet bss;
	    SeisIOObjInfo sii( MultiID( attrinfo_->ioobjids_.get(idx) ) );
	    sii.getDefKeys( bss, true );
	    storedidx += bss.size();
	    if ( (curitem-attrsz) <= storedidx )
	    {
		BufferString ioobjid( attrinfo_->ioobjids_[idx]->buf() );
		MultiID mid( ioobjid );
		return mid;
	    }
	}

	return MultiID();
    }
    else
    {
	const Attrib::Desc* desc = ads_.getDesc( attrinfo_->attrids_[curitem] );
	MultiID mid( desc->getStoredID(true) );
	return mid;
    }
}


void uiAttribCrossPlot::lineChecked( CallBacker* )
{
    MultiID selid = getSelectedID();
    const int selitem = selidxs_.indexOf( attrsfld_->currentItem() );
    if ( selitem < 0 ) return;

    linenmsset_[selitem].erase();
    for ( int lidx=0; lidx<lnmfld_->size(); lidx++ )
    {
	if ( lnmfld_->isSelected(lidx) )
	    linenmsset_[selitem].addIfNew( lnmfld_->textOfItem(lidx) );
    }
}


void uiAttribCrossPlot::attrChecked( CallBacker* )
{
    MultiID selid = getSelectedID();
    const bool ischked = attrsfld_->isItemChecked( attrsfld_->currentItem() );
    if ( ischked && selidxs_.addIfNew(attrsfld_->currentItem()) )
    {
	selids_ += selid;
	linenmsset_ += BufferStringSet();
	const int lsidx = linenmsset_.size()-1;
	for ( int lidx=0; lidx<lnmfld_->size(); lidx++ )
	{
	    lnmfld_->setSelected( lidx, true );
	    linenmsset_[lsidx].add( lnmfld_->textOfItem(lidx) );
	}
    }
    else if ( !ischked )
    {
	const int selitem = selidxs_.indexOf( attrsfld_->currentItem() );
	if ( selitem >= 0 )
	{
	    selidxs_.remove( selitem );
	    selids_.remove( selitem );
	    for ( int lidx=0; lidx<lnmfld_->size(); lidx++ )
		lnmfld_->setSelected( lidx, false );

	    linenmsset_.remove( selitem );
	}
    }
}


void uiAttribCrossPlot::attrChanged( CallBacker* )
{
    if ( !lnmfld_ ) return;

    MultiID mid = getSelectedID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    const int idxof = selidxs_.indexOf( attrsfld_->currentItem() );
    NotifyStopper notifystop( lnmfld_->selectionChanged );

    lnmfld_->setEmpty();
    SeisIOObjInfo seisinfo( mid );
    BufferStringSet linenames;
    BufferString attrnm( attrsfld_->getText() );
    removeCharacter( attrnm.buf(), '[' );
    removeCharacter( attrnm.buf(), ']' );
    LineKey lk( attrnm );
    seisinfo.getLineNamesWithAttrib( lk.attrName(), linenames );
    for ( int lidx=0; lidx<linenames.size(); lidx++ )
    {
	lnmfld_->addItem( linenames.get(lidx) );
	if ( idxof >= 0 && linenmsset_[idxof].isPresent(linenames.get(lidx)) )
	    lnmfld_->setSelected( lidx, true );
	else
	    lnmfld_->setSelected( lidx, false );
    }
}


#define mErrRet(s) { if ( emiterr ) uiMSG().error(s); return; }

/*void uiAttribCrossPlot::useLineName( bool emiterr )
    if ( !lnmfld_ ) return;

    const Attrib::Desc* desc = ads_.getFirstStored( false );
    if ( !desc )
	mErrRet("No line set information in attribute set")
    BufferString storedid = desc->getStoredID();
    LineKey lk( storedid.buf() );
    const MultiID mid( lk.lineName() );
    if ( mid.isEmpty() )
	mErrRet("No line set found in attribute set")
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	mErrRet("Cannot find line set in object management")
    Seis2DLineSet ls( ioobj->fullUserExpr(true) );
    if ( ls.nrLines() < 1 )
	mErrRet("Line set is empty")
    //lk.setLineName( lnmfld_->getInput() );
    const int idxof = ls.indexOf( lk );
    if ( idxof < 0 )
	mErrRet("Cannot find selected line in line set")

    //TODO: use l2ddata_ for something
}*/


#define mDPM DPM(DataPackMgr::PointID())

#undef mErrRet
#define mErrRet(s) \
{ \
    if ( dps ) mDPM.release(dps->id()); \
    if ( s ) uiMSG().error(s); return false; \
}

bool uiAttribCrossPlot::acceptOK( CallBacker* )
{
    DataPointSet* dps = 0;
    PtrMan<Pos::Provider> prov = posprovfld_->createProvider();
    if ( !prov )
	mErrRet("Internal: no Pos::Provider")

    mDynamicCastGet(Pos::Provider2D*,p2d,prov.ptr())
    if ( lnmfld_ )
    {
	for ( int lsidx=0; lsidx<selids_.size(); lsidx++ )
	{
	    PtrMan<IOObj> lsobj = IOM().get( selids_[lsidx] );
	    if ( !lsobj ) continue;

	    S2DPOS().setCurLineSet( lsobj->name() );

	    BufferStringSet lnms = linenmsset_[lsidx];
	    for ( int lidx=0; lidx<lnms.size(); lidx++ )
	    {
		PosInfo::GeomID geomid = S2DPOS().getGeomID( lsobj->name(),
							     lnms.get(lidx) );
		if ( geomid.isOK() )
		    p2d->addLineID( geomid );
	    }
	}
    }

    uiTaskRunner tr( this );
    if ( !prov->initialize( &tr ) )
	return false;

    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<attrsfld_->size(); idx++ )
    {
	if ( ads_.is2D() ? attrsfld_->isItemChecked(idx)
			 : attrsfld_->isSelected(idx) )
	    dcds += new DataColDef( attrsfld_->textOfItem(idx) );
    }
    
    if ( dcds.isEmpty() )
	mErrRet("Please select at least one attribute to evaluate")

    MouseCursorManager::setOverride( MouseCursor::Wait );
    IOPar iop; posfiltfld_->fillPar( iop );
    PtrMan<Pos::Filter> filt = Pos::Filter::make( iop, prov->is2D() );
    MouseCursorManager::restoreOverride();
    if ( filt && !filt->initialize(&tr) )
	return false;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    dps = new DataPointSet( *prov, dcds, filt );
    MouseCursorManager::restoreOverride();
    if ( dps->isEmpty() )
	mErrRet("No positions selected")

    BufferString dpsnm; prov->getSummary( dpsnm );
    if ( filt )
    {
	BufferString filtsumm; filt->getSummary( filtsumm );
	dpsnm += " / "; dpsnm += filtsumm;
    }
    dps->setName( dpsnm );
    IOPar descsetpars;
    ads_.fillPar( descsetpars );
    const_cast<PosVecDataSet*>( &(dps->dataSet()) )->pars() = descsetpars;
    mDPM.addAndObtain( dps );

    BufferString errmsg; Attrib::EngineMan aem;
    //if ( lnmfld_ )
	//aem.setLineKey( lnmfld_->getInput() );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    PtrMan<Executor> tabextr = aem.getTableExtractor( *dps, ads_, errmsg );
    MouseCursorManager::restoreOverride();
    if ( !errmsg.isEmpty() ) mErrRet(errmsg)
	    
    if ( !tr.execute(*tabextr) )
    {
	mDPM.release( dps->id() );
	return false;
    }

    uiDataPointSet* uidps = new uiDataPointSet( this, *dps,
		uiDataPointSet::Setup("Attribute data",false),dpsdispmgr_ );
    dpsset_ += uidps;
    mDPM.release( dps->id() );
    return uidps->go() ? true : false;
}


const DataPointSet& uiAttribCrossPlot::getDPS() const
{ return *curdps_; }

