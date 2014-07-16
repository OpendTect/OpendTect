/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2004
 ________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visseis2ddisplay.h"

#include "visdrawstyle.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistexturechannels.h"
#include "vistexturepanelstrip.h"

#include "array2dresample.h"
#include "arrayndslice.h"
#include "attribdataholder.h"
#include "attribdatapackzaxistransformer.h"
#include "bendpointfinder.h"
#include "mousecursor.h"
#include "zaxistransform.h"

//For parsing old pars
#include "attribsel.h"

#define mMaxImageSize 300000000	// 32767

using namespace Attrib;

namespace visSurvey
{

const char* Seis2DDisplay::sKeyLineSetID()	{ return "LineSet ID"; }
const char* Seis2DDisplay::sKeyTrcNrRange()	{ return "Trc Nr Range"; }
const char* Seis2DDisplay::sKeyZRange()		{ return "Z Range"; }
const char* Seis2DDisplay::sKeyShowLineName()	{ return "Show linename"; }
const char* Seis2DDisplay::sKeyTextureID()	{ return "Texture ID"; }
const char* Seis2DDisplay::sKeyShowPanel()	{ return "Show panel"; }
const char* Seis2DDisplay::sKeyShowPolyLine()	{ return "Show polyline"; }

Seis2DDisplay::Seis2DDisplay()
    : transformation_(0)
    , geometry_(*new PosInfo::Line2DData)
    , panelstrip_(visBase::TexturePanelStrip::create())
    , polyline_(visBase::PolyLine::create())
    , linename_(visBase::Text2::create())
    , geomchanged_(this)
    , maxtrcnrrg_(0,mUdf(int),1)
    , datatransform_(0)
    , voiidx_(-1)
    , prevtrcidx_(0)
    , pixeldensity_( getDefaultPixelDensity() )
{
    geometry_.setZRange( StepInterval<float>(mUdf(float),mUdf(float),1) );
    cache_.allowNull();

    polyline_->ref();
    polyline_->setMaterial( new visBase::Material );
    addChild( polyline_->osgNode() );

    polylineds_ = polyline_->addNodeState( new visBase::DrawStyle );
    polylineds_->setLineStyle( LineStyle(LineStyle::Solid,3,Color::White()) );
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
    deepUnRef( cache_ );

    DataPackMgr& dpm = DPM( DataPackMgr::FlatID() );
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpm.release( datapackids_[idx] );

    for ( int idx=0; idx<dispdatapackids_.size(); idx++ )
    {
	const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[idx];
	for ( int idy=dpids.size()-1; idy>=0; idy-- )
	    dpm.release( dpids[idy] );
    }

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


const LineStyle* Seis2DDisplay::lineStyle() const
{ return &polylineds_->lineStyle(); }


void Seis2DDisplay::setLineStyle( const LineStyle& ls )
{
    polylineds_->setLineStyle( ls );
    requestSingleRedraw();
}


void Seis2DDisplay::showPanel( bool yn )
{ panelstrip_->turnOn( yn ); }

void Seis2DDisplay::showPolyLine( bool yn )
{ polyline_->turnOn( yn ); }

bool Seis2DDisplay::isPanelShown() const
{ return panelstrip_->isOn(); }

bool Seis2DDisplay::isPolyLineShown() const
{ return polyline_->isOn(); }


void Seis2DDisplay::setGeomID( Pos::GeomID geomid )
{
    geomid_ = geomid;
    BufferString lnm = Survey::GM().getName( geomid_ );
    if ( lnm.isEmpty() )
	lnm = geomid;

    setName( lnm );
    linename_->text()->setText( lnm );

    if ( scene_ )
    {
	setAnnotColor( scene_->getAnnotColor() );
	linename_->text()->setFontData( scene_->getAnnotFont(),
					getPixelDensity() );
    }
}


const char* Seis2DDisplay::getLineName() const
{
    if ( !Survey::GM().getName(geomid_) )
	return name().buf();

    return Survey::GM().getName(geomid_);
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
    const bool hasdata = !cache_.isEmpty() && cache_[0];
    if ( hasdata && trcdisplayinfo_.zrg_.isEqual(zrg,mDefEps) )
	return;

    trcdisplayinfo_.zrg_.setFrom( zrg );
    if ( trcdisplayinfo_.zrg_.nrSteps()+1 > mMaxImageSize )
    {
	trcdisplayinfo_.zrg_.stop = trcdisplayinfo_.zrg_.start +
				    (mMaxImageSize-1)*trcdisplayinfo_.zrg_.step;
    }

    updatePanelStripZRange();
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

void Seis2DDisplay::setTraceNrRange( const Interval<int>& trcrg )
{
    trcdisplayinfo_.rg_.start = -1;
    trcdisplayinfo_.rg_.stop = -1;

    if ( maxtrcnrrg_.isRev() )
	mRetErrGeo;

    const Interval<int> rg( maxtrcnrrg_.limitValue(trcrg.start),
			    maxtrcnrrg_.limitValue(trcrg.stop) );

    if ( !rg.width() )
	return;

    for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs_.size(); idx++ )
    {
	if ( trcdisplayinfo_.alltrcnrs_[idx]>=rg.start )
	{
	    trcdisplayinfo_.rg_.start = idx;
	    break;
	}
    }

    for ( int idx=mMIN(trcdisplayinfo_.alltrcnrs_.size(),mMaxImageSize)-1;
	  idx>=0; idx-- )
    {
	if ( trcdisplayinfo_.alltrcnrs_[idx]<=rg.stop )
	{
	    trcdisplayinfo_.rg_.stop = idx;
	    break;
	}
    }

    if ( trcdisplayinfo_.rg_.stop<0 || trcdisplayinfo_.rg_.start<0 )
	mRetErrGeo;

    trcdisplayinfo_.size_ = trcdisplayinfo_.rg_.width(false) + 1;
    updatePanelStripPath();
}


Interval<int> Seis2DDisplay::getTraceNrRange() const
{
    if ( !trcdisplayinfo_.alltrcnrs_.validIdx( trcdisplayinfo_.rg_.start ) ||
	 !trcdisplayinfo_.alltrcnrs_.validIdx( trcdisplayinfo_.rg_.stop ) )
	return Interval<int>( mUdf(int), mUdf(int) );

    return Interval<int>( trcdisplayinfo_.alltrcnrs_[trcdisplayinfo_.rg_.start],
		          trcdisplayinfo_.alltrcnrs_[trcdisplayinfo_.rg_.stop]);
}


const StepInterval<int>& Seis2DDisplay::getMaxTraceNrRange() const
{ return maxtrcnrrg_; }


bool Seis2DDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				   TaskRunner* tr )
{
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const Flat2DDHDataPack*,dp2d,datapack);
    if ( !dp2d )
    {
	dpman.release( dpid );
	return false;
    }

