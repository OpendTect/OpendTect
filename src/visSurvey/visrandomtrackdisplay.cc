/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2003
 ________________________________________________________________________

-*/



#include "visrandomtrackdisplay.h"

#include "array2dresample.h"
#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "seisdatapack.h"
#include "volumedatapackzaxistransformer.h"
#include "randomlinegeom.h"
#include "randomlineprobe.h"
#include "survinfo.h"
#include "mousecursor.h"
#include "polylinend.h"
#include "settings.h"
#include "uistrings.h"
#include "keystrs.h"

#include "visdragger.h"
#include "visevent.h"
#include "vishorizondisplay.h"
#include "vismaterial.h"
#include "vismarkerset.h"
#include "vispolyline.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdragger.h"
#include "visselman.h"
#include "vistexturechannels.h"
#include "vistexturepanelstrip.h"
#include "vistopbotimage.h"
#include "zaxistransform.h"


namespace visSurvey
{

const char* RandomTrackDisplay::sKeyTrack()	    { return "Random track"; }
const char* RandomTrackDisplay::sKeyNrKnots()	    { return "Nr. Knots"; }
const char* RandomTrackDisplay::sKeyKnotPrefix()    { return "Knot "; }
const char* RandomTrackDisplay::sKeyDepthInterval() { return "Depth Interval"; }
const char* RandomTrackDisplay::sKeyLockGeometry()  { return "Lock geometry"; }

RandomTrackDisplay::RandomTrackDisplay()
    : MultiTextureSurveyObject()
    , panelstrip_( visBase::TexturePanelStrip::create() )
    , dragger_( visBase::RandomTrackDragger::create())
    , polyline_( visBase::PolyLine::create())
    , polylinemode_(false)
    , nodemoving_(this)
    , moving_(this)
    , poschanged_(this)
    , selnodeidx_(mUdf(int))
    , ismanip_(false)
    , interactivetexturedisplay_( false )
    , originalresolution_(-1)
    , datatransform_(0)
    , lockgeometry_(false)
    , eventcatcher_(0)
    , depthrg_(SI().zRange(OD::UsrWork))
    , voiidx_(-1)
    , markerset_( visBase::MarkerSet::create() )
    , rl_(0)
    , nrgeomchangecbs_(0)
    , premovingselids_(0)
    , geomnodejustmoved_(false)
    , ispicking_(false)
    , pickstartnodeidx_(-1)
    , mousecursor_( *new MouseCursor )
{
    datapacks_.setNullAllowed();
    transfdatapacks_.setNullAllowed();

    material_->setColor( Color::White() );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.2 );


    dragger_->ref();
    addChild( dragger_->osgNode() );

    mAttachCB( dragger_->motion, RandomTrackDisplay::nodeMoved );
    mAttachCB( dragger_->movefinished,RandomTrackDisplay::draggerMoveFinished );
    mAttachCB( dragger_->rightClicked(),RandomTrackDisplay::draggerRightClick );

    int dragkey = OD::NoButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPanelDepthKey(), dragkey );
    dragger_->setTransDragKeys( true, dragkey, 0 );
    dragkey = OD::ShiftButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPanelPlaneKey(), dragkey );
    dragger_->setTransDragKeys( false, dragkey, 0 );
    dragkey = OD::ControlButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPanelRotateKey(), dragkey );
    dragger_->setTransDragKeys( true, dragkey, 1 );

    panelstrip_->ref();
    addChild( panelstrip_->osgNode() );
    panelstrip_->setTextureChannels( channels_ );
    panelstrip_->swapTextureAxes();

    polyline_->ref();
    addChild( polyline_->osgNode() );
    polyline_->setMaterial( new visBase::Material );
    polyline_->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,1) );
    polyline_->setPickable( false, false );

    markerset_->ref();
    addChild( markerset_->osgNode() );
    markerset_->setMarkersSingleColor( polyline_->getMaterial()->getColor() );

    const auto zrg = SI().zRange( OD::UsrWork );
    const auto inlrg = SI().inlRange( OD::UsrWork );
    const auto crlrg = SI().crlRange( OD::UsrWork );
    dragger_->setLimits(
	    Coord3( inlrg.start, crlrg.start, zrg.start ),
	    Coord3( inlrg.stop, crlrg.stop, zrg.stop ),
	    Coord3( inlrg.step, crlrg.step, zrg.step ) );

    StepInterval<float> finlrg, fcrlrg;
    assign( finlrg, inlrg ); assign( fcrlrg, crlrg );
    dragger_->setDragCtrlSpacing( finlrg, fcrlrg, zrg );

    init();		// sets default resolution -> update texture mapping
    updatePanelStripPath();
    setPanelStripZRange( panelstrip_->getZRange() );

    showManipulator( dragger_->isOn() );
}


RandomTrackDisplay::~RandomTrackDisplay()
{
    detachAllNotifiers();

    setPolyLineMode( false );
    setSceneEventCatcher( 0 );
    panelstrip_->unRef();
    dragger_->unRef();
    removeChild( polyline_->osgNode() );
    polyline_->unRef();

    removeChild( markerset_->osgNode() );
    markerset_->unRef();

    if ( rl_ )
    {
	rl_->nodeChanged.remove( mCB(this,RandomTrackDisplay,geomChangeCB) );
	rl_->unRef();
    }

    setZAxisTransform( 0, 0 );

    if ( premovingselids_ )
	delete premovingselids_;
    delete &mousecursor_;
}


void RandomTrackDisplay::setProbe( Probe* probe )
{
    if ( probe_.ptr()==probe )
	return;

    mDynamicCastGet(RandomLineProbe*,rdlprobe,probe);
    if ( !rdlprobe )
	return;

    probe_ = probe;
    if ( rl_ )
    {
	rl_->nodeChanged.remove( mCB(this,RandomTrackDisplay,geomChangeCB) );
	rl_->unRef();
    }

    rl_ = Geometry::RLM().get( rdlprobe->randomeLineID() );
    if ( !rl_ ) return;

    rl_->ref();
    rl_->nodeChanged.notify( mCB(this,RandomTrackDisplay,geomChangeCB) );

    setName( rl_->name() );
    TypeSet<BinID> bids;
    rl_->getNodePositions( bids );
    setNodePositions( bids, true );
    setDepthInterval( rl_->zRange() );
}


BufferString RandomTrackDisplay::getRandomLineName() const
{
    mDynamicCastGet( const RandomLineProbe*,rdlprobe, probe_.ptr() );
    return rdlprobe ? rdlprobe->displayName().getString()
		    : BufferString(name());
}


int RandomTrackDisplay::getRandomLineID() const
{
    return rl_ ? rl_->ID() : -1;
}


Geometry::RandomLine* RandomTrackDisplay::getRandomLine()
{
    return rl_;
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
	step = scene_->getTrcKeyZSampling().zsamp_.step;

    return step;
}


