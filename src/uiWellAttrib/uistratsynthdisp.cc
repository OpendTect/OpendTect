/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratsynthdisp.h"

#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimultiflatviewcontrol.h"
#include "uipsviewer2dmainwin.h"
#include "uislider.h"
#include "uistratlaymodtools.h"
#include "uistratsynthexport.h"
#include "uisynthgendlg.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "coltabsequence.h"
#include "dataclipper.h"
#include "flatviewzoommgr.h"
#include "ioman.h"
#include "ioobj.h"
#include "prestackgather.h"
#include "ptrman.h"
#include "seisbufadapters.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlith.h"
#include "syntheticdataimpl.h"
#include "wavelet.h"

using namespace StratSynth;
static const int cMarkerSize = 3;


namespace StratSynth
{

static const char* sKeyDispPar()	{ return "Display Parameter"; }
static const char* sKeyDefSeismicCtab() { return "Seismics"; }
static const char* sKeyDefAttribCtab()	{ return "Pastel"; }
static const char* sKeyDefStratPropCtab()
{ return ColTab::Sequence::sKeyRainbow(); }
static float cDefOverlap()		{ return 1.f; }
static float cDefSeisOverlap()		{ return 2.f; }
static Interval<float> cDefClipRate()	{ return Interval<float>(0.f,0.f); }
static Interval<float> cDefSeisClipRate()
{ return Interval<float>(0.025f,0.025f); }
static const char* sKeyViewArea()	{ return "Start View Area"; }

class SynthSpecificPars
{
public:

    class VwrDataPack
    {
    public:
			VwrDataPack( const DataPackID& dpid, int lmsidx,
				     const Strat::LevelID& flatlvlid,
				     int offsidx )
			    : dpid_(dpid)
			    , lmsidx_(lmsidx)
			    , flatlvlid_(flatlvlid)
			    , offsidx_(offsidx)
			{}

	bool		operator ==( const VwrDataPack& oth ) const
			{
			    return dpid_ == oth.dpid_ &&
				 lmsidx_ == oth.lmsidx_ &&
				 flatlvlid_ == oth.flatlvlid_ &&
				 offsidx_ == oth.offsidx_;
			}

	DataPackID	id() const		{ return dpid_; }
	int		curLayerModelIdx() const { return lmsidx_; }
	Strat::LevelID levelID() const	{ return flatlvlid_; }
	int		getOffsIdx() const	{ return offsidx_; }

    private:

	const DataPackID dpid_;
	const int	lmsidx_;
	const Strat::LevelID flatlvlid_;
	const int	offsidx_;

    };

SynthSpecificPars( SynthID sid, uiFlatViewer* vwr )
    : id_(sid)
    , vwr_(vwr)
{
}


SynthSpecificPars( const SynthSpecificPars& oth )
{
    *this = oth;
}


~SynthSpecificPars()
{
    delete prevtype_;
    if ( vwr_ )
    {
	for ( const auto* dpobj : dpobjs_ )
	    vwr_->removePack( dpobj->id() );
    }
}


SynthSpecificPars& operator =( const SynthSpecificPars& oth )
{
    if ( &oth != this )
    {
	vwr_ = oth.vwr_;
	id_ = oth.id_;
	inited_ = oth.inited_;
	delete prevtype_;
	prevtype_ = oth.prevtype_
		  ? new SynthGenParams::SynthType( *oth.prevtype_ ) : nullptr;
	offsetrg_ = oth.offsetrg_;
	dpobjs_ = oth.dpobjs_;
	wvamapper_.set( oth.wvamapper_ ? new ColTab::Mapper( *oth.wvamapper_ )
				       : nullptr );
	vdmapper_.set( oth.vdmapper_ ? new ColTab::Mapper( *oth.vdmapper_ )
				     : nullptr );
	overlap_ = oth.overlap_;
	ctab_ = oth.ctab_;
	prevwvasu_ = oth.prevwvasu_;
	prevoverlap_ = oth.prevoverlap_;
	prevvdsu_ = oth.prevvdsu_;
	prevctab_ = oth.prevctab_;
    }

    return *this;
}


ConstRefMan<FlatDataPack> find( int lmsidx, const Strat::LevelID& flatlvlid,
			  int offsidx ) const
{
    DataPackID dpid = DataPackID::udf();
    for ( const auto* dpobj : dpobjs_ )
    {
	if ( lmsidx == dpobj->curLayerModelIdx() &&
	     flatlvlid == dpobj->levelID() && offsidx == dpobj->getOffsIdx() )
	{
	    dpid = dpobj->id();
	    break;
	}
    }

    if ( dpid == DataPackID::udf() )
	return nullptr;

    return DPM( DataPackMgr::FlatID() ).get<FlatDataPack>( dpid );
}


void addIfNew( DataPackID dpid, int lmsidx, const Strat::LevelID& flatlvlid,
	       int offsidx )
{
    PtrMan<VwrDataPack> newobj = new VwrDataPack( dpid, lmsidx,
						  flatlvlid, offsidx );
    for ( const auto* dpobj : dpobjs_ )
	if ( *newobj.ptr() == *dpobj )
	    return;

    dpobjs_.add( newobj.release() );
}


void initFrom( const SyntheticData& sd )
{
    if ( inited_ || sd.id() != id_ )
	return;

    setMappers( sd );
    if ( sd.isPS() )
	setOffsets( static_cast<const PreStackSyntheticData&>( sd ) );

    delete prevtype_;
    prevtype_ = new SynthGenParams::SynthType( sd.synthType() );
    prevwvltrms_ = getWvltRMS( sd.getGenParams().getWaveletID() );

    inited_ = true;
}


void update()
{
    if ( vwr_ )
    {
	for ( const auto* dpobj : dpobjs_ )
	    vwr_->removePack( dpobj->id() );
    }
    dpobjs_.setEmpty();

    prevoverlap_ = overlap_;
    prevctab_ = ctab_;
    if ( wvamapper_ )
	prevwvasu_ = wvamapper_->setup_;
    if ( vdmapper_ )
	prevvdsu_ = vdmapper_->setup_;

    inited_ = false;
}


static float getWvltRMS( const MultiID& wvltid )
{
    if ( wvltid.isUdf() )
	return mUdf(float);

    PtrMan<IOObj> ioobj = IOM().get( wvltid );
    if ( !ioobj )
	return mUdf(float);

    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
    if ( !wvlt )
	return mUdf(float);

    const float* wvltamps = wvlt->samples();
    const int sz =  wvlt->size();
    float ret = 0.f;
    for ( int idx=0; idx<sz; idx++ )
	ret += wvltamps[idx] * wvltamps[idx];

    return Math::Sqrt( ret );
}


void setMappers( const SyntheticData& sd )
{
    const SynthGenParams& sgp = sd.getGenParams();
    bool sametype = prevtype_;
    if ( prevtype_ )
    {
	const SynthGenParams prevsgp( *prevtype_ );
	sametype = sgp.isRawOutput() && prevsgp.isRawOutput()
		 ? true : sgp.synthtype_ == prevsgp.synthtype_;
	if ( sametype && sgp.isRawOutput() && !mIsUdf(prevwvltrms_) )
	{
	    const float wvltrms = getWvltRMS( sd.getGenParams().getWaveletID());
	    if ( !mIsUdf(wvltrms) )
	    {
		const float rmsratio = wvltrms / prevwvltrms_;
		if ( rmsratio < 0.7 || rmsratio > 1.3f )
		    sametype = false;
	    }
	}
    }

    wvamapper_ = new ColTab::Mapper();
    if ( sametype )
    {
	wvamapper_->setup_ = prevwvasu_;
	overlap_ = prevoverlap_;
    }
    else
    {
	wvamapper_->setup_.type( ColTab::MapperSetup::Auto )
			  .cliprate( cDefClipRate() )
			  .autosym0( !sgp.isRawOutput() )
			  .symmidval( sgp.isRawOutput() ? 0.f : mUdf(float) );
	if ( !sgp.isStratProp() && !sgp.isAttribute() )
	    overlap_ = cDefSeisOverlap();
    }

    if ( sd.isPS() )
    {
	const auto& presd = static_cast<const PreStackSyntheticData&>( sd );
	wvamapper_->setData( presd.preStackPack().data() );
    }
    else
    {
	const auto& postsd = static_cast<const PostStackSyntheticData&>( sd );
	wvamapper_->setData( postsd.postStackPack().data() );
    }

    vdmapper_ = new ColTab::Mapper( *wvamapper_.ptr(), true );
    if ( sametype )
    {
	vdmapper_->setup_ = prevvdsu_;
	ctab_ = prevctab_;
    }
    else
    {
	if ( !sgp.isStratProp() && !sgp.isAttribute() )
	{
	    vdmapper_->setup_.cliprate( cDefSeisClipRate() );
	    if ( ColTab::SM().indexOf(sKeyDefSeismicCtab()) >= 0 )
		ctab_ = sKeyDefSeismicCtab();
	}
	else
	{
	    const char* coltabnm = sgp.isAttribute() ? sKeyDefAttribCtab()
						     : sKeyDefStratPropCtab();
	    if ( ColTab::SM().indexOf(coltabnm) >= 0 )
		ctab_ = coltabnm;
	}
    }

    vdmapper_->update( false );
}


void setOffsets( const PreStackSyntheticData& pssd )
{
    offsetrg_.set( pssd.offsetRange(), pssd.offsetRangeStep() );
}


const StepInterval<float>& getOffsetRg() const
{
    return offsetrg_;
}


void useWVADispPars( const FlatView::DataDispPars::WVA& wvapars )
{
    if ( !id_.isValid() )
	return;

    if ( wvamapper_ )
	wvamapper_->setup_ = wvapars.mappersetup_;

    overlap_ = wvapars.overlap_;
}


void useVDDispPars( const FlatView::DataDispPars::VD& vdpars )
{
    if ( !id_.isValid() )
	return;

    if ( vdmapper_ )
	vdmapper_->setup_ = vdpars.mappersetup_;

    ctab_ = vdpars.ctab_;
}


void useDispPars( const IOPar& iop, bool forseis )
{
    PtrMan<IOPar> disppar = iop.subselect( sKeyDispPar() );
    if ( !disppar )
	return;

    if ( wvamapper_ )
    {
	PtrMan<IOPar> wvamapperpar =
			disppar->subselect( FlatView::DataDispPars::sKeyWVA() );
	const Interval<float> disprg( wvamapper_->setup_.range_ );
	if ( wvamapperpar )
	{
	    wvamapper_->setup_.usePar( *wvamapperpar.ptr() );
	    wvamapperpar->get( FlatView::DataDispPars::sKeyOverlap(), overlap_);
	}
	else
	{ // Older par file
	    wvamapper_->setup_.type_ = ColTab::MapperSetup::Fixed;
	    disppar->get( sKey::Range(), wvamapper_->setup_.range_ );
	    disppar->get( FlatView::DataDispPars::sKeyOverlap(), overlap_ );
	}

	if ( forseis && wvamapper_->setup_.type_ != ColTab::MapperSetup::Fixed )
	{
	    wvamapper_->setup_.symmidval( 0.f ).autosym0( false );
	    overlap_ = cDefSeisOverlap();
	}

	wvamapper_->update( false );
    }

    if ( vdmapper_ )
    {
	PtrMan<IOPar> vdmapperpar =
		      disppar->subselect( FlatView::DataDispPars::sKeyVD() );
	const Interval<float> disprg( vdmapper_->setup_.range_ );
	if ( vdmapperpar )
	{
	    vdmapper_->setup_.usePar( *vdmapperpar.ptr() );
	    vdmapperpar->get( FlatView::DataDispPars::sKeyColTab(), ctab_ );
	}
	else
	{ // Older par file
	    vdmapper_->setup_.type_ = ColTab::MapperSetup::Fixed;
	    disppar->get( sKey::Range(), vdmapper_->setup_.range_ );
	    disppar->get( FlatView::DataDispPars::sKeyColTab(), ctab_ );
	}

	if ( forseis && vdmapper_->setup_.type_ != ColTab::MapperSetup::Fixed )
	    vdmapper_->setup_.symmidval( 0.f ).autosym0( false );

	vdmapper_->update( false );
    }
}


void fillDispPars( IOPar& iop ) const
{
    IOPar disppar;
    if ( vdmapper_ )
    {
	IOPar vdmapperpar;
	vdmapperpar.set( FlatView::DataDispPars::sKeyColTab(), ctab_ );
	ColTab::MapperSetup vdmappersu( vdmapper_->setup_ );
	if ( vdmappersu.type_ != ColTab::MapperSetup::Fixed )
	    vdmappersu.range_.setUdf();
	vdmappersu.fillPar( vdmapperpar );
	disppar.mergeComp( vdmapperpar, FlatView::DataDispPars::sKeyVD() );
    }

    if ( wvamapper_ )
    {
	IOPar wvamapperpar;
	wvamapperpar.set( FlatView::DataDispPars::sKeyOverlap(), overlap_ );
	ColTab::MapperSetup wvamappersu( wvamapper_->setup_ );
	if ( wvamappersu.type_ != ColTab::MapperSetup::Fixed )
	    wvamappersu.range_.setUdf();
	wvamappersu.fillPar( wvamapperpar );
	disppar.mergeComp( wvamapperpar, FlatView::DataDispPars::sKeyWVA() );
    }

    if ( !disppar.isEmpty() )
	iop.mergeComp( disppar, sKeyDispPar() );
}