    DataPack::ID oldid = datapackids_[attrib];
    datapackids_[attrib] = dpid;
    dpman.release( oldid );

    createDisplayDataPacks( attrib );
    setTraceData( attrib, *dp2d->dataarray(), tr );
    return true;
}


void Seis2DDisplay::setDisplayDataPackIDs( int attrib,
				const TypeSet<DataPack::ID>& newdpids )
{
    TypeSet<DataPack::ID>& dpids = *dispdatapackids_[attrib];
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idx] );

    dpids = newdpids;
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).obtain( dpids[idx] );
}


DataPack::ID Seis2DDisplay::getDataPackID( int attrib ) const
{
    return datapackids_[attrib];
}


DataPack::ID Seis2DDisplay::getDisplayedDataPackID( int attrib ) const
{
    if ( !dispdatapackids_.validIdx(attrib) ) return DataPack::cNoID();
    const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[attrib];
    const int curversion = channels_->currentVersion(attrib);
    return dpids.validIdx(curversion) ? dpids[curversion] : DataPack::cNoID();
}


void Seis2DDisplay::setTraceData( int attrib,
				  const Attrib::Data2DArray& dataset,
				  TaskRunner* tr )
{
    setData( attrib, dataset, tr );
    if ( cache_[attrib] ) cache_[attrib]->unRef();

    cache_.replace( attrib, &dataset );
    cache_[attrib]->ref();
}


const Attrib::Data2DArray* Seis2DDisplay::getCache( int attrib ) const
{ return cache_[attrib]; }


