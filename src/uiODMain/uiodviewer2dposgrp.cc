/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno/Satyaki
Date:	       Aug 2010/March 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uiodviewer2dposgrp.h"


#include "uiattrsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiseissubsel.h"
#include "uiodscenemgr.h"
#include "uiwellto2dlinedlg.h"
#include "uiwellpartserv.h"

#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attribdesc.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "randomlinetr.h"
#include "randomlinegeom.h"
#include "survinfo.h"
#include "zdomain.h"


DefineEnumNames(uiODViewer2DPosGrp,PosType,0,"Position Type")
{ "Random Line", "Inline", "Crossline", "2D Line", "Z-Slice", 0 };

uiODViewer2DPosGrp::uiODViewer2DPosGrp( uiParent* p, Viewer2DPosDataSel& sd,
					bool wantz )
    : uiGroup( p )
    , applmgr_(0)
    , scenemgr_(0)
    , posdatasel_(sd)
    , rdmlinefld_(0)
    , postypefld_(0)
    , inp2dfld_(0)
    , inp3dfld_(0)
    , subsel2dfld_(0)
    , withz_(wantz)
    , inpSelected(this)
{
    const CallBack inpcb( mCB(this,uiODViewer2DPosGrp,inpSel) );
    BufferStringSet geoms;
    if ( SI().has3D() )
    {
	geoms += new BufferString( toString(uiODViewer2DPosGrp::InLine) );
	geoms += new BufferString( toString(uiODViewer2DPosGrp::CrossLine) );
	if ( withz_ )
	    geoms += new BufferString( toString(uiODViewer2DPosGrp::ZSlice) );
	geoms += new BufferString( toString(uiODViewer2DPosGrp::RdmLine) );
    }
    if ( SI().has2D() )
	geoms += new BufferString( toString(uiODViewer2DPosGrp::Line2D) );

    uiLabeledComboBox* postypelcb =
	new uiLabeledComboBox( this, tr("Select data type") );
    postypefld_ = postypelcb->box();
    postypefld_->addItems( geoms );
    postypefld_->selectionChanged.notify( inpcb );

    static const char* createlinetxt = "Create from Wells";

    if ( SI().has2D() )
    {
	Attrib::DescSet* ads = Attrib::eDSHolder().getDescSet(true,true);
	inp2dfld_ = new uiAttrSel( this, *ads, 0, ads->getID(0) );
	inp2dfld_->selectionDone.notify( inpcb );
	if ( postypelcb )
	    inp2dfld_->attach( alignedBelow, postypelcb );
	inp2dfld_->selectionDone.notify(
		mCB(this,uiODViewer2DPosGrp,attr2DSelected));

	gen2dlinebut_ =
	    new uiPushButton( this, createlinetxt,
			      mCB(this,uiODViewer2DPosGrp,gen2DLine), true);
	gen2dlinebut_->attach( rightOf, inp2dfld_ );

	subsel2dfld_ = new uiSeis2DSubSel( this, Seis::SelSetup(true) );
	subsel2dfld_->attachObj()->attach( alignedBelow, inp2dfld_ );
	attr2DSelected(0);
    }

    if ( SI().has3D() )
    {
	const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet(false,false);

	inp3dfld_ = new uiAttrSel( this, *ads, 0, ads->getID(0) );
	inp3dfld_->selectionDone.notify( inpcb );
	if ( postypelcb )
	    inp3dfld_->attach( alignedBelow, postypelcb );

	createSliceSel( uiSliceSel::Inl );
	createSliceSel( uiSliceSel::Crl );
	if ( withz_ )
	    createSliceSel( uiSliceSel::Tsl );
	rdmlinefld_ = new uiIOObjSel( this, mIOObjContext(RandomLineSet),
				      tr("Input Random Line") );
	rdmlinefld_->attach( alignedBelow, inp3dfld_ );
	rdmlinefld_->selectionDone.notify( inpcb );

	genrdmlinebut_ =
	    new uiPushButton( this, createlinetxt,
			      mCB(this,uiODViewer2DPosGrp,genRdmLine),true);
	genrdmlinebut_->attach( rightOf, rdmlinefld_ );
    }

    inpSel( 0 );
}


uiODViewer2DPosGrp::~uiODViewer2DPosGrp()
{
    detachAllNotifiers();
}


void uiODViewer2DPosGrp::setApplSceneMgr( uiODMain& appl )
{
    applmgr_ = &appl.applMgr();
    scenemgr_ = &appl.sceneMgr();
}


bool uiODViewer2DPosGrp::is2D() const
{
    return tp_ == uiODViewer2DPosGrp::Line2D;
}