    SynthID	id_;
    ManagedObjectSet<VwrDataPack> dpobjs_;
    PtrMan<ColTab::Mapper>	wvamapper_;
    PtrMan<ColTab::Mapper>	vdmapper_;
    BufferString		ctab_;
    float			overlap_ = cDefOverlap();

private:

    uiFlatViewer*		vwr_;
    bool			inited_ = false;
    StepInterval<float>		offsetrg_;
    SynthGenParams::SynthType*	prevtype_ = nullptr;
    float			prevwvltrms_ = mUdf(float);
    ColTab::MapperSetup		prevwvasu_;
    ColTab::MapperSetup		prevvdsu_;
    float			prevoverlap_ = mUdf(float);
    BufferString		prevctab_;

};


class SynthSpecificParsSet : public ManagedObjectSet<SynthSpecificPars>
{
public:

SynthSpecificPars* add( SynthID sid, uiFlatViewer* vwr )
{
    auto* ret = new SynthSpecificPars( sid, vwr );
    *this += ret;
    return ret;
}

int find( SynthID sid ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( get(idx)->id_ == sid )
	    return idx;
    return -1;
}

SynthSpecificPars* getByID( SynthID sid )
{
    const int idx = find( sid );
    return validIdx(idx) ? get( idx ) : nullptr;
}

const SynthSpecificPars* getByID( SynthID sid ) const
{
    return mSelf().getByID( sid );
}

ConstRefMan<FlatDataPack> find( SynthID sid, int lmsidx,
			  const Strat::LevelID flatlvlid, int offsidx ) const
{
    const SynthSpecificPars* ent = getByID( sid );
    if ( !ent )
	return nullptr;

    return ent->find( lmsidx, flatlvlid, offsidx );
}

void addIfNew( SynthID sid, DataPackID dpid, int lmsidx,
	       const Strat::LevelID flatlvlid, int offsidx )
{
    SynthSpecificPars* ent = getByID( sid );
    if ( ent )
	ent->addIfNew( dpid, lmsidx, flatlvlid, offsidx );
}

};


static bool isEmpty( uiWorldRect wr )
{
    wr.sortCorners();
    return (mIsZero(wr.width(),1e-6f) && mIsZero(wr.height(),1e-6f)) ||
	   (mIsZero(wr.top(),1e-6f) && mIsEqual(wr.bottom(),1.f,1e-6f) &&
	    mIsZero(wr.left(),1e-6f) && mIsEqual(wr.right(),1.f,1e-6f));
}


static bool isEqual( uiWorldRect a, uiWorldRect b )
{
    a.sortCorners();
    b.sortCorners();
    return mIsEqual(a.left(),b.left(),1e-6f) &&
	   mIsEqual(a.right(),b.right(),1e-6f) &&
	   mIsEqual(a.top(),b.top(),1e-6f) &&
	   mIsEqual(a.bottom(),b.bottom(),1e-6f);
}

} // namespace StratSynth