void Seis2DDisplay::setData( int attrib,
			     const Attrib::Data2DArray& data2dh,
			     TaskRunner* tr )
{
    if ( data2dh.isEmpty() )
    {
	channels_->setUnMappedVSData( attrib, 0, 0, OD::UsePtr, tr );
	channels_->turnOn( false );
	return;
    }

    const TypeSet<DataPack::ID> dpids = *dispdatapackids_[attrib];
    const int nrseries = dpids.size();
    channels_->setNrVersions( attrib, nrseries );

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    int sz0=mUdf(int), sz1=mUdf(int);
    for ( int seriesidx=0; seriesidx<nrseries; seriesidx++ )
    {
	DataPackRef<Flat2DDHDataPack> dp2ddh =
	    DPM(DataPackMgr::FlatID()).obtain( dpids[seriesidx] );
	Array2D<float>& data = dp2ddh->data();

	if ( !seriesidx )
	{
	    sz0 = 1 + (data.info().getSize(0)-1) * (resolution_+1);
	    sz1 = 1 + (data.info().getSize(1)-1) * (resolution_+1);

	    //If the size is too big to display, use low resolution only
	    if ( sz0 > mMaxImageSize && resolution_ > 0 )
		sz0 = data.info().getSize(0);

	    if ( sz1 > mMaxImageSize && resolution_ > 0 )
		sz1 = data.info().getSize(1);
	}

	ValueSeries<float>* stor = !resolution_ ? data.getStorage() : 0;
	bool ownsstor = false;

	//We are only interested in the global, permanent storage
	if ( stor && stor!=data2dh.dataset_->getStorage() )
	    stor = 0;

	if ( !stor )
	{
	    stor = new MultiArrayValueSeries<float,float>(sz0*sz1);
	    ownsstor = true;
	}

	if ( !stor || !stor->isOK() )
	{
	    channels_->turnOn( false );
	    pErrMsg("Insufficient memory; cannot display the 2D seismics.");
	    if ( ownsstor ) delete stor;
	    return;
	}

	if ( ownsstor )
	{
	    if ( resolution_==0 )
		data.getAll( *stor );
	    else
	    {
		// Copy all the data from data to an Array2DImpl and pass
		// this object to Array2DReSampler. This copy is done because
		// Array2DReSampler will access the input using the "get"
		// method. The get method of Array2DImpl is much faster than
		// that of Seis2DArray.
		Array2DImpl<float> sourcearr2d( data.info() );
		if ( !sourcearr2d.isOK() )
		{
		    channels_->turnOn( false );
		    pErrMsg(
			"Insufficient memory; cannot display the 2D seismics.");
		    return;
		}

		sourcearr2d.copyFrom( data );
		Array2DReSampler<float,float> resampler(
				sourcearr2d, *stor, sz0, sz1, true );
		resampler.setInterpolate( true );
		TaskRunner::execute( tr, resampler );
	    }
	}

	channels_->setSize( 1, sz0, sz1 );
	channels_->setUnMappedVSData(attrib, seriesidx, stor,
			ownsstor ? OD::TakeOverPtr : OD::UsePtr, tr);
    }

    channels_->turnOn( true );
}


