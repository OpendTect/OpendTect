/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2003
 ________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "visrandomtrackdisplay.h"

#include "array2dresample.h"
#include "arrayndimpl.h"
#include "attribdatapack.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "randomlinegeom.h"
#include "mousecursor.h"

#include "visevent.h"
#include "vishorizondisplay.h"
#include "vismaterial.h"
#include "vismarkerset.h"
#include "vispolyline.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdragger.h"
#include "vistexturechannels.h"
#include "vistexturepanelstrip.h"
#include "zaxistransform.h"


namespace visSurvey
{

const char* RandomTrackDisplay::sKeyTrack()	    { return "Random track"; }
const char* RandomTrackDisplay::sKeyNrKnots()	    { return "Nr. Knots"; }
const char* RandomTrackDisplay::sKeyKnotPrefix()    { return "Knot "; }
const char* RandomTrackDisplay::sKeyDepthInterval() { return "Depth Interval"; }
const char* RandomTrackDisplay::sKeyLockGeometry()  { return "Lock geometry"; }

RandomTrackDisplay::RandomTrackDisplay()
    : panelstrip_( visBase::TexturePanelStrip::create() )
    , dragger_( visBase::RandomTrackDragger::create())
    , polyline_( visBase::PolyLine::create())
    , polylinemode_(false)
    , knotmoving_(this)
    , moving_(this)
    , selknotidx_(-1)
    , ismanip_(false)
    , datatransform_(0)
    , lockgeometry_(false)
    , eventcatcher_(0)
    , depthrg_(SI().zRange(true))
    , voiidx_(-1)
    , markerset_( visBase::MarkerSet::create() )
{
    TypeSet<int> randomlines;
    visBase::DM().getIDs( typeid(*this), randomlines );
    int highestnamenr = 0;
    for ( int idx=0; idx<randomlines.size(); idx++ )
    {
	mDynamicCastGet( const RandomTrackDisplay*, rtd,
			 visBase::DM().getObject(randomlines[idx]) );
	if ( rtd == this )
	    continue;

	if ( rtd->nameNr()>highestnamenr )
	    highestnamenr = rtd->nameNr();
    }

    namenr_ = highestnamenr+1;
    BufferString nm( "Random Line "); nm += namenr_;
    setName( nm );

    material_->setColor( Color::White() );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.2 );


    dragger_->ref();
    addChild( dragger_->osgNode() );

    dragger_->motion.notify( mCB(this,visSurvey::RandomTrackDisplay,knotMoved));

    panelstrip_->ref();
    addChild( panelstrip_->osgNode() );
    panelstrip_->setTextureChannels( channels_ );
    panelstrip_->swapTextureAxes();

    polyline_->ref();
    addChild( polyline_->osgNode() );
    polyline_->setMaterial( new visBase::Material );
    polyline_->setLineStyle( LineStyle(LineStyle::Solid,1) );

    markerset_->ref();
    addChild( markerset_->osgNode() );
    markerset_->setMarkersSingleColor( polyline_->getMaterial()->getColor() );

    const StepInterval<float>& survinterval = SI().zRange(true);
    const StepInterval<float> inlrange(
			    mCast(float,SI().sampling(true).hrg.start.inl()),
			    mCast(float,SI().sampling(true).hrg.stop.inl()),
			    mCast(float,SI().inlStep()) );
    const StepInterval<float> crlrange(
			    mCast(float,SI().sampling(true).hrg.start.crl()),
			    mCast(float,SI().sampling(true).hrg.stop.crl()),
			    mCast(float,SI().crlStep()) );

    const BinID start( mNINT32(inlrange.center()), mNINT32(crlrange.start) );
    const BinID stop(start.inl(), mNINT32(crlrange.stop) );

    addKnot( start );
    addKnot( stop );

    setDepthInterval( Interval<float>( survinterval.start,
				       survinterval.stop ));
    dragger_->setLimits(
	    Coord3( inlrange.start, crlrange.start, survinterval.start ),
	    Coord3( inlrange.stop, crlrange.stop, survinterval.stop ),
	    Coord3( inlrange.step, crlrange.step, survinterval.step ) );

    init();
}


RandomTrackDisplay::~RandomTrackDisplay()
{
    setSceneEventCatcher( 0 );
    panelstrip_->unRef();
    dragger_->unRef();
    removeChild( polyline_->osgNode() );
    polyline_->unRef();

    removeChild( markerset_->osgNode() );
    markerset_->unRef();

    deepErase( cache_ );
    DataPackMgr& dpm = DPM( DataPackMgr::FlatID() );
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpm.release( datapackids_[idx] );

    for ( int idx=0; idx<dispdatapackids_.size(); idx++ )
    {
	const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[idx];
	for ( int idy=dpids.size()-1; idy>=0; idy-- )
	    dpm.release( dpids[idy] );
    }

    setZAxisTransform( 0, 0 );
}


void RandomTrackDisplay::setDisplayTransformation( const mVisTrans* t )
{
    panelstrip_->setDisplayTransformation( t );
    polyline_->setDisplayTransformation( t );
    markerset_->setDisplayTransformation( t );
    dragger_->setDisplayTransformation( t );
}