class uiStratSynthDispDSSel : public uiGroup
{
public:

uiStratSynthDispDSSel( uiParent* p, uiStratSynthDisp& synthdisp,
		       const DataMgr& mgr, bool wva )
    : uiGroup( p, wva ? "wva ds sel" : "vd ds sel" )
    , datamgr_(mgr)
    , wva_(wva)
    , synthdisp_(synthdisp)
    , selChange(this)
{
    sel_ = new uiComboBox( this, wva_ ? "wva ds sel" : "vd ds sel" );
    sel_->setHSzPol( uiObject::Medium );
    auto* lbl = new uiLabel( this, toUiString("---"), sel_ );
    lbl->setIcon( wva_ ? "wva" : "vd" );
    mAttachCB( sel_->selectionChanged, uiStratSynthDispDSSel::selChgCB );
}

~uiStratSynthDispDSSel()
{
    detachAllNotifiers();
}


void setCurrentItem( const SynthID id )
{
    sel_->setCurrentItem( sel_->getItemIndex(id.asInt()) );
}


bool update()
{
    const BufferString curnm( sel_->text() );
    const SynthID previd = datamgr_.find( curnm );
    bool havesamesel = !previd.isValid() && isNoneSelected();
    const SynthSpecificParsSet& entries = synthdisp_.entries_;
    if ( !havesamesel )
    {
	for ( const auto* entry : entries )
	{
	    if ( entry->id_ == previd )
	    {
		havesamesel = true;
		break;
	    }
	}
    }

    NotifyStopper ns( sel_->selectionChanged );
    sel_->setEmpty();
    sel_->addItem( toUiString("---"), 0 );
    for ( const auto* entry : entries )
    {
	if ( !entry->id_.isValid() ||
	     (wva_ && (datamgr_.isAttribute(entry->id_) ||
		       datamgr_.isStratProp(entry->id_))) )
	    continue;

	const BufferString dsnm( datamgr_.nameOf(entry->id_) );
	if ( !dsnm.isEmpty() )
	    sel_->addItem( toUiString(dsnm.str()), entry->id_.asInt() );
    }

    if ( havesamesel )
    {
	if ( previd.isValid() )
	    setCurrentItem( previd );
	else
	    setCurrentItem( SynthID(0) );
    }
    else
	setCurrentItem( entries.size()>1 ? entries.get(1)->id_ : SynthID(0) );

    return !havesamesel;
}


void updateName( SynthID id, const char* nm )
{
    const int idx = sel_->getItemIndex( id.asInt() );
    if ( idx < 0 || idx >= sel_->size() )
	return;

    sel_->setItemText( idx, toUiString(nm) );
}


BufferString currentName() const
{
    BufferString ret( sel_->text() );
    return ret;
}


bool isNoneSelected() const
{
    return sel_->currentItem() == 0;
}


SynthID curID() const
{
    return SynthID( sel_->currentItemID() );
}


const ColTab::Mapper* curMapper() const
{
    if ( isNoneSelected() )
	return nullptr;

    const SynthSpecificPars* disppars = synthdisp_.entries_.getByID( curID() );
    if ( !disppars )
	return nullptr;

    return wva_ ? disppars->wvamapper_.ptr() : disppars->vdmapper_.ptr();
}


float overlap() const
{
    if ( isNoneSelected() )
	return cDefOverlap();

    const SynthSpecificPars* disppars = synthdisp_.entries_.getByID( curID() );
    return disppars ? disppars->overlap_ : cDefOverlap();
}


const char* seqName() const
{
    if ( isNoneSelected() )
	return nullptr;

    const SynthSpecificPars* disppars = synthdisp_.entries_.getByID( curID() );
    return disppars ? disppars->ctab_.buf() : nullptr;
}


DataPackID packID() const
{
    return datapackid_;
}


bool canDoWiggle() const
{
    if ( wva_ )
	return true;

    const SynthGenParams* sgp = datamgr_.getGenParams( curID() );
    if ( !sgp )
	return false;

    return !sgp->isStratProp() && !sgp->isAttribute();
}

    Notifier<uiStratSynthDispDSSel> selChange;
    DataPackID datapackid_ = DataPackID::udf();

private:

void selChgCB( CallBacker* )
{
    selChange.trigger();
}

    const DataMgr&	datamgr_;
    uiComboBox*		sel_;
    const bool		wva_;
    uiStratSynthDisp&	synthdisp_;

};


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, StratSynth::DataMgr& datamgr,
				    uiStratLayModEditTools& et, uiSize uisz )
    : uiGroup(p,"LayerModel synthetics display")
    , datamgr_(datamgr)
    , edtools_(et)
    , initialsz_(uisz)
    , entries_(*new StratSynth::SynthSpecificParsSet())
    , viewChanged(this)
{
    auto* topgrp = new uiGroup( this, "Top group" );
    topgrp->setStretch( 2, 0 );

    auto* edbut = new uiToolButton( topgrp, "edit",
			tr("Define Synthetic DataSets"),
			mCB(this,uiStratSynthDisp,dataMgrCB) );

    wvltfld_ = new uiLineEdit( topgrp, "Wavelet Name" );
    wvltfld_->setHSzPol( uiObject::Medium );
    wvltfld_->attach( rightOf, edbut );
    wvltfld_->setReadOnly();
//    wvltfld_->setIcon( "wavelet" ); TODO: add support

    wvaselfld_ = new uiStratSynthDispDSSel( topgrp, *this, datamgr_, true );
    wvaselfld_->attach(  rightTo, wvltfld_ );
    vdselfld_ = new uiStratSynthDispDSSel( topgrp, *this, datamgr_, false );
    vdselfld_->attach( rightBorder );
    wvaselfld_->attach( leftOf, vdselfld_ );

    auto* vwrgrp = new uiGroup( this, "Viewer area group" );
    createViewer( vwrgrp );
    vwrgrp->attach( stretchedBelow, topgrp );

    mAttachCB( postFinalize(), uiStratSynthDisp::initGrp );
}


void uiStratSynthDisp::createViewer( uiGroup* vwrgrp )
{
    vwr_ = new uiFlatViewer( vwrgrp );
    vwr_->rgbCanvas().disableImageSave();
    vwr_->setInitialSize( initialsz_ );
    vwr_->setStretch( 2, 2 );
    setDefaultAppearance( vwr_->appearance() );

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withcoltabed( false ).tba( (int)uiToolBar::Right )
	.withflip( false ).withsnapshot( false );

    control_ = new uiMultiFlatViewControl( *vwr_, fvsu );
    control_->setViewerType( vwr_, true );
    float zshift = -1.f * mCast(float,SI().seismicReferenceDatum());
    if ( SI().depthsInFeet() )
    {
	zshift *= mToFeetFactorF;
	control_->setZFactor( mToFeetFactorF );
    }
    control_->setDepthShift( zshift );

    const int sz = 25; // looks best
    auto* exportbut = new uiToolButton( vwrgrp, "export",
				uiStrings::phrExport(tr("Synthetic Data")),
				mCB(this,uiStratSynthDisp,exportCB) );
    exportbut->setMaximumHeight( sz );
    exportbut->setMaximumWidth( sz );
    exportbut->attach( rightBorder, 6 ); // for a single pixel spacing
    exportbut->attach( topBorder, 2 ); // for a single pixel spacing

    psgrp_ = new uiGroup( vwrgrp, "Pre-Stack controls group" );
    auto* psvwbut = new uiToolButton( psgrp_, "nonmocorr64",
				tr("View Prestack Offset Display Panel"),
				mCB(this,uiStratSynthDisp,viewPSCB) );
    psvwbut->setMaximumHeight( sz );
    psvwbut->setMaximumWidth( sz );

    const int slsz = initialsz_.height() - uiObject::iconSize();
    uiSlider::Setup slsu;
    slsu.isvertical( true ).sldrsize( slsz );
    offsslider_ = new uiSlider( psgrp_, slsu, "Offset Slider" );
    offsslider_->setStretch( 0, 2 );
    offsslider_->attach( ensureBelow, psvwbut );
    offsslider_->slider()->setVSzPol( uiObject::WideVar );
    offsslider_->slider()->setPrefWidth( uiObject::iconSize() );
    psgrp_->attach( rightBorder, 2 );
    psgrp_->attach( ensureBelow, exportbut );
    psgrp_->setStretch( 0, 2 );
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    detachAllNotifiers();
    delete psvwrwin_;
    delete uidatamgr_;
    delete &entries_;
}


void uiStratSynthDisp::initGrp( CallBacker* )
{
    mAttachCB( datamgr_.entryAdded, uiStratSynthDisp::synthAddedCB );
    mAttachCB( datamgr_.entryRenamed, uiStratSynthDisp::synthRenamedCB );
    mAttachCB( datamgr_.entryChanged, uiStratSynthDisp::synthRemovedCB );
    mAttachCB( datamgr_.entryRemoved, uiStratSynthDisp::synthRemovedCB );
    mAttachCB( wvaselfld_->selChange, uiStratSynthDisp::wvaSelCB );
    mAttachCB( vdselfld_->selChange, uiStratSynthDisp::vdSelCB );
    mAttachCB( offsslider_->valueChanged, uiStratSynthDisp::offsSliderChgCB );
    mAttachCB( vwr_->viewChanged, uiStratSynthDisp::viewChgCB );
    mAttachCB( vwr_->rgbCanvas().reSize, uiStratSynthDisp::canvasResizeCB );
    mAttachCB( vwr_->dispPropChanged, uiStratSynthDisp::dispPropChgCB );
    if ( control() )
	mAttachCB( control()->zoomChanged, uiStratSynthDisp::zoomChangedCB );
    mAttachCB( edtools_.selLevelChg, uiStratSynthDisp::lvlChgCB );
    mAttachCB( edtools_.showFlatChg, uiStratSynthDisp::flatChgCB );
    mAttachCB( edtools_.dispEachChg, uiStratSynthDisp::dispEachChgCB );
    mAttachCB( datamgr_.layerModelSuite().curChanged,
	       uiStratSynthDisp::curModEdChgCB );

    psgrp_->display( false );
}