void uiODViewer2DPosGrp::createSliceSel( uiSliceSel::Type dir )
{
    ZDomain::Info zinfo( ZDomain::SI() );
    uiSliceSel* sliceselfld = new uiSliceSel( this, dir, zinfo );
    sliceselfld->attach( alignedBelow, inp3dfld_ );
    sliceselfld->enableScrollButton( false );
    const TrcKeyZSampling& seltkzs = posdatasel_.tkzs_;
    TrcKeyZSampling sliceseltkzs = posdatasel_.tkzs_;
    if( dir == uiSliceSel::Inl )
    {
	sliceseltkzs.hsamp_.start.inl() = seltkzs.hsamp_.lineRange().center();
	sliceseltkzs.hsamp_.stop.inl() = sliceseltkzs.hsamp_.start.inl();
    }
    else if ( dir == uiSliceSel::Crl )
    {
	sliceseltkzs.hsamp_.start.crl() = seltkzs.hsamp_.trcRange().center();
	sliceseltkzs.hsamp_.stop.crl() = sliceseltkzs.hsamp_.start.crl();
    }
    else
    {
	sliceseltkzs.zsamp_.start = seltkzs.zsamp_.center();
	sliceseltkzs.zsamp_.stop = sliceseltkzs.zsamp_.start;
    }

    sliceselfld->setTrcKeyZSampling( sliceseltkzs );
    sliceselflds_ += sliceselfld;
}

IOObj* uiODViewer2DPosGrp::get2DObj()
{
    const Attrib::DescSet& ads = inp2dfld_->getAttrSet();
    const Attrib::Desc* desc = ads.getDesc( inp2dfld_->attribID() );
    if ( !desc ) desc = ads.getFirstStored();
    if ( !desc ) return 0;
    MultiID stored2did( desc->getStoredID(true) );
    return IOM().get( stored2did );
}


void uiODViewer2DPosGrp::attr2DSelected( CallBacker* )
{
    if ( !is2D() || !inp2dfld_ ) return;
    PtrMan<IOObj> ioobj = get2DObj();
    if ( !ioobj ) return;

    subsel2dfld_->setInput( *ioobj );
}


void uiODViewer2DPosGrp::getPosSubSel()
{
    if ( is2D() )
    {
	posdatasel_.geomid_ =
	    Survey::GM().getGeomID( subsel2dfld_->selectedLine() );
	subsel2dfld_->getSampling( posdatasel_.tkzs_ );
	return;
    }

    posdatasel_.geomid_ = Survey::GeometryManager::cUndefGeomID();
    if ( sliceselflds_.isEmpty() )
	return;

    if ( tp_ == uiODViewer2DPosGrp::InLine )
    {
	sliceselflds_[0]->acceptOK();
	posdatasel_.tkzs_ = sliceselflds_[0]->getTrcKeyZSampling();
    }

    if ( tp_ == uiODViewer2DPosGrp::CrossLine )
    {
	sliceselflds_[1]->acceptOK();
	posdatasel_.tkzs_ = sliceselflds_[1]->getTrcKeyZSampling();
    }

    if ( withz_ && tp_ == uiODViewer2DPosGrp::ZSlice )
    {
	sliceselflds_[2]->acceptOK();
	posdatasel_.tkzs_ = sliceselflds_[2]->getTrcKeyZSampling();
    }

    if (  tp_ == uiODViewer2DPosGrp::RdmLine )
    {
	if ( !rdmlinefld_ || !rdmlinefld_->commitInput() )
	    return;

    }
}