const mVisTrans* RandomTrackDisplay::getDisplayTransformation() const
{
    return panelstrip_->getDisplayTransformation();
}


/* OSG-TODO: Thorough testing of the (highly inscrutable) datatransform_
   dependency. Other than for Seis2DDisplay, the setting of the Coin-based
   SplitTexture was depending on it. Which is a strange difference! (JCG) */

float RandomTrackDisplay::appliedZRangeStep() const
{
    float step = datatransform_ ? datatransform_->getGoodZStep() : SI().zStep();
    if ( scene_ )
	step = scene_->getCubeSampling().zrg.step;

    return step;
}


CubeSampling RandomTrackDisplay::getCubeSampling( int attrib ) const
{
    CubeSampling cs( false );
    TypeSet<BinID> knots;
    getAllKnotPos( knots );
    for ( int idx=0; idx<knots.size(); idx++ )
	cs.hrg.include( knots[idx] );

    cs.zrg.setFrom( getDepthInterval() );
    cs.zrg.step = appliedZRangeStep();

    return cs;
}


void RandomTrackDisplay::setDepthInterval( const Interval<float>& intv )
{
    depthrg_ = intv;
    if ( datatransform_ )
	return;

    setPanelStripZRange( intv );
    dragger_->setDepthRange( intv );

    moving_.trigger();
}


Interval<float> RandomTrackDisplay::getDepthInterval() const
{
    return panelstrip_->getZRange();
}


void RandomTrackDisplay::setResolution( int res, TaskRunner* tr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;

    for ( int idx=0; idx<cache_.size(); idx++ )
    {
	if ( !cache_[idx] || !cache_[idx]->size() )
	    continue;

	setData( idx, *cache_[idx] );
    }

    updatePanelStripPath();
    setPanelStripZRange( panelstrip_->getZRange() );
}


Interval<float> RandomTrackDisplay::getDataTraceRange() const
{ return depthrg_; }


int RandomTrackDisplay::nrKnots() const
{ return knots_.size(); }


void RandomTrackDisplay::addKnot( const BinID& bid )
{
    const BinID sbid = snapPosition( bid );
    if ( checkPosition(sbid) )
    {
	knots_ += sbid;
	dragger_->insertKnot( knots_.size()-1, Coord(sbid.inl(),sbid.crl()) );

	if ( ismanip_ )
	    dragger_->showAdjacentPanels( knots_.size()-1, true );
	else
	    updatePanelStripPath();

	moving_.trigger();
    }
}


void RandomTrackDisplay::insertKnot( int knotidx, const BinID& bid )
{
    const BinID sbid = snapPosition( bid );
    if ( checkPosition(sbid) )
    {
	knots_.insert( knotidx, sbid );
	dragger_->insertKnot( knotidx, Coord(sbid.inl(),sbid.crl()) );

	if ( ismanip_ )
	    dragger_->showAdjacentPanels( knotidx, true );
	else
	{
	    for ( int idx=0; idx<nrAttribs(); idx++ )
	    {
		if ( cache_[idx] )
		    setData( idx, *cache_[idx] );
	    }
	    updatePanelStripPath();
	}

	moving_.trigger();
    }
}


BinID RandomTrackDisplay::getKnotPos( int knotidx ) const
{
    return knots_[knotidx];
}


BinID RandomTrackDisplay::getManipKnotPos( int knotidx ) const
{
    const Coord crd = dragger_->getKnot( knotidx );
    return BinID( mNINT32(crd.x), mNINT32(crd.y) );
}


void RandomTrackDisplay::getAllKnotPos( TypeSet<BinID>& knots ) const
{
    const int nrknots = nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	knots += getManipKnotPos( idx );
}


void RandomTrackDisplay::setKnotPos( int knotidx, const BinID& bid )
{ setKnotPos( knotidx, bid, true ); }


void RandomTrackDisplay::setKnotPos( int knotidx, const BinID& bid, bool check )
{
    const BinID sbid = snapPosition(bid);
    if ( !check || checkPosition(sbid) )
    {
	knots_[knotidx] = sbid;
	dragger_->setKnot( knotidx, Coord(sbid.inl(),sbid.crl()) );
	updatePanelStripPath();
	moving_.trigger();
    }
}


static bool decoincideKnots( const TypeSet<BinID>& knots,
			     TypeSet<BinID>& uniqueknots )
{
    uniqueknots.erase();
    if ( knots.isEmpty() )
	return false;
    uniqueknots += knots[0];

    for ( int idx=1; idx<knots.size(); idx++ )
    {
	const BinID prev = uniqueknots[uniqueknots.size()-1];
	const BinID biddif = prev - knots[idx];
	const int nrsteps = mMAX( abs(biddif.inl())/SI().inlStep(),
				  abs(biddif.crl())/SI().crlStep() );
	const Coord dest = SI().transform( knots[idx] );
	const Coord crddif = SI().transform(prev) - dest;
	for ( int step=0; step<nrsteps; step++ )
	{
	    const BinID newknot = SI().transform( dest+(crddif*step)/nrsteps );
	    if ( !uniqueknots.isPresent(newknot) )
	    {
		uniqueknots += newknot;
		break;
	    }
	}
    }
    return true;
}


