/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodviewer2dposgrp.h"


#include "uiattrsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiseissubsel.h"
#include "uiwellpartserv.h"
#include "uiwellattribpartserv.h"

#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attribdesc.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "randomlinetr.h"
#include "randomlinegeom.h"
#include "zdomain.h"


mDefineEnumUtils(Viewer2DPosDataSel,PosType,"Position Type")
{ "In-line", "Cross-line", "2D line", "Z-slice", "Random line", 0 };


template <>
void EnumDefImpl<Viewer2DPosDataSel::PosType>::init()
{
    uistrings_ += uiStrings::sInline();
    uistrings_ += uiStrings::sCrossline();
    uistrings_ += uiStrings::s2DLine();
    uistrings_ += uiStrings::sZSlice();
    uistrings_ += uiStrings::sRandomLine();
}


#define mToPosTypeStr( postype ) \
    Viewer2DPosDataSel::toString( postype )

uiODViewer2DPosGrp::uiODViewer2DPosGrp( uiParent* p,
	Viewer2DPosDataSel* posdatasel, bool onlyvertical, bool withpostype )
    : uiGroup(p)
    , applmgr_(0)
    , rdmlinefld_(0)
    , postypefld_(0)
    , inp2dfld_(0)
    , inp3dfld_(0)
    , subsel2dfld_(0)
    , topgrp_(0)
    , botgrp_(0)
    , onlyvertical_(onlyvertical)
    , posdatasel_(posdatasel ? posdatasel : new Viewer2DPosDataSel() )
    , inpSelected(this)
{
    const CallBack inpcb( mCB(this,uiODViewer2DPosGrp,inpSel) );
    BufferStringSet geoms;
    if ( SI().has3D() )
    {
	geoms += new BufferString(mToPosTypeStr(Viewer2DPosDataSel::InLine));
	geoms += new BufferString(mToPosTypeStr(Viewer2DPosDataSel::CrossLine));
	if ( !onlyvertical_ )
	    geoms +=
		new BufferString( mToPosTypeStr(Viewer2DPosDataSel::ZSlice) );
	geoms += new BufferString( mToPosTypeStr(Viewer2DPosDataSel::RdmLine) );
    }
    if ( SI().has2D() )
	geoms += new BufferString( mToPosTypeStr(Viewer2DPosDataSel::Line2D) );

    if ( !withpostype )
    {
	postypefld_ = new uiLabeledComboBox( this, tr("Position type") );
	postypefld_->box()->addItems( geoms );
	postypefld_->box()->selectionChanged.notify( inpcb );
	topgrp_ = postypefld_;
    }

    uiString createlinetxt = tr("Create from Wells");

    if ( SI().has2D() )
    {
	Attrib::DescSet* ads = Attrib::eDSHolder().getDescSet(true,false);
	inp2dfld_ = new uiAttrSel( this, *ads, 0, ads->getID(0) );
	inp2dfld_->selectionDone.notify( inpcb );
	if ( postypefld_ )
	    inp2dfld_->attach( alignedBelow, postypefld_ );
	inp2dfld_->selectionDone.notify(
		mCB(this,uiODViewer2DPosGrp,attr2DSelected));

	gen2dlinebut_ =
	    new uiPushButton( this, createlinetxt,
			      mCB(this,uiODViewer2DPosGrp,gen2DLine), true );
	gen2dlinebut_->attach( rightOf, inp2dfld_ );

	subsel2dfld_ = new uiSeis2DSubSel( this, Seis::SelSetup(true) );
	subsel2dfld_->attachObj()->attach( alignedBelow, inp2dfld_ );
	botgrp_ = subsel2dfld_;
	attr2DSelected(0);
	setHAlignObj( inp2dfld_ );
    }

    if ( SI().has3D() )
    {
	const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet(false,false);

	inp3dfld_ = new uiAttrSel( this, *ads, 0, ads->getID(0) );
	inp3dfld_->selectionDone.notify( inpcb );
	if ( postypefld_ )
	    inp3dfld_->attach( alignedBelow, postypefld_ );

	createSliceSel( uiSliceSel::Inl );
	createSliceSel( uiSliceSel::Crl );
	if ( !onlyvertical_ )
	    createSliceSel( uiSliceSel::Tsl );
	rdmlinefld_ = new uiIOObjSel( this, mIOObjContext(RandomLineSet),
				      tr("Input Random line") );
	rdmlinefld_->attach( alignedBelow, inp3dfld_ );
	rdmlinefld_->selectionDone.notify( inpcb );
	botgrp_ = sliceselflds_.first();

	genrdmlinebut_ =
	    new uiPushButton( this, createlinetxt,
			      mCB(this,uiODViewer2DPosGrp,genRdmLine),true);
	genrdmlinebut_->attach( rightOf, rdmlinefld_ );
	setHAlignObj( inp3dfld_ );
    }

    updatePosFlds();
    updateDataSelFld();
}


