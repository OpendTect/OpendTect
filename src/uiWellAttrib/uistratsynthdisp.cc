/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratsynthdisp.h"
#include "uistratsynthdatamgr.h"
#include "uistratsynthexport.h"
#include "uistratlaymodtools.h"
#include "uisynthtorealscale.h"
#include "uiwaveletsel.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimultiflatviewcontrol.h"
#include "uipsviewer2dmainwin.h"
#include "uispinbox.h"
#include "uitaskrunnerprovider.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "envvars.h"
#include "prestacksynthdataset.h"
#include "stratsynthdatamgr.h"
#include "coltabsequence.h"
#include "stratsynthlevel.h"
#include "stratlith.h"
#include "stratunitref.h"
#include "synthseisdataset.h"
#include "flatviewzoommgr.h"
#include "flatposdata.h"
#include "ptrman.h"
#include "propertyref.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "synthseisgenerator.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "waveletio.h"


static const int cMarkerSize = 6;


class uiStratSynthDispDSSel : public uiGroup
{
public:

    typedef StratSynth::DataMgr	DataMgr;
    typedef DataMgr::SynthID	SynthID;
    typedef ColTab::Mapper	Mapper;
    typedef DataPack::ID	PackID;

    class Entry
    {
    public:

			Entry( SynthID sid, Mapper* mpr=0 )
			    : id_(sid), mapper_(mpr)
			{
			    if ( !mapper_ )
				mapper_ = new Mapper;
			}

	SynthID		id_;
	RefMan<Mapper>	mapper_;

    };

    class EntryList : public ManagedObjectSet<Entry>
    {
    public:

	Entry* add( SynthID sid, Mapper* mpr=0 )
	{
	    Entry* ret = new Entry( sid, mpr );
	    *this += ret;
	    return ret;
	}

	idx_type find( SynthID sid ) const
	{
	    for ( int idx=0; idx<size(); idx++ )
		if ( get(idx)->id_ == sid )
		    return idx;
	    return -1;
	}

    };

    typedef EntryList::idx_type	idx_type;

uiStratSynthDispDSSel( uiParent* p, const DataMgr& mgr, bool wva )
    : uiGroup( p, wva ? "wva ds sel" : "vd ds sel" )
    , mgr_(mgr)
    , wva_(wva)
    , selChange(this)
{
    sel_ = new uiComboBox( this, wva_ ? "wva ds sel" : "vd ds sel" );
    sel_->setHSzPol( uiObject::Medium );
    auto* lbl = new uiLabel( this, toUiString("---"), sel_ );
    lbl->setIcon( wva_ ? "wva" : "vd" );
    sel_->selectionChanged.notify( mCB(this,uiStratSynthDispDSSel,selChgCB) );
}

void selChgCB( CallBacker* )
{
    selChange.trigger();
}

void setContext( uiFlatViewer* vwr, uiStratSynthDispDSSel* oth )
{
    vwr_ = vwr;
    other_ = oth;
}

bool update() // returns whether curID() is new
{
    const int curidx = sel_->currentItem();
    const SynthID previd( curidx < 0 ? SynthID() : entries_.get(curidx)->id_ );
    EntryList preventries( entries_ );

    entries_.setEmpty();
    TypeSet<SynthID> ids;
    mgr_.getIDs( ids, wva_ ? DataMgr::NoProps : DataMgr::NoSubSel );
    entries_.add( SynthID() );
    for ( auto id : ids )
    {
	idx_type previdx = preventries.find( id );
	if ( previdx < 0 )
	    entries_.add( id );
	else
	    entries_ += preventries.removeAndTake( previdx, false );
    }

    const bool havesamesel = ids.isPresent( previd );
    NotifyStopper ns( sel_->selectionChanged );
    sel_->setEmpty();
    for ( auto entry : entries_ )
    {
	if ( entry->id_.isValid() )
	    sel_->addItem( mgr_.nameOf(entry->id_) );
	else
	    sel_->addItem( "---" );
    }

    if ( havesamesel )
	sel_->setCurrentItem( mgr_.nameOf(previd) );
    else
	sel_->setCurrentItem( entries_.size() > 1 ? 1 : 0 );

    return !havesamesel;
}

SynthID curID() const
{
    const int selidx = sel_->currentItem();
    return selidx > 0 ? entries_.get(selidx)->id_ : SynthID();
}

RefMan<Mapper> curMapper()
{
    int selidx = sel_->currentItem();
    if ( selidx < 0 )
	selidx = 0;
    return entries_.get(selidx)->mapper_;
}

void set( SynthID newid )
{
    int newidx = entries_.find( newid );
    if ( newidx < 0 )
	newidx = 0;
    const int curidx = sel_->currentItem();
    if ( curidx != newidx )
    {
	sel_->setCurrentItem( newidx );
	updateViewer();
    }
}

void updateViewer()
{
    auto curid = curID();
    const SynthSeis::DataSet* ds = 0;
    if ( curid.isValid() )
    {
	uiTaskRunnerProvider trprov( this );
	mgr_.ensureGenerated( curid, trprov );
	ds = mgr_.getDataSet( curid );
    }

    // TODO: for flattening and PS we need to create new datapacks

    auto newfullpackid = ds ? ds->dataPackID() : DataPack::FullID();
    auto newpackid = newfullpackid.objID();
    if ( newpackid.isValid() && newfullpackid.mgrID() != DataPackMgr::FlatID() )
	{ pErrMsg("TODO: support Pre-stack"); newpackid = PackID(); }
    if ( packid_ == newpackid )
	return;

    const bool hadpack = packid_.isValid();
    if ( !newpackid.isValid() )
	vwr_->usePack( wva_, newpackid, !hadpack );
    else
    {
	auto& dpm = DPM( DataPackMgr::FlatID() );
	if ( !dpm.isPresent(newpackid) )
	    dpm.add( ds->dataPack() );
	if ( !vwr_->isAvailable(newpackid) )
	    vwr_->addPack( newpackid );
	vwr_->usePack( wva_, newpackid, !hadpack );
    }

    // This object maintains the list of available packs, so keep it tidy
    if ( hadpack && packid_ != other_->packid_ )
	vwr_->removePack( packid_ );

    packid_ = newpackid;
    vwr_->setMapper( wva_, *curMapper() );
    vwr_->setVisible( wva_, true );
}