bool RandomTrackDisplay::setKnotPositions( const TypeSet<BinID>& newbids )
{
    TypeSet<BinID> uniquebids;
   if ( !decoincideKnots( newbids, uniquebids ) ) return false;

    if ( uniquebids.size() < 2 )
	return false;
    while ( nrKnots() > uniquebids.size() )
	removeKnot( nrKnots()-1 );

    if ( uniquebids.size() > 50 ) // Workaround when having a lot of knots
    {				  // TODO: Make better fix
	while ( nrKnots()>0 )
	{
	    knots_.removeSingle( 0 );
	    dragger_->removeKnot( 0 );
	}

	for ( int idx=0; idx<uniquebids.size(); idx++ )
	{
	    const BinID sbid = snapPosition( uniquebids[idx] );
	    if ( checkPosition(sbid) )
	    {
		knots_ += sbid;
		dragger_->insertKnot( knots_.size()-1,
				      Coord(sbid.inl(), sbid.crl()) );
	    }
	}

	updatePanelStripPath();
	moving_.trigger();
	return true;
    }

    for ( int idx=0; idx<uniquebids.size(); idx++ )
    {
	const BinID bid = uniquebids[idx];

	if ( idx < nrKnots() )
	    setKnotPos( idx, bid, false );
	else
	    addKnot( bid );
    }

    return true;
}


void RandomTrackDisplay::removeKnot( int knotidx )
{
    if ( nrKnots()< 3 )
    {
	pErrMsg("Can't remove knot");
	return;
    }

    knots_.removeSingle(knotidx);
    dragger_->removeKnot( knotidx );
    updatePanelStripPath();
}


void RandomTrackDisplay::removeAllKnots()
{
    for ( int idx=0; idx<knots_.size(); idx++ )
	dragger_->removeKnot( idx );

    knots_.erase();
    updatePanelStripPath();
}


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids ) const
{ getDataTraceBids( bids, 0 ); }


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids,
					   TypeSet<int>* segments ) const
{
    const_cast<RandomTrackDisplay*>(this)->trcspath_.erase();
    TypeSet<BinID> knots;
    getAllKnotPos( knots );
    Geometry::RandomLine::getPathBids( knots, bids, true, segments );
    for ( int idx=0; idx<bids.size(); idx++ )
	const_cast<RandomTrackDisplay*>(this)->trcspath_.addIfNew( bids[idx] );
}


TypeSet<Coord> RandomTrackDisplay::getTrueCoords() const
{
    const int nrknots = nrKnots();
    TypeSet<Coord> coords;
    for ( int kidx=1; kidx<nrknots; kidx++ )
    {
	BinID start = getKnotPos(kidx-1);
	BinID stop = getKnotPos(kidx);
	const int nrinl = int(abs(stop.inl()-start.inl()) / SI().inlStep() + 1);
	const int nrcrl = int(abs(stop.crl()-start.crl()) / SI().crlStep() + 1);
	const int nrtraces = nrinl > nrcrl ? nrinl : nrcrl;
	const Coord startcoord = SI().transform( start );
	const Coord stopcoord = SI().transform( stop );
	const float delx = (float) ( stopcoord.x - startcoord.x ) / nrtraces;
	const float dely = (float) ( stopcoord.y - startcoord.y ) / nrtraces;

	for ( int idx=0; idx<nrtraces; idx++ )
	{
	    const float x = (float) ( startcoord.x + delx * idx );
	    const float y = (float) ( startcoord.y + dely * idx );
	    coords += Coord( x, y );
	}
    }
    return coords;
}


bool RandomTrackDisplay::setDataPackID( int attrib, DataPack::ID dpid,
			TaskRunner* tr )
{
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,datapack);
    if ( !dprdm )
    {
	dpman.release( dpid );
	SeisTrcBuf trcbuf( false );
	setTraceData( attrib, trcbuf, tr );
	return false;
    }

    DataPack::ID oldid = datapackids_[attrib];
    datapackids_[attrib] = dpid;
    dpman.release( oldid );

    createDisplayDataPacks( attrib );
    SeisTrcBuf tmpbuf( dprdm->seisBuf() );
    setTraceData( attrib, tmpbuf, tr );
    return true;
}


void RandomTrackDisplay::setDisplayDataPackIDs( int attrib,
				const TypeSet<DataPack::ID>& newdpids )
{
    TypeSet<DataPack::ID>& dpids = *dispdatapackids_[attrib];
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idx] );

    dpids = newdpids;
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).obtain( dpids[idx] );
}


DataPack::ID RandomTrackDisplay::getDataPackID( int attrib ) const
{
    return datapackids_[attrib];
}


DataPack::ID RandomTrackDisplay::getDisplayedDataPackID( int attrib ) const
{
    if ( !dispdatapackids_.validIdx(attrib) ) return DataPack::cNoID();
    const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[attrib];
    const int curversion = channels_->currentVersion(attrib);
    return dpids.validIdx(curversion) ? dpids[curversion] : DataPack::cNoID();
}


