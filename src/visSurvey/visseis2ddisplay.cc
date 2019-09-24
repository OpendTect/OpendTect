/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2004
 ________________________________________________________________________

-*/


#include "visseis2ddisplay.h"

#include "visdrawstyle.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistexturechannels.h"
#include "vistexturepanelstrip.h"

#include "array2dresample.h"
#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "bendpointfinder.h"
#include "probeimpl.h"
#include "seisdatapack.h"
#include "volumedatapackzaxistransformer.h"
#include "zaxistransform.h"
#include "survinfo.h"
#include "uistrings.h"
#include "keystrs.h"

//For parsing old pars
#include "attribsel.h"


namespace visSurvey
{

const char* Seis2DDisplay::sKeyLineSetID()	{ return "LineSet ID"; }
const char* Seis2DDisplay::sKeyTrcNrRange()	{ return "Trc Nr Range"; }
const char* Seis2DDisplay::sKeyZRange()		{ return "Z Range"; }
const char* Seis2DDisplay::sKeyShowLineName()	{ return "Show linename"; }
const char* Seis2DDisplay::sKeyShowPanel()	{ return "Show panel"; }
const char* Seis2DDisplay::sKeyShowPolyLine()	{ return "Show polyline"; }

Seis2DDisplay::Seis2DDisplay()
    : transformation_(0)
    , geometry_(*new PosInfo::Line2DData)
    , panelstrip_(visBase::TexturePanelStrip::create())
    , polyline_(visBase::PolyLine::create())
    , linename_(visBase::Text::create())
    , geomchanged_(this)
    , geomidchanged_(this)
    , maxtrcnrrg_(0,mUdf(int),1)
    , datatransform_(0)
    , voiidx_(-1)
    , prevtrcidx_(0)
    , pixeldensity_( getDefaultPixelDensity() )
{
    datapacks_.setNullAllowed();
    transformedpacks_.setNullAllowed();

    geometry_.setZRange( StepInterval<float>(mUdf(float),mUdf(float),1) );

    polyline_->ref();
    polyline_->setMaterial( new visBase::Material );
    addChild( polyline_->osgNode() );

    polylineds_ = polyline_->addNodeState( new visBase::DrawStyle );
    polylineds_->setLineStyle(
			OD::LineStyle(OD::LineStyle::Solid,3,Color::White()) );
    setColor( Color::White() );
    polylineds_->ref();

    panelstrip_->ref();
    addChild( panelstrip_->osgNode() );
    panelstrip_->setTextureChannels( channels_ );
    panelstrip_->swapTextureAxes();
    panelstrip_->smoothNormals();

    linename_->ref();
    addChild( linename_->osgNode() );
    linename_->addText();

    getMaterial()->setColor( Color::White() );
    getMaterial()->setAmbience( 0.8 );
    getMaterial()->setDiffIntensity( 0.2 );

    setColor( Color(0,150,75) );

    init();
    showPanel( false );
}


Seis2DDisplay::~Seis2DDisplay()
{
    removeChild( linename_->osgNode() );
    linename_->unRef();

    delete &geometry_;

    if ( transformation_ ) transformation_->unRef();

    polyline_->removeNodeState( polylineds_ );
    polylineds_->unRef();
    polyline_->unRef();

    panelstrip_->unRef();
    setZAxisTransform( 0,0 );
}


void Seis2DDisplay::setColor( Color nc )
{ polyline_->getMaterial()->setColor( nc ); }


Color Seis2DDisplay::getColor() const
{ return polyline_->getMaterial()->getColor(); }


const OD::LineStyle* Seis2DDisplay::lineStyle() const
{ return &polylineds_->lineStyle(); }


void Seis2DDisplay::setLineStyle( const OD::LineStyle& ls )
{
    polylineds_->setLineStyle( ls );
    requestSingleRedraw();
}


void Seis2DDisplay::showPanel( bool yn )
{
    panelstrip_->turnOn( yn );
    geomchanged_.trigger();
}


void Seis2DDisplay::showPolyLine( bool yn )
{ polyline_->turnOn( yn ); }

bool Seis2DDisplay::isPanelShown() const
{ return panelstrip_->isOn(); }

bool Seis2DDisplay::isPolyLineShown() const
{ return polyline_->isOn(); }


void Seis2DDisplay::enableAttrib( int attrib, bool yn )
{
    MultiTextureSurveyObject::enableAttrib( attrib, yn );
    geomchanged_.trigger();
}


void Seis2DDisplay::setProbe( Probe* probe )
{
    if ( probe_.ptr()==probe )
	return;

    probe_ = probe;
    const TrcKeyZSampling probepos = probe_->position();
    setTraceNrRange( probepos.hsamp_.trcRange() );
    setZRange( probepos.zsamp_ );
    const uiString dispnm( probe_->displayName() );
    setName( toString(dispnm) );
    linename_->text()->setText( dispnm );

    if ( scene_ )
    {
	setAnnotColor( scene_->getAnnotColor() );
	linename_->text()->setFontData( scene_->getAnnotFont(),
					getPixelDensity() );
    }

    geomidchanged_.trigger();
}


Pos::GeomID Seis2DDisplay::getGeomID() const
{
    mDynamicCastGet(const Line2DProbe*,l2dprobe,probe_.ptr());
    return l2dprobe ? l2dprobe->geomID() : Pos::GeomID();
}


BufferString Seis2DDisplay::getLineName() const
{
    mDynamicCastGet(const Line2DProbe*,l2dprobe,probe_.ptr());
    return l2dprobe ? l2dprobe->displayName().getString()
		    : BufferString(name());
}


void Seis2DDisplay::setGeometry( const PosInfo::Line2DData& geometry )
{
    geometry_ = geometry;
    const TypeSet<PosInfo::Line2DPos>& linepositions = geometry.positions();
    const int tracestep = geometry.trcNrRange().step;

    const int possz = linepositions.size();
    trcdisplayinfo_.alltrcnrs_.erase();
    trcdisplayinfo_.alltrcpos_.erase();
    trcdisplayinfo_.alltrcnrs_.setCapacity( possz, false );
    trcdisplayinfo_.alltrcpos_.setCapacity( possz, false );

    maxtrcnrrg_.set( 0, mUdf(int), tracestep );
    for ( int idx=0; idx<possz; idx++ )
    {
	if ( idx == 0 )
	    maxtrcnrrg_.start = maxtrcnrrg_.stop = linepositions[idx].nr_;
	else
	    maxtrcnrrg_.include( linepositions[idx].nr_, false );
	trcdisplayinfo_.alltrcnrs_ += linepositions[idx].nr_;
	trcdisplayinfo_.alltrcpos_ += linepositions[idx].coord_;
    }

    trcdisplayinfo_.zrg_.step = datatransform_ ? datatransform_->getGoodZStep()
					       : geometry_.zRange().step;
    setTraceNrRange( maxtrcnrrg_ );
    setZRange( geometry_.zRange() );

    updateRanges( false, true );
    geomchanged_.trigger();
}


void Seis2DDisplay::getTraceKeyPath( TrcKeyPath& res,TypeSet<Coord>* ) const
{
    mDynamicCastGet(const Line2DProbe*,l2dprobe,probe_.ptr());
    if ( trcdisplayinfo_.rg_.isUdf() || l2dprobe )
	return;

    for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs_.size(); idx++ )
    {
	if ( trcdisplayinfo_.alltrcnrs_[idx] < trcdisplayinfo_.rg_.start )
	    continue;
	else if ( trcdisplayinfo_.alltrcnrs_[idx] > trcdisplayinfo_.rg_.stop )
	    break;

	res += TrcKey( l2dprobe->geomID(), trcdisplayinfo_.alltrcnrs_[idx] );
    }
}