    Notifier<uiStratSynthDispDSSel> selChange;

    const DataMgr&	mgr_;
    EntryList		entries_;
    uiComboBox*		sel_;
    const bool		wva_;

    PackID		packid_;

    uiFlatViewer*	vwr_;
    const uiStratSynthDispDSSel* other_	= 0;

};


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, DataMgr& datamgr,
				    uiEdTools& et, uiSize uisz )
    : uiGroup(p,"LayerModel synthetics display")
    , datamgr_(datamgr)
    , edtools_(et)
    , initialsz_(uisz)
    , viewChanged(this)
    , elasticPropsSelReq(this)
{
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    topgrp->setStretch( 2, 0 );

    auto* elmbut = new uiToolButton( topgrp, "defraytraceprops",
			tr("Specify elastic model generation properties"),
			mCB(this,uiStratSynthDisp,elPropEdCB));
    auto* edbut = new uiToolButton( topgrp, "edit",
			tr("Define Synthetic DataSets"),
			mCB(this,uiStratSynthDisp,dataMgrCB) );
    edbut->attach( rightOf, elmbut );

    uiWaveletIOObjSel::Setup wvsu;
    wvsu.withextract( false ).withman( false ).compact( true );
    wvsu.szpol( uiObject::Medium );
    wvltfld_ = new uiWaveletIOObjSel( topgrp, wvsu );
    wvltfld_->attach( rightOf, edbut );

    wvaselfld_ = new uiStratSynthDispDSSel( topgrp, datamgr_, true );
    wvaselfld_->attach( rightOf, wvltfld_ );

    vdselfld_ = new uiStratSynthDispDSSel( topgrp, datamgr_, false );
    vdselfld_->attach( rightTo, wvaselfld_ );

    auto* expbut = new uiToolButton( topgrp, "export",
			    uiStrings::phrExport( tr("Synthetic DataSet(s)")),
			    mCB(this,uiStratSynthDisp,expSynthCB) );
    expbut->attach( rightBorder );
    vdselfld_->attach( leftOf, expbut );

    vwr_ = new uiFlatViewer( this );
    vwr_->rgbCanvas().disableImageSave();
    vwr_->setInitialSize( initialsz_ );
    vwr_->setStretch( 2, 2 );
    vwr_->attach( stretchedBelow, topgrp );
    setDefaultAppearance( vwr_->appearance() );

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withcoltabed( false ).tba( (int)uiToolBar::Right )
	.withflip( false ).withsnapshot( false );
    control_ = new uiMultiFlatViewControl( *vwr_, fvsu );
    control_->setViewerType( vwr_, true );

    wvaselfld_->setContext( vwr_, vdselfld_ );
    vdselfld_->setContext( vwr_, wvaselfld_ );

    mAttachCB( postFinalise(), uiStratSynthDisp::initGrp );
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    detachAllNotifiers();
}