void RandomTrackDisplay::setTraceData( int attrib, SeisTrcBuf& trcbuf,
					TaskRunner* )
{
    setData( attrib, trcbuf );

    if ( !cache_[attrib] )
	cache_.replace( attrib, new SeisTrcBuf(false) );

    cache_[attrib]->deepErase();
    cache_[attrib]->stealTracesFrom( trcbuf );

    //TODO Find other means of reseting ismanip_
    //ismanip_ = false;
}


const ZAxisTransform* RandomTrackDisplay::getZAxisTransform() const
{ return datatransform_; }


bool RandomTrackDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* t )
{
    if ( datatransform_ )
    {
	if ( voiidx_!=-1 )
	    datatransform_->removeVolumeOfInterest(voiidx_);

	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,RandomTrackDisplay,dataTransformCB) );
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    voiidx_ = -1;
    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, true );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,RandomTrackDisplay,dataTransformCB) );
    }
    return true;
}


void RandomTrackDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int idx=0; idx<cache_.size(); idx++ )
    {
	if ( cache_[idx] )
	    setTraceData( idx, *cache_[idx], 0 );
    }
}


void RandomTrackDisplay::updateRanges(bool resetinlcrl, bool resetz )
{
    if ( resetz )
    {
	const Interval<float>& depthrg = datatransform_->getZInterval(false);
	setPanelStripZRange( depthrg );
	dragger_->setDepthRange( depthrg );

	moving_.trigger();
    }
}


void RandomTrackDisplay::setData( int attrib, const SeisTrcBuf& trcbuf )
{
    const int nrtrcs = trcbuf.size();
    if ( !nrtrcs )
    {
	channels_->setUnMappedData( attrib, 0, 0, OD::CopyPtr, 0 );
	channels_->turnOn( false );
	return;
    }

    const TypeSet<DataPack::ID> dpids = *dispdatapackids_[attrib];
    const int nrversions = dpids.size();
    channels_->setNrVersions( attrib, nrversions );

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    for ( int sidx=0; sidx<nrversions; sidx++ )
    {
	DataPackRef<Attrib::FlatRdmTrcsDataPack> dprdm =
		DPM(DataPackMgr::FlatID()).obtain( dpids[sidx] );
	const Array2D<float>& array = dprdm->data();
	const int sz0 = 1 + (array.info().getSize(0)-1) * (resolution_+1);
	const int sz1 = 1 + (array.info().getSize(1)-1) * (resolution_+1);
	channels_->setSize( 1, sz0, sz1 );

	if ( resolution_==0 )
	    channels_->setUnMappedData(attrib,sidx,array.getData(),
					OD::CopyPtr,0);
	else
	{
	    mDeclareAndTryAlloc( float*, arr, float[sz0*sz1] );
	    Array2DReSampler<float,float> resampler( array,arr,sz0,sz1,true );
	    resampler.setInterpolate( true );
	    resampler.execute();
	    channels_->setUnMappedData(attrib,sidx,arr,OD::TakeOverPtr,0);
	}
    }

    channels_->turnOn( true );
}


void RandomTrackDisplay::createDisplayDataPacks( int attrib )
{
    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    const DataPack::ID dpid = getDataPackID( attrib );
    ConstDataPackRef<Attrib::FlatRdmTrcsDataPack>rdmtrcsdp = dpm.obtain( dpid );

    TypeSet<BinID> path;
    getDataTraceBids( path );
    const int pathsz = path.size();
    StepInterval<float> zrg = getDataTraceRange();
    zrg.step = SI().zStep();

    const int nrsamp = zrg.nrSteps()+1;
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, array,
			 Array2DImpl<float>( pathsz, nrsamp ) );
    if ( !array->isOK() )
	return;

    TypeSet<DataPack::ID> dpids;
    const SeisTrcBuf& seisbuf = rdmtrcsdp->seisBuf();
    const int nrslices = seisbuf.get(0)->nrComponents();
    for ( int sidx=0; sidx<nrslices; sidx++ )
    {
	array->setAll( mUdf(float) );
	float* dataptr = array->getData();
	for ( int posidx=pathsz-1; posidx>=0; posidx-- )
	{
	    const BinID bid = path[posidx];
	    const int trcidx = seisbuf.find( bid, false );
	    if ( trcidx<0 )
		continue;

	    const SeisTrc* trc = seisbuf.get( trcidx );
	    if ( !trc || sidx>trc->nrComponents() )
		continue;

	    float* arrptr = dataptr + array->info().getOffset( posidx, 0 );
	    for ( int ids=0; ids<nrsamp; ids++ )
	    {
		const float ctime = zrg.start + ids*zrg.step;
		if ( trc->dataPresent(ctime) )
		    arrptr[ids] = trc->getValue( ctime, sidx );
	    }
	}

	SamplingData<float> sd( zrg.start, zrg.step );
	const Attrib::DescID descid = rdmtrcsdp->descID();
	Attrib::FlatRdmTrcsDataPack* dprdm =
	    new Attrib::FlatRdmTrcsDataPack( descid, array, sd, &path );
	dprdm->setName( rdmtrcsdp->name() );
	dpm.add( dprdm );
	dpids += dprdm->id();
    }

    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	CubeSampling cs( false );
	for ( int pi=0; pi<pathsz; pi++ )
	    cs.hrg.include( path[pi] );
	cs.zrg = panelstrip_->getZRange();
	cs.zrg.step = scene_ ? scene_->getCubeSampling().zrg.step
			     : datatransform_->getGoodZStep();

	if ( voiidx_<0 )
	    voiidx_ = datatransform_->addVolumeOfInterest( cs, true );
	else
	    datatransform_->setVolumeOfInterest( voiidx_, cs, true );
	datatransform_->loadDataIfMissing( voiidx_ );

	for ( int idx=0; idx<dpids.size(); idx++ )
	{
	    TypeSet<DataPack::ID> tdpids;
	    DataPackRef<FlatDataPack> dprdm = dpm.obtain( dpids[idx] );
	    Attrib::FlatDataPackZAxisTransformer transformer( *datatransform_ );
	    transformer.setInput( dprdm.ptr() );
	    transformer.setOutput( tdpids );
	    transformer.setInterpolate( textureInterpolationEnabled() );
	    transformer.execute();
	    dpids[idx] = tdpids.size() ? tdpids[0] : DataPack::cNoID();
	}
    }

    setDisplayDataPackIDs( attrib, dpids );
}


