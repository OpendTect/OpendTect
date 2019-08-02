/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/


#include "visosg.h"
#include "vishorizonsection.h"
#include "vishordatahandler.h"
#include "vishorizontexturehandler.h"
#include "vishorizonsectiontile.h"
#include "vishorthreadworks.h"
#include "vishortilescreatorandupdator.h"
#include "vishorizonsectiontileglue.h"

#include "binidsurface.h"
#include "uistrings.h"
#include "survinfo.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include <osgGeo/LayeredTexture>

#include <osg/Switch>
#include <osgUtil/CullVisitor>


mCreateFactoryEntry( visBase::HorizonSection );

namespace visBase
{

class TileCoordinatesUpdator: public ParallelTask
{
public:
    TileCoordinatesUpdator(HorizonSection* hrsecion, const od_int64 size,
		         const Transformation* trans, bool transback= false);

    od_int64	totalNr() const { return totaltiles_; }

protected:
    bool	doWork(od_int64 start, od_int64 stop, int);
    od_int64	nrIterations() const { return totaltiles_; }

private:
    void	updateCoordinates(HorizonSectionTile*);
    HorizonSection*		hrsection_;
    Threads::Atomic<od_int64>	totaltiles_;
    ConstRefMan<Transformation>	transform_;
    bool			transback_;
};


TileCoordinatesUpdator::TileCoordinatesUpdator( HorizonSection* hrsecion,
    const od_int64 size, const Transformation* trans, bool transback)
    : hrsection_( hrsecion )
    , totaltiles_( size )
    , transform_( trans )
    , transback_( transback )
{}


bool TileCoordinatesUpdator::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !transform_ || !hrsection_ )
	return false;

    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	HorizonSectionTile* tile = hrsection_->getTitle( idx );
	updateCoordinates( tile );
    }
    return true;
}


void TileCoordinatesUpdator::updateCoordinates( HorizonSectionTile* tile )
{
    if ( !tile ) return;
    const int nrcoords = hrsection_->nrcoordspertileside_*
			 hrsection_->nrcoordspertileside_;

    osg::Vec3Array* vertices = mGetOsgVec3Arr( tile->getOsgVertexArray() );

    for ( int idx=0; idx<nrcoords; idx++ )
    {
	if ( !mIsOsgVec3Def((*vertices)[idx]) )
	    continue;

	Coord3 pos = Conv::to<Coord3>( (*vertices)[idx] );
	if ( !transback_ )
	{
	    transform_->transform( pos );
	    tile->computeNormal(
		idx,(*mGetOsgVec3Arr(tile->getNormals()))[idx] );
	}
	else
	    transform_->transformBack( pos );

	(*vertices)[idx] = Conv::to<osg::Vec3f>(pos);

	if ( !transback_ && mIsOsgVec3Def((*vertices)[idx]) )
	    tile->bbox_.expandBy( (*vertices)[idx] );
    }

}


class HorizonSection::TextureCallbackHandler :
				    public osgGeo::LayeredTexture::Callback
{
public:
    TextureCallbackHandler( HorizonSection& section )
	: horsection_( section )
    {}

    virtual void requestRedraw() const		{ horsection_.forceRedraw(); }

protected:
    HorizonSection&		horsection_;
};


class HorizonSection::NodeCallbackHandler : public osg::NodeCallback
{
public:
    NodeCallbackHandler( HorizonSection* section );
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

protected:
    const osg::Vec3 getProjectionDirection( const osgUtil::CullVisitor* );
    bool eyeChanged( const osg::Vec3 );

    HorizonSection* hrsection_;
    osg::Vec3 projectiondirection_;
    int initialtimes_;
};


HorizonSection::NodeCallbackHandler::NodeCallbackHandler(
						HorizonSection* hrsection )
    : hrsection_( hrsection )
    , projectiondirection_( osg::Vec3( 0,0,0 ) )
    , initialtimes_( 0 )
{
}