bool uiStratSynthDisp::curIsPS( FlatView::Viewer::VwrDest dest ) const
{
    if ( dest != FlatView::Viewer::VD && !wvaselfld_->isNoneSelected() )
	return datamgr_.isPS( wvaselfld_->curID() );
    if ( dest != FlatView::Viewer::WVA && !vdselfld_->isNoneSelected() )
	return datamgr_.isPS( vdselfld_->curID() );
    return false;
}


void uiStratSynthDisp::addViewerToControl( uiFlatViewer& vwr )
{
    if ( !control_ )
	return;

    control_->addViewer( vwr );
    control_->setViewerType( &vwr, false );
}


void uiStratSynthDisp::enableDispUpdate( bool yn )
{
    canupdatedisp_ = yn;
}


void uiStratSynthDisp::handleModelChange( bool full )
{
    enableDispUpdate( false );
    datamgr_.modelChange();
    selseq_ = -1;
    updFlds( full );
    reDisp();
    curModEdChgCB( nullptr );
    if ( full )
	initialboundingbox_ = vwr_->boundingBox();

    enableDispUpdate( true );
}


void uiStratSynthDisp::updFlds( bool full )
{
    updateEntries( full );
    wvaselfld_->update();
    vdselfld_->update();
    if ( wvaselfld_->isNoneSelected() && vdselfld_->isNoneSelected() &&
	 entries_.validIdx(1) )
    {
	NotifyStopper wvans( wvaselfld_->selChange );
	NotifyStopper vdns( vdselfld_->selChange );
	const SynthID id = entries_.get(1)->id_;
	wvaselfld_->setCurrentItem( id );
	vdselfld_->setCurrentItem( id );
    }
}


void uiStratSynthDisp::updateEntries( bool full )
{
    StratSynth::SynthSpecificParsSet preventries;
    if ( full )
	entries_.setEmpty();
    else
    {
	while ( !entries_.isEmpty() )
	    preventries += entries_.removeAndTake( 0 );
    }

    TypeSet<SynthID> ids;
    static SynthID emptyid( 0 );
    ids += emptyid;
    datamgr_.getIDs( ids, StratSynth::DataMgr::NoProps );
    datamgr_.getIDs( ids, StratSynth::DataMgr::OnlyProps );
    for ( const auto& id : ids )
    {
	const int previdx = preventries.find( id );
	if ( previdx < 0 )
	    entries_.add( id, vwr_ );
	else
	    entries_ += preventries.removeAndTake( previdx, false );
    }

    preventries.setEmpty();
}


void uiStratSynthDisp::updWvltFld()
{
    BufferString wvltnm;
    const SynthGenParams* sgp = datamgr_.getGenParams( wvaselfld_->curID() );
    if ( sgp && sgp->isRawOutput() )
    {
	wvltnm.set( sgp->getWaveletNm() );
	PtrMan<IOObj> ioobj = Wavelet::getIOObj( sgp->getWaveletNm() );
	if ( !ioobj )
	    wvltnm.setEmpty();
    }

    wvltfld_->setText( wvltnm );
}


void uiStratSynthDisp::handleChange( od_uint32 ctyp )
{
    if ( ctyp > 0 )
	vwr_->handleChange( ctyp );
}


void uiStratSynthDisp::reDisp( bool preserveview )
{
    NotifyStopper ns( control_->zoomChanged, this );
    if ( preserveview )
    {
	const uiWorldRect curview = vwr_->curView();
	if ( isEmpty(curview) && (initialboundingbox_.height() > 0 ||
				  initialboundingbox_.width() > 0) )
	    vwr_->setView( initialboundingbox_ );
    }

    od_uint32 ctyp = 0;
    if ( wvaselfld_->curID() == vdselfld_->curID() )
	setViewerData( FlatView::Viewer::Both, ctyp, preserveview );
    else
    {
	setViewerData( FlatView::Viewer::WVA, ctyp, preserveview );
	setViewerData( FlatView::Viewer::VD, ctyp, preserveview );
    }

    updWvltFld();
    psgrp_->display( curIsPS(FlatView::Viewer::Both) );
    drawLevels( ctyp );
    handleChange( ctyp );
}


void uiStratSynthDisp::setViewerData( FlatView::Viewer::VwrDest dest,
				      od_uint32& ctyp, bool preserveview )
{
    if ( dest == FlatView::Viewer::None )
	return;

    const uiWorldRect curview = vwr_->curView();

    uiStratSynthDispDSSel& selfld = dest == FlatView::Viewer::VD
				  ? *vdselfld_ : *wvaselfld_;
    const SynthID curid = selfld.curID();
    if ( !curid.isValid() )
    {
	selfld.datapackid_ = DataPack::cNoID();
	if ( dest == FlatView::Viewer::Both )
	    vdselfld_->datapackid_ = DataPack::cNoID();

	vwr_->setVisible( dest, false, &ctyp );
	return;
    }

    ConstRefMan<SyntheticData> sd;
    if ( curid.isValid() )
    {
	enableDispUpdate( false );
	uiTaskRunner trprov( this );
	if ( datamgr_.ensureGenerated(curid,&trprov) )
	    sd = datamgr_.getDataSet( curid );
	enableDispUpdate( true );

	if ( sd )
	{
	    SynthSpecificPars* disppars = entries_.getByID( curid );
	    if ( disppars )
		disppars->initFrom( *sd.ptr() );

	    if ( sd->isPS() )
	    {
		NotifyStopper ns( offsslider_->valueChanged );
		const StepInterval<float>& offsetrg = disppars->getOffsetRg();
		const float prevoff = offsslider_->getFValue();
		offsslider_->setInterval( offsetrg );
		const int idx = offsetrg.nearestIndex( prevoff );
		curoffs_ = offsetrg.atIndex( idx );
		curoffs_ = offsetrg.limitValue( curoffs_ );
		offsslider_->setValue( curoffs_ );
		updateOffSliderTxt();
	    }
	    else
		curoffs_ = 0.f;
	}
    }

    ConstRefMan<FlatDataPack> pack2use;
    DataPackMgr& dpm = DPM( DataPackMgr::FlatID() );
    const int lmsidx = datamgr_.layerModelSuite().curIdx();
    int curoffsidx = -1;
    Strat::LevelID flatlvlid;
    if ( sd )
    {
	const bool hascuroffset = sd->hasOffset();
	if ( hascuroffset )
	    curoffsidx = sd->synthGenDP().getOffsetIdx( curoffs_ );

	const Strat::LevelID sellvlid = edtools_.selLevelID();
	const bool showflattened =
			sellvlid.isValid() && edtools_.showFlattened();
	if ( showflattened )
	    flatlvlid = sellvlid;

	pack2use = entries_.find( curid, lmsidx, flatlvlid, curoffsidx );
	if ( !pack2use )
	{
	    mDynamicCastGet(const PreStackSyntheticData*,presd,sd.ptr());
	    if ( showflattened )
	    {
		TypeSet<float> zvals;
		datamgr_.getLevelDepths( sellvlid, zvals );
		pack2use = presd
			 ? presd->getFlattenedTrcDP( zvals, false, curoffsidx )
			 : sd->getFlattenedTrcDP( zvals, false );
	    }
	    else
	    {
		pack2use = presd ? presd->getTrcDPAtOffset( curoffsidx )
				 : sd->getTrcDP();
	    }
	}
    }

    const DataPackID newpackid = pack2use ? pack2use->id()
					    : DataPack::cNoID();
    if ( selfld.packID() == newpackid )
    {
	if ( dest == FlatView::Viewer::Both )
	    vdselfld_->datapackid_ = newpackid;

	updateDispPars( dest, &ctyp );
	return;
    }

    const bool hadpack = selfld.packID().isValid() &&
			 selfld.packID() != DataPack::cNoID();
    selfld.datapackid_ = newpackid;
    if ( dest == FlatView::Viewer::Both )
	vdselfld_->datapackid_ = newpackid;

    RefMan<FlatDataPack> fdp = pack2use.getNonConstPtr();
    if ( newpackid != DataPack::cNoID() )
    {
	if ( !dpm.isPresent(newpackid) )
	    dpm.add<FlatDataPack>( fdp );

	vwr_->setPack( dest, fdp );
	entries_.addIfNew( curid, newpackid, lmsidx, flatlvlid, curoffsidx );
    }

    const bool updateview = vwr_->enableChange( false );
    vwr_->setPack( dest, fdp, !hadpack );
    ctyp = Math::SetBits( ctyp, FlatView::Viewer::BitmapData, true );

    if ( control_ && sd )
    {
	const bool uncorrected =
		   sd->isPS() && !sd->getGenParams().isCorrected();
	ObjectSet<const TimeDepthModel> d2tmodels;
	for ( int itrc=0; itrc<sd->nrPositions(); itrc++ )
	    d2tmodels.add( uncorrected ? sd->getTDModel( itrc, curoffsidx )
				       : sd->getTDModel( itrc ) );
	control_->setD2TModels( d2tmodels );
    }

    vwr_->enableChange( updateview );
    if ( preserveview && !isEmpty(curview) )
	vwr_->setView( curview );
    else
	vwr_->setViewToBoundingBox();

    updateDispPars( dest, &ctyp );
}