Interval<float> Seis2DDisplay::getDataTraceRange() const
{
    return getZRange( false, -1 );
}


StepInterval<float> Seis2DDisplay::getMaxZRange( bool displayspace ) const
{
    if ( !datatransform_ || !displayspace )
	return geometry_.zRange();

    StepInterval<float> zrg;
    zrg.setFrom( datatransform_->getZInterval(false) );
    zrg.step = geometry_.zRange().step;
    return zrg;
}


void Seis2DDisplay::setZRange( const StepInterval<float>& nzrg )
{
    if ( mIsUdf(geometry_.zRange().start) )
	return;

    const StepInterval<float> maxzrg = getMaxZRange( true );
    const Interval<float> zrg( mMAX(maxzrg.start,nzrg.start),
			       mMIN(maxzrg.stop,nzrg.stop) );
    const bool hasdata = !datapacks_.isEmpty() && datapacks_[0];
    if ( hasdata && trcdisplayinfo_.zrg_.isEqual(zrg,mDefEps) )
	return;

    trcdisplayinfo_.zrg_.setFrom( zrg );

    updatePanelStripZRange();
    updatePanelStripPath();
    geomchanged_.trigger();
}


StepInterval<float> Seis2DDisplay::getZRange( bool displayspace,
					      int attrib ) const
{
    const bool alreadytransformed = alreadyTransformed( attrib );
    if ( alreadytransformed )
	return trcdisplayinfo_.zrg_;

    if ( datatransform_ && !displayspace )
    {
	StepInterval<float> zrg = datatransform_->getZInterval( true );
	zrg.step = SI().zStep();
	return zrg;
    }

    return trcdisplayinfo_.zrg_;
}


const Interval<int> Seis2DDisplay::getSampleRange() const
{
    StepInterval<float> maxzrg = getMaxZRange( true );
    Interval<int> samplerg( maxzrg.nearestIndex(trcdisplayinfo_.zrg_.start),
			    maxzrg.nearestIndex(trcdisplayinfo_.zrg_.stop) );
    return samplerg;
}


#define mRetErrGeo \
{ pErrMsg("Geometry not set"); return; }

