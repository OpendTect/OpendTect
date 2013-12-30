    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vishorizonsection.h"
#include "vishorizonsectiondef.h"
#include "vishordatahandler.h"
#include "vishorizontexturehandler.h"
#include "vishorizonsectiontile.h"
#include "vishorthreadworks.h"
#include "vishortilescreatorandupdator.h"

#include "binidsurface.h"
#include "mousecursor.h"
#include "survinfo.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include <osgGeo/LayeredTexture>

#include <osg/Switch>
#include <osgUtil/CullVisitor>

mCreateFactoryEntry( visBase::HorizonSection );

namespace visBase
{

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
    if( nv->getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR )
    {
	hrsection_->forceRedraw( false );

	if ( hrsection_->getOsgTexture()->needsRetiling() )
	{
	    hrsection_->hortexturehandler_->updateTileTextureOrigin();
	}
    }
    else if( nv->getVisitorType()==osg::NodeVisitor::CULL_VISITOR )
    {
	osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
	const osg::Vec3 projectiondirection = getProjectionDirection( cv );

	if ( hrsection_->getOsgTexture()->getSetupStateSet() )
	    cv->pushStateSet( hrsection_->getOsgTexture()->getSetupStateSet() );

	if( ( hrsection_->forceupdate_ || eyeChanged( projectiondirection ) ||
	    initialtimes_<cMinInitialTimes ) )
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
    if( !cv ) return projectiondirection;
    const osg::Camera* ca =
	const_cast<osgUtil::CullVisitor*>(cv)->getCurrentCamera();
    osg::Vec3 up;
    osg::Vec3 eye;
    osg::Vec3 center;
    ca->getViewMatrixAsLookAt( eye,center,up );
    projectiondirection = center - eye;
    projectiondirection.normalize();
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
    , userchangedisplayrg_( false )			      
    , tiles_( 0, 0 )					  
    , desiredresolution_( cNoneResolution )
    , usewireframe_( false )
    , tesselationlock_( false )
    , nrcoordspertileside_( 0 )
    , tilesidesize_( 0 )
    , totalnormalsize_( 0 )
    , lowestresidx_( 0 )
    , nrhorsectnrres_( 0 )
    , osghorizon_( new osg::Group )
    , forceupdate_( false )
    , displaygeometrytype_( 0 )
    , hordatahandler_( new HorizonSectionDataHandler( this ) )
    , hortexturehandler_( new HorizonTextureHandler( this ) )
    , hortilescreatorandupdator_( new HorTilesCreatorAndUpdator(this) )
    , nodecallbackhandler_( 0 )
    , texturecallbackhandler_( 0 )
    , isredrawing_( false )
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
}


HorizonSection::~HorizonSection()
{
    hortexturehandler_->getOsgTexture()->removeCallback(
						    texturecallbackhandler_ );
    texturecallbackhandler_->unref();

    forceRedraw( false );
    osghorizon_->setCullCallback( 0 );
    nodecallbackhandler_->unref();

    osghorizon_->unref();
    hortexturehandler_->unRef();

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;

	writeLock();
	removeChild( tileptrs[idx]->osgswitchnode_ );
	delete tileptrs[idx];
	writeUnLock();
    }
    
    if ( geometry_ )
    {
	CallBack cb =  mCB(this,HorizonSection,surfaceChangeCB);
	geometry_->movementnotifier.remove( cb );
	geometry_->nrpositionnotifier.remove( cb );
    }

    if ( transformation_ ) transformation_->unRef();
   
    hordatahandler_->unRef();
    hortilescreatorandupdator_->unRef();
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


void HorizonSection::setUpdateVar( bool& variable, bool yn )
{
    if ( !variable && yn )
	forceRedraw( true );

    variable = yn;
}


void HorizonSection::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();

    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();

    for ( int idx=0; idx<tilesz; idx++ )
    {
	tileptrs[idx]->setDisplayTransformation( nt );
	tileptrs[idx]->dirtyGeometry();
    }

}


void HorizonSection::setWireframeColor( Color col )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    spinlock_.lock();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( tileptrs[idx] )
	    tileptrs[idx]->setLineColor( col );
    }
    spinlock_.unLock();
}


void HorizonSection::configSizeParameters()
{
    mDefineRCRange(,);
    const int maxsz = mMAX( rrg.nrSteps()+1, crg.nrSteps()+1 );

    nrcoordspertileside_ = cNumberNodePerTileSide;
    nrhorsectnrres_ = cMaximumResolution;
    while ( maxsz > (nrcoordspertileside_ * cMaxNrTiles) &&
	    nrhorsectnrres_ < cMaxNrResolutions )
    {
	nrcoordspertileside_ *= 2;
	nrhorsectnrres_++;
    }

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
       				 TaskRunner* tr )
{
    if ( !surf ) return;

    if ( connect )
    {
	geometry_ = surf;
	CallBack cb =  mCB( this, HorizonSection, surfaceChangeCB );
	geometry_->movementnotifier.notify( cb );
	geometry_->nrpositionnotifier.notify( cb );
    }

    configSizeParameters();
    rowdistance_ = geometry_->rowRange().step*SI().inlDistance();
    coldistance_ = geometry_->colRange().step*SI().crlDistance();
    surfaceChange( 0, tr );
}