void HorizonSection::NodeCallbackHandler::operator()( osg::Node* node,
						      osg::NodeVisitor* nv )
{
    hrsection_->executePendingUpdates();

    if( nv->getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR )
    {
	hrsection_->forceRedraw( false );

	if ( hrsection_->getOsgTexture()->needsRetiling() )
	{
	    hrsection_->getOsgTexture()->reInitTiling();
	    hrsection_->hortexturehandler_->updateTileTextureOrigin();
	}
    }
    else if( nv->getVisitorType()==osg::NodeVisitor::CULL_VISITOR )
    {
	if ( hrsection_->tiles_.totalSize() < 1 )
	    return;

	osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
	const osg::Vec3 projectiondirection = getProjectionDirection( cv );

	if ( hrsection_->getOsgTexture()->getSetupStateSet() )
	    cv->pushStateSet( hrsection_->getOsgTexture()->getSetupStateSet() );

	if( hrsection_->forceupdate_ || eyeChanged( projectiondirection ) ||
	    initialtimes_<cMinInitialTimes )
	 // during initialization, we update two times,first time is lowest
	 // resolution, second is current resolution
	{
	    const osg::CullStack* cs = dynamic_cast<osg::CullStack*>(nv);
	    hrsection_->updateAutoResolution( cs );
	    hrsection_->updatePrimitiveSets();
	    projectiondirection_ = projectiondirection;
	    if ( initialtimes_ < cMinInitialTimes )
	    {
		initialtimes_++;
		hrsection_->forceRedraw( true );
	    }

	}

	traverse( node,nv );
	if ( hrsection_->getOsgTexture()->getSetupStateSet() )
	    cv->popStateSet();
    }
}


const osg::Vec3 HorizonSection::NodeCallbackHandler::getProjectionDirection(
					    const osgUtil::CullVisitor* cv )
{
     osg::Vec3 projectiondirection( 0, 0, 0 );
     if ( cv )
	projectiondirection = cv->getEyePoint();

     return projectiondirection;
}


bool HorizonSection::NodeCallbackHandler::eyeChanged( const osg::Vec3 projdir )
{
    bool retval = projdir!=projectiondirection_ ? true : false;
    initialtimes_ = retval ? 0 : initialtimes_;
    return retval;
}


//===========================================================================


HorizonSection::HorizonSection()
    : VisualObjectImpl( false )
    , transformation_( 0 )
    , geometry_( 0 )
    , origin_( 0, 0 )
    , displayrrg_( -1, -1, 0 )
    , displaycrg_( -1, -1, 0 )
    , texturerowrg_( 0, mUdf(int), 1 )
    , texturecolrg_( 0, mUdf(int), 1 )
    , userchangedisplayrg_( false )
    , tiles_( 0, 0 )
    , desiredresolution_( cNoneResolution )
    , tesselationlock_( false )
    , nrcoordspertileside_( 0 )
    , tilesidesize_( 0 )
    , totalnormalsize_( 0 )
    , lowestresidx_( 0 )
    , nrhorsectnrres_( 0 )
    , osghorizon_( new osg::Group )
    , forceupdate_( false )
    , wireframedisplayed_( 0 )
    , hordatahandler_( new HorizonSectionDataHandler( this ) )
    , hortexturehandler_( new HorizonTextureHandler( this ) )
    , hortilescreatorandupdator_( new HorTilesCreatorAndUpdator(this) )
    , nodecallbackhandler_( 0 )
    , texturecallbackhandler_( 0 )
    , isredrawing_( false )
    , zaxistransform_( 0 )
    , useneighbors_( true )
    , linewidth_(1)
{
    setLockable();
    osghorizon_->ref();

    nodecallbackhandler_ = new NodeCallbackHandler( this );
    nodecallbackhandler_->ref();
    osghorizon_->setCullCallback( nodecallbackhandler_ );
    texturecallbackhandler_ = new TextureCallbackHandler( *this );
    texturecallbackhandler_->ref();
    hortexturehandler_->getOsgTexture()->addCallback( texturecallbackhandler_ );

    addChild( osghorizon_ );

    hortexturehandler_->ref();
    addChild( hortexturehandler_->getOsgNode() );
    hordatahandler_->ref();
    hortilescreatorandupdator_->ref();

    queueid_ = Threads::WorkManager::twm().addQueue(
		Threads::WorkManager::Manual, "HorizonSection" );
}