TrcKeyZSampling RandomTrackDisplay::getTrcKeyZSampling( int attrib ) const
{
    TrcKeyZSampling cs( false );
    cs.hsamp_.setIs3D();
    TypeSet<BinID> nodes;
    getAllNodePos( nodes );
    for ( int idx=0; idx<nodes.size(); idx++ )
	cs.hsamp_.include( nodes[idx] );

    cs.zsamp_.setFrom( getDepthInterval() );
    cs.zsamp_.step = appliedZRangeStep();
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


void RandomTrackDisplay::setResolution( int res, TaskRunner* taskr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, taskr );

    updatePanelStripPath();
    setPanelStripZRange( panelstrip_->getZRange() );
}


Interval<float> RandomTrackDisplay::getDataTraceRange() const
{ return depthrg_; }


int RandomTrackDisplay::nrNodes() const
{ return nodes_.size(); }


#define mUpdateRandomLineGeometry( functioncall ) \
{ \
    if ( rl_ && nrgeomchangecbs_==0 ) \
    { \
	rl_->nodeChanged.remove( mCB(this,RandomTrackDisplay,geomChangeCB)); \
	rl_->functioncall; \
	rl_->nodeChanged.notify( mCB(this,RandomTrackDisplay,geomChangeCB)); \
    } \
}


void RandomTrackDisplay::addNode( const BinID& bid )
{
    const BinID sbid = snapPosition( bid );
    if ( checkPosition(sbid) )
	rl_->addNode( sbid );
}


void RandomTrackDisplay::addNodeInternal( const BinID& bid )
{
    nodes_ += bid;
    dragger_->insertKnot( nodes_.size()-1, Coord(bid.inl(),bid.crl()) );

    if ( ismanip_ )
	dragger_->showAdjacentPanels( nodes_.size()-1, true );
    else
	updatePanelStripPath();

    if ( nodes_.size() > 1 && !panelstrip_->isOn() )
	panelstrip_->turnOn( true );

    moving_.trigger();
}


void RandomTrackDisplay::insertNode( int nodeidx, const BinID& bid )
{
    const BinID sbid = snapPosition( bid );
    if ( checkPosition(sbid) )
	rl_->insertNode( nodeidx, sbid );
}


void RandomTrackDisplay::insertNodeInternal( int nodeidx, const BinID& bid )
{
    nodes_.insert( nodeidx, bid );
    dragger_->insertKnot( nodeidx, Coord(bid.inl(),bid.crl()) );

    if ( ismanip_ )
	dragger_->showAdjacentPanels( nodeidx, true );
    else
	updatePanelStripPath();

    moving_.trigger();
}


BinID RandomTrackDisplay::getNodePos( int nodeidx ) const
{
    return nodes_[nodeidx];
}


BinID RandomTrackDisplay::getManipNodePos( int nodeidx ) const
{
    const Coord crd = dragger_->getKnot( nodeidx );
    return BinID( SI().inlRange( OD::UsrWork ).snap(crd.x_),
		  SI().crlRange( OD::UsrWork ).snap(crd.y_) );
}


void RandomTrackDisplay::getAllNodePos( TypeSet<BinID>& nodes ) const
{
    const int nrnodes = nrNodes();
    for ( int idx=0; idx<nrnodes; idx++ )
	nodes += getManipNodePos( idx );
}


void RandomTrackDisplay::setNodePos( int nodeidx, const BinID& bid )
{ setNodePos( nodeidx, bid, true ); }


void RandomTrackDisplay::setNodePos( int nodeidx, const BinID& bid, bool check )
{
    const BinID sbid = snapPosition(bid);
    if ( !check || checkPosition(sbid) )
    {
	nodes_[nodeidx] = sbid;
	dragger_->setKnot( nodeidx, Coord(sbid.inl(),sbid.crl()) );
	mUpdateRandomLineGeometry( setNodePosition(nodeidx,sbid) );
	updatePanelStripPath();
	moving_.trigger();
    }
}


static bool decoincideNodes( const TypeSet<BinID>& nodes,
			     TypeSet<BinID>& uniquenodes )
{
    uniquenodes.erase();
    if ( nodes.isEmpty() )
	return false;
    uniquenodes += nodes[0];

    for ( int idx=1; idx<nodes.size(); idx++ )
    {
	const BinID prev = uniquenodes[uniquenodes.size()-1];
	const BinID biddif = prev - nodes[idx];
	const int nrsteps = mMAX( abs(biddif.inl())/SI().inlStep(),
				  abs(biddif.crl())/SI().crlStep() );
	const Coord dest = SI().transform( nodes[idx] );
	const Coord crddif = SI().transform(prev) - dest;
	for ( int step=0; step<nrsteps; step++ )
	{
	    const BinID newnode = SI().transform( dest+(crddif*step)/nrsteps );
	    if ( !uniquenodes.isPresent(newnode) )
	    {
		uniquenodes += newnode;
		break;
	    }
	}
    }
    return true;
}


bool RandomTrackDisplay::setNodePositions( const TypeSet<BinID>& newbids )
{
    TypeSet<BinID> uniquebids;
    if ( !decoincideNodes(newbids,uniquebids) || uniquebids.size()<2 )
	return false;

    setNodePositions( uniquebids, false );
    return true;
}


void RandomTrackDisplay::setNodePositions( const TypeSet<BinID>& bids,
					   bool onlyinternal )
{
    NotifyStopper movingnotifystopper( moving_ );

    while ( nrNodes() > bids.size() )
    {
	if ( onlyinternal )
	    removeNodeInternal( nrNodes()-1 );
	else
	    rl_->removeNode( nrNodes()-1 );
    }

    for ( int idx=0; idx<bids.size(); idx++ )
    {
	if ( idx < nrNodes() )
	    setNodePos( idx, bids[idx], false );
	else if ( onlyinternal )
	    addNodeInternal( bids[idx] );
	else
	    rl_->addNode( bids[idx] );
    }

    updatePanelStripPath();

    movingnotifystopper.enableNotification();
    moving_.trigger();
}


void RandomTrackDisplay::removeNode( int nodeidx )
{
    if ( nrNodes()< 3 )
    {
	pErrMsg("Can't remove node");
	return;
    }

    rl_->removeNode( nodeidx );
}


void RandomTrackDisplay::removeNodeInternal( int nodeidx )
{
    if ( !nodes_.validIdx(nodeidx) )
	return;

    nodes_.removeSingle(nodeidx);
    dragger_->removeKnot( nodeidx );
    updatePanelStripPath();

    moving_.trigger();
}


void RandomTrackDisplay::removeAllNodes()
{
    for ( int idx=nodes_.size()-1; idx>=0; idx-- )
    {
	dragger_->removeKnot( idx );
	mUpdateRandomLineGeometry( removeNode(idx) );
    }

    for ( int idx=0; idx<nrAttribs(); idx++ )
	setDataPackID( idx, DataPack::ID::getInvalid(), 0 );

    nodes_.erase();
    updatePanelStripPath();
    panelstrip_->turnOn( false );
    moving_.trigger();
}