void Seis2DDisplay::setTraceNrRange( const Interval<int>& reqtrcnrrg )
{
    trcdisplayinfo_.rg_.setUdf();

    if ( maxtrcnrrg_.isRev() )
	mRetErrGeo;

    const Interval<int> trcnrrg( maxtrcnrrg_.limitValue(reqtrcnrrg.start),
				 maxtrcnrrg_.limitValue(reqtrcnrrg.stop) );
    if ( trcnrrg.width() < 1 )
	return;

    Interval<int> idxrg( mUdf(int), mUdf(int) );
    for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs_.size(); idx++ )
    {
	const auto trcnr = trcdisplayinfo_.alltrcnrs_[idx];
	if ( trcnrrg.includes(trcnr,false) )
	{
	    if ( mIsUdf(idxrg.start) )
		idxrg.start = idxrg.stop = idx;
	    else
		idxrg.include( idx, false );
	}
    }

    if ( mIsUdf(idxrg.start) )
	mRetErrGeo;

    trcdisplayinfo_.rg_.start = trcdisplayinfo_.alltrcnrs_[idxrg.start];
    trcdisplayinfo_.rg_.stop = trcdisplayinfo_.alltrcnrs_[idxrg.stop];
    trcdisplayinfo_.size_ = idxrg.width() + 1;
    updatePanelStripPath();
}


const Interval<int>& Seis2DDisplay::getTraceNrRange() const
{
    return trcdisplayinfo_.rg_;
}


const StepInterval<int>& Seis2DDisplay::getMaxTraceNrRange() const
{ return maxtrcnrrg_; }


bool Seis2DDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				   TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp || regsdp->isEmpty() )
    {
	channels_->setUnMappedVSData( attrib, 0, 0, OD::UsePtr,
				      ExistingTaskRunnerProvider(taskr) );
	channels_->turnOn( false );
	return false;
    }

    datapacks_.replace( attrib, regsdp );

    createTransformedDataPack( attrib, taskr );
    updateChannels( attrib, taskr );
    return true;
}


DataPack::ID Seis2DDisplay::getDataPackID( int attrib ) const
{
    return datapacks_.validIdx(attrib) && datapacks_[attrib]
	? datapacks_[attrib]->id()
        : DataPack::cNoID();
}


DataPack::ID Seis2DDisplay::getDisplayedDataPackID( int attrib ) const
{
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
        return transformedpacks_.validIdx(attrib) && transformedpacks_[attrib]
            ? transformedpacks_[attrib]->id()
            : DataPack::cNoID();
        }

    return getDataPackID( attrib );
}


void Seis2DDisplay::updateTexOriginAndScale( int attrib,
					     const TrcKeyZSampling& tkzs )
{
    if ( !tkzs.isDefined() || tkzs.isEmpty() )
	return;

    const TraceDisplayInfo& tdi = trcdisplayinfo_;

    const Coord origin( (tkzs.zsamp_.start-tdi.zrg_.start) / tdi.zrg_.step,
		tkzs.hsamp_.trcRange().start - tdi.rg_.start );

    const Coord scale( tkzs.zsamp_.step / tdi.zrg_.step,
		       tkzs.hsamp_.trcRange().step );

    channels_->setOrigin( attrib, origin*(resolution_+1) );
    channels_->setScale( attrib, scale );
}


void Seis2DDisplay::updateChannels( int attrib, TaskRunner* taskr )
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp )
	return;

    updateTexOriginAndScale( attrib, TrcKeyZSampling(regsdp->subSel()) );

    const int nrversions = regsdp->nrComponents();
    channels_->setNrVersions( attrib, nrversions );

    UserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    int sz0=mUdf(int), sz1=mUdf(int);
    for ( int idx=0; idx<nrversions; idx++ )
    {
	const auto& array = regsdp->data( idx );

	if ( !idx )
	{
	    sz0 = 1 + (array.getSize(1)-1) * (resolution_+1);
	    sz1 = 1 + (array.getSize(2)-1) * (resolution_+1);
	}

	const ValueSeries<float>* vsptr
		= !resolution_ ? array.valueSeries() : 0;
	bool ownsvsptr = false;

	if ( !vsptr )
	{
	    vsptr = new MultiArrayValueSeries<float,float>(sz0*sz1);
	    ownsvsptr = true;
	}

	if ( !vsptr || !vsptr->isOK() )
	{
	    channels_->turnOn( false );
	    pErrMsg("Insufficient memory; cannot display the 2D seismics.");
	    if ( ownsvsptr )
		delete vsptr;
	    return;
	}

	if ( ownsvsptr )
	{
	    auto* myvsptr = const_cast< ValueSeries<float>* >( vsptr );
	    if ( resolution_ == 0 )
		array.getAll( *myvsptr );
	    else
	    {
		Array2DSlice<float> slice2d( array );
		slice2d.setDimMap( 0, 1 );
		slice2d.setDimMap( 1, 2 );
		slice2d.setPos( 0, 0 );
		slice2d.init();

		Array2DReSampler<float,float> resampler(
				slice2d, *myvsptr, sz0, sz1, true );
		resampler.setInterpolate( true );
		TaskRunner::execute( taskr, resampler );
	    }
	}

	channels_->setSize( attrib, 1, sz0, sz1 );
	channels_->setUnMappedVSData( attrib, idx, vsptr,
			ownsvsptr ? OD::TakeOverPtr : OD::UsePtr,
			ExistingTaskRunnerProvider(taskr) );
    }

    channels_->turnOn( true );
}