HorizonSection::~HorizonSection()
{
    detachAllNotifiers();

    hortexturehandler_->getOsgTexture()->removeCallback(
						    texturecallbackhandler_ );
    texturecallbackhandler_->unref();

    forceRedraw( false );
    osghorizon_->setCullCallback( 0 );
    nodecallbackhandler_->unref();

    osghorizon_->unref();
    hortexturehandler_->unRef();

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.totalSize(); idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;

	writeLock();
	removeChild( tileptrs[idx]->osgswitchnode_ );
	delete tileptrs[idx];
	writeUnLock();
    }

    if ( transformation_ ) transformation_->unRef();

    hordatahandler_->unRef();
    hortilescreatorandupdator_->unRef();

    Threads::WorkManager::twm().removeQueue( queueid_, false );
}


void HorizonSection::forceRedraw( bool yn )
{
    Threads::Locker lckr( redrawlock_, Threads::Locker::WriteLock );
    if ( isredrawing_ != yn )
    {
	isredrawing_ = yn;
	osghorizon_->setUpdateCallback( yn ? nodecallbackhandler_ : 0 );
    }
}


bool HorizonSection::executePendingUpdates()
{
    return Threads::WorkManager::twm().executeQueue( queueid_ );
}


void HorizonSection::setUpdateVar( bool& variable, bool yn )
{
    if ( yn )
	forceRedraw( true );

    variable = yn;
}


void HorizonSection::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ == nt )
	return;

    HorizonSectionTile** tileptrs = tiles_.getData();

    if ( transformation_ )
    {
	if ( tileptrs && tiles_.totalSize()>0 )
	{
	    spinlock_.lock();
	    TileCoordinatesUpdator backupdator(
		this, od_int64(tiles_.totalSize()),
		transformation_,true);
	    backupdator.execute();
	    spinlock_.unLock();
	}
	transformation_->unRef();
    }

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();

    if ( (transformation_ && tileptrs) || (tiles_.totalSize()>0) )
    {
	for ( int idx=0; idx<tiles_.totalSize(); idx++ )
	    if ( tileptrs[idx] ) tileptrs[idx]->bbox_.init();
	spinlock_.lock();
        TileCoordinatesUpdator forwardupdator(
	    this, od_int64(tiles_.totalSize()),
	    transformation_,false );
	forwardupdator.execute();
	spinlock_.unLock();
    }

    for ( int idx=0; idx<tiles_.totalSize(); idx++ )
    {
	if ( tileptrs[idx] )
	{
	    tileptrs[idx]->righttileglue_->setDisplayTransformation(
		transformation_ );
	    tileptrs[idx]->bottomtileglue_->setDisplayTransformation(
		transformation_ );
	    tileptrs[idx]->dirtyGeometry();
	}
    }

}


HorizonSectionTile* HorizonSection::getTitle(int idx)
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    if ( !tileptrs || (idx>=tiles_.totalSize()) )
	return 0;
    return tileptrs[idx];
}


void HorizonSection::setWireframeColor( Color col )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    spinlock_.lock();
    for ( int idx=0; idx<tiles_.totalSize(); idx++ )
    {
	if ( tileptrs[idx] )
	    tileptrs[idx]->setWireframeColor( col );
    }
    spinlock_.unLock();
}


void HorizonSection::setLineWidth( int width )
{
    linewidth_ = width;
    HorizonSectionTile** tileptrs = tiles_.getData();
    spinlock_.lock();
    for ( int idx=0; idx<tiles_.totalSize(); idx++ )
    {
	if ( tileptrs[idx] )
	    tileptrs[idx]->setLineWidth( width );
    }
    spinlock_.unLock();
}


int HorizonSection::getLineWidth() const
{
    return linewidth_;
}