void uiStratSynthDisp::updateDispPars( FlatView::Viewer::VwrDest dest,
				       od_uint32* ctyp )
{
    if ( dest == FlatView::Viewer::None )
	return;

    const SynthID wvacurid = wvaselfld_->curID();
    const SynthID vdcurid = vdselfld_->curID();
    if ( (dest == FlatView::Viewer::WVA && !wvacurid.isValid()) ||
	 (dest == FlatView::Viewer::VD && !vdcurid.isValid()) ||
	 (!wvacurid.isValid() && !vdcurid.isValid()) )
	return;

    FlatView::DataDispPars& ddpars = vwr_->appearance().ddpars_;
    bool notif = false;
    if ( dest != FlatView::Viewer::VD )
    {
	const ColTab::Mapper* mapper = wvaselfld_->curMapper();
	if ( mapper && (mapper->setup_ != ddpars.wva_.mappersetup_ ||
	     mapper->setup_.range_ != ddpars.wva_.mappersetup_.range_) )
	{
	    ddpars.wva_.mappersetup_ = mapper->setup_;
	    notif = true;
	}

	const float overlap = wvaselfld_->overlap();
	if ( !mIsEqual(overlap,ddpars.wva_.overlap_,1e-2f) )
	{
	    ddpars.wva_.overlap_ = overlap;
	    notif = true;
	}
    }

    if ( dest != FlatView::Viewer::WVA )
    {
	const ColTab::Mapper* mapper = vdselfld_->curMapper();
	if ( mapper && (mapper->setup_ != ddpars.vd_.mappersetup_ ||
	     mapper->setup_.range_ != ddpars.vd_.mappersetup_.range_) )
	{
	    ddpars.vd_.mappersetup_ = mapper->setup_;
	    notif = true;
	}

	const BufferString ctabnm( vdselfld_->seqName() );
	if ( ctabnm != ddpars.vd_.ctab_ )
	{
	    ddpars.vd_.ctab_ = ctabnm;
	    notif = true;
	}
    }

    vwr_->setVisible( dest, true, ctyp );
    if ( !notif )
	return;

    const od_uint32 ctype = sCast(od_uint32,FlatView::Viewer::DisplayPars);
    if ( ctyp )
	*ctyp = Math::SetBits( *ctyp, FlatView::Viewer::DisplayPars, true );
    else
	handleChange( ctype );
}


void uiStratSynthDisp::setSelectedSequence( int seqnr )
{
    selseq_ = seqnr;
    // TODO take action
}


void uiStratSynthDisp::drawLevels( od_uint32& ctyp )
{
    vwr_->removeAuxDatas( levelaux_ );
    levelaux_.setEmpty();

    TypeSet<SynthID> validids;
    datamgr_.getIDs( validids, DataMgr::NoSubSel, true );
    ConstRefMan<SyntheticData> sd;
    if ( !validids.isEmpty() )
	sd = datamgr_.getDataSet( validids.first() );

    const float srd = mCast(float,SI().seismicReferenceDatum());
    if ( sd )
    {
	const Strat::LevelID sellvlid = edtools_.selLevelID();
	const bool showflattened =
		sellvlid.isValid() && edtools_.showFlattened();
	const int dispeach = dispEach();
	const StratSynth::LevelSet& lvls = datamgr_.levels();
	TypeSet<float> sellvldepths;
	if ( showflattened )
	    datamgr_.getLevelDepths( sellvlid, sellvldepths );

	const bool dodecim = edtools_.canSetDispEach();
	for( int lvlidx=0; lvlidx<lvls.size(); lvlidx++ )
	{
	    const StratSynth::Level& lvl = lvls.getByIdx( lvlidx );
	    const Strat::Level stratlvl = Strat::LVLS().getByName( lvl.name() );
	    if ( stratlvl.isUndef() )
		continue;

	    TypeSet<float> depths;
	    datamgr_.getLevelDepths( stratlvl.id(), depths );
	    if ( depths.isEmpty() )
		continue;
	    else if ( showflattened )
		for ( int idpth=0; idpth<depths.size(); idpth++ )
		    depths[idpth] -= sellvldepths[idpth];

	    const bool issellvl = stratlvl.id() == sellvlid;
	    FlatView::AuxData* auxd = vwr_->createAuxData( stratlvl.name() );
	    auxd->linestyle_.type_ = dodecim ? OD::LineStyle::None
					     : OD::LineStyle::Solid;
	    if ( !dodecim )
	    {
		auxd->linestyle_.color_ = stratlvl.color();
		auxd->linestyle_.width_ = issellvl ? 3 : 2;
	    }

	    auxd->zvalue_ = issellvl ? 5 : 3;
	    for ( int itrc=0; itrc<depths.size(); itrc++ )
	    {
		const TimeDepthModel* d2tmdl = sd->getTDModel( itrc );
		const float depth = depths[itrc];
		if ( mIsUdf(depth) || !d2tmdl )
		    continue;

		const float time = d2tmdl->getTime( depth + srd );
		if ( mIsUdf(time) )
		    continue;

		if ( dodecim )
		{
		    int mrkrsz = cMarkerSize;
		    if ( issellvl && datamgr_.nrTraces() <= 25 )
			mrkrsz += 2;

		    auxd->markerstyles_ += MarkerStyle2D( MarkerStyle2D::Target,
							  mrkrsz,
							  stratlvl.color() );
		}

		auxd->poly_ += FlatView::Point( itrc * dispeach+1, time );
	    }

	    if ( auxd->poly_.isEmpty() )
		delete auxd;
	    else
	    {
		vwr_->addAuxData( auxd );
		levelaux_.add( auxd );
	    }
	}
    }

    ctyp = Math::SetBits( ctyp, FlatView::Viewer::Auxdata, true );
}


void uiStratSynthDisp::setPSVwrData()
{
    if ( !psvwrwin_ )
	return;

    const SynthID curid = wvaselfld_->isNoneSelected()
				    ? vdselfld_->curID() : wvaselfld_->curID();
    ConstRefMan<SyntheticData> sd = datamgr_.getDataSet( curid );
    if ( !sd || !sd->isPS() )
	return;

    mDynamicCastGet(const PreStackSyntheticData*,presd,sd.ptr());
    if ( !presd )
	return;

    psvwrwin_->removeGathers();
    const_cast<PreStackSyntheticData*>( presd )->obtainGathers();

    TypeSet<PreStackView::GatherInfo> gatherinfos;

    const int nrgathers = presd->nrPositions();
    const int dispeachgather = nrgathers/8 + 1;
    for ( int idx=0; idx<nrgathers; idx++ )
    {
	gatherinfos += PreStackView::GatherInfo();
	PreStackView::GatherInfo& newgi = gatherinfos.last();

	newgi.isstored_ = false;
	newgi.gathernm_ = sd->name();
	newgi.bid_ = presd->getTrcKey( idx ).binID();
	newgi.wvadp_ = presd->getGather( idx, false );
	newgi.vddp_ = presd->getGather( idx, true );

	newgi.isselected_ = idx%dispeachgather==0 || idx == nrgathers-1;
    }

    psvwrwin_->setGathers( gatherinfos );
}