void Seis2DDisplay::createTransformedDataPack( int attrib, TaskRunner* taskr )
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDataPackID( attrib );
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp || regsdp->isEmpty() )
	return;

    RefMan<RegularSeisDataPack> output = 0;
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, attrib );
	if ( datatransform_->needsVolumeOfInterest() )
	{
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest( tkzs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, tkzs, true );
	    ExistingTaskRunnerProvider trprov( taskr );
	    datatransform_->loadDataIfMissing( voiidx_, trprov );
	}

	VolumeDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( regsdp.ptr() );
	transformer.setInterpolate( textureInterpolationEnabled() );
	transformer.setOutputZRange( tkzs.zsamp_ );
	transformer.execute();
        output = transformer.getOutput();
    }

    transformedpacks_.replace( attrib, output );
}


void Seis2DDisplay::updatePanelStripPath()
{
    TraceDisplayInfo& tdi = trcdisplayinfo_;
    BendPointFinder2D finder( tdi.alltrcpos_.arr(), tdi.alltrcpos_.size(), 1.0);
    finder.execute();

    const TypeSet<int>& bends = finder.bendPoints();
    tdi.alljoints_.erase();

    for ( int idx=0; idx<bends.size(); idx++ )
    {
	if ( tdi.alltrcnrs_[bends[idx]]>tdi.rg_.start &&
	    (!tdi.alljoints_.isEmpty()
	       && tdi.alltrcnrs_[tdi.alljoints_.last()] < tdi.rg_.start) )
	{
	    const int trcidx = tdi.alltrcnrs_.indexOf( tdi.rg_.start );
	    if ( tdi.alltrcnrs_.validIdx(trcidx) )
		tdi.alljoints_ += trcidx;
	}

	if ( tdi.alltrcnrs_[bends[idx]]>tdi.rg_.stop &&
	    (!tdi.alljoints_.isEmpty()
	       && tdi.alltrcnrs_[tdi.alljoints_.last()] < tdi.rg_.stop) )
	{
	    const int trcidx = tdi.alltrcnrs_.indexOf( tdi.rg_.stop );
	    if ( tdi.alltrcnrs_.validIdx(trcidx) )
		tdi.alljoints_ += trcidx;
	}

	tdi.alljoints_ += bends[idx];
    }

    TypeSet<int> knots;

    for ( int idx=0; idx<tdi.alljoints_.size(); idx++ )
    {
	const int trcidx = tdi.alljoints_[idx];
	if ( !tdi.rg_.includes(tdi.alltrcnrs_[trcidx],true) )
	    continue;

	if ( !knots.isEmpty() )
	{
	    for ( int posidx=tdi.alljoints_[idx-1]+1;
		  posidx<tdi.alljoints_[idx]; posidx++ )
	    {
		const Coord pos = tdi.alltrcpos_[posidx];
		float d0 = pos.distTo<float>( tdi.alltrcpos_[posidx-1] );
		float d1 = pos.distTo<float>( tdi.alltrcpos_[posidx+1] );
		d0 *= abs( tdi.alltrcnrs_[posidx+1]-tdi.alltrcnrs_[posidx] );
		d1 *= abs( tdi.alltrcnrs_[posidx-1]-tdi.alltrcnrs_[posidx] );
		if ( (d0+d1)>0.0f && fabs(d0-d1)/(d0+d1)>0.1f )
		    knots += posidx;
	    }
	}
	knots += tdi.alljoints_[idx];
    }

    TypeSet<Coord> path;
    TypeSet<float> mapping;
    path.setCapacity( knots.size(), false );
    mapping.setCapacity( knots.size(), false );

    polyline_->removeAllPoints();
    for ( int idx=0; idx<knots.size(); idx++ )
    {
	path += tdi.alltrcpos_[knots[idx]];
	const float diff =
		mCast(float,tdi.alltrcnrs_[knots[idx]]-tdi.rg_.start);
	mapping += diff * (resolution_+1);

	const Coord3 linepos( path[idx], tdi.zrg_.start );
	polyline_->addPoint( linepos );
    }

    panelstrip_->setPath( path );
    panelstrip_->setPath2TextureMapping( mapping );

    if ( getUpdateStageNr() )
    {
	const float diff =
		mCast(float,updatestageinfo_.oldtrcrgstart_-tdi.rg_.start);
	panelstrip_->setPathTextureShift( diff*(resolution_+1) );
    }

    updateLineNamePos();
}


void Seis2DDisplay::updatePanelStripZRange()
{
    panelstrip_->setZRange( trcdisplayinfo_.zrg_ );
    const Interval<float> mapping( 0.0f,
		mCast(float,trcdisplayinfo_.zrg_.nrSteps())*(resolution_+1) );
    panelstrip_->setZRange2TextureMapping( mapping );

    if ( getUpdateStageNr() )
    {
	const float diff =
		    updatestageinfo_.oldzrgstart_ - trcdisplayinfo_.zrg_.start;
	const float factor = (resolution_+1) / trcdisplayinfo_.zrg_.step;
	panelstrip_->setZTextureShift( diff*factor );
    }

    updateLineNamePos();
}