uiODViewer2DPosGrp::~uiODViewer2DPosGrp()
{
    detachAllNotifiers();
    delete posdatasel_;
}


void uiODViewer2DPosGrp::fillPar( IOPar& selpar ) const
{
    posdatasel_->fillPar( selpar );
}


void uiODViewer2DPosGrp::usePar( const IOPar& selpar )
{
    posdatasel_->usePar( selpar );

    if ( postypefld_ )
	postypefld_->box()->setText( mToPosTypeStr(posdatasel_->postype_) );

    updateFlds();
}


void uiODViewer2DPosGrp::updateFlds()
{
    updatePosFlds();
    updateDataSelFld();
    updateTrcKeySampFld();
}


void uiODViewer2DPosGrp::setApplSceneMgr( uiODMain& appl )
{
    applmgr_ = &appl.applMgr();
}


bool uiODViewer2DPosGrp::is2D() const
{
    return posdatasel_->postype_ == Viewer2DPosDataSel::Line2D;
}


void uiODViewer2DPosGrp::getSelAttrSamp( TrcKeyZSampling& seltkzs )
{
    uiAttrSel* attr = is2D() ? inp2dfld_ : inp3dfld_;
    attr->getRanges( seltkzs );
}


void uiODViewer2DPosGrp::createSliceSel( uiSliceSel::Type dir )
{

    ZDomain::Info zinfo( SI().zDomain() );
    uiSliceSel* sliceselfld = new uiSliceSel( this, dir, zinfo );
    sliceselfld->attach( alignedBelow, inp3dfld_ );
    sliceselfld->enableScrollButton( false );

    TrcKeyZSampling seltkzs;
    getSelAttrSamp( seltkzs );
    TrcKeyZSampling sliceseltkzs;

    if( dir == uiSliceSel::Inl )
    {
	sliceseltkzs.hsamp_.start_.inl() = seltkzs.hsamp_.lineRange()
								    .center();
	sliceseltkzs.hsamp_.stop_.inl() = sliceseltkzs.hsamp_.start_.inl();
    }
    else if ( dir == uiSliceSel::Crl )
    {
	sliceseltkzs.hsamp_.start_.crl() = seltkzs.hsamp_.trcRange()
								    .center();
	sliceseltkzs.hsamp_.stop_.crl() = sliceseltkzs.hsamp_.start_.crl();
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

    const MultiID stored2did( desc->getStoredID(true).buf() );
    return IOM().get( stored2did );
}


void uiODViewer2DPosGrp::attr2DSelected( CallBacker* )
{
    if ( !is2D() || !inp2dfld_ ) return;
    PtrMan<IOObj> ioobj = get2DObj();
    if ( !ioobj ) return;

    subsel2dfld_->setInput( *ioobj );
}


#define mErrRet(s) { if ( emiterror ) uiMSG().error(s); return false; }
bool uiODViewer2DPosGrp::commitSel( bool emiterror )
{
    posdatasel_->geomid_ = Survey::GeometryManager::cUndefGeomID();

    switch ( posdatasel_->postype_ )
    {
	case Viewer2DPosDataSel::Line2D :
	    posdatasel_->geomid_ =
		Survey::GM().getGeomID( subsel2dfld_->selectedLine() );
	    subsel2dfld_->getSampling( posdatasel_->tkzs_,
				       posdatasel_->geomid_ );
	    break;
	case Viewer2DPosDataSel::InLine:
	    if ( !sliceselflds_.validIdx(0) ) return false;
	    sliceselflds_[0]->acceptOK();
	    posdatasel_->tkzs_ = sliceselflds_[0]->getTrcKeyZSampling();
	    break;
	case Viewer2DPosDataSel::CrossLine:
	    if ( !sliceselflds_.validIdx(1) ) return false;
	    sliceselflds_[1]->acceptOK();
	    posdatasel_->tkzs_ = sliceselflds_[1]->getTrcKeyZSampling();
	    break;
	case Viewer2DPosDataSel::ZSlice:
	    if ( !sliceselflds_.validIdx(2) ) return false;
	    sliceselflds_[2]->acceptOK();
	    posdatasel_->tkzs_ = sliceselflds_[2]->getTrcKeyZSampling();
	    break;
	case Viewer2DPosDataSel::RdmLine:
	    const IOObj* rdlobj = rdmlinefld_->ioobj(!emiterror);
	    if ( !rdlobj )
		return false;
	    posdatasel_->rdmlinemultiid_ = rdlobj->key();
	    const Geometry::RandomLine* rdlgeom =
				Geometry::RLM().get( rdlobj->key() );
	    posdatasel_->rdmlineid_ =
			rdlgeom ? rdlgeom->ID() : RandomLineID::udf();
	    break;
    }

    uiAttrSel* attrsel = is2D() ? inp2dfld_ : inp3dfld_;
    if ( attrsel && posdatasel_->selectdata_ )
    {
	BufferString attrnm = attrsel->getAttrName();
	if ( attrnm.isEmpty() )
	    { mErrRet( tr("Please select a valid attribute") ) }

	attrsel->fillSelSpec( posdatasel_->selspec_ );
	posdatasel_->selspec_.setUserRef( attrnm );
	posdatasel_->selspec_.set2DFlag( is2D() );
    }

    return true;
}


void uiODViewer2DPosGrp::showDataSelField( bool yn )
{
    posdatasel_->selectdata_ = yn;
    updateDataSelFld();
}


void uiODViewer2DPosGrp::updateDataSelFld()
{
    uiAttrSel* attrsel = is2D() ? inp2dfld_ : inp3dfld_;
    if ( attrsel )
    {
	attrsel->display( posdatasel_->selectdata_ );
	if ( posdatasel_->selspec_.id().isValid() )
	    attrsel->setSelSpec( &posdatasel_->selspec_ );
    }

}


void uiODViewer2DPosGrp::updateTrcKeySampFld()
{
    const TrcKeyZSampling seltkzs = posdatasel_->tkzs_;
    switch ( posdatasel_->postype_ )
    {
	case Viewer2DPosDataSel::Line2D :
	{
	    subsel2dfld_->setSelectedLine(
		    Survey::GM().getName(posdatasel_->geomid_) );
	    subsel2dfld_->uiSeisSubSel::setInput( seltkzs.hsamp_ );
	    break;
	}
	case Viewer2DPosDataSel::InLine:
	    sliceselflds_[0]->setTrcKeyZSampling( seltkzs );
	    break;
	case Viewer2DPosDataSel::CrossLine:
	    sliceselflds_[1]->setTrcKeyZSampling( seltkzs );
	    break;
	case Viewer2DPosDataSel::ZSlice:
	    sliceselflds_[2]->setTrcKeyZSampling( seltkzs );
	    break;
	case Viewer2DPosDataSel::RdmLine:
	    rdmlinefld_->setInput( posdatasel_->rdmlinemultiid_ );
	    break;
    }
}


void uiODViewer2DPosGrp::gen2DLine( CallBacker* )
{
    if ( !applmgr_ ) return;

    MultiID newseis2did;
    Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
    if ( applmgr_->wellAttribServer()->create2DFromWells(newseis2did,geomid) &&
	 geomid != Survey::GeometryManager::cUndefGeomID() )
    {
	const char* sellinenm = Survey::GM().getName( geomid );
	subsel2dfld_->uiSeisSubSel::setInput( newseis2did );
	subsel2dfld_->setSelectedLine( sellinenm );
	inpSelected.trigger();
    }
}


void uiODViewer2DPosGrp::genRdmLine( CallBacker* )
{
    if ( !applmgr_ ) return;

    applmgr_->wellServer()->selectWellCoordsForRdmLine();
    mAttachCBIfNotAttached(applmgr_->wellServer()->randLineDlgClosed,
			   uiODViewer2DPosGrp::rdmLineDlgClosed);
}


void uiODViewer2DPosGrp::rdmLineDlgClosed( CallBacker* )
{
    if ( !applmgr_ ) return;

    const MultiID mid = applmgr_->wellServer()->getRandLineMultiID();
    if ( !mid.isUdf() && rdmlinefld_ )
	rdmlinefld_->setInput( mid );

    inpSelected.trigger();
}


void uiODViewer2DPosGrp::inpSel( CallBacker* cb )
{
    if ( postypefld_ )
    {
	const char* txtofinp = postypefld_->box()->text();
	Viewer2DPosDataSel::parseEnum( txtofinp, posdatasel_->postype_ );
    }

    updatePosFlds();
    if ( cb == inp3dfld_ )
    {
	getSelAttrSamp( posdatasel_->tkzs_ );
	updateTrcKeySampFld();
    }

    inpSelected.trigger();
}


void uiODViewer2DPosGrp::updatePosFlds()
{
    if ( SI().has2D() )
    {
	inp2dfld_->display( is2D() && posdatasel_->selectdata_ );
	subsel2dfld_->display( is2D() );
	gen2dlinebut_->display( is2D() && applmgr_ );
	attr2DSelected(0);
    }

    if ( SI().has3D() )
    {
	inp3dfld_->display( !is2D() && posdatasel_->selectdata_ );
	genrdmlinebut_->display(
		posdatasel_->postype_==Viewer2DPosDataSel::RdmLine && applmgr_);
	sliceselflds_[0]->display(
		posdatasel_->postype_==Viewer2DPosDataSel::InLine );
	sliceselflds_[1]->display(
		posdatasel_->postype_==Viewer2DPosDataSel::CrossLine );
	if ( !onlyvertical_ )
	    sliceselflds_[2]->display(
		    posdatasel_->postype_==Viewer2DPosDataSel::ZSlice );

	if ( rdmlinefld_ )
	{
	    NotifyStopper rdmlinefldselstopper( rdmlinefld_->selectionDone );
	    rdmlinefld_->display(
		    posdatasel_->postype_ == Viewer2DPosDataSel::RdmLine );
	}
    }
}


void Viewer2DPosDataSel::fillPar( IOPar& iop ) const
{
    tkzs_.fillPar( iop );
    selspec_.fillPar( iop );
    if ( postype_ == Viewer2DPosDataSel::RdmLine )
    {
	iop.set( sKeyRdmLineMultiID(), rdmlinemultiid_ );
	iop.set( sKeyRdmLineID(), rdmlineid_.asInt() );
    }

    iop.setYN( sKeySelectData(), selectdata_ );
    iop.set( sKey::GeomID(), geomid_ );
    iop.set( PosTypeDef().name(), toString(postype_) );
}


void Viewer2DPosDataSel::usePar( const IOPar& iop )
{
    tkzs_.usePar( iop );
    selspec_.usePar( iop );
    parseEnum( iop, PosTypeDef().name(), postype_ );
    if ( postype_ == Viewer2DPosDataSel::RdmLine )
    {
	iop.get( sKeyRdmLineID(), rdmlineid_ );
	iop.get( sKeyRdmLineMultiID(), rdmlinemultiid_ );
    }
    else
	rdmlinemultiid_.setUdf();

    iop.getYN( sKeySelectData(), selectdata_ );
    iop.get( sKey::GeomID(), geomid_ );
}