void RandomTrackDisplay::getTraceKeyPath( TrcKeyPath& path,
                                          TypeSet<Coord>* crds ) const
{
    if ( crds ) crds->erase();

    TypeSet<BinID> nodes;
    getAllNodePos( nodes );

    TypeSet<BinID> bids;
    TypeSet<int> segments;
    Geometry::RandomLine::getPathBids( nodes, s3dgeom_, bids,
			Geometry::RandomLine::NoConsecutiveDups, &segments );

    path.erase();
    int curlinesegment = -1;
    Line2 curline;
    for ( int idx=0; idx<bids.size(); idx++ )
    {
	path += TrcKey( bids[idx] );
        if ( !crds )
            continue;

        if ( curlinesegment!=segments[idx] )
        {
            curlinesegment = -1;

            const int cursegment = segments[idx];
            if ( cursegment<nodes.size()-1 )
            {
                const BinID startnode = nodes[segments[idx]];
                const BinID stopnode = nodes[segments[idx]+1];
                const Coord startpos = s3dgeom_->transform ( startnode );
                const Coord stoppos = s3dgeom_->transform( stopnode );

                if ( startpos.isDefined() && stoppos.isDefined() &&
                     startpos.sqDistTo( stoppos )>1e-3 )
                {
                    curline = Line2( startpos, stoppos );
                    curlinesegment = cursegment;
                }
            }
        }

        Coord pathpos = s3dgeom_->transform(bids[idx]);
        if ( curlinesegment>=0 )
	    pathpos = curline.closestPoint( pathpos );

        (*crds) += pathpos;
    }
}


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids ) const
{ getDataTraceBids( bids, 0 ); }


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids,
					   TypeSet<int>* segments ) const
{
    const_cast<RandomTrackDisplay*>(this)->trcspath_.erase();
    const_cast<RandomTrackDisplay*>(this)->tkpath_.erase();
    TypeSet<BinID> nodes;
    getAllNodePos( nodes );
    Geometry::RandomLine::getPathBids( nodes, s3dgeom_, bids,
				    Geometry::RandomLine::AllDups, segments );
    for ( int idx=0; idx<bids.size(); idx++ )
    {
	if ( !idx || bids[idx]!=trcspath_.last() )
	{
	    const_cast<RandomTrackDisplay*>(this)->trcspath_.add( bids[idx] );
	    const_cast<RandomTrackDisplay*>(this)->tkpath_.add(
							    TrcKey(bids[idx]) );
	}
    }
}


TypeSet<Coord> RandomTrackDisplay::getTrueCoords() const
{
    const int nrnodes = nrNodes();
    TypeSet<Coord> coords;
    for ( int kidx=1; kidx<nrnodes; kidx++ )
    {
	BinID start = getNodePos(kidx-1);
	BinID stop = getNodePos(kidx);
	const int nrinl = int(abs(stop.inl()-start.inl()) / SI().inlStep() + 1);
	const int nrcrl = int(abs(stop.crl()-start.crl()) / SI().crlStep() + 1);
	const int nrtraces = nrinl > nrcrl ? nrinl : nrcrl;
	const Coord startcoord = SI().transform( start );
	const Coord stopcoord = SI().transform( stop );
	const float delx = (float) ( stopcoord.x_ - startcoord.x_ ) / nrtraces;
	const float dely = (float) ( stopcoord.y_ - startcoord.y_ ) / nrtraces;

	for ( int idx=0; idx<nrtraces; idx++ )
	{
	    const float x = (float) ( startcoord.x_ + delx * idx );
	    const float y = (float) ( startcoord.y_ + dely * idx );
	    coords += Coord( x, y );
	}
    }
    return coords;
}


bool RandomTrackDisplay::setDataPackID( int attrib, DataPack::ID dpid,
					TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    auto randsdp = dpm.get<RandomSeisDataPack>(dpid);
    if ( !randsdp || randsdp->isEmpty() )
    {
	channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr,
				    SilentTaskRunnerProvider() );
	channels_->turnOn( false );
	return false;
    }

    datapacks_.replace( attrib, randsdp );

    createTransformedDataPack( attrib, taskr );
    updateChannels( attrib, taskr );
    return true;
}


DataPack::ID RandomTrackDisplay::getDataPackID( int attrib ) const
{
    return datapacks_.validIdx(attrib) && datapacks_[attrib]
	? datapacks_[attrib]->id()
	: DataPack::cNoID();
}


DataPack::ID RandomTrackDisplay::getDisplayedDataPackID( int attrib ) const
{
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	return transfdatapacks_.validIdx(attrib) && transfdatapacks_[attrib]
            ? transfdatapacks_[attrib]->id()
            : DataPack::cNoID();
    }

    return getDataPackID( attrib );
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
    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	createTransformedDataPack( idx, 0 );
	updateChannels( idx, 0 );
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


bool RandomTrackDisplay::isMappingTraceOfBid( BinID bid, int trcidx,
					      bool forward ) const
{
    if ( !trcspath_.validIdx(trcidx) )
	return false;

    // Allow round-off differences from elsewhere
    if ( abs(trcspath_[trcidx].inl()-bid.inl()) > SI().inlStep() )
	return false;
    if ( abs(trcspath_[trcidx].crl()-bid.crl()) > SI().crlStep() )
	return false;

    if ( (!forward && !trcidx) || (forward && trcidx==trcspath_.size()-1) )
	return true;

    const int dir = forward ? 1 : -1;
    return trcspath_[trcidx].sqDistTo(bid)<=trcspath_[trcidx+dir].sqDistTo(bid);
}


void RandomTrackDisplay::updateTexOriginAndScale( int attrib,
			const TrcKeyPath& path, const StepInterval<float>& zrg )
{
    if ( path.size()<2 || zrg.isUdf() )
	return;

    int idx0 = 0;
    while ( idx0<trcspath_.size() &&
	    !isMappingTraceOfBid(path.first().binID(),idx0,true) )
	idx0++;

    int idx1 = trcspath_.size()-1;
    while ( idx1>=0 && !isMappingTraceOfBid(path.last().binID(),idx1,false) )
	idx1--;

    if ( idx0 >= idx1 )
    {
	pErrMsg( "Texture trace path does not match random line geometry" );
	return;
    }

    const Coord origin(
	    (zrg.start-getDepthInterval().start)/appliedZRangeStep(), idx0 );

    const Coord scale( zrg.step/appliedZRangeStep(),
		       mCast(float,idx1-idx0+1) / path.size() );

    channels_->setOrigin( attrib, origin*(resolution_+1) );
    channels_->setScale( attrib, scale );
}


