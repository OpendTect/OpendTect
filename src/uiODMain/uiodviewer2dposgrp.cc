/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno/Satyaki
Date:	       Aug 2010/March 2015
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
#include "attribdesc.h"
#include "attribprobelayer.h"
#include "ioobjctxt.h"
#include "keystrs.h"
#include "probeimpl.h"
#include "randomlinetr.h"
#include "randomlinegeom.h"
#include "randomlineprobe.h"
#include "seisselsetup.h"
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
	const Attrib::DescSet& ads = Attrib::DescSet::global( true );
	inp2dfld_ = new uiAttrSel( this, ads, uiAttrSel::sDefLabel(),
				   ads.getID(0) );
	inp2dfld_->selectionChanged.notify( inpcb );
	inp2dfld_->selectionChanged.notify(
		mCB(this,uiODViewer2DPosGrp,attr2DSelected));
	if ( postypefld_ )
	    inp2dfld_->attach( alignedBelow, postypefld_ );

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
	const Attrib::DescSet& ads = Attrib::DescSet::global( false );

	inp3dfld_ = new uiAttrSel( this, ads, uiAttrSel::sDefLabel(),
				   ads.getID(0) );
	inp3dfld_->selectionChanged.notify( inpcb );
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

    if ( postypefld_)
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