void Seis2DDisplay::annotateNextUpdateStage( bool yn )
{
    if ( !yn )
    {
	panelstrip_->setPathTextureShift( 0.0 );
	panelstrip_->setZTextureShift( 0.0 );
    }
    else if ( !getUpdateStageNr() )
    {
	updatestageinfo_.oldtrcrgstart_ = trcdisplayinfo_.rg_.start;
	updatestageinfo_.oldzrgstart_ = trcdisplayinfo_.zrg_.start;
    }
    else
	panelstrip_->freezeDisplay( false );	// thaw to refreeze

    panelstrip_->freezeDisplay( yn );
    SurveyObject::annotateNextUpdateStage( yn );
}


void Seis2DDisplay::updateLineNamePos()
{
    const int trcidx =
		trcdisplayinfo_.alltrcnrs_.indexOf( trcdisplayinfo_.rg_.start );
    if ( trcidx < 0 )
	return;

    Coord3 pos( trcdisplayinfo_.alltrcpos_[trcidx],trcdisplayinfo_.zrg_.start );
    linename_->text()->setPosition( pos );
}


SurveyObject* Seis2DDisplay::duplicate( TaskRunner* taskr ) const
{
    Seis2DDisplay* s2dd = new Seis2DDisplay;
    s2dd->setGeometry( geometry_ );
    s2dd->setZRange( trcdisplayinfo_.zrg_ );
    s2dd->setTraceNrRange( getTraceNrRange() );
    s2dd->setResolution( getResolution(), taskr );
    s2dd->setProbe( const_cast<Probe*> (probe_.ptr()) );
    s2dd->setZAxisTransform( datatransform_, taskr );
    s2dd->showPanel( panelstrip_->isOn() );

    while ( nrAttribs() > s2dd->nrAttribs() )
	s2dd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	const Attrib::SelSpecList* selspecs = getSelSpecs( idx );
	if ( selspecs ) s2dd->setSelSpecs( idx, *selspecs );
	s2dd->setDataPackID( idx, getDataPackID(idx), taskr );
	s2dd->setColTabMapper( idx, getColTabMapper(idx), taskr );
	const ColTab::Sequence& colseq = getColTabSequence( idx );
	s2dd->setColTabSequence( idx, colseq, taskr );
    }

    return s2dd;
}


float Seis2DDisplay::calcDist( const Coord3& pos ) const
{
    Coord3 xytpos;
    mVisTrans::transformBack( scene_ ? scene_->getUTM2DisplayTransform() : 0,
			      pos, xytpos );

    int trcidx; float minsqdist;
    getNearestTrace( xytpos, trcidx, minsqdist );
    if ( minsqdist<0 || mIsUdf(minsqdist) )
	return mUdf(float);

    StepInterval<float> zrg = getZRange( true );
    float zdif = 0;
    if ( !zrg.includes(xytpos.z_,false) )
    {
	zdif = (float) mMIN(fabs(xytpos.z_-zrg.start),fabs(xytpos.z_-zrg.stop));
	const float zscale = scene_
	    ? scene_->getZScale() * scene_->getFixedZStretch()
	    : SI().zScale();
	zdif *= zscale;
    }

    return Math::Sqrt( minsqdist + zdif*zdif );
}


void Seis2DDisplay::setDisplayTransformation( const mVisTrans* tf )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = tf;
    transformation_->ref();

    polyline_->setDisplayTransformation( transformation_ );
    panelstrip_->setDisplayTransformation( transformation_ );
    linename_->setDisplayTransformation( transformation_ );
}


const mVisTrans* Seis2DDisplay::getDisplayTransformation() const
{
    return transformation_;
}


void Seis2DDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );
    pixeldensity_ = dpi;

    if ( linename_ )
	linename_->text()->updateFontSize( dpi );
}


void Seis2DDisplay::showLineName( bool yn )
{
    if ( !linename_ )
	return;

    linename_->turnOn( yn );
    updateLineNamePos();
}


bool Seis2DDisplay::isLineNameShown() const
{ return linename_->isOn(); }


int Seis2DDisplay::nrResolutions() const
{ return 3; }


void Seis2DDisplay::setResolution( int res, TaskRunner* taskr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, taskr );

    updatePanelStripPath();
    updatePanelStripZRange();
}


TrcKeyZSampling Seis2DDisplay::getTrcKeyZSampling( bool displayspace,
						   int attrib ) const
{
    TrcKeyZSampling tkzs;
    tkzs.hsamp_.start_.inl() = tkzs.hsamp_.stop_.inl() = getGeomID().lineNr();
    tkzs.hsamp_.start_.crl() = getTraceNrRange().start;
    tkzs.hsamp_.stop_.crl() = getTraceNrRange().stop;
    tkzs.hsamp_.step_.inl() = tkzs.hsamp_.step_.crl() = 1;
    tkzs.hsamp_.setIs2D();
    tkzs.zsamp_.setFrom( getZRange(displayspace,attrib) );
    return tkzs;
}


SurveyObject::AttribFormat Seis2DDisplay::getAttributeFormat( int ) const
{
    return SurveyObject::Cube;
}


void Seis2DDisplay::addCache()
{
    datapacks_ += 0;
    transformedpacks_ += 0;
}


void Seis2DDisplay::removeCache( int attrib )
{
    datapacks_.removeSingle( attrib );
    transformedpacks_.removeSingle( attrib );
}