void uiStratSynthDisp::handlePSViewDisp( FlatView::Viewer::VwrDest dest )
{
    const bool isanyps = curIsPS( FlatView::Viewer::Both );
    psgrp_->display( isanyps );
    if ( psvwrwin_ )
	psvwrwin_->display( isanyps );
    if ( !curIsPS(dest) )
	return;

    const SynthID synthid =
	dest == FlatView::Viewer::WVA
	     ? wvaselfld_->curID()
	     : (dest == FlatView::Viewer::VD
		   ? vdselfld_->curID()
		   : (wvaselfld_->isNoneSelected()
				? vdselfld_->curID() : wvaselfld_->curID()));

    ConstRefMan<SyntheticData> sds = datamgr_.getDataSet( synthid );
    if ( !sds || !sds->isPS() )
	{ pErrMsg("Huh"); return; }

    mDynamicCastGet(const PreStackSyntheticData*,psds,sds.ptr());
    StepInterval<float> offsdef( psds->offsetRange() );
    offsdef.step = psds->offsetRangeStep();

    offsslider_->setInterval( offsdef );
    const int idx = offsdef.nearestIndex( curoffs_ );
    curoffs_ = offsdef.atIndex( idx );
    offsslider_->setValue( curoffs_ );
    updateOffSliderTxt();

    setPSVwrData();
}


void uiStratSynthDisp::curModEdChgCB( CallBacker* )
{
    const Strat::LayerModelSuite& lms = datamgr_.layerModelSuite();
    if ( !modtypetxtitm_ )
    {
	uiGraphicsScene& scene = vwr_->rgbCanvas().scene();
	modtypetxtitm_ = scene.addItem( new uiTextItem );
	modtypetxtitm_->setAlignment( mAlignment(Left,VCenter) );
	modtypetxtitm_->setPenColor( OD::Color::Black() );
	modtypetxtitm_->setZValue( 999999 );
	modtypetxtitm_->setMovable( true );
    }

    const uiString moddesc = lms.uiDesc( lms.curIdx() );
    const bool dodisp = !moddesc.isEmpty();
    modtypetxtitm_->setVisible( dodisp );
    if ( dodisp )
    {
	modtypetxtitm_->setText( moddesc );
	canvasResizeCB( nullptr );
    }

    if ( !canupdatedisp_ )
	return;

    reDisp();

    if ( wvaselfld_->isNoneSelected() && vdselfld_->isNoneSelected() )
	return;

    const SynthID synthid = wvaselfld_->isNoneSelected()
					 ? vdselfld_->curID()
					 : wvaselfld_->curID();
    ConstRefMan<SyntheticData> sd = datamgr_.getDataSet( synthid );
    if ( !sd )
	return;

    const uiWorldRect& curvw = vwr_->curView();
    Interval<double> dispzrg( curvw.top(), curvw.bottom() );
    dispzrg.sort();
    const ZSampling synthzrg = sd->zRange();
    if ( synthzrg.start < dispzrg.start || synthzrg.stop > dispzrg.stop )
    {
	initialboundingbox_.setTop( synthzrg.start );
	initialboundingbox_.setBottom( synthzrg.stop );
    }

    /*TODO: Find a way to set the cancel zoom icon active (except in the rare
    case where the previous view is equal to initialboundingbox_) */
}


void uiStratSynthDisp::canvasResizeCB( CallBacker* )
{
    if ( modtypetxtitm_ )
	modtypetxtitm_->setPos( 20, 10 );
}


void uiStratSynthDisp::dispPropChgCB( CallBacker* )
{
    const FlatView::DataDispPars& ddpars = vwr_->appearance().ddpars_;
    const SynthID wvaid = wvaselfld_->curID();
    const SynthID vdid = vdselfld_->curID();

    StratSynth::SynthSpecificPars* wvaent = entries_.getByID( wvaid );
    if ( wvaent )
	wvaent->useWVADispPars( ddpars.wva_ );

    StratSynth::SynthSpecificPars* vdent = entries_.getByID( vdid );
    if ( vdent )
	vdent->useVDDispPars( ddpars.vd_ );
}


void uiStratSynthDisp::zoomChangedCB( CallBacker* cb )
{
    const uiWorldRect bbwr = vwr_->boundingBox();
    if ( StratSynth::isEmpty(bbwr) )
	return;

    const uiWorldRect& curvw = vwr_->curView();
    if ( StratSynth::isEmpty(curvw) )
	return;

    if ( control_->zoomMgr().atStart() && StratSynth::isEqual(bbwr,curvw) )
	initialboundingbox_ = uiWorldRect();
}


int uiStratSynthDisp::dispEach() const
{
    return edtools_.dispEach();
}


bool uiStratSynthDisp::dispFlattened() const
{
    return edtools_.showFlattened();
}


void uiStratSynthDisp::viewChgCB( CallBacker* )
{
    viewChanged.trigger();
}


void uiStratSynthDisp::lvlChgCB( CallBacker* )
{
    if ( edtools_.showFlattened() )
	reDisp( false );
    else
    {
	od_uint32 ctyp = 0;
	drawLevels( ctyp );
	handleChange( ctyp );
    }
}


void uiStratSynthDisp::flatChgCB( CallBacker* )
{
    const mUnusedVar uiWorldRect curview = vwr_->curView();
    const mUnusedVar bool haszoom = control_ ? !control_->zoomMgr().atStart()
					     : false;
    reDisp( false );
    if ( !control_ )
	return;

    control_->reInitZooms();
/* TODO: make it work, and handle smart Z positioning
    if ( !haszoom )
    {
	control_->reInitZooms();
	return;
    }

    uiWorldRect newview = vwr_->curView();
    newview.setLeft( curview.left() );
    newview.setRight( curview.right() );
    control_->setNewView( newview.centre(), newview.size(), vwr_ );
    vwr_->handleChange( sCast(od_uint32,FlatView::Viewer::Auxdata) );*/
}


void uiStratSynthDisp::dispEachChgCB( CallBacker* )
{
    enableDispUpdate( false );
    datamgr_.setCalcEach( edtools_.dispEach() );
    reDisp( false );
    enableDispUpdate( true );
}


void uiStratSynthDisp::setSavedViewRect()
{
    if ( !control_ || mIsUdf(initialboundingbox_.left()) )
	return;

    control_->setNewView( initialboundingbox_.centre(),
			  initialboundingbox_.size(), vwr_ );
}


void uiStratSynthDisp::useDispPars( const IOPar& iop, od_uint32* ctyp )
{
    PtrMan<IOPar> par = iop.subselect( StratSynth::DataMgr::sKeySynthetics() );
    if ( !par )
	return;

    TypeSet<double> startviewareapts;
    if ( control_ && par->get(sKeyViewArea(),startviewareapts) &&
	 startviewareapts.size() == 4 )
    {
	initialboundingbox_.setLeft( startviewareapts[0] );
	initialboundingbox_.setTop( startviewareapts[1] );
	initialboundingbox_.setRight( startviewareapts[2] );
	initialboundingbox_.setBottom( startviewareapts[3] );
    }

    ManagedObjectSet<SynthGenParams> sgpset;
    if ( !datamgr_.getAllGenPars(*par.ptr(),sgpset) )
	return;

    bool updatewvadisp = false, updatevddisp = false;
    for ( int idx=0; idx<sgpset.size(); idx++ )
    {
	const SynthGenParams* sgp = sgpset.get( idx );
	const SynthID sid = datamgr_.find( sgp->name_ );
	if ( !sid.isValid() )
	    continue;

	StratSynth::SynthSpecificPars* ent = entries_.getByID( sid );
	if ( !ent )
	    continue;

	PtrMan<IOPar> subiop = par->subselect(
		IOPar::compKey(StratSynth::DataMgr::sKeySyntheticNr(),idx) );
	if ( subiop )
	    ent->useDispPars( *subiop.ptr(),
			      !sgp->isAttribute() && !sgp->isStratProp() );

	if ( !updatewvadisp && sid == wvaselfld_->curID() )
	    updatewvadisp = true;

	if ( !updatevddisp && sid == vdselfld_->curID() )
	    updatevddisp = true;
    }

    PtrMan<NotifyStopper> ns;
    if ( updatewvadisp || updatevddisp )
	ns = new NotifyStopper( control_->zoomChanged, this );

    const FlatView::Viewer::VwrDest dest =
	FlatView::Viewer::getDest( updatewvadisp, updatevddisp );
    if ( dest == FlatView::Viewer::None )
	return;

    updateDispPars( dest, ctyp );
}