void HorizonSection::setDisplayRange( const StepInterval<int>& rrg,
				      const StepInterval<int>& crg )
{
    if ( rrg.isUdf() || crg.isUdf() || (displayrrg_==rrg && displaycrg_==crg) )
	return;

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
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


bool HorizonSection::usesWireframe() const
{ return usewireframe_; }


const mVisTrans* HorizonSection::getDisplayTransformation() const
{ return transformation_; }


void HorizonSection::useWireframe( bool yn )
{
    if ( usewireframe_==yn )
	return;

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    usewireframe_ = yn;

    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();

    Threads::MutexLocker renderemutex( updatelock_ );
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->useWireframe( yn );
}


void HorizonSection::setDisplayGeometryType( int dispgeometrytype )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();

    Threads::MutexLocker renderemutex( updatelock_ );
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setDisplayGeometryType(
	    dispgeometrytype );
    displaygeometrytype_ = dispgeometrytype;
}


void HorizonSection::turnOsgOn( bool yn )
{  turnOn( yn ); }


void HorizonSection::surfaceChange( const TypeSet<GeomPosID>* gpids,
				    TaskRunner* tr )
{
    if ( !geometry_ || !geometry_->getArray() )
	return;

    if ( hordatahandler_->zaxistransform_ &&
	hordatahandler_->zaxistransformvoi_!=-1 )
    {
	updateZAxisVOI();
	if ( !hordatahandler_->zaxistransform_->loadDataIfMissing(
	      hordatahandler_->zaxistransformvoi_,tr) )
	    return;
    }

    if ( !gpids || !tiles_.info().getSize(0) || !tiles_.info().getSize(1) )

	hortilescreatorandupdator_->createAllTiles( tr );
    else
	hortilescreatorandupdator_->updateTiles( gpids, tr );
}


void HorizonSection::setZAxisTransform( ZAxisTransform* zt, TaskRunner* )
{
    if (!hordatahandler_) return;
    hordatahandler_->setZAxisTransform( zt );

    if ( geometry_ )
	surfaceChangeCB( 0 );
}


void HorizonSection::updateZAxisVOI()
{ hordatahandler_->updateZAxisVOI(); }


void HorizonSection::getDataPositions( DataPointSet& res, double zshift,
    int sid, TaskRunner* tr ) const
{ hordatahandler_->generatePositionData( res, zshift, sid ); }


void HorizonSection::setColTabSequence(int channel, const ColTab::Sequence& se)
{ hortexturehandler_->setColTabSequence( channel , se ); }


const ColTab::Sequence* HorizonSection::getColTabSequence( int channel ) const
{ return hortexturehandler_->getColTabSequence( channel ); }


void HorizonSection::setColTabMapperSetup( int channel,
    const ColTab::MapperSetup& mapper, TaskRunner* tr )
{ hortexturehandler_->setColTabMapperSetup( channel, mapper, tr ); }


const ColTab::MapperSetup* HorizonSection::getColTabMapperSetup( int ch ) const
{ return hortexturehandler_->getColTabMapperSetup( ch ); }


const TypeSet<float>* HorizonSection::getHistogram( int ch ) const
{ return hortexturehandler_->getHistogram( ch ); }


void HorizonSection::setTransparency( int ch, unsigned char yn )
{ hortexturehandler_->setTransparency( ch, yn ); }


unsigned char HorizonSection::getTransparency( int ch ) const
{ return hortexturehandler_->getTransparency( ch ); }


void HorizonSection::inValidateCache( int channel )
{ hortexturehandler_->inValidateCache( channel ); }


const BinIDValueSet* HorizonSection::getCache( int channel ) const
{ return hortexturehandler_->getCache( channel ); }


void HorizonSection::setTextureData( int channel, const DataPointSet* dpset,
    int sid, TaskRunner* tr )
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
{ hortexturehandler_->useChannel( yn ); }


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


void HorizonSection::setResolution( char res, TaskRunner* tr )
{
    desiredresolution_ = res;
    hortilescreatorandupdator_->setFixedResolution( desiredresolution_, tr );
    setUpdateVar( forceupdate_, true );
}


void HorizonSection::updateAutoResolution( const osg::CullStack* cs )
{
    hortilescreatorandupdator_->updateTilesAutoResolution( cs );
}


void HorizonSection::updatePrimitiveSets()
{
    hortilescreatorandupdator_->updateTilesPrimitiveSets();
}


void HorizonSection::updateTiles()
{
    if ( !updatedtiles_.size() )
	return;

    for ( int idx = 0; idx<updatedtiles_.size(); idx++ )
    {
	HorizonSectionTile* tileptrs = updatedtiles_[idx];
	tileptrs->addTileTesselator( updatedtileresolutions_[idx] );
	tileptrs->setDisplayGeometryType( Triangle );
    }

    for ( int idx = 0; idx<updatedtiles_.size(); idx++ )
    {
	HorizonSectionTile* tileptrs = updatedtiles_[idx];
	tileptrs->glueneedsretesselation_ = true;
	tileptrs->addTileGlueTesselator();
    }

    setUpdateVar( forceupdate_, true );
    updatedtiles_.erase();
    updatedtileresolutions_.setEmpty();

}

}; // namespace visBase