void RandomTrackDisplay::updateChannels( int attrib, TaskRunner* taskr )
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    auto randsdp = dpm.get<RandomSeisDataPack>( dpid );
    if ( !randsdp )
	return;

    updateTexOriginAndScale( attrib, randsdp->path(), randsdp->zRange() );

    const int nrversions = randsdp->nrComponents();
    channels_->setNrVersions( attrib, nrversions );

    for ( int idx=0; idx<nrversions; idx++ )
    {
	const Array3DImpl<float>& array = randsdp->data( idx );
	const int sz0 = 1 + (array.getSize(1)-1) * (resolution_+1);
	const int sz1 = 1 + (array.getSize(2)-1) * (resolution_+1);
	const float* arr = array.getData();
	OD::PtrPolicy cp = OD::UsePtr;

	if ( !arr || resolution_>0 )
	{
	    mDeclareAndTryAlloc( float*, tmparr, float[sz0*sz1] );
	    if ( !tmparr ) continue;

	    if ( resolution_ == 0 )
		array.getAll( tmparr );
	    else
	    {
		Array2DSlice<float> slice2d( array );
		slice2d.setDimMap( 0, 1 );
		slice2d.setDimMap( 1, 2 );
		slice2d.setPos( 0, 0 );
		slice2d.init();

		UserShowWait usw( this, uiStrings::sCollectingData() );
		Array2DReSampler<float,float> resampler(
			    slice2d, tmparr, sz0, sz1, true );
		resampler.setInterpolate( true );
		TaskRunner::execute( 0, resampler );
	    }

	    arr = tmparr;
	    cp = OD::TakeOverPtr;
	}

	channels_->setSize( attrib, 1, sz0, sz1 );
	channels_->setUnMappedData( attrib, idx, arr, cp,
				    SilentTaskRunnerProvider() );
    }

    channels_->turnOn( true );
}


void RandomTrackDisplay::createTransformedDataPack(
				int attrib, TaskRunner* tskr )
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDataPackID( attrib );
    auto randsdp = dpm.get<RandomSeisDataPack>( dpid );
    if ( !randsdp || randsdp->isEmpty() )
	return;

    RefMan<RandomSeisDataPack> output = 0;
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	if ( datatransform_->needsVolumeOfInterest() )
	{
	    const TrcKeyPath& path = randsdp->path();
	    TrcKeyZSampling tkzs( false );
	    for ( int idx=0; idx<path.size(); idx++ )
		tkzs.hsamp_.include( path[idx].binID() );
	    tkzs.zsamp_ = panelstrip_->getZRange();
	    tkzs.zsamp_.step = scene_ ? scene_->getTrcKeyZSampling().zsamp_.step
				      : datatransform_->getGoodZStep();
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest( tkzs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, tkzs, true );
	    ExistingTaskRunnerProvider trprov( tskr );
	    datatransform_->loadDataIfMissing( voiidx_, trprov );
	}

	VolumeDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( randsdp.ptr() );
	transformer.setInterpolate( textureInterpolationEnabled() );
	transformer.execute();
        output = transformer.getOutput();
    }

    transfdatapacks_.replace( attrib, output.ptr() );
}


void RandomTrackDisplay::updatePanelStripPath()
{
    if ( nodes_.size()<2 || getUpdateStageNr() )
	return;

    TypeSet<BinID> trcbids;
    getDataTraceBids( trcbids );	// Will update trcspath_

    TypeSet<Coord> pathcrds;
    TypeSet<float> mapping;
    pathcrds.setCapacity( nodes_.size(), false );
    mapping.setCapacity( nodes_.size(), false );

    int nodeidx = 0;
    for ( int trcidx=0; trcidx<trcspath_.size(); trcidx++ )
    {
	while ( nodeidx<nodes_.size() &&
		isMappingTraceOfBid(nodes_[nodeidx],trcidx,true) )
	{
	    pathcrds += Coord( nodes_[nodeidx].inl(), nodes_[nodeidx].crl() );
	    mapping += mCast( float, trcidx*(resolution_+1) );
	    nodeidx++;
	}
    }

    if ( mapping.size()!=nodes_.size() )
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
    const Interval<float> mapping( 0.0, zrg.nrfSteps()*(resolution_+1) );
    panelstrip_->setZRange2TextureMapping( mapping );

    if ( getUpdateStageNr() )
    {
	const float factor = (resolution_+1) / zrg.step;
	const float diff = updatestageinfo_.oldzrgstart_ - zrg.start;
	panelstrip_->setZTextureShift( diff*factor );
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

	for ( int idx=0; idx<nrNodes(); idx++ )
	    dragger_->showAdjacentPanels( idx, false );
    }
    else
    {
	if ( !getUpdateStageNr() )
	    updatestageinfo_.oldzrgstart_ = getDepthInterval().start;
	else
	    panelstrip_->freezeDisplay( false );	// thaw to refreeze

	SurveyObject::annotateNextUpdateStage( true );
	panelstrip_->freezeDisplay( true );
    }
}


bool RandomTrackDisplay::canAddNode( int nodenr ) const
{
    if ( lockgeometry_ ) return false;
    if ( nodenr<0 ) nodenr=0;
    if ( nodenr>nrNodes() ) nodenr=nrNodes();

    const BinID newpos = proposeNewPos(nodenr);
    return checkPosition(newpos);
}


void RandomTrackDisplay::addNode( int nodenr )
{
    if ( nodenr<0 ) nodenr=0;
    if ( nodenr>nrNodes() ) nodenr=nrNodes();

    if ( !canAddNode(nodenr) ) return;

    ismanip_ = true;

    const BinID newpos = proposeNewPos( nodenr );
    rl_->insertNode( nodenr, newpos );
}


BinID RandomTrackDisplay::proposeNewPos( int nodenr ) const
{
    BinID res;
    if ( !nodenr )
	res = getNodePos(0)-(getNodePos(1)-getNodePos(0));
    else if ( nodenr>=nrNodes() )
	res = getNodePos(nrNodes()-1) +
	      (getNodePos(nrNodes()-1)-getNodePos(nrNodes()-2));
    else
    {
	res = getNodePos(nodenr)+getNodePos(nodenr-1);
	res.inl() /= 2;
	res.crl() /= 2;
    }

    res.inl() = mMIN( SI().inlRange(OD::UsrWork).stop, res.inl() );
    res.inl() = mMAX( SI().inlRange(OD::UsrWork).start, res.inl() );
    res.crl() = mMIN( SI().crlRange(OD::UsrWork).stop, res.crl() );
    res.crl() = mMAX( SI().crlRange(OD::UsrWork).start, res.crl() );

    SI().snap( res );
    return res;
}


bool RandomTrackDisplay::isManipulated() const
{
    return ismanip_;
}