#define mWvaNotif wvaselfld_->selChange
#define mVDNotif vdselfld_->selChange
#define mWvltNotif wvltfld_->selectionDone
#define mViewChgNotif vwr_->viewChanged

void uiStratSynthDisp::initGrp( CallBacker* )
{
    mAttachCB( mWvaNotif, uiStratSynthDisp::wvaSelCB );
    mAttachCB( mVDNotif, uiStratSynthDisp::vdSelCB );
    mAttachCB( mWvltNotif, uiStratSynthDisp::wvltChgCB );
    mAttachCB( mViewChgNotif, uiStratSynthDisp::viewChgCB );
    mAttachCB( vwr_->rgbCanvas().reSize, uiStratSynthDisp::canvasResizeCB );
    mAttachCB( edtools_.selLevelChg, uiStratSynthDisp::lvlChgCB );
    mAttachCB( edtools_.showFlatChg, uiStratSynthDisp::flatChgCB );
    mAttachCB( datamgr_.layerModelSuite().curChanged,
	       uiStratSynthDisp::curModEdChgCB );

    //TODO
    wvltfld_->setSensitive( false );
}


void uiStratSynthDisp::addViewerToControl( uiFlatViewer& vwr )
{
    if ( control_ )
    {
	control_->addViewer( vwr );
	control_->setViewerType( &vwr, false );
    }
}


void uiStratSynthDisp::modelChanged()
{
    datamgr_.modelChange();
    updFlds();
    reDisp();
}


void uiStratSynthDisp::updFlds()
{
    wvaselfld_->update();
    vdselfld_->update();

    NotifyStopper ns( mWvltNotif );
    wvltfld_->setInput( datamgr_.waveletID(wvaselfld_->curID()) );
}


void uiStratSynthDisp::reDisp()
{
    doDisp( true );
    doDisp( false );
    vwr_->setViewToBoundingBox();
    vwr_->handleChange( FlatView::Viewer::BitmapData );
    drawLevels();
}


void uiStratSynthDisp::doDisp( bool wva )
{
    auto& selfld = *(wva ? wvaselfld_ : vdselfld_);
    selfld.updateViewer();
}


void uiStratSynthDisp::drawLevels()
{
    vwr_->removeAuxDatas( levelaux_ );

    const auto& d2tmdls = datamgr_.d2TModels();
    if ( !d2tmdls.isEmpty() )
    {
	const auto sellvlid = edtools_.selLevelID();
	const int dispeach = dispEach();
	const auto& lvls = datamgr_.levels();

	for( int lvlidx=0; lvlidx<lvls.size(); lvlidx++ )
	{
	    const auto& lvl = lvls.getByIdx( lvlidx );
	    const auto stratlvl = Strat::LVLS().getByName( lvl.name() ) ;
	    if ( stratlvl.isUndef() )
		continue;
	    TypeSet<float> depths;
	    datamgr_.getLevelDepths( stratlvl.id(), depths );
	    if ( depths.isEmpty() )
		continue;

	    const bool issellvl = stratlvl.id() == sellvlid;
	    FlatView::AuxData* auxd = vwr_->createAuxData( stratlvl.name() );
	    auxd->linestyle_.type_ = OD::LineStyle::None;
	    for ( int itrc=0; itrc<depths.size(); itrc++ )
	    {
		float depth = depths[itrc];
		if ( mIsUdf(depth) )
		    continue;
		const float time = d2tmdls[itrc]->getTime( depth );
		if ( mIsUdf(time) )
		    continue;

		int mrkrsz = cMarkerSize;
		if ( issellvl )
		    mrkrsz *= 2;
		auto mrkrstyletype = OD::MarkerStyle2D::Target;

		auxd->markerstyles_ += OD::MarkerStyle2D( mrkrstyletype,
					    mrkrsz, stratlvl.color() );
		auxd->poly_ += FlatView::Point( (itrc*dispeach)+1, time );
		auxd->zvalue_ = issellvl ? 5 : 3;
	    }

	    vwr_->addAuxData( auxd );
	    levelaux_ += auxd;
	}
    }

    vwr_->handleChange( FlatView::Viewer::Auxdata );
}


