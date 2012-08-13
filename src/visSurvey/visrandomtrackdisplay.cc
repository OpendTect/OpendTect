/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2003
 ________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: visrandomtrackdisplay.cc,v 1.135 2012-08-13 04:04:40 cvsaneesh Exp $";


#include "visrandomtrackdisplay.h"

#include "array2dresample.h"
#include "arrayndimpl.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "coltabmapper.h"
#include "iopar.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "interpol1d.h"
#include "randomlinegeom.h"
#include "scaler.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "ptrman.h"

#include "viscolortab.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visevent.h"
#include "vishorizondisplay.h"
#include "vismaterial.h"
#include "vismarker.h"
#include "vismultitexture2.h"
#include "vispolyline.h"
#include "visplanedatadisplay.h"
#include "visrandomtrack.h"
#include "visrandomtrackdragger.h"
#include "vissplittexturerandomline.h"
#include "vistexturechannels.h"
#include "vistexturechannel2rgba.h"
#include "vistexturecoords.h"
#include "vistransform.h"
#include "zaxistransform.h"


#include <math.h>

mCreateFactoryEntry( visSurvey::RandomTrackDisplay );

namespace visSurvey
{

const char* RandomTrackDisplay::sKeyTrack() 	    { return "Random track"; }
const char* RandomTrackDisplay::sKeyNrKnots() 	    { return "Nr. Knots"; }
const char* RandomTrackDisplay::sKeyKnotPrefix()    { return "Knot "; }
const char* RandomTrackDisplay::sKeyDepthInterval() { return "Depth Interval"; }
const char* RandomTrackDisplay::sKeyLockGeometry()  { return "Lock geometry"; }

RandomTrackDisplay::RandomTrackDisplay()
    : MultiTextureSurveyObject(true)
    , triangles_(visBase::SplitTextureRandomLine::create())
    , dragger_(visBase::RandomTrackDragger::create())
    , polyline_(visBase::PolyLine::create())
    , markergrp_(visBase::DataObjectGroup::create())
    , polylinemode_(false)
    , knotmoving_(this)
    , moving_(this)
    , selknotidx_(-1)
    , ismanip_(false)
    , datatransform_(0)
    , lockgeometry_(false)
    , eventcatcher_(0)
{
    TypeSet<int> randomlines;
    visBase::DM().getIds( typeid(*this), randomlines );
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
    int channelidx = 
	childIndex( channels_->getInventorNode() );
    insertChild( channelidx, dragger_->getInventorNode() );

    dragger_->motion.notify( mCB(this,visSurvey::RandomTrackDisplay,knotMoved));

    triangles_->ref();
    addChild( triangles_->getInventorNode() );
    
    polyline_->ref();
    addChild( polyline_->getInventorNode() );
    polyline_->setMaterial( visBase::Material::create() );
   
    markergrp_->ref();
    addChild( markergrp_->getInventorNode() );

    const StepInterval<float>& survinterval = SI().zRange(true);
    const StepInterval<float> inlrange( SI().sampling(true).hrg.start.inl,
	    				SI().sampling(true).hrg.stop.inl,
					SI().inlStep() );
    const StepInterval<float> crlrange( SI().sampling(true).hrg.start.crl,
	    				SI().sampling(true).hrg.stop.crl,
	    				SI().crlStep() );

    const BinID start( mNINT32(inlrange.center()), mNINT32(crlrange.start) );
    const BinID stop(start.inl, mNINT32(crlrange.stop) );

    addKnot( start );
    addKnot( stop );

    setDepthInterval( Interval<float>( survinterval.start,
				       survinterval.stop ));

    dragger_->setLimits(
	    Coord3( inlrange.start, crlrange.start, survinterval.start ),
	    Coord3( inlrange.stop, crlrange.stop, survinterval.stop ),
	    Coord3( inlrange.step, crlrange.step, survinterval.step ) );

    const int baselen = mNINT32((inlrange.width()+crlrange.width())/2);
    
    dragger_->setSize( Coord3(baselen/50,baselen/50,survinterval.width()/50) );
}


RandomTrackDisplay::~RandomTrackDisplay()
{
    setSceneEventCatcher( 0 );
    triangles_->unRef();
    dragger_->unRef();
    removeChild( polyline_->getInventorNode() );
    polyline_->unRef();
    markergrp_->removeAll();
    removeChild( markergrp_->getInventorNode() );
    markergrp_->unRef();
    deepErase( cache_ );
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpman.release( datapackids_[idx] );
}


CubeSampling RandomTrackDisplay::getCubeSampling( int attrib ) const
{
    CubeSampling cs( false );
    TypeSet<BinID> knots;
    getAllKnotPos( knots );
    for ( int idx=0; idx<knots.size(); idx++ )
	cs.hrg.include( knots[idx] );

    cs.zrg.setFrom( getDepthInterval() );
    return cs;
}


void RandomTrackDisplay::setDepthInterval( const Interval<float>& intv )
{ 
    const Interval<float>& curint = getDepthInterval();
    if ( mIsEqual(curint.start,intv.start, 1e-3 ) &&
	 mIsEqual(curint.stop,intv.stop, 1e-3 ) )
	return;

    triangles_->setDepthRange( intv );
    dragger_->setDepthRange( intv );

    moving_.trigger();
}


const Interval<float>& RandomTrackDisplay::getDepthInterval() const
{
    return triangles_->getDepthRange();
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
}


Interval<float> RandomTrackDisplay::getDataTraceRange() const
{
    //TODO Adapt if ztransform is present
    return triangles_->getDepthRange();
}


int RandomTrackDisplay::nrKnots() const
{ return knots_.size(); }


void RandomTrackDisplay::addKnot( const BinID& bid )
{
    const BinID sbid = snapPosition( bid );
    if ( checkPosition(sbid) )
    {
	knots_ += sbid;
	triangles_->setDepthRange( getDataTraceRange() );
	triangles_->setLineKnots( knots_ );	
	dragger_->setKnot( knots_.size()-1, Coord(sbid.inl,sbid.crl) );
	moving_.trigger();
    }
}


void RandomTrackDisplay::insertKnot( int knotidx, const BinID& bid )
{
    const BinID sbid = snapPosition(bid);
    if ( checkPosition(sbid) )
    {
	knots_.insert( knotidx, sbid );
	triangles_->setDepthRange( getDataTraceRange() );
	triangles_->setLineKnots( knots_ );	
	dragger_->insertKnot( knotidx, Coord(sbid.inl,sbid.crl) );
	for ( int idx=0; idx<nrAttribs(); idx++ )
	    if ( cache_[idx] ) setData( idx, *cache_[idx] );

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

	triangles_->setDepthRange( getDataTraceRange() );
	triangles_->setLineKnots( knots_ );	
	dragger_->setKnot( knotidx, Coord(sbid.inl,sbid.crl) );
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
	const int nrsteps = mMAX( abs(biddif.inl)/SI().inlStep(), 
				  abs(biddif.crl)/SI().crlStep() );
	const Coord dest = SI().transform( knots[idx] );
	const Coord crddif = SI().transform(prev) - dest;
	for ( int step=0; step<nrsteps; step++ )
	{
	    const BinID newknot = SI().transform( dest+(crddif*step)/nrsteps );
	    if ( uniqueknots.indexOf(newknot) < 0 )
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
	    knots_.remove( 0 );
	    dragger_->removeKnot( 0 );
	}

	for ( int idx=0; idx<uniquebids.size(); idx++ )
	{
	    const BinID sbid = snapPosition( uniquebids[idx] );
	    if ( checkPosition(sbid) )
	    {
		knots_ += sbid;
		dragger_->setKnot( knots_.size()-1, Coord(sbid.inl,sbid.crl) );
	    }
	}

	triangles_->setDepthRange( getDataTraceRange() );
	triangles_->setLineKnots( knots_ );	
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

    knots_.remove(knotidx);
    triangles_->setLineKnots( knots_ );	
    dragger_->removeKnot( knotidx );
}


void RandomTrackDisplay::removeAllKnots()
{
    for ( int idx=0; idx<knots_.size(); idx++ )
    	dragger_->removeKnot( idx );
    
    knots_.erase();	
    triangles_->setLineKnots( knots_ );
}


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids ) const
{ getDataTraceBids( bids, 0 ); }


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids,
       					   TypeSet<int>* segments ) const
{
    const_cast<RandomTrackDisplay*>(this)->trcspath_.erase(); 
    TypeSet<BinID> knots; getAllKnotPos( knots );
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
	const int nrinl = int(abs(stop.inl-start.inl) / SI().inlStep() + 1);
	const int nrcrl = int(abs(stop.crl-start.crl) / SI().crlStep() + 1);
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

    SeisTrcBuf tmpbuf( dprdm->seisBuf() );
    setTraceData( attrib, tmpbuf, tr );

    DataPack::ID oldid = datapackids_[attrib];
    datapackids_[attrib] = dpid;
    dpman.release( oldid );
    return true;
}