void RandomTrackDisplay::acceptManipulation()
{
    NotifyStopper movingnotifystopper( moving_ );

    if ( !datatransform_ )
	setDepthInterval( dragger_->getDepthRange() );
    else
    {
	setPanelStripZRange( dragger_->getDepthRange() );
	moving_.trigger();
    }

    for ( int idx=0; idx<nrNodes(); idx++ )
    {
	setNodePos( idx, getManipNodePos(idx), false );
	if ( !getUpdateStageNr() )
	    dragger_->showAdjacentPanels( idx, false );
    }

    updatePanelStripPath();
    dragger_->showAllPanels( false );
    ismanip_ = false;

    movingnotifystopper.enableNotification();
    moving_.trigger();
}


void RandomTrackDisplay::resetManipulation()
{
    if ( !datatransform_ )
	dragger_->setDepthRange( getDepthInterval() );

    for ( int idx=0; idx<nrNodes(); idx++ )
    {
	const BinID bid = getNodePos(idx);
	dragger_->setKnot(idx, Coord(bid.inl(),bid.crl()));
	dragger_->showAdjacentPanels( idx, false );
    }

    dragger_->showAllPanels( false );
    ismanip_ = false;
    dragger_->turnOn( false );
}


void RandomTrackDisplay::showManipulator( bool yn )
{
    if ( lockgeometry_ ) yn = false;
    dragger_->turnOn( yn );
    panelstrip_->enableTraversal(visBase::cDraggerIntersecTraversalMask(),!yn);

    if ( polylinemode_ )
    {
	polyline_->turnOn( yn );
	markerset_->turnOn( yn );
	if ( scene_ )
	    scene_->blockMouseSelection( yn );
    }
}


bool RandomTrackDisplay::isManipulatorShown() const
{ return dragger_->isOn(); }


BufferString RandomTrackDisplay::getManipulationString() const
{
    BufferString str;
    const int sel = getSelNodeIdx();

    const int start = sel>=0 ? sel : -sel-1;
    const int stop = mMIN( abs(sel), nrNodes()-1 );

    for ( int idx=start; idx<=stop; idx++ )
    {
	str += idx==start ? "" : ", ";
	const BinID binid = getManipNodePos( idx );
	str += "Node "; str += idx+1;
	str += " Inl/Crl: ";
	str += binid.inl(); str += "/"; str += binid.crl();
    }

    return str;
}


void RandomTrackDisplay::geomChangeCB( CallBacker* cb )
{
    if ( isPicking() )			// Just abort polyline instead of
	setPolyLineMode( false );	// synchronizing with e.g. basemap

    nrgeomchangecbs_++;

    mCBCapsuleUnpack(const Geometry::RandomLine::ChangeData&,cd,cb);

    if ( cd.ev_ == Geometry::RandomLine::ChangeData::Added )
    {
	if ( cd.nodeidx_ < nrNodes() )
	    geomNodeMoveCB( cb );
	else
	    addNodeInternal( rl_->nodePosition(cd.nodeidx_) );
    }
    else if ( cd.ev_ == Geometry::RandomLine::ChangeData::Inserted )
    {
	insertNodeInternal( cd.nodeidx_, rl_->nodePosition(cd.nodeidx_) );
    }
    else if ( cd.ev_ == Geometry::RandomLine::ChangeData::Removed )
    {
	removeNodeInternal( cd.nodeidx_ );
    }
    else
	geomNodeMoveCB( cb );

    geomnodejustmoved_ = cd.ev_==Geometry::RandomLine::ChangeData::Moved &&
			 cd.nodeidx_>=0;
    nrgeomchangecbs_--;
}


void RandomTrackDisplay::geomNodeMoveCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const Geometry::RandomLine::ChangeData&, cd, cb );

    if ( cd.ev_!=Geometry::RandomLine::ChangeData::Added &&
	 cd.ev_!=Geometry::RandomLine::ChangeData::Moving &&
	 cd.ev_!=Geometry::RandomLine::ChangeData::Moved )
	return;

    if ( cd.nodeidx_>=0 && cd.nodeidx_<rl_->nrNodes() )
    {
	const BinID nodepos = rl_->nodePosition( cd.nodeidx_ );

	if ( nodepos != getManipNodePos(cd.nodeidx_) )
	{
	    dragger_->setKnot( cd.nodeidx_, Coord(nodepos.inl(),nodepos.crl()));
	    dragger_->showAdjacentPanels( cd.nodeidx_, true );

	    if ( !isSelected() )
	    {
		if ( premovingselids_ )
		    delete premovingselids_;

		premovingselids_ =
			new TypeSet<int>( visBase::DM().selMan().selected() );
		select();
	    }

	    movingNodeInternal( cd.nodeidx_ );
	}
    }

    if ( cd.ev_ == Geometry::RandomLine::ChangeData::Moved )
    {
	if ( premovingselids_ )
	{
	    visBase::DM().selMan().deSelectAll();
	    for ( int idx=0; idx<premovingselids_->size(); idx++ )
		visBase::DM().selMan().select( (*premovingselids_)[idx],true);

	    delete premovingselids_;
	    premovingselids_ = 0;
	}

	if ( cd.nodeidx_ < 0 )
	{
	    if ( geomnodejustmoved_ )
		return;

	    ismanip_ = true;
	}

	finishNodeMoveInternal();
	poschanged_.trigger();
    }
}


void RandomTrackDisplay::nodeMoved( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);

    for ( int idx=abs(sel); idx>=abs(sel)-1; idx-- )
    {
	const BinID bid = getManipNodePos( idx );
	dragger_->showAdjacentPanels( idx, getNodePos(idx)!=bid );
	mUpdateRandomLineGeometry( setNodePosition(idx,bid,true) );

	if ( sel>=0 ) break;
    }

    if ( getDepthInterval().isEqual(dragger_->getDepthRange(),1e-5) )
	dragger_->showAllPanels( false );
    else
	dragger_->showAllPanels( true );

    movingNodeInternal( sel );
}


void RandomTrackDisplay::movingNodeInternal( int selnodeidx )
{
    ismanip_ = true;
    selnodeidx_ = selnodeidx;

    nodemoving_.trigger();

    if ( canDisplayInteractively() )
    {
	if ( originalresolution_ < 0 )
	    originalresolution_ = resolution_;

	resolution_ = 0;
	interactivetexturedisplay_ = true;
	updateSel();
    }
}


bool RandomTrackDisplay::checkPosition( const BinID& binid ) const
{
    TrcKeySampling hs( OD::UsrWork );
    if ( !hs.includes(binid) )
	return false;

    BinID snapped( binid );
    SI().snap( snapped );
    if ( snapped != binid )
	return false;

    for ( int idx=0; idx<nrNodes(); idx++ )
    {
	if ( getNodePos(idx) == binid )
	    return false;
    }

    return true;
}