void uiODViewer2DPosGrp::createSliceSel( uiSliceSel::Type dir )
{
    ZDomain::Info zinfo( ZDomain::SI() );
    uiSliceSel* sliceselfld = new uiSliceSel( this, dir, zinfo );
    sliceselfld->attach( alignedBelow, inp3dfld_ );
    sliceselfld->enableScrollButton( false );
    const TrcKeyZSampling& seltkzs = posdatasel_->tkzs_;
    TrcKeyZSampling sliceseltkzs = posdatasel_->tkzs_;
    if( dir == uiSliceSel::Inl )
    {
	sliceseltkzs.hsamp_.start_.inl() = seltkzs.hsamp_.lineRange().center();
	sliceseltkzs.hsamp_.stop_.inl() = sliceseltkzs.hsamp_.start_.inl();
    }
    else if ( dir == uiSliceSel::Crl )
    {
	sliceseltkzs.hsamp_.start_.crl() = seltkzs.hsamp_.trcRange().center();
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
    const Attrib::DescSet& ads = inp2dfld_->attrSet();
    const Attrib::Desc* desc = ads.getDesc( inp2dfld_->attribID() );
    if ( !desc ) desc = ads.getFirstStored();
    if ( !desc ) return 0;

    const DBKey stored2did( desc->getStoredID(true) );
    return stored2did.getIOObj();
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
    posdatasel_->geomid_ = Pos::GeomID();

    switch ( posdatasel_->postype_ )
    {
	case Viewer2DPosDataSel::Line2D :
	    posdatasel_->geomid_ =
		SurvGeom::getGeomID( subsel2dfld_->selectedLine() );
	    subsel2dfld_->getSampling( posdatasel_->tkzs_ );
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
	    posdatasel_->rdmlinedbkey_ = rdlobj->key();
	    posdatasel_->rdmlineid_ =
		Geometry::RLM().get( rdlobj->key() )->ID();
	    break;
    }

    uiAttrSel* attrsel = is2D() ? inp2dfld_ : inp3dfld_;
    if ( attrsel && posdatasel_->selectdata_ )
    {
	BufferString attrnm = attrsel->getAttrName();
	if ( attrnm.isEmpty() )
	    { mErrRet( uiStrings::phrSelect(tr("a valid attribute")) ) }

	attrsel->fillSelSpec( posdatasel_->selspec_ );
	posdatasel_->selspec_.setUserRef( attrnm );
	posdatasel_->selspec_.set2D( is2D() );
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
    const TrcKeyZSampling& tkzs = posdatasel_->tkzs_;
    switch ( posdatasel_->postype_ )
    {
	case Viewer2DPosDataSel::Line2D :
	{
	    subsel2dfld_->setSelectedLine( posdatasel_->geomid_.name() );
	    subsel2dfld_->uiSeisSubSel::setInput( tkzs.hsamp_ );
	    break;
	}
	case Viewer2DPosDataSel::InLine:
	    sliceselflds_[0]->setTrcKeyZSampling( tkzs );
	    break;
	case Viewer2DPosDataSel::CrossLine:
	    sliceselflds_[1]->setTrcKeyZSampling( tkzs );
	    break;
	case Viewer2DPosDataSel::ZSlice:
	    sliceselflds_[2]->setTrcKeyZSampling( tkzs );
	    break;
	case Viewer2DPosDataSel::RdmLine:
	    rdmlinefld_->setInput( posdatasel_->rdmlinedbkey_ );
	    break;
    }
}


void uiODViewer2DPosGrp::gen2DLine( CallBacker* )
{
    if ( !applmgr_ )
	return;

    DBKey newseis2did;
    Pos::GeomID geomid;
    if ( applmgr_->wellAttribServer()->create2DFromWells(newseis2did,geomid) &&
	 geomid.isValid() )
    {
	const BufferString sellinenm = geomid.name();
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

    const DBKey dbky = applmgr_->wellServer()->getRandLineDBKey();
    if ( dbky.isValid() && rdmlinefld_ )
	rdmlinefld_->setInput( dbky );

    inpSelected.trigger();
}


void uiODViewer2DPosGrp::inpSel( CallBacker* )
{
    if ( postypefld_ )
    {
	const char* txtofinp = postypefld_->box()->text();
	Viewer2DPosDataSel::PosTypeDef().parse( txtofinp,
                                                posdatasel_->postype_ );
    }

    updatePosFlds();
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
	iop.set( sKeyRdmLineDBKey(), rdmlinedbkey_ );
	iop.set( sKeyRdmLineID(), rdmlineid_ );
    }

    iop.setYN( sKeySelectData(), selectdata_ );
    iop.set( sKey::GeomID(), geomid_ );
    iop.set( PosTypeDef().name(), toString(postype_) );
}


void Viewer2DPosDataSel::usePar( const IOPar& iop )
{
    tkzs_.usePar( iop );
    selspec_.usePar( iop );
    PosTypeDef().parse( iop, PosTypeDef().name(), postype_ );
    if ( postype_ == Viewer2DPosDataSel::RdmLine )
    {
	iop.get( sKeyRdmLineID(), rdmlineid_ );
	iop.get( sKeyRdmLineDBKey(), rdmlinedbkey_ );
    }
    else
	rdmlinedbkey_.setInvalid();

    iop.getYN( sKeySelectData(), selectdata_ );
    iop.get( sKey::GeomID(), geomid_ );
}


Probe* Viewer2DPosDataSel::createNewProbe()
{
    Probe* probe = 0;
    switch ( postype_ )
    {
	case InLine:
	{
	    probe = new InlineProbe();
	    break;
	}
	case CrossLine:
	{
	    probe = new CrosslineProbe();
	    break;
	}
	case Line2D:
	{
	    Line2DProbe* l2dprobe = new Line2DProbe();
	    l2dprobe->setGeomID( geomid_ );
	    probe = l2dprobe;
	    break;
	}
	case ZSlice:
	{
	    probe = new ZSliceProbe();
	    break;
	}
	case RdmLine:
	{
	    RandomLineProbe* rdlprobe = new RandomLineProbe();
	    rdlprobe->setRandomLineID( rdmlineid_ );
	    probe = rdlprobe;
	    break;
	}
	default: break;
    }

    probe->setPos( tkzs_ );

    AttribProbeLayer* attrlayer = new AttribProbeLayer();
    attrlayer->setSelSpec( selspec_ );
    probe->addLayer( attrlayer );
    return probe;
}


void Viewer2DPosDataSel::fillFromProbe( const Probe& probe )
{
    if ( probe.type()==InlineProbe::sFactoryKey() )
	postype_ = Viewer2DPosDataSel::InLine;
    else if ( probe.type()==CrosslineProbe::sFactoryKey() )
	postype_ = Viewer2DPosDataSel::CrossLine;
    else if ( probe.type()==ZSliceProbe::sFactoryKey() )
	postype_ = Viewer2DPosDataSel::ZSlice;
    else if ( probe.type()==Line2DProbe::sFactoryKey() )
    {
	postype_ = Viewer2DPosDataSel::Line2D;
	geomid_ = probe.position().hsamp_.getGeomID();
    }
    else if ( probe.type()==RandomLineProbe::sFactoryKey() )
    {
	postype_ = Viewer2DPosDataSel::RdmLine;
	mDynamicCastGet(const RandomLineProbe*,rdlprobe,&probe);
	if ( rdlprobe )
	    rdmlineid_ = rdlprobe->randomeLineID();
    }

    tkzs_ = probe.position();

    for ( int idx=0; idx<probe.nrLayers(); idx++ )
    {
	mDynamicCastGet(const AttribProbeLayer*,attrlayer,
			probe.getLayerByIdx(idx));
	if ( !attrlayer || attrlayer->dispType()!=AttribProbeLayer::VD )
	    continue;

	selspec_ = attrlayer->selSpec();
	break;
    }
}