DataPack::ID RandomTrackDisplay::getDataPackID( int attrib ) const
{
    return datapackids_[attrib];
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


void RandomTrackDisplay::setData( int attrib, const SeisTrcBuf& trcbuf )
{
    const int nrtrcs = trcbuf.size();
    if ( !nrtrcs )
    {
	channels_->setUnMappedData( attrib, 0, 0, OD::CopyPtr, 0 );
	channels_->turnOn( false );
	return;
    }

    const Interval<float> zrg = getDataTraceRange();
    const float step = trcbuf.get(0)->info().sampling.step;
    const int nrsamp = mNINT32( zrg.width() / step ) + 1;

    TypeSet<BinID> path;
    getDataTraceBids( path );

    const int nrslices = trcbuf.get(0)->nrComponents();
    channels_->setNrVersions( attrib, nrslices );

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, array,
	    Array2DImpl<float>( path.size(), nrsamp ) );
    if ( !array->isOK() )
	return;

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    for ( int sidx=0; sidx<nrslices; sidx++ )
    {
	array->setAll( mUdf(float) );
	float* dataptr = array->getData();

	for ( int posidx=path.size()-1; posidx>=0; posidx-- )
	{
	    const BinID bid = path[posidx];
	    const int trcidx = trcbuf.find( bid, false );
	    if ( trcidx<0 )
		continue;

	    const SeisTrc* trc = trcbuf.get( trcidx );
	    if ( !trc || sidx>trc->nrComponents() )
		continue;

	    float* arrptr = dataptr + array->info().getOffset( posidx, 0 );

	    if ( !datatransform_ )
	    {
		for ( int ids=0; ids<nrsamp; ids++ )
		{
		    const float ctime = zrg.start + ids*step;
		    if ( !trc->dataPresent(ctime) )
			continue;

		    arrptr[ids] = trc->getValue(ctime,sidx);
		}
	    }
	    else
	    {
		//todo
	    }
	}

	const int sz0 = array->info().getSize(0) * (resolution_+1);
	const int sz1 = array->info().getSize(1) * (resolution_+1);
	channels_->setSize( 1, sz0, sz1 );
	
	if ( resolution_==0 )
	    channels_->setUnMappedData(attrib,sidx,dataptr,OD::CopyPtr,0);
	else
	{
	    mDeclareAndTryAlloc( float*, arr, float[sz0*sz1] );
	    Array2DReSampler<float,float>
		resampler( *array, arr, sz0, sz1, true );
	    resampler.setInterpolate( true );
	    resampler.execute();
	    channels_->setUnMappedData(attrib,sidx,arr,OD::TakeOverPtr,0);
	}

	triangles_->setDepthRange( zrg );
	triangles_->setTexturePathAndPixels( path, resolution_+1, sz1 );
    }
    
    channels_->turnOn( true );
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

    const BinID newpos = proposeNewPos(knotnr);
    if ( knotnr==nrKnots() )
	addKnot( newpos );
    else
	insertKnot( knotnr, newpos );
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
	res.inl /= 2;
	res.crl /= 2;
    }

    res.inl = mMIN( SI().inlRange(true).stop, res.inl );
    res.inl = mMAX( SI().inlRange(true).start, res.inl );
    res.crl = mMIN( SI().crlRange(true).stop, res.crl );
    res.crl = mMAX( SI().crlRange(true).start, res.crl );

    SI().snap(res, BinID(0,0) );

    return res;
}