BinID RandomTrackDisplay::snapPosition( const BinID& bid ) const
{
    BinID binid( bid );
    const TrcKeySampling hs( OD::UsrWork );
    if ( binid.inl() < hs.start_.inl() ) binid.inl() = hs.start_.inl();
    if ( binid.inl() > hs.stop_.inl() ) binid.inl() = hs.stop_.inl();
    if ( binid.crl() < hs.start_.crl() ) binid.crl() = hs.start_.crl();
    if ( binid.crl() > hs.stop_.crl() ) binid.crl() = hs.stop_.crl();

    SI().snap( binid );
    return binid;
}


SurveyObject::AttribFormat RandomTrackDisplay::getAttributeFormat( int ) const
{ return SurveyObject::Traces; }


int RandomTrackDisplay::getClosestPanelIdx( const Coord& xypos,
					    double* fracptr ) const
{
    TypeSet<Coord> coords;
    for ( int idx=0; idx<nodes_.size(); idx++ )
	coords += SI().transform( nodes_[idx] );

    const PolyLineND<Coord> polyline( coords );
    int panelidx = -1;

    polyline.distTo( xypos, &panelidx, fracptr );

    return panelidx;
}


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
    BinID binid = s3dgeom_->transform( xytpos.getXY() );

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
    const BinID bid0( mNINT32(pos0.x_), mNINT32(pos0.y_));
    const BinID bid1( mNINT32(pos1.x_), mNINT32(pos1.y_));

    const Coord dir = SI().transform(bid0)-SI().transform(bid1);
    const float dist = dir.abs<float>();

    if ( dist<=mMIN(SI().inlDistance(),SI().crlDistance()) )
	return Coord3::udf();

    return Coord3( dir.y_, -dir.x_, 0 );
}

#undef mFindTrc


float RandomTrackDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pos, xytpos );
    BinID binid = s3dgeom_->transform( xytpos.getXY() );

    bool ontrack = false;
    for ( int idx=0; idx<nodes_.size()-1; idx++ )
    {
	int x = binid.inl(); int y = binid.crl();
	int x0 = nodes_[ idx ].inl(); int y0 = nodes_[ idx ].crl();
	int x1 = nodes_[idx+1].inl(); int y1 = nodes_[idx+1].crl();

	if ( x0<x1 ? (x<x0 || x>x1) : (x>x0 || x<x1) )
	    continue;
	if ( y0<y1 ? (y<y0 || y>y1) : (y>y0 || y<y1) )
	    continue;

	if ( x0==x1 && y0==y1 )
	{
	    if ( x!=x0 || y!=y0 )
		continue;

	    ontrack = true;
	    break;
	}

	if ( abs(x1-x0) < abs(y1-y0) )
	    { std::swap(x,y); std::swap(x0,y0); std::swap(x1,y1); }

	const float slope = mCast(float,y1-y0) / mCast(float,x1-x0);
	const float ydiff = y - y0 - slope * (x-x0);

	if ( fabs(ydiff) > 0.5 )
	    continue;

	ontrack = true;
	break;
    }

    if ( !ontrack )
	return mUdf(float);

    float zdiff = 0;
    const Interval<float> intv = getDataTraceRange();
    if ( xytpos.z_ < intv.start )
	zdiff = (float) ( intv.start - xytpos.z_ );
    else if ( xytpos.z_ > intv.stop )
	zdiff = (float) ( xytpos.z_ - intv.stop );

    return zdiff;
}


void RandomTrackDisplay::lockGeometry( bool yn )
{
    lockgeometry_ = yn;
    if ( yn ) showManipulator( false );
}


bool RandomTrackDisplay::isGeometryLocked() const
{ return lockgeometry_; }


SurveyObject* RandomTrackDisplay::duplicate( TaskRunner* tskr ) const
{
    RandomTrackDisplay* rtd = new RandomTrackDisplay;
    rtd->setDepthInterval( getDataTraceRange() );
    TypeSet<BinID> positions;
    for ( int idx=0; idx<nrNodes(); idx++ )
	positions += getNodePos( idx );
    rtd->setNodePositions( positions, false );
    rtd->lockGeometry( isGeometryLocked() );
    rtd->setZAxisTransform( datatransform_, tskr );

    while ( nrAttribs() > rtd->nrAttribs() )
	rtd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	const Attrib::SelSpecList* selspecs = getSelSpecs( idx );
	if ( selspecs ) rtd->setSelSpecs( idx, *selspecs );
	rtd->setDataPackID( idx, getDataPackID(idx), tskr );
	rtd->setColTabMapper( idx, getColTabMapper(idx), tskr );
	const ColTab::Sequence& colseq = getColTabSequence( idx );
	rtd->setColTabSequence( idx, colseq, tskr );
    }

    return rtd;
}


DBKey RandomTrackDisplay::getDBKey() const
{
    return rl_ ? rl_->getDBKey() : DBKey::getInvalid();
}


void RandomTrackDisplay::fillPar( IOPar& par ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par );

    par.set( sKey::Name(), name() );
    const Interval<float> depthrg = getDataTraceRange();
    par.set( sKeyDepthInterval(), depthrg );

    const int nrnodes = nrNodes();
    par.set( sKeyNrKnots(), nrnodes );

    for ( int idx=0; idx<nrnodes; idx++ )
    {
	BufferString key = sKeyKnotPrefix(); key += idx;
	par.set( key, getNodePos(idx) );
    }

    par.set( sKey::Version(), 3 );
    par.setYN( sKeyLockGeometry(), lockgeometry_ );
}


bool RandomTrackDisplay::usePar( const IOPar& par )
{
    if ( !visSurvey::MultiTextureSurveyObject::usePar( par ) )
	return false;

    BufferString linename( getName() );
    par.get( sKey::Name(), linename );
    if ( !linename.isEmpty() )
	setName( linename );

    par.getYN( sKeyLockGeometry(), lockgeometry_ );

    Interval<float> intv;
    if ( par.get( sKeyDepthInterval(), intv ) )
	setDepthInterval( intv );

    int nrnodes = 0;
    par.get( sKeyNrKnots(), nrnodes );

    BufferString key; BinID pos;
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	key = sKeyKnotPrefix(); key += idx;
	par.get( key, pos );
	if ( idx < 2 )
	    setNodePos( idx, pos, false );
	else
	    rl_->addNode( pos );
    }

    return true;
}


void RandomTrackDisplay::getMousePosInfo( const visBase::EventInfo& ei,
					  IOPar& par ) const
{
    MultiTextureSurveyObject::getMousePosInfo( ei, par );
}


void RandomTrackDisplay::getMousePosInfo( const visBase::EventInfo&,
					  Coord3& pos, BufferString& val,
					  BufferString& info ) const
{
    info = name();
    getValueString( pos, val );
}