void uiStratSynthDisp::elPropEdCB( CallBacker* )
{
    elasticPropsSelReq.trigger();
}


void uiStratSynthDisp::curModEdChgCB( CallBacker* )
{
    const auto& lms = datamgr_.layerModelSuite();
    const bool ised = lms.curIsEdited();

    if ( !modtypetxtitm_ )
    {
	if ( !ised )
	    return;

	uiGraphicsScene& scene = vwr_->rgbCanvas().scene();
	const uiPoint pos( mNINT32( scene.nrPixX()/2 ),
			   mNINT32( scene.nrPixY()-10 ) );
	modtypetxtitm_ = scene.addItem(
				new uiTextItem(pos,uiString::empty(),
					       mAlignment(HCenter,VCenter)) );
	modtypetxtitm_->setPenColor( Color::Black() );
	modtypetxtitm_->setZValue( 999999 );
	modtypetxtitm_->setMovable( true );
    }

    modtypetxtitm_->setVisible( ised );
    if ( ised )
	modtypetxtitm_->setText( lms.uiDesc(lms.curIdx()) );
}


void uiStratSynthDisp::canvasResizeCB( CallBacker* )
{
    if ( modtypetxtitm_ )
    {
	const uiGraphicsScene& scene = vwr_->rgbCanvas().scene();
	const uiPoint pos( mNINT32( scene.nrPixX()/2 ),
			   mNINT32( scene.nrPixY()-10 ) );
	modtypetxtitm_->setPos( pos );
    }
}


int uiStratSynthDisp::dispEach() const
{
    return edtools_.dispEach();
}


bool uiStratSynthDisp::dispFlattened() const
{
    return edtools_.showFlattened();
}


void uiStratSynthDisp::wvltChgCB( CallBacker* )
{
    const DBKey wvltid = wvltfld_->key( true );
    const auto synthid = wvaselfld_->curID();
    if ( wvltid.isValid() && synthid.isValid() )
	datamgr_.setWavelet( synthid, wvltid );
}


void uiStratSynthDisp::viewChgCB( CallBacker* )
{
    viewChanged.trigger();
}


void uiStratSynthDisp::lvlChgCB( CallBacker* )
{
    drawLevels();
}


void uiStratSynthDisp::flatChgCB( CallBacker* )
{
    reDisp();
}


void uiStratSynthDisp::dataMgrCB( CallBacker* )
{
    if ( !uidatamgr_ )
    {
	uidatamgr_ = new uiStratSynthDataMgr( this, datamgr_ );
	mAttachCB( uidatamgr_->applyReq, uiStratSynthDisp::applyReqCB );
	mAttachCB( uidatamgr_->windowClosed, uiStratSynthDisp::dataMgrClosedCB);
    }
    uidatamgr_->go();
}


void uiStratSynthDisp::applyReqCB( CallBacker* )
{
    updFlds();
    if ( uidatamgr_ )
	wvaselfld_->set( uidatamgr_->curID() );
    reDisp();
}


void uiStratSynthDisp::expSynthCB( CallBacker* )
{
    if ( datamgr_.layerModel().isEmpty() )
	return;

    uiStratSynthExport dlg( this, datamgr_ );
    dlg.go();
}


void uiStratSynthDisp::wvaSelCB( CallBacker* )
{
    doDisp( true );
    vwr_->handleChange( FlatView::Viewer::BitmapData );
}