bool RandomTrackDisplay::isManipulated() const
{
    return ismanip_;
}

 
void RandomTrackDisplay::acceptManipulation()
{
    setDepthInterval( dragger_->getDepthRange() );
    for ( int idx=0; idx<nrKnots(); idx++ )
    {
	const Coord crd = dragger_->getKnot(idx);
	setKnotPos( idx, BinID( mNINT32(crd.x), mNINT32(crd.y) ));
    }

    ismanip_ = false;
}


void RandomTrackDisplay::resetManipulation()
{
    dragger_->setDepthRange( getDepthInterval() );
    for ( int idx=0; idx<nrKnots(); idx++ )
    {
	const BinID bid = getKnotPos(idx);
	dragger_->setKnot(idx, Coord(bid.inl,bid.crl));
    }

    ismanip_ = false;
    dragger_->turnOn( false );
}


void RandomTrackDisplay::showManipulator( bool yn )
{
    if ( polylinemode_ ) return;

    if ( lockgeometry_ ) yn = false;
    if ( !yn ) dragger_->showFeedback( false );
    dragger_->turnOn( yn );
}
   

bool RandomTrackDisplay::isManipulatorShown() const
{ return false; /* track->isDraggerShown();*/ }


BufferString RandomTrackDisplay::getManipulationString() const
{
    BufferString str;
    int knotidx = getSelKnotIdx();
    if ( knotidx >= 0 )
    {
	BinID binid  = getManipKnotPos( knotidx );
	str = "Node "; str += knotidx;
	str += " Inl/Crl: ";
	str += binid.inl; str += "/"; str += binid.crl;
    }

    return str;
}
 