void RandomTrackDisplay::updatePanelStripPath()
{
    if ( knots_.size()<2 || getUpdateStageNr() )
	return;

    TypeSet<BinID> trcbids;
    getDataTraceBids( trcbids );

    TypeSet<Coord> pathcrds;
    TypeSet<float> mapping;
    pathcrds.setCapacity( knots_.size() );
    mapping.setCapacity( knots_.size() );

    int knotidx = 0;
    for ( int trcidx=0; trcidx<trcbids.size(); trcidx++ )
    {
	if ( trcbids[trcidx] == knots_[knotidx] )
	{
	    pathcrds += Coord( knots_[knotidx].inl(), knots_[knotidx].crl() );
	    mapping += mCast( float, trcidx*(resolution_+1) );
	    knotidx++;
	}
    }

    if ( mapping.size()!=knots_.size() ||
	 mNINT64(mapping.last())!=(trcbids.size()-1)*(resolution_+1) )
    {
	pErrMsg( "Unexpected state while texture mapping" );
    }

    panelstrip_->setPath( pathcrds );
    panelstrip_->setPath2TextureMapping( mapping );
}


void RandomTrackDisplay::setPanelStripZRange( const Interval<float>& rg )
{
    const StepInterval<float> zrg( rg.start, rg.stop, appliedZRangeStep() );
    panelstrip_->setZRange( zrg );
    const Interval<float> mapping(0,mCast(float,zrg.nrSteps()*(resolution_+1)));
    panelstrip_->setZRange2TextureMapping( mapping );

    if ( getUpdateStageNr() )
    {
	const float factor = (resolution_+1) / zrg.step;
	panelstrip_->setZTextureShift( (oldzrgstart_-zrg.start)*factor );
    }
}


void RandomTrackDisplay::annotateNextUpdateStage( bool yn )
{
    if ( !yn )
    {
	panelstrip_->setZTextureShift( 0.0 );
	SurveyObject::annotateNextUpdateStage( false );
	updatePanelStripPath();
	panelstrip_->freezeDisplay( false );

	for ( int idx=0; idx<nrKnots(); idx++ )
	    dragger_->showAdjacentPanels( idx, false );
    }
    else
    {
	if ( !getUpdateStageNr() )
	    oldzrgstart_ = getDepthInterval().start;
	else
	    panelstrip_->freezeDisplay( false );	// thaw to refreeze

	SurveyObject::annotateNextUpdateStage( true );
	panelstrip_->freezeDisplay( true );
    }
}


bool RandomTrackDisplay::canAddKnot( int knotnr ) const
{
    if ( lockgeometry_ ) return false;
    if ( knotnr<0 ) knotnr=0;
    if ( knotnr>nrKnots() ) knotnr=nrKnots();

    const BinID newpos = proposeNewPos(knotnr);
    return checkPosition(newpos);
}


void RandomTrackDisplay::addKnot( int knotnr )
{
    if ( knotnr<0 ) knotnr=0;
    if ( knotnr>nrKnots() ) knotnr=nrKnots();

    if ( !canAddKnot(knotnr) ) return;

    ismanip_ = true;
    const BinID newpos = proposeNewPos( knotnr );
    insertKnot( knotnr, newpos );
    if ( knotnr!=0 && knotnr!=nrKnots()-1 )
	dragger_->showAdjacentPanels( knotnr, false );
}


BinID RandomTrackDisplay::proposeNewPos(int knotnr ) const
{
    BinID res;
    if ( !knotnr )
	res = getKnotPos(0)-(getKnotPos(1)-getKnotPos(0));
    else if ( knotnr>=nrKnots() )
	res = getKnotPos(nrKnots()-1) +
	      (getKnotPos(nrKnots()-1)-getKnotPos(nrKnots()-2));
    else
    {
	res = getKnotPos(knotnr)+getKnotPos(knotnr-1);
	res.inl() /= 2;
	res.crl() /= 2;
    }

    res.inl() = mMIN( SI().inlRange(true).stop, res.inl() );
    res.inl() = mMAX( SI().inlRange(true).start, res.inl() );
    res.crl() = mMIN( SI().crlRange(true).stop, res.crl() );
    res.crl() = mMAX( SI().crlRange(true).start, res.crl() );

    SI().snap(res, BinID(0,0) );

    return res;
}