void Seis2DDisplay::createDisplayDataPacks( int attrib )
{
    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    const DataPack::ID dpid = getDataPackID( attrib );
    ConstDataPackRef<Flat2DDHDataPack> dp2ddh = dpm.obtain( dpid );

    const Attrib::Data2DArray& data2dh = *dp2ddh->dataarray();
    Array2DSlice<float> slice2d( *data2dh.dataset_ );
    slice2d.setDimMap( 0, 1 );
    slice2d.setDimMap( 1, 2 );

    TypeSet<DataPack::ID> dpids;
    const int nrseries = data2dh.dataset_->info().getSize( 0 );
    for ( int seriesidx=0; seriesidx<nrseries; seriesidx++ )
    {
	slice2d.setPos( 0, seriesidx );
	slice2d.init();

	Flat2DDHDataPack* dp2d = new Flat2DDHDataPack( dp2ddh->descID(),
			&slice2d, data2dh.cubesampling_, dp2ddh->trcKeys() );
	dp2d->setName( dp2ddh->name() );
	dpm.add( dp2d );
	dpids += dp2d->id();
    }

    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	CubeSampling cs;
	cs.hrg.start.inl() = cs.hrg.stop.inl() = 0;
	cs.hrg.start.crl() =
	    trcdisplayinfo_.alltrcnrs_[trcdisplayinfo_.rg_.start];
	cs.hrg.stop.crl() =
	    trcdisplayinfo_.alltrcnrs_[trcdisplayinfo_.rg_.stop];
	cs.hrg.step.crl() = 1;
	assign( cs.zrg, trcdisplayinfo_.zrg_ );
	// use survey step here?
	if ( voiidx_ < 0 )
	    voiidx_ = datatransform_->addVolumeOfInterest2D(
			getLineName(), cs, true );
	else
	    datatransform_->setVolumeOfInterest2D( voiidx_, getLineName(),
		    cs, true );
	datatransform_->loadDataIfMissing( voiidx_ );

	for ( int idx=0; idx<dpids.size(); idx++ )
	{
	    ConstDataPackRef<FlatDataPack> dp2d = dpm.obtain( dpids[idx] );
	    Attrib::FlatDataPackZAxisTransformer transformer( *datatransform_ );
	    transformer.setInput( dp2d.ptr() );
	    transformer.setOutput( dpids[idx] );
	    transformer.setInterpolate( textureInterpolationEnabled() );
	    transformer.execute();
	}
    }

    setDisplayDataPackIDs( attrib, dpids );
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
	if ( bends[idx]>tdi.rg_.start && tdi.alljoints_.last()<tdi.rg_.start )
	    tdi.alljoints_ += tdi.rg_.start;

	if ( bends[idx]>tdi.rg_.stop && tdi.alljoints_.last()<tdi.rg_.stop )
	    tdi.alljoints_ += tdi.rg_.stop;

	tdi.alljoints_ += bends[idx];
    }

    TypeSet<int> knots;

    for ( int idx=0; idx<tdi.alljoints_.size(); idx++ )
    {
	if ( !tdi.rg_.includes(tdi.alljoints_[idx],true) )
	    continue;

	if ( !knots.isEmpty() )
	{
	    for ( int posidx=tdi.alljoints_[idx-1]+1;
		  posidx<tdi.alljoints_[idx]; posidx++ )
	    {
		const Coord pos = tdi.alltrcpos_[posidx];
		const double d0 = pos.distTo( tdi.alltrcpos_[posidx-1] );
		const double d1 = pos.distTo( tdi.alltrcpos_[posidx+1] );
		if ( (d0+d1)>0.0 && fabs(d0-d1)/(d0+d1)>0.1 )
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
	mapping += mCast( float, (knots[idx]-tdi.rg_.start)*(resolution_+1) );

	polyline_->addPoint( Coord3(path[idx],SI().zRange(true).start) );
    }

    panelstrip_->setPath( path );
    panelstrip_->setPath2TextureMapping( mapping );

    if ( getUpdateStageNr() )
    {
	const float diff = updatestageinfo_.oldtrcrgstart_ - tdi.rg_.start;
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
	updatestageinfo_.oldtrcrgstart_ =mCast(float,trcdisplayinfo_.rg_.start);
	updatestageinfo_.oldzrgstart_ = mCast(float,trcdisplayinfo_.zrg_.start);
    }
    else
	panelstrip_->freezeDisplay( false );	// thaw to refreeze

    panelstrip_->freezeDisplay( yn );
    SurveyObject::annotateNextUpdateStage( yn );
}


void Seis2DDisplay::updateLineNamePos()
{
    const int trcidx = trcdisplayinfo_.rg_.start;
    if ( !trcdisplayinfo_.alltrcpos_.validIdx(trcidx) )
	return;

    Coord3 pos( trcdisplayinfo_.alltrcpos_[trcidx],trcdisplayinfo_.zrg_.start );
    linename_->text()->setPosition( pos );
}


SurveyObject* Seis2DDisplay::duplicate( TaskRunner* tr ) const
{
    Seis2DDisplay* s2dd = new Seis2DDisplay;
    s2dd->setGeometry( geometry_ );
    s2dd->setZRange( trcdisplayinfo_.zrg_ );
    s2dd->setTraceNrRange( getTraceNrRange() );
    s2dd->setResolution( getResolution(), tr );
    s2dd->setGeomID( geomid_ );

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( idx )
	    s2dd->addAttrib();

	s2dd->setSelSpec( idx, *getSelSpec(idx) );

	if ( getCache( idx ) )
	    s2dd->setData( idx, *getCache( idx ), tr );
    }

    return s2dd;
}