void RandomTrackDisplay::knotMoved( CallBacker* cb )
{
    ismanip_ = true;
    mCBCapsuleUnpack(int,sel,cb);
    
    selknotidx_ = sel;
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
	if ( getKnotPos(idx) == binid )
	    return false;

    return true;
}


BinID RandomTrackDisplay::snapPosition( const BinID& binid_ ) const
{
    BinID binid( binid_ );
    const HorSampling& hs = SI().sampling(true).hrg;
    if ( binid.inl < hs.start.inl ) binid.inl = hs.start.inl;
    if ( binid.inl > hs.stop.inl ) binid.inl = hs.stop.inl;
    if ( binid.crl < hs.start.crl ) binid.crl = hs.start.crl;
    if ( binid.crl > hs.stop.crl ) binid.crl = hs.stop.crl;

    SI().snap( binid, BinID(0,0) );
    return binid;
}


SurveyObject::AttribFormat RandomTrackDisplay::getAttributeFormat( int ) const
{ return SurveyObject::Traces; }


#define mFindTrc(inladd,crladd) \
    if ( idx<0 ) \
    { \
	BinID bid( binid.inl + step.inl * (inladd),\
		   binid.crl + step.crl * (crladd) );\
	idx = bids.indexOf( bid ); \
    }
Coord3 RandomTrackDisplay::getNormal( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
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

    const visBase::Coordinates* coords = triangles_->getCoordinates();
    const Coord pos0 = coords->getPos( segments[idx]*2 );
    const Coord pos1 = coords->getPos( segments[idx]*2+2 );
    const BinID bid0( mNINT32(pos0.x), mNINT32(pos0.y));
    const BinID bid1( mNINT32(pos1.x), mNINT32(pos1.y));

    const Coord dir = SI().transform(bid0)-SI().transform(bid1);
    const float dist = dir.abs();

    if ( dist<=mMIN(SI().inlDistance(),SI().crlDistance()) )
	return Coord3::udf();

    return Coord3( dir.y, -dir.x, 0 );
}