bool RandomTrackDisplay::isManipulated() const
{
    return ismanip_;
}


void RandomTrackDisplay::acceptManipulation()
{
    if ( !datatransform_ )
	setDepthInterval( dragger_->getDepthRange() );
    else
    {
	setPanelStripZRange( dragger_->getDepthRange() );
	moving_.trigger();
    }

    for ( int idx=0; idx<nrKnots(); idx++ )
    {
	const Coord crd = dragger_->getKnot(idx);
	setKnotPos( idx, BinID( mNINT32(crd.x), mNINT32(crd.y) ));
	if ( !getUpdateStageNr() )
	    dragger_->showAdjacentPanels( idx, false );
    }

    updatePanelStripPath();
    dragger_->showAllPanels( false );
    ismanip_ = false;
}


void RandomTrackDisplay::resetManipulation()
{
    if ( !datatransform_ )
	dragger_->setDepthRange( getDepthInterval() );

    for ( int idx=0; idx<nrKnots(); idx++ )
    {
	const BinID bid = getKnotPos(idx);
	dragger_->setKnot( idx, Coord(bid.inl(),bid.crl()) );
	dragger_->showAdjacentPanels( idx, false );
    }

    dragger_->showAllPanels( false );
    ismanip_ = false;
    dragger_->turnOn( false );
}


void RandomTrackDisplay::showManipulator( bool yn )
{
    if ( polylinemode_ ) return;

    if ( lockgeometry_ ) yn = false;
    dragger_->turnOn( yn );
}


bool RandomTrackDisplay::isManipulatorShown() const
{ return dragger_->isOn(); }


BufferString RandomTrackDisplay::getManipulationString() const
{
    BufferString str;
    int knotidx = getSelKnotIdx();
    if ( knotidx >= 0 )
    {
	BinID binid = getManipKnotPos( knotidx );
	str = "Node "; str += knotidx;
	str += " Inl/Crl: ";
	str += binid.inl(); str += "/"; str += binid.crl();
    }

    return str;
}


void RandomTrackDisplay::knotMoved( CallBacker* cb )
{
    ismanip_ = true;
    mCBCapsuleUnpack(int,sel,cb);
    selknotidx_ = sel;

    const Coord crd = dragger_->getKnot( sel );
    dragger_->showAdjacentPanels( sel,
		    getKnotPos(sel)!=BinID(mNINT32(crd.x),mNINT32(crd.y)) );
    dragger_->showAllPanels( getDepthInterval()!=dragger_->getDepthRange() );
    knotmoving_.trigger();
}


void RandomTrackDisplay::knotNrChanged( CallBacker* )
{
    ismanip_ = true;
    for ( int idx=0; idx<cache_.size(); idx++ )
    {
	if ( !cache_[idx] || !cache_[idx]->size() )
	    continue;

	setData( idx, *cache_[idx] );
    }
}


bool RandomTrackDisplay::checkPosition( const BinID& binid ) const
{
    const HorSampling& hs = SI().sampling(true).hrg;
    if ( !hs.includes(binid) )
	return false;

    BinID snapped( binid );
    SI().snap( snapped, BinID(0,0) );
    if ( snapped != binid )
	return false;

    for ( int idx=0; idx<nrKnots(); idx++ )
    {
	if ( getKnotPos(idx) == binid )
	    return false;
    }

    return true;
}


BinID RandomTrackDisplay::snapPosition( const BinID& binid_ ) const
{
    BinID binid( binid_ );
    const HorSampling& hs = SI().sampling(true).hrg;
    if ( binid.inl() < hs.start.inl() ) binid.inl() = hs.start.inl();
    if ( binid.inl() > hs.stop.inl() ) binid.inl() = hs.stop.inl();
    if ( binid.crl() < hs.start.crl() ) binid.crl() = hs.start.crl();
    if ( binid.crl() > hs.stop.crl() ) binid.crl() = hs.stop.crl();

    SI().snap( binid, BinID(0,0) );
    return binid;
}


SurveyObject::AttribFormat RandomTrackDisplay::getAttributeFormat( int ) const
{ return SurveyObject::Traces; }


#define mFindTrc(inladd,crladd) \
    if ( idx<0 ) \
    { \
	BinID bid( binid.inl() + step.inl() * (inladd),\
		   binid.crl() + step.crl() * (crladd) );\
	idx = bids.indexOf( bid ); \
    }