bool uiStratSynthDisp::usePar( const IOPar& iop )
{
    const bool ret = datamgr_.usePar( iop );
    if ( !ret )
    {
	TypeSet<SynthID> ids;
	datamgr_.getIDs( ids, StratSynth::DataMgr::NoProps );
	if ( ids.isEmpty() )
	{
	    const SynthGenParams sgp;
	    datamgr_.addSynthetic( sgp );
	}
    }

    return ret;
}


void uiStratSynthDisp::fillPar( IOPar& iop ) const
{
    TypeSet<SynthID> ids;
    datamgr_.getIDs( ids, StratSynth::DataMgr::NoProps );
    ManagedObjectSet<IOPar> dispiops;
    dispiops.setNullAllowed();
    for ( const auto& id : ids )
    {
	const SynthSpecificPars* disppars = entries_.getByID( id );
	const SynthGenParams* sgp = datamgr_.getGenParams( id );
	if ( !sgp || sgp->isStratProp() || !disppars )
	{
	    dispiops.add( nullptr );
	    continue;
	}

	auto* dispiop = new IOPar;
	disppars->fillDispPars( *dispiop );
	if ( dispiop->isEmpty() )
	{
	    delete dispiop;
	    dispiops.add( nullptr );
	    continue;
	}

	dispiops.add( dispiop );
    }

    datamgr_.fillPar( iop, &dispiops );
    if ( control_ && !control_->zoomMgr().atStart() &&
	 !StratSynth::isEmpty(initialboundingbox_) )
    {
	TypeSet<double> startviewareapts;
	startviewareapts.setSize( 4 );
	startviewareapts[0] = initialboundingbox_.left();
	startviewareapts[1] = initialboundingbox_.top();
	startviewareapts[2] = initialboundingbox_.right();
	startviewareapts[3] = initialboundingbox_.bottom();
	iop.set( IOPar::compKey(StratSynth::DataMgr::sKeySynthetics(),
		    sKeyViewArea()), startviewareapts );
    }
}


void uiStratSynthDisp::synthAddedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(SynthID,id,cb);
    updFlds( false );
    if ( wvaselfld_->curID() == id || vdselfld_->curID() == id )
	packSelCB( cb );
    else if ( wvaselfld_->curID() == id )
	wvaSelCB( cb );
    else if ( vdselfld_->curID() == id )
	vdSelCB( cb );
}


void uiStratSynthDisp::synthRenamedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(const TypeSet<SynthID>&,ids,cb);
    for ( const auto& id : ids )
    {
	const BufferString newnm = datamgr_.nameOf( id );
	wvaselfld_->updateName( id, newnm );
	vdselfld_->updateName( id, newnm );

	const SynthSpecificPars* disppars = entries_.getByID( id );
	if ( !disppars )
	    continue;

	ConstRefMan<SyntheticData> sd = datamgr_.getDataSet( id );
	if ( !sd )
	    continue;

	DataPackMgr& dpm = DPM( DataPackMgr::FlatID() );
	for ( const auto* dpobj : disppars->dpobjs_ )
	{
	    const DataPackID dpid = dpobj->id();
	    if ( !dpm.isPresent(dpid) )
		continue;

	    RefMan<DataPack> dp = dpm.getDP( dpid );
	    if ( !dp )
		continue;

	    if ( dp != &sd->getPack() )
		dp->setName( newnm );
	}
    }
}


void uiStratSynthDisp::synthRemovedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(const TypeSet<SynthID>&,ids,cb);
    const SynthID prevwvaid = wvaselfld_->curID();
    const SynthID prevdid = vdselfld_->curID();

    if ( canupdatedisp_ )
	updFlds( false );

    bool wvachanged = false;
    bool vdchanged = false;
    for ( const auto& id : ids )
    {
	if ( id.asInt() <= 0 )
	    continue;

	wvachanged = wvachanged || id == prevwvaid;
	vdchanged = vdchanged || id == prevdid;
	if ( (id == prevwvaid || id == prevdid) && control_ )
	{
	    ObjectSet<const TimeDepthModel> d2tmodels;
	    control_->setD2TModels( d2tmodels );
	}

	SynthSpecificPars* disppars = entries_.getByID( id );
	if ( disppars )
	    disppars->update();
    }

    if ( !canupdatedisp_ )
	return;

    if ( wvachanged && vdchanged && wvaselfld_->curID() == vdselfld_->curID() )
	packSelCB( cb );
    else
    {
	if ( wvachanged )
	    wvaSelCB( cb );
	if ( vdchanged )
	    vdSelCB( cb );
    }
}


void uiStratSynthDisp::dataMgrCB( CallBacker* )
{
    if ( !uidatamgr_ )
    {
	uidatamgr_ = new uiSynthGenDlg( this, datamgr_ );
	uiSynthParsGrp* uidatamgrgrp = uidatamgr_->grp();
	mAttachCB( uidatamgrgrp->synthAdded, uiStratSynthDisp::newAddedCB );
	mAttachCB( uidatamgrgrp->synthSelected, uiStratSynthDisp::newSelCB );
    }

    uidatamgr_->go();
}


void uiStratSynthDisp::newAddedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    // Not changing the current selection unless it is invalid
    const bool validwva = datamgr_.hasValidDataSet( wvaselfld_->curID() );
    const bool validvd = datamgr_.hasValidDataSet( vdselfld_->curID() );
    if ( validwva && validvd )
	return;
    else if ( !validwva && !validvd )
    {
	newSelCB( cb );
	return;
    }

    uiStratSynthDispDSSel* selfld = validwva ? vdselfld_ : wvaselfld_;
    mCBCapsuleUnpack(SynthID,id,cb);
    selfld->setCurrentItem( id );
    selfld->selChange.trigger();
}


void uiStratSynthDisp::newSelCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(SynthID,id,cb);

    updFlds( true );
    NotifyStopper wvans( wvaselfld_->selChange );
    NotifyStopper vdns( vdselfld_->selChange );
    wvaselfld_->setCurrentItem( id );
    vdselfld_->setCurrentItem( id );
    packSelCB( cb );
}


void uiStratSynthDisp::offsSliderChgCB( CallBacker* )
{
    curoffs_ = offsslider_->getFValue();
    const FlatView::Viewer::VwrDest dest =
	  FlatView::Viewer::getDest( curIsPS(FlatView::Viewer::WVA),
				     curIsPS(FlatView::Viewer::VD) );
    od_uint32 ctyp = 0;
    setViewerData( dest, ctyp, true );
    updateOffSliderTxt();
    handleChange( ctyp );
}


void uiStratSynthDisp::updateOffSliderTxt()
{
    offsslider_->setToolTip( toUiString("%1: %2")
			     .arg( uiStrings::sOffset() ).arg( curoffs_ ) );
}


void uiStratSynthDisp::exportCB( CallBacker* )
{
    uiStratSynthExport dlg( this, datamgr_ );
    dlg.go();
}


void uiStratSynthDisp::viewPSCB( CallBacker* )
{
    if ( psvwrwin_ )
	{ psvwrwin_->show(); return; }

    psvwrwin_ =
	new PreStackView::uiSyntheticViewer2DMainWin( this,
						      "PreStack Synthetics" );
    psvwrwin_->setDeleteOnClose( false );
    mAttachCB( psvwrwin_->seldatacalled_, uiStratSynthDisp::setPSVwrDataCB );
    setPSVwrData();
    psvwrwin_->show();
}


void uiStratSynthDisp::setPSVwrDataCB( CallBacker* )
{
    BufferStringSet allgnms, selgnms;
    datamgr_.getNames( allgnms, DataMgr::OnlyPS );

    const TypeSet<PreStackView::GatherInfo> ginfos = psvwrwin_->gatherInfos();
    psvwrwin_->getGatherNames( selgnms );
    for ( const auto* selnm : selgnms )
    {
	const int gidx = allgnms.indexOf( selnm->buf() );
	if ( !allgnms.validIdx(gidx) )
	    allgnms.removeSingle( gidx );
    }

    PreStackView::uiViewer2DSelDataDlg seldlg( psvwrwin_, allgnms, selgnms );
    if ( !seldlg.go() )
	return;

    psvwrwin_->removeGathers();
    TypeSet<PreStackView::GatherInfo> newginfos;
    for ( const auto* selnm : selgnms )
    {
	const SynthID sid = datamgr_.find( selnm->buf() );
	ConstRefMan<SyntheticData> sd;
	if ( datamgr_.isPS(sid) )
	{
	    enableDispUpdate( false );
	    uiTaskRunner trprov( this );
	    if ( datamgr_.ensureGenerated(sid,&trprov) )
		sd = datamgr_.getDataSet( sid );
	    enableDispUpdate( true );
	}

	if ( !sd || !sd->isPS() )
	    continue;

	mDynamicCastGet(const PreStackSyntheticData*,presd,sd.ptr());
	if ( !presd )
	    continue;

	const_cast<PreStackSyntheticData*>( presd )->obtainGathers();
	const PreStack::GatherSetDataPack& seisgdp = presd->preStackPack();
	const PreStack::GatherSetDataPack& anglegdp = presd->angleData();
	for ( const auto& ginfo : ginfos )
	{
	    PreStackView::GatherInfo newginfo = ginfo;
	    newginfo.gathernm_ = sd->name();
	    newginfo.wvadp_ = seisgdp.getGather( newginfo.bid_ );
	    newginfo.vddp_ = anglegdp.getGather( newginfo.bid_ );
	    if ( newginfo.wvadp_ && newginfo.vddp_ )
		newginfos.addIfNew( newginfo );
	}
    }

    psvwrwin_->setGathers( newginfos );
}