bool RandomTrackDisplay::getSelMousePosInfo( const visBase::EventInfo& ei,
					     Coord3& pos, BufferString& val,
					     BufferString& info ) const
{
    if ( !isPicking() || !scene_ || OD::ctrlKeyboardButton(ei.buttonstate_)
				 || OD::altKeyboardButton(ei.buttonstate_) )
	return false;

    const bool shiftclick = OD::shiftKeyboardButton( ei.buttonstate_ );
    pos = scene_->getTopBottomSurveyPos( ei, shiftclick, shiftclick,
					 false, &info );
    return shiftclick || !mIsUdf(pos);
}


bool RandomTrackDisplay::getCacheValue( int attrib,int version,
					const Coord3& pos,float& val ) const
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    auto randsdp = dpm.get<RandomSeisDataPack>( dpid );
    if ( !randsdp || randsdp->isEmpty() )
	return false;

    const TrcKey trckey( s3dgeom_->transform(pos.getXY()) );
    const int trcidx = randsdp->getNearestGlobalIdx( trckey );
    const int sampidx = randsdp->zRange().nearestIndex( pos.z_ );
    const Array3DImpl<float>& array = randsdp->data( version );
    if ( !array.info().validPos(0,trcidx,sampidx) )
	return false;

    val = array.get( 0, trcidx, sampidx );
    return true;
}


void RandomTrackDisplay::addCache()
{
    datapacks_ += 0;
    transfdatapacks_ += 0;
}


void RandomTrackDisplay::removeCache( int attrib )
{
    datapacks_.removeSingle( attrib );
    transfdatapacks_.removeSingle( attrib );
}


void RandomTrackDisplay::swapCache( int a0, int a1 )
{
    datapacks_.swap( a0, a1 );
    transfdatapacks_.swap( a0, a1 );
}


void RandomTrackDisplay::emptyCache( int attrib )
{
    datapacks_.replace( attrib, 0 );
    transfdatapacks_.replace( attrib, 0 );

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedData( attrib, 0, 0, OD::CopyPtr,
				SilentTaskRunnerProvider() );
}


bool RandomTrackDisplay::hasCache( int attrib ) const
{
    return datapacks_[attrib];
}


void RandomTrackDisplay::setSceneEventCatcher( visBase::EventCatcher* evnt )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
			    mCB(this,RandomTrackDisplay,mouseCB) );
	eventcatcher_->eventhappened.remove(
			    mCB(this,RandomTrackDisplay,updateMouseCursorCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = evnt;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
			    mCB(this,RandomTrackDisplay,mouseCB) );
	eventcatcher_->eventhappened.notify(
			    mCB(this,RandomTrackDisplay,updateMouseCursorCB) );
    }
}


void RandomTrackDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    dragger_->setRightHandSystem( yn );
}


void RandomTrackDisplay::updateMouseCursorCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const visBase::EventInfo&, eventinfo, cb );

    const visBase::Dragger* nodedragger = 0;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	const visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	mDynamicCast( const visBase::Dragger*, nodedragger, dataobj );
	if ( nodedragger )
	    break;
    }

    if ( !isManipulatorShown() || !isOn() || isLocked()
					  || nodedragger || polylinemode_ )
	mousecursor_.shape_ = MouseCursor::NotSet;
    else
    {
	initAdaptiveMouseCursor( cb, id(), dragger_->getTransDragKeys(false,0),
				 mousecursor_ );

	if ( mousecursor_.shape_ != MouseCursor::GreenArrow )
	    return;

	initAdaptiveMouseCursor( cb, id(), dragger_->getTransDragKeys(true,1),
				 mousecursor_ );

	if ( mousecursor_.shape_ != MouseCursor::GreenArrow )
	    mousecursor_.shape_ = MouseCursor::Rotator;
    }
}


bool RandomTrackDisplay::isPicking() const
{ return ispicking_; }


void RandomTrackDisplay::mouseCB( CallBacker* cb )
{
    if ( !isOn() || eventcatcher_->isHandled() || !isSelected() || locked_ )
	return;

    mCBCapsuleUnpack( const visBase::EventInfo&, eventinfo, cb );

    if ( !OD::leftMouseButton(eventinfo.buttonstate_) ||
	 OD::altKeyboardButton(eventinfo.buttonstate_) )
	return;

    const bool doubleclick = eventinfo.type==visBase::MouseDoubleClick;
    int nodeidx = dragger_->getKnotIdx( eventinfo.pickedobjids );

    if ( doubleclick )
    {
	if ( nodeidx>=0 || pickstartnodeidx_<0 )
	    return;

	if ( !eventinfo.pickedobjids.isPresent(markerset_->id()) )
	    return;

	if ( pickstartnodeidx_>0 && pickstartnodeidx_<nrNodes()-1 )
	{
	    double frac = 0.5;
	    nodeidx=getClosestPanelIdx(eventinfo.worldpickedpos.getXY(),&frac);
	    if ( nodeidx==pickstartnodeidx_ || frac>=0.5 )
		nodeidx++;
	    if ( nodeidx == pickstartnodeidx_ )
		nodeidx--;
	}
	else
	    nodeidx = pickstartnodeidx_;
    }
    else if ( eventinfo.type != visBase::MouseClick )
	return;

    if ( nodeidx<0 )
    {
	if ( polylinemode_ )
	    pickCB( cb );

	return;
    }

    const bool ctrlclick = OD::ctrlKeyboardButton(eventinfo.buttonstate_);
    const bool shiftclick = OD::shiftKeyboardButton(eventinfo.buttonstate_);

    if ( ctrlclick && shiftclick )
	return;

    eventcatcher_->setHandled();

    Coord3 inlcrlnodepos( eventinfo.worldpickedpos );
    const BinID nodebid = getNodePos( nodeidx );
    inlcrlnodepos.x_ = nodebid.inl();
    inlcrlnodepos.y_ = nodebid.crl();

    if ( shiftclick && pickstartnodeidx_<0 )
    {
	if ( !eventinfo.pressed )
	{
	    setPolyLineMode( true );
	    pickstartnodeidx_ = nodeidx;
	    setColor( Color(255,0,255) );
	    polyline_->addPoint( inlcrlnodepos );
	}
	else
	    ispicking_ = true;	// Change to pick-cursor early

	return;
    }

    if ( ctrlclick )
    {
	if ( pickstartnodeidx_<0 && !eventinfo.pressed )
	{
	    removeNode( nodeidx );
	    channels_->turnOn( false );
	    ismanip_ = true;
	    updateSel();
	}
	return;
    }

    if ( pickstartnodeidx_>=0 )
    {
	if ( eventinfo.pressed )
	{
	    ispicking_ = false; // Makes geomChangeCB(.) keep polyline mode
	    return;
	}

	int nrinserts = polyline_->size()-1;
	if ( nrinserts > 0 )
	{
	    const bool terminal = nodeidx==0 || nodeidx==nrNodes()-1;
	    if ( nodeidx!=pickstartnodeidx_ || !terminal )
	    {
		polyline_->addPoint( inlcrlnodepos );
		if ( nodeidx == pickstartnodeidx_ )
		    nrinserts++;
	    }

	    const bool forward = nodeidx>=pickstartnodeidx_ &&
				 ( pickstartnodeidx_ || nodeidx );

	    int curidx = pickstartnodeidx_;
	    for ( int posidx=1; posidx<=nrinserts; posidx++ )
	    {
		if ( forward )
		    curidx++;

		const Coord pos = polyline_->getPoint( posidx ).getXY();
		rl_->insertNode( curidx, BinID(mNINT32(pos.x_),
				 mNINT32(pos.y_)) );
	    }

	    int nrremoves = abs(nodeidx-pickstartnodeidx_) - 1;
	    curidx += forward ? 1 : nrremoves;
	    while ( (nrremoves--) > 0 )
		removeNode( curidx );

	    channels_->turnOn( false );
	    ismanip_ = true;
	    updateSel();
	}

	setPolyLineMode( false );
    }
}


