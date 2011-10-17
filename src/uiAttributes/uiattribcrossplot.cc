/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra / Bert
 Date:          March 2003 / Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattribcrossplot.cc,v 1.54 2011-10-17 10:19:19 cvsbert Exp $";

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
    	, l2ddata_(0)
    	, curdps_(0)
    	, dpsdispmgr_(0)
{
    uiLabeledListBox* llb = new uiLabeledListBox( this,
	    					  "Attributes to calculate" );
    attrsfld_ = llb->box();
    attrsfld_->setMultiSelect( true );

    uiGroup* attgrp = llb;
    if ( ads_.is2D() )
    {
	lnmfld_ = new uiSeis2DLineNameSel( this, true );
	lnmfld_->attach( alignedBelow, llb );
	attgrp = lnmfld_;
	lnmfld_->nameChanged.notify( mCB(this,uiAttribCrossPlot,lnmChg) );
    }

    uiPosProvider::Setup psu( ads_.is2D(), true, true );
    psu.seltxt( "Select locations by" ).choicetype( uiPosProvider::Setup::All );
    posprovfld_ = new uiPosProvider( this, psu );
    posprovfld_->setExtractionDefaults();
    posprovfld_->attach( alignedBelow, attgrp );

    uiPosFilterSet::Setup fsu( ads_.is2D() );
    fsu.seltxt( "Location filters" ).incprovs( true );
    posfiltfld_ = new uiPosFilterSetSel( this, fsu );
    posfiltfld_->attach( alignedBelow, posprovfld_ );

    setDescSet( d );
    finaliseDone.notify( mCB(this,uiAttribCrossPlot,initWin) );
}


void uiAttribCrossPlot::setDescSet( const Attrib::DescSet& newads )
{
    const_cast<Attrib::DescSet&>(ads_) = newads;
    adsChg();
}


void uiAttribCrossPlot::adsChg()
{
    attrsfld_->setEmpty();

    Attrib::SelInfo attrinf( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
	attrsfld_->addItem( attrinf.attrnms_.get(idx), false );
    
    for ( int idx=0; idx<attrinf.ioobjids_.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids_.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms_.get(idx).buf();
	    attrsfld_->addItem( attrinf.is2D(defkey)
		    ? SeisIOObjInfo::defKey2DispName(defkey,ioobjnm)
		    : SeisIOObjInfo::def3DDispName(defkey,ioobjnm) );
	}
    }

    if ( !attrsfld_->isEmpty() )
	attrsfld_->setCurrentItem( int(0) );

    if ( lnmfld_ )
    {
	const Attrib::Desc* desc = ads_.getFirstStored( false );
	if ( !desc ) return;
	const MultiID storedid( desc->getStoredID() );
	if ( storedid.isEmpty() ) return;
	lnmfld_->setLineSet( storedid );
    }
}


uiAttribCrossPlot::~uiAttribCrossPlot()
{
    delete const_cast<Attrib::DescSet*>(&ads_);
    delete l2ddata_;
}


void uiAttribCrossPlot::initWin( CallBacker* )
{
    useLineName( false );
}

void uiAttribCrossPlot::lnmChg( CallBacker* )
{
    useLineName( true );
}



#define mErrRet(s) { if ( emiterr ) uiMSG().error(s); return; }

void uiAttribCrossPlot::useLineName( bool emiterr )
{
    delete l2ddata_; l2ddata_ = 0;
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
    lk.setLineName( lnmfld_->getInput() );
    const int idxof = ls.indexOf( lk );
    if ( idxof < 0 )
	mErrRet("Cannot find selected line in line set")

    S2DPOS().setCurLineSet( ls.name() );
    l2ddata_ = new PosInfo::Line2DData( lk.lineName() );
    if ( !S2DPOS().getGeometry( *l2ddata_ ) )
    {
	delete l2ddata_; l2ddata_ = 0;
	mErrRet("Cannot get geometry of selected line")
    }

    //TODO: use l2ddata_ for something
}


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
	if ( !l2ddata_ )
	    mErrRet("Cannot work without line set position information")
	p2d->setLineData( new PosInfo::Line2DData(*l2ddata_) );
    }

    uiTaskRunner tr( this );
    if ( !prov->initialize( &tr ) )
	return false;

    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<attrsfld_->size(); idx++ )
    {
	if ( attrsfld_->isSelected(idx) )
	    dcds += new DataColDef( attrsfld_->textOfItem(idx) );
    }
    
    if ( dcds.isEmpty() )
	mErrRet("Please select at least one attribute to evaluate")

    MouseCursorManager::setOverride( MouseCursor::Wait );
    IOPar iop; posfiltfld_->fillPar( iop );
    PtrMan<Pos::Filter> filt = Pos::Filter::make( iop, prov->is2D() );
    mDynamicCastGet(Pos::Filter2D*,f2d,filt.ptr())
    if ( f2d )
	f2d->setLineData( new PosInfo::Line2DData(*l2ddata_) );
    MouseCursorManager::restoreOverride();
    if ( filt && !filt->initialize(&tr) )
	return false;

    float estpos = prov->estNrPos();
    estpos *= prov->estNrZPerPos();
    if ( filt )
	estpos *= filt->estRatio( *prov );
    estpos *= 0.001;
    if ( estpos > 1000 )
    {
	BufferString msg( "The estimated number of positions is:\n" );
	od_int64 rounded = (od_int64)estpos;
	msg.add( rounded * 1000 ).add( "\nDo you want to continue?" );
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

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
    if ( lnmfld_ )
	aem.setLineKey( lnmfld_->getInput() );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    PtrMan<Executor> tabextr = aem.getTableExtractor( *dps, ads_, errmsg );
    MouseCursorManager::restoreOverride();
    if ( !errmsg.isEmpty() ) mErrRet(errmsg)
	    
    if ( !tr.execute(*tabextr) )
	return false;

    uiDataPointSet* uidps = new uiDataPointSet( this, *dps,
		uiDataPointSet::Setup("Attribute data",false),dpsdispmgr_ );
    return uidps->go() ? true : false;
}


const DataPointSet& uiAttribCrossPlot::getDPS() const
{ return *curdps_; }