void Seis2DDisplay::swapCache( int a0, int a1 )
{
    datapacks_.swap( a0, a1 );
    transformedpacks_.swap( a0, a1 );
}


void Seis2DDisplay::emptyCache( int attrib )
{
    datapacks_.replace( attrib, 0 );
    transformedpacks_.replace( attrib, 0 );

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedVSData( attrib, 0, 0, OD::UsePtr,
				  SilentTaskRunnerProvider() );
}


bool Seis2DDisplay::hasCache( int attrib ) const
{
    return datapacks_[attrib];
}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo& evinfo,
				     IOPar& par ) const
{
    par.setEmpty();
    SurveyObject::getMousePosInfo( evinfo, par );
    par.set( sKey::XCoord(), evinfo.worldpickedpos.x_ );
    par.set( sKey::YCoord(), evinfo.worldpickedpos.y_ );
    par.set( sKey::LineKey(), name() );

    int dataidx = -1;
    float minsqdist;
    mDynamicCastGet(const Line2DProbe*,l2dprobe,probe_.ptr());
    if ( getNearestTrace(evinfo.worldpickedpos,dataidx,minsqdist) && l2dprobe )
	par.set( sKey::TraceKey(),
		TrcKey(l2dprobe->geomID(),geometry_.positions()[dataidx].nr_) );
}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo&,
				     Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    getObjectInfo( info );
    getValueString( pos, val );

    int dataidx = -1;
    float mindist;
    if ( getNearestTrace(pos,dataidx,mindist) )
	info.add( " [" ).add( geometry_.positions()[dataidx].nr_ ).add( "]" );
}


void Seis2DDisplay::getObjectInfo( BufferString& info ) const
{
    info = "Line: "; info.add( getLineName() );
}


bool Seis2DDisplay::getCacheValue( int attrib, int version,
				    const Coord3& pos, float& res ) const
{
    mDynamicCastGet(const Line2DProbe*,l2dprobe,probe_.ptr());
    if ( !l2dprobe )
	return false;

    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp || regsdp->isEmpty() )
	return false;

    int traceidx = -1;
    float mindist;
    if ( !getNearestTrace(pos, traceidx, mindist) )
	return false;

    const int trcnr = geometry_.positions()[traceidx].nr_;
    const TrcKey trckey( l2dprobe->geomID(), trcnr );
    const int trcidx = regsdp->getNearestGlobalIdx( trckey );
    const int sampidx = regsdp->zRange().nearestIndex( pos.z_ );
    const Array3DImpl<float>& array = regsdp->data( version );
    if ( !array.info().validPos(0,trcidx,sampidx) )
	return false;

    res = array.get( 0, trcidx, sampidx );
    return true;
}


int Seis2DDisplay::getNearestTraceNr( const Coord3& pos ) const
{
    int trcidx = -1;
    float minsqdist;
    if ( getNearestTrace(pos,trcidx,minsqdist) )
	return geometry_.positions()[trcidx].nr_;

    return mUdf(int);
}


Coord3 Seis2DDisplay::getNearestSubPos( const Coord3& pos,
					bool usemaxrange ) const
{
    int trcnr1st, trcnr2nd;
    float frac;
    if ( getNearestSegment(pos, usemaxrange, trcnr1st, trcnr2nd, frac) < 0.0 )
	return Coord3::udf();

    const Coord subpos = getCoord(trcnr1st)*(1-frac) + getCoord(trcnr2nd)*frac;
    const Interval<float> zrg = usemaxrange ? getMaxZRange(false) :
					      getZRange(false);
    return Coord3( subpos, zrg.limitValue(pos.z_) );
}


float Seis2DDisplay::getNearestSegment( const Coord3& pos, bool usemaxrange,
					int& trcnr1st, int& trcnr2nd,
					float& frac ) const
{
    float mindist2 = MAXFLOAT;
    Interval<int> trcrg = getTraceNrRange();
    if ( usemaxrange ) trcrg = getMaxTraceNrRange();

    const TypeSet<PosInfo::Line2DPos>& posns = geometry_.positions();
    for ( int aidx=0; aidx<posns.size()-1; aidx++ )
    {
	const Coord posa = posns[aidx].coord_;
	if ( !posa.isDefined() || !trcrg.includes(posns[aidx].nr_,false) )
	    continue;

	Coord posb = Coord::udf();
	int bidx = aidx;

	while ( !posb.isDefined() && bidx<posns.size()-1 &&
		trcrg.includes(posns[bidx+1].nr_,false) )
	{
	    bidx++;
	    posb = posns[bidx].coord_;
	}

	if ( !posb.isDefined() )
	{
	    bidx = aidx;
	    posb = posa;
	}

	const float dist2a = (float) posa.sqDistTo( pos.getXY() );
	const float dist2b = (float) posb.sqDistTo( pos.getXY() );
	const float dist2c = (float) posa.sqDistTo( posb );

	if ( dist2b >= dist2a+dist2c )
	{
	    if ( mindist2 > dist2a )
	    {
		mindist2 = dist2a;
		trcnr1st = posns[aidx].nr_;
		trcnr2nd = posns[bidx].nr_;
		frac = 0.0;
	    }
	    continue;
	}

	if ( dist2a >= dist2b+dist2c )
	{
	    if ( mindist2 > dist2b )
	    {
		mindist2 = dist2b;
		trcnr1st = posns[aidx].nr_;
		trcnr2nd = posns[bidx].nr_;
		frac = 1.0;
	    }
	    continue;
	}

	const float dista = Math::Sqrt( dist2a );
	const float distb = Math::Sqrt( dist2b );
	const float distc = Math::Sqrt( dist2c );
	const float sp = (dista + distb + distc) / 2;
	const float height2 = 4*sp*(sp-dista)*(sp-distb)*(sp-distc) / dist2c;

	if ( mindist2 > height2 )
	{
	    mindist2 = height2;
	    trcnr1st = posns[aidx].nr_;
	    trcnr2nd = posns[bidx].nr_;
	    frac = Math::Sqrt( dist2a - height2 ) / distc;
	}
    }
    return mindist2!=MAXFLOAT ? Math::Sqrt(mindist2) : -1.0f;
}