float Seis2DDisplay::calcDist( const Coord3& pos ) const
{
    Coord3 xytpos;
    mVisTrans::transformBack( scene_ ? scene_->getUTM2DisplayTransform() : 0,
			      pos, xytpos );

    int trcidx; float mindist;
    getNearestTrace( xytpos, trcidx, mindist );
    if ( mindist<0 || mIsUdf(mindist) )
	return mUdf(float);

    StepInterval<float> zrg = getZRange( true );
    float zdif = 0;
    if ( !zrg.includes(xytpos.z,false) )
    {
	zdif = (float) mMIN(fabs(xytpos.z-zrg.start), fabs(xytpos.z-zrg.stop));
	const float zscale = scene_
	    ? scene_->getZScale() * scene_->getFixedZStretch()
	    : SI().zScale();
	zdif *= zscale;
    }

    return Math::Sqrt( mindist + zdif*zdif );
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


void Seis2DDisplay::setResolution( int res, TaskRunner* tr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    updateDataFromCache( tr );
    updatePanelStripPath();
    updatePanelStripZRange();
}


SurveyObject::AttribFormat Seis2DDisplay::getAttributeFormat( int ) const
{ return SurveyObject::OtherFormat; }


void Seis2DDisplay::addCache()
{
    cache_ += 0;
    datapackids_ += -1;
    dispdatapackids_ += new TypeSet<DataPack::ID>;
}


void Seis2DDisplay::removeCache( int attrib )
{
    if ( cache_[attrib] ) cache_[attrib]->unRef();
    cache_.removeSingle( attrib );

    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_.removeSingle( attrib );

    const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[attrib];
    for ( int idy=dpids.size()-1; idy>=0; idy-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idy] );
    delete dispdatapackids_.removeSingle( attrib );
}


void Seis2DDisplay::swapCache( int a0, int a1 )
{
    cache_.swap( a0, a1 );
    datapackids_.swap( a0, a1 );
    dispdatapackids_.swap( a0, a1 );
}


void Seis2DDisplay::emptyCache( int attrib )
{
    if ( cache_[attrib] )
	cache_[attrib]->unRef();

    cache_.replace( attrib, 0 );
    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_[attrib] = DataPack::cNoID();

    const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[attrib];
    for ( int idy=dpids.size()-1; idy>=0; idy-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idy] );
    dispdatapackids_[attrib]->setAll( DataPack::cNoID() );

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedVSData( attrib, 0, 0, OD::UsePtr, 0 );
}


bool Seis2DDisplay::hasCache( int attrib ) const
{ return cache_[attrib] && cache_[attrib]->nrTraces(); }


void Seis2DDisplay::updateDataFromCache( TaskRunner* tr )
{
    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
	if ( cache_[idx] ) setData( idx, *cache_[idx], tr );
}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo& evinfo,
				     IOPar& par ) const
{
    par.setEmpty();
    par.set( sKey::XCoord(), evinfo.worldpickedpos.x );
    par.set( sKey::YCoord(), evinfo.worldpickedpos.y );
    par.set( sKey::LineKey(), name() );

    int dataidx = -1;
    float mindist;
    if ( getNearestTrace(evinfo.worldpickedpos,dataidx,mindist) )
	par.set( sKey::TraceNr(), geometry_.positions()[dataidx].nr_ );

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
    if ( attrib>=cache_.size() || !cache_[attrib] )
	return false;

    int trcidx = -1;
    float mindist;
    if ( !getNearestTrace(pos, trcidx, mindist) )
	return false;

    const int trcnr = geometry_.positions()[trcidx].nr_;
    for ( int idx=0; idx<cache_[attrib]->trcinfoset_.size(); idx++ )
    {
	if ( cache_[attrib]->trcinfoset_[idx]->nr != trcnr )
	    continue;

	const int sampidx =
	    cache_[attrib]->trcinfoset_[idx]->sampling.nearestIndex( pos.z );
	if ( cache_[attrib]->dataset_->info().validPos( version, idx, sampidx ))
	{
	    res = cache_[attrib]->dataset_->get( version, idx, sampidx );

	    //TODO: validSeriesIdx ??
	    return true;
	}
    }

    return false;
}