#undef mFindTrc


float RandomTrackDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    TypeSet<BinID> bids;
    getDataTraceBids( bids );
    if ( bids.indexOf(binid)==-1 )
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
    RandomTrackDisplay* rtd = create();

    rtd->setDepthInterval( getDataTraceRange() );
    TypeSet<BinID> positions;
    for ( int idx=0; idx<nrKnots(); idx++ )
	positions += getKnotPos( idx );
    rtd->setKnotPositions( positions );

    rtd->lockGeometry( isGeometryLocked() );

    return rtd;
}


void RandomTrackDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par, saveids );

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


int RandomTrackDisplay::usePar( const IOPar& par )
{
    par.getYN( sKeyLockGeometry(), lockgeometry_ );
    const int res =  visSurvey::MultiTextureSurveyObject::usePar( par );
    if ( res != 1 ) return res;

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

    return 1;
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
	bid.inl = reqbid.inl + step.inl * (inladd); \
	bid.crl = reqbid.crl + step.crl * (crladd); \
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
}


void RandomTrackDisplay::removeCache( int attrib )
{
    delete cache_[attrib];
    cache_.remove( attrib );

    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_.remove( attrib );
}


void RandomTrackDisplay::swapCache( int a0, int a1 )
{
    cache_.swap( a0, a1 );
    datapackids_.swap( a0, a1 );
}


void RandomTrackDisplay::emptyCache( int attrib )
{
    if ( cache_[attrib] ) delete cache_[attrib];
	cache_.replace( attrib, 0 );

    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_[attrib] = DataPack::cNoID();
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
	Coord3 pos = eventinfo.worldpickedpos;

	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		    !OD::altKeyboardButton(eventinfo.buttonstate_) &&
		    !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	{
	    removePickPos( eventinfo.pickedobjids );
	    eventcatcher_->setHandled();
	    return;
	}

	if ( !checkValidPick( eventinfo, pos ) )
	    return;
	BinID bid = SI().transform( pos );
	pos.x = bid.inl; pos.y = bid.crl;
	setPickPos( pos );
	eventcatcher_->setHandled();
    }
} 


void RandomTrackDisplay::setPolyLineMode( bool mode )
{
    polylinemode_ = mode;
    polyline_->turnOn( polylinemode_ );
    for ( int idx=0; idx<markergrp_->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergrp_->getObject(idx));
	if ( marker ) marker->turnOn( polylinemode_ );
    }

    triangles_->turnOn( !polylinemode_ );
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
	if ( eventid==-1 && pickedobj->pickable() )
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
		(pdd && pdd->getOrientation() == PlaneDataDisplay::Zslice);

	if ( eventid!=-1 )
	    break;
    }

    return validpicksurface;
 }


void RandomTrackDisplay::setPickPos( const Coord3& pos )
{
    polyline_->addPoint( pos );
    visBase::Marker* marker = visBase::Marker::create();
    marker->setMaterial( visBase::Material::create() );
    marker->getMaterial()->setColor( polyline_->getMaterial()->getColor() );
    marker->setCenterPos( pos );
    markergrp_->addObject( marker );
    marker->turnOn( true );
}
  

void RandomTrackDisplay::removePickPos( const TypeSet<int>& ids )
{
    for ( int idx=0; idx<markergrp_->size(); idx++ )
    {
	mDynamicCastGet(const visBase::Marker*,marker,
			markergrp_->getObject(idx));
	if ( marker && ids.isPresent(marker->id()) )
	{ 
	    polyline_->removePoint( idx );
	    markergrp_->removeObject( idx );
	    break;
	}
    }
 
}


void RandomTrackDisplay::setColor( Color color )
{
    polyline_->getMaterial()->setColor( color );
    for ( int idx=0; idx<markergrp_->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergrp_->getObject(idx));
	if ( marker ) marker->getMaterial()->setColor( color );
    }
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

} // namespace visSurvey