void uiODViewer2DPosGrp::showDataSelField( bool yn )
{
    posdatasel_.selectdata_ = yn;
    uiAttrSel* attrsel = is2D() ? inp2dfld_ : inp3dfld_;
    if ( attrsel )
	attrsel->display( yn );
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiODViewer2DPosGrp::acceptOK()
{
    uiAttrSel* attrsel = is2D() ? inp2dfld_ : inp3dfld_;
    if ( attrsel && posdatasel_.selectdata_ )
    {
	attrsel->processInput();
	BufferString attrnm = attrsel->getAttrName();
	if ( attrnm.isEmpty() )
	    { mErrRet( tr("Please select a valid attribute") ) }

	attrsel->fillSelSpec( posdatasel_.selspec_ );
	posdatasel_.selspec_.setUserRef( attrnm );
    }

    if ( tp_ == uiODViewer2DPosGrp::RdmLine )
    {
	const IOObj* rdlinobj = rdmlinefld_->ioobj( true );
	if ( !rdlinobj )
	    mErrRet( tr("Please select a random line") )
	posdatasel_.rdmlineid_ = rdlinobj->key();
    }

    getPosSubSel();
    return true;
}


void uiODViewer2DPosGrp::gen2DLine( CallBacker* )
{
    if ( !applmgr_ ) return;
    uiWellTo2DLineDlg dlg( this );
    dlg.wantspreview_.notify( mCB(this,uiODViewer2DPosGrp,preview2DLine) );
    if ( dlg.go() )
    {
	Pos::GeomID geomid = -1;
	BufferString nm;
	dlg.get2DLineID( geomid, nm );
	subsel2dfld_->setSelectedLine( nm.buf() );
	if ( dlg.dispOnCreation() )
	    scenemgr_->add2DLineItem( geomid, -1 );

	inpSelected.trigger();
    }
}


void uiODViewer2DPosGrp::genRdmLine( CallBacker* )
{
    if ( !applmgr_ ) return;
    applmgr_->wellServer()->setSceneID( scenemgr_->askSelectScene() );
    applmgr_->wellServer()->selectWellCoordsForRdmLine();
    mAttachCBIfNotAttached(applmgr_->wellServer()->randLineDlgClosed,
			   uiODViewer2DPosGrp::rdmLineDlgClosed);
}


void uiODViewer2DPosGrp::rdmLineDlgClosed( CallBacker* )
{
    if ( !applmgr_ ) return;
    const char* multiid = applmgr_->wellServer()->getRandLineMultiID();
    if ( multiid && rdmlinefld_ )
    {
	MultiID mid( multiid );
	rdmlinefld_->setInput( mid );
    }

    inpSelected.trigger();
}


void uiODViewer2DPosGrp::preview2DLine( CallBacker* cb )
{
    if ( !applmgr_ ) return;
    mDynamicCastGet(uiWellTo2DLineDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("can't find 2d line dlg"); }
    TypeSet<Coord> coords;
    dlg->getCoordinates( coords );
    applmgr_->setupRdmLinePreview( coords );
}


void uiODViewer2DPosGrp::inpSel( CallBacker* )
{
    if ( !postypefld_ ) return;
    const char* txtofinp = postypefld_->text();
    uiODViewer2DPosGrp::parseEnum( txtofinp, tp_ );

    if ( SI().has2D() )
    {
	inp2dfld_->display( is2D() );
	subsel2dfld_->display( is2D() );
	gen2dlinebut_->display( is2D() && applmgr_ );
	attr2DSelected(0);
    }
    if ( SI().has3D() )
    {
	inp3dfld_->display( !is2D() && posdatasel_.selectdata_ );
	genrdmlinebut_->display(tp_ == uiODViewer2DPosGrp::RdmLine && applmgr_);
	sliceselflds_[0]->display( tp_ == uiODViewer2DPosGrp::InLine );
	sliceselflds_[1]->display( tp_ == uiODViewer2DPosGrp::CrossLine );
	if ( withz_ )
	    sliceselflds_[2]->display( tp_ == uiODViewer2DPosGrp::ZSlice );
	posdatasel_.tkzs_ =
	    tp_ == uiODViewer2DPosGrp::InLine
		?  sliceselflds_[0]->getTrcKeyZSampling()
		: tp_ == uiODViewer2DPosGrp::CrossLine
			? sliceselflds_[1]->getTrcKeyZSampling()
			: tp_ == uiODViewer2DPosGrp::ZSlice
				? sliceselflds_[2]->getTrcKeyZSampling()
				: 0;
	if ( rdmlinefld_ )
	{
	    NotifyStopper rdmlinefldselstopper( rdmlinefld_->selectionDone );
	    rdmlinefld_->display( tp_ == uiODViewer2DPosGrp::RdmLine );
	}
    }

    inpSelected.trigger();
}


void uiODViewer2DPosGrp::getRdmLineGeom( TypeSet<BinID>& knots,
					 StepInterval<float>* zrg )
{
    Geometry::RandomLineSet rls; BufferString errmsg;
    const PtrMan<IOObj> rdmline = IOM().get( posdatasel_.rdmlineid_ );
    RandomLineSetTranslator::retrieve( rls, rdmline, errmsg );
    if ( !errmsg.isEmpty() || rls.isEmpty() )
	return;

    rls.lines()[0]->allNodePositions( knots );
    if ( zrg )
	(*zrg) = rls.lines()[0]->zRange();
}


void Viewer2DPosDataSel::fillPar( IOPar& iop ) const
{
    tkzs_.fillPar( iop );
    selspec_.fillPar( iop );
    iop.set( sKeyRdmLineID(), rdmlineid_ );
    iop.setYN( sKeySelectData(), selectdata_ );
    iop.set( sKey::GeomID(), geomid_ );
}


void Viewer2DPosDataSel::usePar( const IOPar& iop )
{
    tkzs_.usePar( iop );
    selspec_.usePar( iop );
    iop.get( sKeyRdmLineID(), rdmlineid_ );
    iop.getYN( sKeySelectData(), selectdata_ );
    iop.get( sKey::GeomID(), geomid_ );
}