Coord3 RandomTrackDisplay::getNormal( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pos, xytpos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    TypeSet<BinID> bids;
    TypeSet<int> segments;
    getDataTraceBids( bids, &segments );
    int idx = bids.indexOf(binid);
    if ( idx==-1 )
    {
	const BinID step( SI().inlStep(), SI().crlStep() );
	mFindTrc(1,0) mFindTrc(-1,0) mFindTrc(0,1) mFindTrc(0,-1)
	if ( idx==-1 )
	{
	    mFindTrc(1,1) mFindTrc(-1,1) mFindTrc(1,-1) mFindTrc(-1,-1)
	}

	if ( idx<0 )
	    return Coord3::udf();
    }

    const TypeSet<Coord>& coords = panelstrip_->getPath();
    const Coord pos0 = coords[segments[idx]];
    const Coord pos1 = coords[segments[idx]+1];
    const BinID bid0( mNINT32(pos0.x), mNINT32(pos0.y));
    const BinID bid1( mNINT32(pos1.x), mNINT32(pos1.y));

    const Coord dir = SI().transform(bid0)-SI().transform(bid1);
    const float dist = (float) dir.abs();

    if ( dist<=mMIN(SI().inlDistance(),SI().crlDistance()) )
	return Coord3::udf();

    return Coord3( dir.y, -dir.x, 0 );
}

#undef mFindTrc


float RandomTrackDisplay::calcDist( const Coord3& pos ) const
{
    // TODO: Compared to calcDist(.) of 2d lines, this implementation can't
    // be right. I wonder whether this is actually used somewhere? (JCG)

    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pos, xytpos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    TypeSet<BinID> bids;
    getDataTraceBids( bids );
    if ( !bids.isPresent(binid) )
	return mUdf(float);

    float zdiff = 0;
    const Interval<float> intv = getDataTraceRange();
    if ( xytpos.z < intv.start )
	zdiff = (float) ( intv.start - xytpos.z );
    else if ( xytpos.z > intv.stop )
	zdiff = (float) ( xytpos.z - intv.stop );

    return zdiff;
}


void RandomTrackDisplay::lockGeometry( bool yn )
{
    lockgeometry_ = yn;
    if ( yn ) showManipulator( false );
}


bool RandomTrackDisplay::isGeometryLocked() const
{ return lockgeometry_; }


SurveyObject* RandomTrackDisplay::duplicate( TaskRunner* tr ) const
{
    RandomTrackDisplay* rtd = new RandomTrackDisplay;

    rtd->setDepthInterval( getDataTraceRange() );
    TypeSet<BinID> positions;
    for ( int idx=0; idx<nrKnots(); idx++ )
	positions += getKnotPos( idx );
    rtd->setKnotPositions( positions );

    rtd->lockGeometry( isGeometryLocked() );

    return rtd;
}


void RandomTrackDisplay::fillPar( IOPar& par ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par );

    const Interval<float> depthrg = getDataTraceRange();
    par.set( sKeyDepthInterval(), depthrg );

    const int nrknots = nrKnots();
    par.set( sKeyNrKnots(), nrknots );

    for ( int idx=0; idx<nrknots; idx++ )
    {
	BufferString key = sKeyKnotPrefix(); key += idx;
	par.set( key, getKnotPos(idx) );
    }

    par.set( sKey::Version(), 3 );
    par.setYN( sKeyLockGeometry(), lockgeometry_ );
}


bool RandomTrackDisplay::usePar( const IOPar& par )
{
    if ( !visSurvey::MultiTextureSurveyObject::usePar( par ) )
	return false;

    par.getYN( sKeyLockGeometry(), lockgeometry_ );

    Interval<float> intv;
    if ( par.get( sKeyDepthInterval(), intv ) )
	setDepthInterval( intv );

    int nrknots = 0;
    par.get( sKeyNrKnots(), nrknots );

    BufferString key; BinID pos;
    for ( int idx=0; idx<nrknots; idx++ )
    {
	key = sKeyKnotPrefix(); key += idx;
	par.get( key, pos );
	if ( idx < 2 )
	    setKnotPos( idx, pos );
	else
	    addKnot( pos );
    }

    return true;
}


void RandomTrackDisplay::getMousePosInfo( const visBase::EventInfo&,
					  Coord3& pos, BufferString& val,
					  BufferString& info ) const
{
    info = name();
    getValueString( pos, val );
}


#define mFindTrc(inladd,crladd) \
    if ( trcidx < 0 ) \
    { \
	bid.inl() = reqbid.inl() + step.inl() * (inladd); \
	bid.crl() = reqbid.crl() + step.crl() * (crladd); \
	trcidx = cache_[attrib]->find( bid ); \
    }


bool RandomTrackDisplay::getCacheValue( int attrib,int version,
					const Coord3& pos,float& val ) const
{
    if ( !cache_[attrib] )
	return false;

    BinID reqbid( SI().transform(pos) );
    int trcidx = cache_[attrib]->find( reqbid );
    if ( trcidx<0 )
    {
	const BinID step( SI().inlStep(), SI().crlStep() );
	BinID bid;
	mFindTrc(1,0) mFindTrc(-1,0) mFindTrc(0,1) mFindTrc(0,-1)
	if ( trcidx<0 )
	{
	    mFindTrc(1,1) mFindTrc(-1,1) mFindTrc(1,-1) mFindTrc(-1,-1)
	}

    }

    if ( trcidx<0 ) return false;

    const SeisTrc& trc = *cache_[attrib]->get( trcidx );
    const int sampidx = trc.nearestSample( (float) pos.z );
    if ( sampidx>=0 && sampidx<trc.size() )
    {
	val = trc.get( sampidx, 0 );
	return true;
    }

    return false;
}