void HorizonSection::configSizeParameters()
{
    mDefineRCRange(,);
    nrcoordspertileside_ = cNumberNodePerTileSide;
    nrhorsectnrres_ = cMaximumResolution;
    tilesidesize_ = nrcoordspertileside_ - 1;
    lowestresidx_ = nrhorsectnrres_-1;

    totalnormalsize_ = 0;
    for ( char res=0; res<nrhorsectnrres_; res++ )
    {
	spacing_ += !res ? 1 : 2 * spacing_[res-1];
	normalsidesize_ += tilesidesize_ / spacing_[res] + 1;
	nrcells_ += ( normalsidesize_[res] - (res ? 1 : 0) ) *
			( normalsidesize_[res] - (res ? 1 : 0) );
	normalstartidx_ += totalnormalsize_;
	totalnormalsize_ += normalsidesize_[res] * normalsidesize_[res];
    }

}


void HorizonSection::setSurface( Geometry::BinIDSurface* surf, bool connect,
				 TaskRunner* tskr )
{
    if ( !surf ) return;

    if ( connect )
    {
	geometry_ = surf;
	mAttachCB( geometry_->movementNotifier(),
					    HorizonSection::surfaceChangeCB );
	mAttachCB( geometry_->nrpositionNotifier(),
					    HorizonSection::surfaceChangeCB );
    }

    configSizeParameters();
    rowdistance_ = geometry_->rowRange().step*SI().inlDistance();
    coldistance_ = geometry_->colRange().step*SI().crlDistance();
    surfaceChange( 0, tskr );
}


void HorizonSection::setDisplayRange( const StepInterval<int>& rrg,
				      const StepInterval<int>& crg )
{
    if ( rrg.isUdf() || crg.isUdf() || (displayrrg_==rrg && displaycrg_==crg) )
	return;

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.totalSize(); idx++ )
    {
	if ( tileptrs[idx] )
	{
	    writeLock();
	    removeChild( tileptrs[idx]->osgswitchnode_ );
	    delete tileptrs[idx];
	    tileptrs[idx] = 0;
	    writeUnLock();
	}
    }

    userchangedisplayrg_ = true;
    displayrrg_ = rrg;
    displaycrg_ = crg;
    origin_ = RowCol( displayrrg_.start, displaycrg_.start );
    rowdistance_ = displayrrg_.step*SI().inlDistance();
    coldistance_ = displaycrg_.step*SI().crlDistance();

    configSizeParameters();
    surfaceChange( 0, 0 );

    setResolution( desiredresolution_, 0 );
}


void HorizonSection::setTextureRange( const StepInterval<int>& rowrg,
				      const StepInterval<int>& colrg )
{
    texturerowrg_ = rowrg;
    texturecolrg_ = colrg;
}


StepInterval<int> HorizonSection::displayedRowRange() const
{
   if ( userchangedisplayrg_ )
       return displayrrg_;

   if ( geometry_ )
       return geometry_->rowRange();
   else
       return StepInterval<int>(0, 0, 0);
}


StepInterval<int> HorizonSection::displayedColRange() const
{
   if ( userchangedisplayrg_ )
       return displaycrg_;

   if ( geometry_ )
       return geometry_->colRange();
   else
       return StepInterval<int>(0, 0, 0);
}


void HorizonSection::surfaceChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, gpids, cb );

    if ( gpids )
    {
	updatelock_.lock();
	surfaceChange( gpids, 0 );
	updatelock_.unLock();
    }

}

char HorizonSection::currentResolution() const
{ return desiredresolution_; }


char  HorizonSection::nrResolutions() const
{ return nrhorsectnrres_; }


const mVisTrans* HorizonSection::getDisplayTransformation() const
{ return transformation_; }


void HorizonSection::enableGeometryTypeDisplay( GeometryType type, bool yn )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.totalSize();

    Threads::MutexLocker renderemutex( updatelock_ );
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->enableGeometryTypeDisplay(
	    type, yn );

    wireframedisplayed_ = ( ( type == WireFrame ) && yn ) ?
			    true : false;

}


void HorizonSection::turnOsgOn( bool yn )
{  turnOn( yn ); }