int Seis2DDisplay::getNearestTraceNr( const Coord3& pos ) const
{
    int trcidx = -1;
    float mindist;
    getNearestTrace( pos, trcidx, mindist );

    return  geometry_.positions()[trcidx].nr_;
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
    return Coord3( subpos, zrg.limitValue(pos.z) );
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

	const float dist2a = (float) posa.sqDistTo( pos );
	const float dist2b = (float) posb.sqDistTo( pos );
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
    float mindist;
    getNearestTrace( pos, trcidx, mindist );

    if ( trcidx<0 ) return;

    const Coord& crd = geometry_.positions()[trcidx].coord_;
    pos.x = crd.x; pos.y = crd.y;
}


bool Seis2DDisplay::getNearestTrace( const Coord3& pos,
				     int& trcidx, float& mindist ) const
{
    if ( geometry_.isEmpty() ) return false;

    const Interval<int> trcnrrg(
	    trcdisplayinfo_.alltrcnrs_[trcdisplayinfo_.rg_.start],
	    trcdisplayinfo_.alltrcnrs_[trcdisplayinfo_.rg_.stop]);
    const int nidx = geometry_.nearestIdx( pos, trcnrrg );
    mindist = (float) geometry_.positions()[nidx].coord_.distTo( pos );
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


bool Seis2DDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
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
    updateDataFromCache( 0 );
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
    channels_->setUnMappedVSData( attribnr, 0, 0, OD::UsePtr, 0 );
    channels_->turnOn( false );

    Attrib::SelSpec as;
    as.set2DFlag(true);
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


Seis2DDisplay* Seis2DDisplay::getSeis2DDisplay( const MultiID& lineset,
						const char* linenmptr )
{
    FixedString linenm = linenmptr;
    TypeSet<int> ids;
    visBase::DM().getIDs( typeid(visSurvey::Seis2DDisplay), ids );

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if (s2dd && lineset==s2dd->lineSetID() && linenm &&
	    linenm==s2dd->getLineName() )
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

	const float dist2a = (float) posa.sqDistTo( pos );
	const float dist2b = (float) posb.sqDistTo( pos );
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

    return Coord3( projpos, pos.z );
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
	const Coord pos = cnt ? tdi.alltrcpos_[tdi.alljoints_[idx]] : projpos2;

	const float dist = mCast( float, pos.distTo(projcoords.last()) );
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
	projcoords[idx].z = projpos1.z*(1.0-frac) + projpos2.z*frac;
    }
}


void Seis2DDisplay::fillPar( IOPar& par ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par );

    par.set( "GeomID", geomid_ );
    par.setYN( sKeyShowLineName(), isLineNameShown() );
    par.setYN( sKeyShowPanel(), isPanelShown() );
    par.setYN( sKeyShowPolyLine(), isPolyLineShown() );
    if ( !trcdisplayinfo_.alltrcnrs_.isEmpty() )
    {
	const Interval<int> trcnrrg(
			trcdisplayinfo_.alltrcnrs_[trcdisplayinfo_.rg_.start],
			trcdisplayinfo_.alltrcnrs_[trcdisplayinfo_.rg_.stop]);
	par.set( sKeyTrcNrRange(), trcnrrg );
    }

    par.set( sKeyZRange(), trcdisplayinfo_.zrg_ );
}


bool Seis2DDisplay::usePar( const IOPar& par )
{
    if ( !MultiTextureSurveyObject::usePar( par ) )
	return false;

    Interval<int> trcnrrg;
    if ( par.get( sKeyTrcNrRange(), trcnrrg ) )
    {
	for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs_.size(); idx++ )
	{
	    if ( trcdisplayinfo_.alltrcnrs_[idx]>=trcnrrg.start )
	    {
		trcdisplayinfo_.rg_.start = idx;
		break;
	    }
	}

	for ( int idx=trcdisplayinfo_.alltrcnrs_.size()-1; idx>=0; idx-- )
	{
	    if ( trcdisplayinfo_.alltrcnrs_[idx]<=trcnrrg.start )
	    {
		trcdisplayinfo_.rg_.stop = idx;
		break;
	    }
	}
    }

    bool doshow = false;
    par.getYN( sKeyShowLineName(), doshow );
    showLineName( doshow );

    if ( par.getYN(sKeyShowPanel(),doshow) )
	showPanel( doshow );

    if ( par.getYN(sKeyShowPolyLine(),doshow) )
	showPolyLine( doshow );

    par.get( sKeyZRange(), trcdisplayinfo_.zrg_ );

    Pos::GeomID geomid;
    if ( par.get("GeomID",geomid) )
	setGeomID( geomid );

    return true;
}

} // namespace visSurvey