void uiStratSynthDisp::vdSelCB( CallBacker* )
{
    doDisp( false );
    vwr_->handleChange( FlatView::Viewer::BitmapData );
}


uiFlatViewer* uiStratSynthDisp::getViewerClone( uiParent* p ) const
{
    uiFlatViewer* vwr = new uiFlatViewer( p );
    vwr->rgbCanvas().disableImageSave();
    vwr->setInitialSize( initialsz_ );
    vwr->setStretch( 2, 2 );
    vwr->appearance() = vwr_->appearance();
    vwr->setPack( true, vwr_->packID(true), false );
    vwr->setPack( false, vwr_->packID(false), false );
    return vwr;
}


void uiStratSynthDisp::setDefaultAppearance( Appearance& app )
{
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.allowuserchangereversedaxis_ = false;
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.name_ = uiStrings::sTWT();
    app.ddpars_.show( true, true );
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
}


void uiStratSynthDisp::makeInfoMsg( uiString& msg, IOPar& pars )
{
    FixedString valstr = pars.find( sKey::TraceNr() );
    if ( valstr.isEmpty() )
	return;

    const int modelidx = toInt( valstr );
    const int seqidx = modelidx - 1;
    if ( seqidx<0 || seqidx>=datamgr_.layerModel().size() )
	return;

    msg.postFixWord( uiStrings::sModelNumber().addMoreInfo( modelidx ) );

    valstr = pars.find( sKey::Z() );
    if ( !valstr )
	valstr = pars.find( "Z-Coord" );
    const float zval = valstr && *valstr ? toFloat(valstr) : mUdf(float);
    if ( mIsUdf(zval) )
      return;
    uiString zstr = uiStrings::sZ().addMoreInfo(mNINT32(zval)).withSurvZUnit();
    msg.postFixWord( zstr );

    FixedString vdstr = pars.find( "Variable density data" );
    FixedString wvastr = pars.find( "Wiggle/VA data" );
    FixedString vdvalstr = pars.find( "VD Value" );
    FixedString wvavalstr = pars.find( "WVA Value" );
    const bool vdsameaswva = vdstr == wvastr;
    if ( !vdvalstr.isEmpty() )
    {
	if ( vdsameaswva && vdstr.isEmpty() )
	    vdstr = wvastr;
	if ( vdstr.isEmpty() )
	    vdstr = "VD";
	const float val = vdvalstr.toFloat();
	uiString toadd = toUiString( vdstr );
	if ( mIsUdf(val) )
	    toadd.addMoreInfo( uiStrings::sUndef() );
	else
	    toadd.addMoreInfo( val );
	msg.postFixWord( toadd );
    }

    if ( !wvavalstr.isEmpty() && !vdsameaswva )
    {
	if ( wvastr.isEmpty() )
	    wvastr = "WVA";
	const float val = wvavalstr.toFloat();
	uiString toadd = toUiString( wvastr );
	if ( mIsUdf(val) )
	    toadd.addMoreInfo( uiStrings::sUndef() );
	else
	    toadd.addMoreInfo( val );
	msg.postFixWord( toadd );
    }

    float val;
    if ( pars.get(sKey::Offset(),val) && !mIsUdf(val) )
    {
	if ( SI().xyInFeet() )
	    val *= mToFeetFactorF;
	uiString toadd = uiStrings::sOffset().addMoreInfo(val).withSurvXYUnit();
	msg.postFixWord( toadd );
    }

    const TimeDepthModelSet& dt2mdls = datamgr_.d2TModels();
    const Strat::LayerModel& laymod = datamgr_.layerModel();
    const int d2tidx = seqidx / dispEach();
    if ( dt2mdls.validIdx(d2tidx) )
    {
	const float realzval = zval / SI().showZ2UserFactor();
	const float depth = dt2mdls.get(d2tidx)->getDepth( realzval );
	const auto& curseq = laymod.sequence( seqidx );
	for ( int lidx=0; lidx<curseq.size(); lidx++ )
	{
	    const auto& lay = *curseq.layers().get( lidx );
	    if ( lay.zTop()<=depth && lay.zBot()>depth )
	    {
		msg.postFixWord( uiStrings::sLayer().addMoreInfo(lay.name()) );
		break;
	    }
	}
    }
}