class HorizonSectionUpdater : public Task
{
public:
HorizonSectionUpdater( HorTilesCreatorAndUpdator& upd,
		       const TypeSet<GeomPosID>& gpids )
    : gpids_(gpids)
    , updater_(upd)
{}

bool execute()
{ updater_.updateTiles( &gpids_, 0 ); return true; }

    TypeSet<GeomPosID> gpids_;
    HorTilesCreatorAndUpdator& updater_;
};


void HorizonSection::surfaceChange( const TypeSet<GeomPosID>* gpids,
				    TaskRunner* tskr )
{
    if ( !geometry_ || !geometry_->getArray() )
	return;

    if ( hordatahandler_->zaxistransform_ &&
	hordatahandler_->zaxistransformvoi_!=-1 )
    {
	updateZAxisVOI();
	ExistingTaskRunnerProvider trprov( tskr );
	if ( !hordatahandler_->zaxistransform_->loadDataIfMissing(
	      hordatahandler_->zaxistransformvoi_,trprov) )
	    return;
    }

    if ( !gpids || !tiles_.getSize(0) || !tiles_.getSize(1) )
	hortilescreatorandupdator_->createAllTiles( tskr );
    else
    {
	if ( isVisualizationThread() )
	    hortilescreatorandupdator_->updateTiles( gpids, tskr );
	else
	{
	    HorizonSectionUpdater* updtask =
		new HorizonSectionUpdater(*hortilescreatorandupdator_,*gpids);
	    Threads::WorkManager::twm().addWork(
		Threads::Work(*updtask,true), 0, queueid_ );
	}
    }
}


void HorizonSection::setZAxisTransform( ZAxisTransform* zt, TaskRunner* )
{
    if (!hordatahandler_) return;
    hordatahandler_->setZAxisTransform( zt );

    zaxistransform_ = zt;

    if ( geometry_ )
    {
	CBCapsule<const TypeSet<GeomPosID>*> caps( 0, geometry_ );
	surfaceChangeCB( &caps );
    }
}


void HorizonSection::updateZAxisVOI()
{ hordatahandler_->updateZAxisVOI(); }


void HorizonSection::getDataPositions( DataPointSet& res, double zshift,
    int sid, TaskRunner* tskr ) const
{ hordatahandler_->generatePositionData( res, zshift, sid ); }


void HorizonSection::setColTabSequence(int channel, const ColTab::Sequence& se)
{ hortexturehandler_->setColTabSequence( channel , se ); }


const ColTab::Sequence& HorizonSection::getColTabSequence( int channel ) const
{
    return hortexturehandler_->getColTabSequence( channel );
}


void HorizonSection::setColTabMapper( int channel,
    const ColTab::Mapper& mapper, const TaskRunnerProvider& trprov )
{ hortexturehandler_->setColTabMapper( channel, mapper, trprov ); }


const ColTab::Mapper& HorizonSection::getColTabMapper( int ch ) const
{ return hortexturehandler_->getColTabMapper( ch ); }


void HorizonSection::setTransparency( int ch, unsigned char yn )
{ hortexturehandler_->setTransparency( ch, yn ); }


unsigned char HorizonSection::getTransparency( int ch ) const
{ return hortexturehandler_->getTransparency( ch ); }


void HorizonSection::inValidateCache( int channel )
{ hortexturehandler_->inValidateCache( channel ); }


const BinnedValueSet* HorizonSection::getCache( int channel ) const
{ return hortexturehandler_->getCache( channel ); }


void HorizonSection::setTextureData( int channel, const DataPointSet* dpset,
    int sid, const TaskRunnerProvider& trprov )
{  hortexturehandler_->setTextureData( channel, sid,  dpset ); }


osgGeo::LayeredTexture*	HorizonSection::getOsgTexture() const
{ return hortexturehandler_->getOsgTexture(); }


TextureChannels* HorizonSection::getChannels() const
{ return hortexturehandler_->getChannels(); }


void HorizonSection::setChannels2RGBA( TextureChannel2RGBA* t )
{ hortexturehandler_->setChannels2RGBA( t ); }