void uiStratSynthDisp::packSelCB( CallBacker* cb )
{
    od_uint32 ctyp = 0;
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::Both;
    setViewerData( dest, ctyp, true );
    updWvltFld();
    handlePSViewDisp( dest );
    handleChange( ctyp );
}


void uiStratSynthDisp::wvaSelCB( CallBacker* cb )
{
    const BufferString wvanm = wvaselfld_->currentName();
    const BufferString vdnm = vdselfld_->currentName();
    if ( !wvaselfld_->isNoneSelected() && !vdselfld_->isNoneSelected() )
    {
	const SynthGenParams* sgp = datamgr_.getGenParams( vdselfld_->curID() );
	if ( sgp && !sgp->isStratProp() && !sgp->isAttribute() )
	{
	    NotifyStopper ns( vdselfld_->selChange );
	    vdselfld_->setCurrentItem( wvaselfld_->curID() );
	    ns.enableNotification();
	    packSelCB( cb );
	    return;
	}
    }

    od_uint32 ctyp = 0;
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::WVA;
    setViewerData( dest, ctyp, true );
    updWvltFld();
    handlePSViewDisp( dest );
    handleChange( ctyp );
}


void uiStratSynthDisp::vdSelCB( CallBacker* cb )
{
    const BufferString vdnm = vdselfld_->currentName();
    const BufferString wvanm = wvaselfld_->currentName();
    const SynthGenParams* vdsgp = datamgr_.getGenParams( vdselfld_->curID() );
    if ( !vdselfld_->isNoneSelected() && !wvaselfld_->isNoneSelected() &&
	 wvanm != vdnm &&
	 vdsgp && !vdsgp->isStratProp() && !vdsgp->isAttribute() )
    {
	NotifyStopper ns( wvaselfld_->selChange );
	wvaselfld_->setCurrentItem( vdselfld_->curID() );
	ns.enableNotification();
	packSelCB( cb );
	return;
    }

    od_uint32 ctyp = 0;
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::VD;
    setViewerData( dest, ctyp, true );
    updWvltFld();
    handlePSViewDisp( dest );
    handleChange( ctyp );
}


uiFlatViewer* uiStratSynthDisp::getViewerClone( uiParent* p ) const
{
    auto* vwr = new uiFlatViewer( p );
    vwr->rgbCanvas().disableImageSave();
    vwr->setInitialSize( initialsz_ );
    vwr->setStretch( 2, 2 );
    vwr->appearance() = vwr_->appearance();
    auto wvadp = vwr_->getPack(true).get();
    auto vddp = vwr_->getPack(false).get();
    if ( wvadp.ptr() == vddp.ptr() )
	vwr->setPack( FlatView::Viewer::Both, wvadp, false );
    else
    {
	const bool canupdate = vwr_->enableChange( false );
	vwr->setPack( FlatView::Viewer::WVA, wvadp, false );
	vwr_->enableChange( canupdate );
	vwr->setPack( FlatView::Viewer::VD, vddp, false );
    }

    return vwr;
}


void uiStratSynthDisp::setDefaultAppearance( FlatView::Appearance& app )
{
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.allowuserchangereversedaxis_ = false;
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.name_ = uiStrings::sTraceNumber();
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.name_ = uiStrings::sTWT();
    app.ddpars_.show( true, true );
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
}


void uiStratSynthDisp::makeInfoMsg( BufferString& mesg, IOPar& pars )
{
    BufferString valstr = pars.find( sKey::TraceNr() );
    if ( valstr.isEmpty() )
	return;

    const int modelidx = toInt( valstr );
    const int seqidx = modelidx-1;
    if ( seqidx<0 || seqidx>=datamgr_.layerModel().size() )
	return;

    BufferString modelnrstr( 24, true );
    od_sprintf( modelnrstr.getCStr(), modelnrstr.bufSize(),
		"Model Number:%5d", modelidx );
    mesg.add( modelnrstr );
    valstr = pars.find( sKey::Z() );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Z-Coord" );

    float zval = mUdf(float);
    if ( !valstr.isEmpty() )
    {
	BufferString depthstr( 16, true );
	zval = toFloat( valstr );
	od_sprintf( depthstr.getCStr(), depthstr.bufSize(),
		    "Depth : %6.0f", zval );
	depthstr.add( SI().getZUnitString() );
	mesg.addSpace().add( depthstr );

	if ( psgrp_->isDisplayed() )
	{
	    BufferString offsetstr( 16, true );
	    zval = offsslider_->getFValue();
	    od_sprintf( offsetstr.getCStr(), offsetstr.bufSize(),
			"Offset : %6.0f", zval );
	    offsetstr.add( SI().getXYUnitString() );
	    mesg.addSpace().add( offsetstr );
	}
    }

    if ( mIsUdf(zval) )
	return;

    mesg.addSpace();
    int nrinfos = 0;
#define mAddSep() if ( nrinfos++ ) mesg += ";\t";
    BufferString vdstr = pars.find( "Variable density data" );
    BufferString wvastr = pars.find( "Wiggle/VA data" );
    BufferString vdvalstr = pars.find( "VD Value" );
    BufferString wvavalstr = pars.find( "WVA Value" );
    const bool issame = !vdstr.isEmpty() && !wvastr.isEmpty() && vdstr==wvastr;
    if ( !vdvalstr.isEmpty() )
    {
	mAddSep();
	if ( issame )
	    { if ( vdstr.isEmpty() ) vdstr = wvastr; }
	else
	    { if ( vdstr.isEmpty() ) vdstr = "VD Val"; }
	float val = !vdvalstr.isEmpty() ? vdvalstr.toFloat() : mUdf(float);
	mesg += "Val="; mesg += mIsUdf(val) ? "undef" : vdvalstr.buf();
	mesg += " ("; mesg += vdstr; mesg += ")";
    }

    if ( !wvavalstr.isEmpty() && !issame )
    {
	mAddSep();
	float val = !wvavalstr.isEmpty() ? wvavalstr.toFloat() : mUdf(float);
	mesg += "Val="; mesg += mIsUdf(val) ? "undef" : wvavalstr.buf();
	if ( wvastr.isEmpty() ) wvastr = "WVA Val";
	mesg += " ("; mesg += wvastr; mesg += ")";
    }

    float val;
    if ( pars.get(sKey::Offset(),val) && !mIsUdf(val) )
    {
	mAddSep(); mesg += "Offs="; mesg += val;
	mesg += " "; mesg += SI().getXYUnitString();
    }

    TypeSet<SynthID> validids;
    datamgr_.getIDs( validids, DataMgr::NoSubSel, true );
    ConstRefMan<SyntheticData> sd;
    if ( !validids.isEmpty() )
	sd = datamgr_.getDataSet( validids.first() );

    const TimeDepthModel* d2tmdl = sd ? sd->getTDModel( seqidx ) : nullptr;
    const Strat::LayerModel& laymod = datamgr_.layerModel();
    const float srd = mCast(float,SI().seismicReferenceDatum());
    if ( d2tmdl )
    {
	const float realzval = zval / SI().showZ2UserFactor();
	const float depth = d2tmdl->getDepth( realzval ) - srd;
	const Strat::LayerSequence& curseq = laymod.sequence( seqidx );
	for ( int lidx=0; lidx<curseq.size(); lidx++ )
	{
	    const Strat::Layer& layer = *curseq.layers().get( lidx );
	    if ( layer.zTop()<=depth && layer.zBot()>depth )
	    {
		mesg.addTab().add( "Layer:" ).add( layer.name() );
		mesg.add( "; Lithology:" ).add( layer.lithology().name() );
		if ( !layer.content().isUnspecified() )
		    mesg.add( "; Content:" ).add( layer.content().name() );
		break;
	    }
	}
    }
}