void Seis2DDisplay::snapToTracePos( Coord3& pos ) const
{
    int trcidx = -1;
    float minsqdist;
    getNearestTrace( pos, trcidx, minsqdist );

    if ( trcidx<0 ) return;

    pos.setXY( geometry_.positions()[trcidx].coord_ );
}


bool Seis2DDisplay::getNearestTrace( const Coord3& pos,
				     int& trcidx, float& minsqdist ) const
{
    if ( geometry_.isEmpty() ) return false;

    const int nidx = geometry_.nearestIdx( pos.getXY(), trcdisplayinfo_.rg_ );
    if ( nidx >= 0 )
	minsqdist =
	    (float) geometry_.positions()[nidx].coord_.sqDistTo(pos.getXY());

    trcidx = nidx;
    return trcidx >= 0;
}


Coord Seis2DDisplay::getCoord( int trcnr ) const
{
    const int sz = geometry_.positions().size();
    if ( !sz )
	return Coord::udf();
    if ( prevtrcidx_ >= sz )
	prevtrcidx_ = sz-1;

    const int prevnr = geometry_.positions()[prevtrcidx_].nr_;
    int dir = (geometry_.positions()[0].nr_-prevnr)*(trcnr-prevnr)<0 ? 1 : -1;

    for ( int cnt=0; cnt<=1; cnt++ )
    {
	for ( int idx=prevtrcidx_+cnt*dir; idx>=0 && idx<sz; idx+=dir )
	{
	    if ( geometry_.positions()[idx].nr_ == trcnr )
	    {
		prevtrcidx_ = idx;
		return geometry_.positions()[idx].coord_;
	    }
	}
	dir = -dir;
    }
    return Coord::udf();
}


Coord Seis2DDisplay::getNormal( int trcnr ) const
{
    return geometry_.getNormal( trcnr );
}


bool Seis2DDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* taskr )
{
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( voiidx_!=-1 )
	    datatransform_->removeVolumeOfInterest(voiidx_);
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,Seis2DDisplay,dataTransformCB) );
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    voiidx_ = -1;

    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,Seis2DDisplay,dataTransformCB) );
    }

    return true;
}


const ZAxisTransform* Seis2DDisplay::getZAxisTransform() const
{ return datatransform_; }


void Seis2DDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	createTransformedDataPack( idx, 0 );
	updateChannels( idx, 0 );
    }
}


void Seis2DDisplay::updateRanges( bool updatetrc, bool updatez )
{
    // TODO: handle update trcrg
    if ( updatez && datatransform_ )
	setZRange( datatransform_->getZInterval(false) );
}


void Seis2DDisplay::clearTexture( int attribnr )
{
    channels_->setNrVersions( attribnr, 1 );
    channels_->setUnMappedVSData( attribnr, 0, 0, OD::UsePtr,
				  SilentTaskRunnerProvider() );
    channels_->turnOn( false );

    Attrib::SelSpec as;
    as.set2D(true);
    setSelSpec( attribnr, as );
}


void Seis2DDisplay::setAnnotColor( Color col )
{
    linename_->getMaterial()->setColor( col );
}


Color Seis2DDisplay::getAnnotColor() const
{
    return linename_->getMaterial()->getColor();
}


Seis2DDisplay* Seis2DDisplay::getSeis2DDisplay( const DBKey& lineset,
						const char* linenmptr )
{
    FixedString linenm = linenmptr;
    TypeSet<int> ids;
    visBase::DM().getIDs( typeid(visSurvey::Seis2DDisplay), ids );

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if (s2dd && !linenm.isEmpty() && linenm==s2dd->getLineName() )
	    return s2dd;
    }

    return 0;
}


Seis2DDisplay* Seis2DDisplay::getSeis2DDisplay( Pos::GeomID geomid )
{
    TypeSet<int> ids;
    visBase::DM().getIDs( typeid(visSurvey::Seis2DDisplay), ids );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if (s2dd && s2dd->getGeomID()==geomid )
	    return s2dd;
    }

    return 0;
}