TextureChannel2RGBA* HorizonSection::getChannels2RGBA()
{ return hortexturehandler_->getChannels2RGBA(); }


const TextureChannel2RGBA* HorizonSection::getChannels2RGBA() const
{ return hortexturehandler_->getChannels2RGBA(); }


void HorizonSection::useChannel( bool yn )
{
    hortexturehandler_->useChannel( yn );
}


int HorizonSection::nrChannels() const
{ return hortexturehandler_->nrChannels(); }


void HorizonSection::addChannel()
{ hortexturehandler_->addChannel(); }


void HorizonSection::removeChannel( int channelidx )
{ hortexturehandler_->removeChannel( channelidx ); }


void HorizonSection::swapChannels( int channel0, int channel1 )
{ hortexturehandler_->swapChannels( channel0, channel1 ); }


int HorizonSection::nrVersions( int channel ) const
{ return hortexturehandler_->nrVersions( channel ); }


void HorizonSection::setNrVersions( int channel, int nrvers )
{ hortexturehandler_->setNrVersions( channel, nrvers); }


int HorizonSection::activeVersion( int channel ) const
{ return hortexturehandler_->activeVersion( channel ); }


void HorizonSection::selectActiveVersion( int channel, int version )
{ hortexturehandler_->selectActiveVersion( channel, version ); }


void HorizonSection::setResolution( char res, TaskRunner* tskr )
{
    desiredresolution_ = res;
    UserShowWait usw( this, uiStrings::sUpdatingDisplay() );

    hortilescreatorandupdator_->setFixedResolution( desiredresolution_, tskr );
    updatePrimitiveSets();
    setUpdateVar( forceupdate_, true );
}


void HorizonSection::updateAutoResolution( const osg::CullStack* cs )
{
    if ( desiredresolution_ == -1 )
	hortilescreatorandupdator_->updateTilesAutoResolution( cs );
}


void HorizonSection::updatePrimitiveSets()
{
    hortilescreatorandupdator_->updateTilesPrimitiveSets();
}


void HorizonSection::setTextureHandler( HorizonTextureHandler& hortexhandler )
{
    hortexturehandler_->getOsgTexture()->removeCallback(
						    texturecallbackhandler_ );
    hortexturehandler_->unRef();
    hortexturehandler_ = &hortexhandler;
    hortexturehandler_->ref();
    hortexturehandler_->getOsgTexture()->addCallback( texturecallbackhandler_ );

    hortexturehandler_->setHorizonSection( *this );
}


HorizonTextureHandler& HorizonSection::getTextureHandler()
{ return *hortexturehandler_; }


int HorizonSection::getNrTitles() const
{
    return tiles_.totalSize();
}


bool HorizonSection::checkTileIndex( int tidx ) const
{
    return tiles_.getData() &&
	   tidx<tiles_.totalSize() &&
	   tiles_.getData()[tidx];
}


const unsigned char* HorizonSection::getTextureData(
    int titleidx, int& width, int& height ) const
{
    osgGeo::LayeredTexture* texture = getOsgTexture();
    const osg::Image* entireimg = texture->getCompositeTextureImage();

    if ( !entireimg )
	return 0;

    width = entireimg->s();
    height = entireimg->t();

    return entireimg->data();
}


int HorizonSection::getTexturePixelSizeInBits() const
{
    osgGeo::LayeredTexture* texture = getOsgTexture();
    const osg::Image* entireimg = texture->getCompositeTextureImage();

    if ( !entireimg )
	return 0;

    return entireimg->getPixelSizeInBits();
}


bool HorizonSection::getTitleTextureCoordinates(
	int titleidx, TypeSet<Coord2f>& coords ) const
{
    if ( !checkTileIndex(titleidx) ) return false;
    return tiles_.getData()[titleidx]->getResolutionTextureCoordinates(coords);

}


void HorizonSection::setUsingNeighborsInIsolatedLine( bool useneighbors )
{
    useneighbors_ = useneighbors;
}


bool HorizonSection::usingNeighborsInIsolatedLine() const
{
    return useneighbors_;
}

} // namespace visBase