void RandomTrackDisplay::pickCB( CallBacker* cb )
{
    if ( !polylinemode_ ) return;

    mCBCapsuleUnpack( const visBase::EventInfo&, eventinfo, cb );

    const bool ctrlclick = OD::ctrlKeyboardButton(eventinfo.buttonstate_);
    const bool shiftclick = OD::shiftKeyboardButton(eventinfo.buttonstate_);

    if ( ctrlclick && shiftclick )
	return;

    const BinID bid = s3dgeom_->transform( eventinfo.worldpickedpos.getXY() );
    Coord3 inlcrlpos( bid.inl(), bid.crl(), eventinfo.worldpickedpos.z_ );

    if ( ctrlclick )
    {
	if ( !eventinfo.pickedobjids.isPresent(markerset_->id()) )
	    return;

	const mVisTrans* transform = markerset_->getDisplayTransformation();

	if ( transform )
	    transform->transform( inlcrlpos );

	if ( !eventinfo.pressed )
	    removePickPos( inlcrlpos );

	eventcatcher_->setHandled();
	return;
    }

    const Coord3 topbotinlcrlpos = !scene_ ? Coord3::udf() :
	    scene_->getTopBottomSurveyPos( eventinfo, shiftclick, shiftclick );

    if ( !mIsUdf(topbotinlcrlpos) )
	inlcrlpos = topbotinlcrlpos;
    else if ( shiftclick || !checkValidPick(eventinfo) )
	return;

    if ( !eventinfo.pressed )
	addPickPos( inlcrlpos );

    eventcatcher_->setHandled();
}


void RandomTrackDisplay::setPolyLineMode( bool yn )
{
    if ( polylinemode_ == yn )
	return;

    polylinemode_ = yn;
    polyline_->removeAllPoints();
    markerset_->clearMarkers();
    polyline_->turnOn( yn );
    markerset_->turnOn( yn );
    dragger_->handleEvents( !yn );

    pickstartnodeidx_ = -1;
    ispicking_ = yn;

    if ( scene_ )
	scene_->blockMouseSelection( yn );
}


bool RandomTrackDisplay::checkValidPick( const visBase::EventInfo& evi ) const
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

	mDynamicCastGet(const visBase::TopBotImage*,tbi,pickedobj );
	mDynamicCastGet(const SurveyObject*,so,pickedobj);

	if ( !tbi && (!so || !so->allowsPicks()) )
	    continue;

	mDynamicCastGet(const HorizonDisplay*,hd,so);
	mDynamicCastGet(const PlaneDataDisplay*,pdd,so);
	validpicksurface = tbi || hd ||
		(pdd && pdd->getOrientation() == OD::ZSlice);

	if ( eventid!=-1 )
	    break;
    }

    return validpicksurface;
}


void RandomTrackDisplay::addPickPos( const Coord3& pos )
{
    const int sz = polyline_->size();
    if ( sz )
    {
	BinID bid( mNINT32(pos.x_), mNINT32(pos.y_) );
	s3dgeom_->snap( bid );

	const Coord3 lastpos = polyline_->getPoint(sz-1);
	BinID lastbid( mNINT32(lastpos.x_), mNINT32(lastpos.y_) );
	s3dgeom_->snap( bid );

	if ( bid == lastbid )
	    removePickPos( sz-1 );
    }

    polyline_->addPoint( pos );
    markerset_->addPos( pos );
}


void RandomTrackDisplay::removePickPos( const Coord3& pickpos )
{
    const int markeridx = markerset_->findClosestMarker( pickpos, true );
    removePickPos( markeridx + polyline_->size()-markerset_->size() );
}


void RandomTrackDisplay::removePickPos( int polyidx )
{
    const int markeridx = polyidx + markerset_->size()-polyline_->size();

    if ( markeridx>=0 && markeridx<markerset_->size() )
	markerset_->removeMarker( markeridx );

    if ( polyidx>=0 && polyidx<polyline_->size() )
	polyline_->removePoint( polyidx );
}


void RandomTrackDisplay::setColor( Color color )
{
    polyline_->getMaterial()->setColor( color );
    markerset_->setMarkersSingleColor( color );
}


bool RandomTrackDisplay::createFromPolyLine()
{
    if ( polyline_->size() < 2 )
	return false;

    ispicking_ = false;		// Makes geomChangeCB(.) keep polyline mode

    TypeSet<BinID> bids;
    for ( int idx=0; idx<polyline_->size(); idx++ )
    {
	Coord pos = polyline_->getPoint( idx ).getXY();
	bids += BinID( (int)pos.x_, (int)pos.y_ );
    }

    rl_->setNodePositions( bids );
    setPolyLineMode( false );
    return true;
}


void RandomTrackDisplay::setPixelDensity( float dpi )
{
    MultiTextureSurveyObject::setPixelDensity( dpi );

    if ( polyline_ )
	polyline_->setPixelDensity( dpi );

    if ( markerset_ )
	markerset_->setPixelDensity( dpi );
}


void RandomTrackDisplay::draggerMoveFinished( CallBacker* cb )
{
    Interval<float> zrg = dragger_->getDepthRange();
    s3dgeom_->snapZ( zrg.start );
    s3dgeom_->snapZ( zrg.stop );
    dragger_->setDepthRange( zrg );

    finishNodeMoveInternal();
    poschanged_.trigger();
    selnodeidx_ = mUdf(int);
}


void RandomTrackDisplay::finishNodeMoveInternal()
{
    if ( originalresolution_ >= 0 )
    {
	resolution_ = originalresolution_;
	originalresolution_ = -1;
    }

    if ( interactivetexturedisplay_ )
	ismanip_ = true;

    interactivetexturedisplay_ = false;
    updateSel();
}


void RandomTrackDisplay::draggerRightClick( CallBacker* )
{
    triggerRightClick( dragger_->rightClickedEventInfo() );
}

} // namespace visSurvey