Coord3 Seis2DDisplay::projectOnNearestPanel( const Coord3& pos,
					     int* nearestpanelidxptr )
{
    float mindist2 = MAXFLOAT;
    int nearestpanelidx = -1;
    Coord projpos = Coord::udf();

    const TraceDisplayInfo& tdi = trcdisplayinfo_;
    for ( int idx=0; idx<tdi.alljoints_.size()-1; idx++ )
    {
	const Coord posa = tdi.alltrcpos_[tdi.alljoints_[idx]];
	const Coord posb = tdi.alltrcpos_[tdi.alljoints_[idx+1]];

	const float dist2a = (float) posa.sqDistTo( pos.getXY() );
	const float dist2b = (float) posb.sqDistTo( pos.getXY() );
	const float dist2c = (float) posa.sqDistTo( posb );

	if ( dist2b >= dist2a+dist2c )
	{
	    if ( mindist2 > dist2a )
	    {
		mindist2 = dist2a;
		nearestpanelidx = idx;
		projpos = posa;
	    }
	    continue;
	}

	if ( dist2a >= dist2b+dist2c )
	{
	    if ( mindist2 > dist2b )
	    {
		mindist2 = dist2b;
		nearestpanelidx = idx;
		projpos = posb;
	    }
	    continue;
	}

	const float dista = Math::Sqrt( dist2a );
	const float distb = Math::Sqrt( dist2b );
	const float distc = Math::Sqrt( dist2c );
	const float sp = (dista + distb + distc) / 2;
	const float height2 = 4*sp*(sp-dista)*(sp-distb)*(sp-distc) / dist2c;

	if ( mindist2 > height2 )
	{
	    mindist2 = height2;
	    nearestpanelidx = idx;
	    const float frac = Math::Sqrt( dist2a - height2 ) / distc;
	    projpos = posa*(1.0-frac) + posb*frac;
	}
    }

    if ( nearestpanelidxptr )
	*nearestpanelidxptr = nearestpanelidx;

    return Coord3( projpos, pos.z_ );
}


void Seis2DDisplay::getLineSegmentProjection( const Coord3 pos1,
					      const Coord3 pos2,
					      TypeSet<Coord3>& projcoords )
{
    const TraceDisplayInfo& tdi = trcdisplayinfo_;
    int panelidx1, panelidx2;
    const Coord3 projpos1 = projectOnNearestPanel( pos1, &panelidx1 );
    const Coord3 projpos2 = projectOnNearestPanel( pos2, &panelidx2 );

    projcoords.erase();
    if ( !projpos1.isDefined() || !projpos2.isDefined() )
	return;

    projcoords += projpos1;
    TypeSet<float> arclen;
    arclen += 0.0;

    for ( int cnt=abs(panelidx2-panelidx1); cnt>=0; cnt-- )
    {
	const int idx = panelidx2 + (panelidx1<panelidx2 ? 1-cnt : cnt );
	const Coord pos = cnt
	    ? tdi.alltrcpos_[tdi.alljoints_[idx]]
	    : Coord(projpos2.getXY());

	const float dist = pos.distTo<float>(projcoords.last().getXY());
	if ( dist > mDefEps )
	{
	    projcoords += Coord3( pos, 0.0 );
	    arclen += arclen.last() + dist;
	}
    }

    for ( int idx=0; idx<projcoords.size(); idx++ )
    {
	const float totalarclen = arclen.last();
	const float frac = totalarclen ? arclen[idx]/totalarclen : 0.5f;
	projcoords[idx].z_ = projpos1.z_*(1.0-frac) + projpos2.z_*frac;
    }
}


void Seis2DDisplay::fillPar( IOPar& par ) const
{
    mDynamicCastGet(const Line2DProbe*,l2dprobe,probe_.ptr());
    if ( !l2dprobe )
	return;

    visSurvey::MultiTextureSurveyObject::fillPar( par );

    par.set( "GeomID", l2dprobe->geomID() );
    par.setYN( sKeyShowLineName(), isLineNameShown() );
    par.setYN( sKeyShowPanel(), isPanelShown() );
    par.setYN( sKeyShowPolyLine(), isPolyLineShown() );
    if ( !trcdisplayinfo_.alltrcnrs_.isEmpty() )
	par.set( sKeyTrcNrRange(), trcdisplayinfo_.rg_ );

    par.set( sKeyZRange(), trcdisplayinfo_.zrg_ );
}


bool Seis2DDisplay::usePar( const IOPar& par )
{
    if ( !MultiTextureSurveyObject::usePar( par ) )
	return false;

    Interval<int> trcnrrg;
    if ( par.get( sKeyTrcNrRange(), trcnrrg ) )
	trcdisplayinfo_.rg_ = trcnrrg;

    bool doshow = false;
    par.getYN( sKeyShowLineName(), doshow );
    showLineName( doshow );

    if ( par.getYN(sKeyShowPanel(),doshow) )
	showPanel( doshow );

    if ( par.getYN(sKeyShowPolyLine(),doshow) )
	showPolyLine( doshow );

    par.get( sKeyZRange(), trcdisplayinfo_.zrg_ );

    /* TODO PrIMPL
    Pos::GeomID geomid;
    if ( par.get("GeomID",geomid) )
	setGeomID( geomid );*/

    return true;
}

} // namespace visSurvey