#undef mFindTrc


void RandomTrackDisplay::addCache()
{
    cache_.allowNull();
    cache_ += 0;
    datapackids_ += -1;
    dispdatapackids_ += new TypeSet<DataPack::ID>;
}


void RandomTrackDisplay::removeCache( int attrib )
{
    delete cache_.removeSingle( attrib );

    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_.removeSingle( attrib );

    const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[attrib];
    for ( int idy=dpids.size()-1; idy>=0; idy-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idy] );
    delete dispdatapackids_.removeSingle( attrib );
}


void RandomTrackDisplay::swapCache( int a0, int a1 )
{
    cache_.swap( a0, a1 );
    datapackids_.swap( a0, a1 );
    dispdatapackids_.swap( a0, a1 );
}


void RandomTrackDisplay::emptyCache( int attrib )
{
    if ( cache_[attrib] ) delete cache_[attrib];
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


bool RandomTrackDisplay::hasCache( int attrib ) const
{
    return cache_[attrib];
}


const SeisTrcBuf* RandomTrackDisplay::getCache( int attrib ) const
{
    return (attrib<0 || attrib>=cache_.size() ) ? 0 : cache_[attrib];
}


void RandomTrackDisplay::setSceneEventCatcher( visBase::EventCatcher* evnt )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
				     mCB(this,RandomTrackDisplay,pickCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = evnt;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
				     mCB(this,RandomTrackDisplay,pickCB) );
    }

}


void RandomTrackDisplay::pickCB( CallBacker* cb )
{
    if ( !polylinemode_ ) return;


    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    if ( !eventinfo.pressed && eventinfo.type==visBase::MouseClick &&
			    OD::leftMouseButton( eventinfo.buttonstate_ ) )
    {
	const Coord3 pos = eventinfo.worldpickedpos;
	Coord3 inlcrlpos(pos);
	BinID bid = SI().transform( inlcrlpos );
	inlcrlpos.x = bid.inl(); inlcrlpos.y = bid.crl();

	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		    !OD::altKeyboardButton(eventinfo.buttonstate_) &&
		    !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	{

	    if ( !eventinfo.pickedobjids.isPresent(markerset_->id()) )
		return;

	    const mVisTrans* transform = markerset_->getDisplayTransformation();

	    if ( transform )
		transform->transform( inlcrlpos );

	    removePickPos( inlcrlpos );
	    eventcatcher_->setHandled();
	    return;
	}

	if ( !checkValidPick( eventinfo, pos ) )
	    return;
	setPickPos( inlcrlpos );
	eventcatcher_->setHandled();
    }
}


void RandomTrackDisplay::setPolyLineMode( bool mode )
{
    polylinemode_ = mode;
    polyline_->turnOn( polylinemode_ );
    markerset_->turnOn( polylinemode_ );
    panelstrip_->turnOn( !polylinemode_ );
    dragger_->turnOn( false );
}


bool RandomTrackDisplay::checkValidPick( const visBase::EventInfo& evi,
					 const Coord3& pos) const
{
    const int sz = evi.pickedobjids.size();
    bool validpicksurface = false;
    int eventid = -1;
    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject( evi.pickedobjids[idx] );
	if ( eventid==-1 && pickedobj->isPickable() )
	{
	    eventid = evi.pickedobjids[idx];
	    if ( validpicksurface )
		break;
	}

	mDynamicCastGet(const SurveyObject*,so,pickedobj);
	if ( !so || !so->allowsPicks() )
	    continue;

	mDynamicCastGet(const HorizonDisplay*,hd,so);
	mDynamicCastGet(const PlaneDataDisplay*,pdd,so);
	validpicksurface = hd ||
		(pdd && pdd->getOrientation() == OD::ZSlice);

	if ( eventid!=-1 )
	    break;
    }

    return validpicksurface;
 }


void RandomTrackDisplay::setPickPos( const Coord3& pos )
{
    polyline_->addPoint( pos );
    markerset_->addPos( pos );
}


void RandomTrackDisplay::removePickPos( const Coord3& pickpos )
{
    const int markeridx = markerset_->findClosestMarker( pickpos, true );
    if ( markeridx == -1 )
	return;
    polyline_->removePoint( markeridx );
    markerset_->removeMarker( markeridx );
}


void RandomTrackDisplay::setColor( Color color )
{
    polyline_->getMaterial()->setColor( color );
    markerset_->setMarkersSingleColor( color );
}


bool RandomTrackDisplay::createFromPolyLine()
{
    TypeSet<BinID> bids;
    for ( int idx=0; idx<polyline_->size(); idx++ )
    {
	Coord pos = polyline_->getPoint( idx );
	bids += BinID( (int)pos.x, (int)pos.y );
    }

    return setKnotPositions( bids );
}


void RandomTrackDisplay::setPixelDensity( float dpi )
{
    MultiTextureSurveyObject::setPixelDensity( dpi );

    if ( polyline_ )
	polyline_->setPixelDensity( dpi );

    if ( markerset_ )
	markerset_->setPixelDensity( dpi );
}


} // namespace visSurvey
