/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: vishorizonsection.cc,v 1.35 2009-06-02 21:41:29 cvsyuancheng Exp $";

#include "vishorizonsection.h"

#include "binidsurface.h"
#include "binidvalset.h"
#include "cubesampling.h"
#include "mousecursor.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "threadwork.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vistexture2.h"
#include "vistexturechannels.h"
#include "vistexturechannel2rgba.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include "SoCameraInfo.h"
#include "SoCameraInfoElement.h"
#include "SoDGBIndexedPointSet.h"
#include "SoLockableSeparator.h"
#include "SoTextureComposer.h"
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>

mCreateFactoryEntry( visBase::HorizonSection );

#define mNrCoordsPerTileSide	64
#define mTotalNrCoordsPerTile	4096
#define mTileSideSize		63
#define mTileLastIdx		63
#define mLowestResIdx		5
#define mTotalNormalSize	5589

namespace visBase
{


int HorizonSection::normalstartidx_[] = { 0, 4096, 5185, 5474, 5555, 5580 };
int HorizonSection::normalsidesize_[] = { 64, 33, 17, 9, 5, 3 };


mClass HorSectTileResolutionTesselator : public ParallelTask
{
public:
		HorSectTileResolutionTesselator( HorizonSectionTile** tiles,
						 int res, int nrtiles )
		: tiles_( tiles )
		, totalnr_( nrtiles )  
		, resolution_( res )	{}

   od_int64	nrIterations() const { return totalnr_; }

   bool		doWork( od_int64 start, od_int64 stop, int )
   		{
	     	    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
		   {
		       if ( tiles_[idx] )
		       {
    			   tiles_[idx]->tesselateResolution( resolution_ );
    			   tiles_[idx]->setActualResolution( resolution_ );
		       }
		   
		       addToNrDone(1);
		   }

		   return true;
	       }

protected:

   HorizonSectionTile**	tiles_;
   int			resolution_;
   int			totalnr_;
};


mClass HorSectTileGlueUpdater : public ParallelTask
{
public:
		HorSectTileGlueUpdater( HorizonSectionTile** tiles, int total )
	    	: tiles_( tiles )
		, totalnr_( total )	{}

   od_int64	nrIterations() const { return totalnr_; }
   
   bool		doWork( od_int64 start, od_int64 stop, int )
   		{
		    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
		    {
			if ( tiles_[idx] ) 
			    tiles_[idx]->updateGlue();

			addToNrDone(1);
		    }

		    return true;
		}
protected:

   HorizonSectionTile**	tiles_;
   int			totalnr_;
};


mClass HorSectTileWireframeUpdater : public ParallelTask
{
public:
		HorSectTileWireframeUpdater( HorizonSectionTile** tiles, 
					     int resolution, int totalnr )
		: tiles_( tiles )
		, totalnr_( totalnr )  
		, resolution_( resolution ) {}

    od_int64	nrIterations() const { return totalnr_; }

    bool	doWork( od_int64 start, od_int64 stop, int )
    		{
		    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
		    {
			if ( tiles_[idx] )
			    tiles_[idx]->turnOnWireframe( resolution_ );
			
			addToNrDone(1);
		    }
		    
		    return true;
		}
protected:

    HorizonSectionTile** tiles_;
    int			 resolution_;
    int			 totalnr_;
};


mClass HorizonSectionTilePosSetup: public ParallelTask
{
public:    
HorizonSectionTilePosSetup( ObjectSet<HorizonSectionTile> tiles, 
	TypeSet<RowCol> rcs, const Geometry::BinIDSurface& geo )
    : tiles_( tiles )
    , geo_( geo )  
    , rcs_( rcs )
{}

od_int64 nrIterations() const { return tiles_.size(); }

const char* message() const { return "Creating Horizon Display"; }
const char* nrDoneText() const { return "Parts completed"; }

protected:

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int rowstep = geo_.rowRange().step;
    const int colstep = geo_.colRange().step;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	HorizonSectionTile* tile = tiles_[idx];
	const int startrow = rcs_[idx].row;
	const int startcol = rcs_[idx].col;

	for ( int row=0; row<mNrCoordsPerTileSide; row++ )
	{
	    for ( int col=0; col<mNrCoordsPerTileSide; col++ )
	    {
		const RowCol rc( row*rowstep+startrow, col*colstep+startcol );
		tile->setPos( row, col, geo_.getKnot(rc,false) );
	    }
	}

	addToNrDone(1);
    }

    return true;
}

    ObjectSet<HorizonSectionTile> 	tiles_;
    TypeSet<RowCol>			rcs_;
    const Geometry::BinIDSurface&	geo_;
};



ArrPtrMan<SbVec2f> HorizonSection::texturecoordptr_ = 0;

HorizonSection::HorizonSection() 
    : VisualObjectImpl( false )
    , callbacker_( new SoCallback )  
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , zaxistransformvoi_( -2 )			  
    , geometry_( 0 )
    , channels_( TextureChannels::create() )		   
    , channel2rgba_( ColTabTextureChannel2RGBA::create() ) 
    , tiles_( 0, 0 )					  
    , texturecrds_( new SoTextureCoordinate2 )
    , desiredresolution_( -1 )
    , usewireframe_( false )
    , cosanglexinl_( cos(SI().computeAngleXInl()) )
    , sinanglexinl_( sin(SI().computeAngleXInl()) )		     
{
    cache_.allowNull( true );
    
    callbacker_->ref();
    callbacker_->setCallback( updateAutoResolution, this );
    addChild( callbacker_ );

    SoShapeHints* hints = new SoShapeHints;
    addChild( hints );
    hints->vertexOrdering = SoShapeHints::CLOCKWISE;
    hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    channel2rgba_->ref();
    channels_->ref();
    addChild( channels_->getInventorNode() );
    channels_->setChannels2RGBA( channel2rgba_ );
    if ( channels_->nrChannels()<1 )
	addChannel();
    else 
	cache_ += 0;

    addChild( texturecrds_ );

    if ( !texturecoordptr_ )
    {
	mTryAlloc( texturecoordptr_, SbVec2f[mTotalNrCoordsPerTile] );
	int idx = 0;
	for ( int irow=0; irow<mNrCoordsPerTileSide; irow++ )
	{
	    for ( int icol=0; icol<mNrCoordsPerTileSide; icol++ )
	    {
		texturecoordptr_[idx] = SbVec2f((icol+0.5)/mNrCoordsPerTileSide,
			(irow+0.5)/mNrCoordsPerTileSide);
		idx++;
	    }
	}
    }

    texturecrds_->point.setValuesPointer( mTotalNrCoordsPerTile,
	    				  texturecoordptr_ );
}


HorizonSection::~HorizonSection()
{
    channels_->unRef();
    channel2rgba_->unRef();
    deepErase( cache_ );

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;

	removeChild( tileptrs[idx]->getNodeRoot() );
	delete tileptrs[idx];
    }
    
    if ( geometry_ )
    {
	CallBack cb =  mCB(this,HorizonSection,surfaceChangeCB);
	geometry_->movementnotifier.remove( cb );
	geometry_->nrpositionnotifier.remove( cb );
    }

    if ( transformation_ ) transformation_->unRef();
    removeZTransform();
    
    callbacker_->unref();
}


void HorizonSection::allowShading( bool yn )
{ 
    channel2rgba_->allowShading( yn ); 

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->useShading( yn );
}


int HorizonSection::maxNrChannels() const
{ return channel2rgba_->maxNrChannels(); }


void HorizonSection::useChannel( bool yn )
{ channels_->turnOn( yn ); }


int HorizonSection::nrChannels() const
{ return channels_->nrChannels(); }


void HorizonSection::addChannel()
{ 
    channels_->addChannel();
    cache_ += 0;
    channel2rgba_->setEnabled( nrChannels()-1, true );
}


void HorizonSection::removeChannel( int channelidx )
{ 
    channels_->removeChannel( channelidx ); 
    delete cache_.remove( channelidx );
}


void HorizonSection::swapChannels( int channel0, int channel1 )
{ 
    channels_->swapChannels( channel0, channel1 ); 
    cache_.swap( channel0, channel1 );
}


int HorizonSection::nrVersions( int channel ) const
{ return channels_->nrVersions( channel ); }


void HorizonSection::setNrVersions( int channel, int nrvers )
{ channels_->setNrVersions( channel, nrvers); }


int HorizonSection::activeVersion( int channel ) const
{ return channels_->currentVersion( channel ); }


void HorizonSection::selectActiveVersion( int channel, int version )
{ channels_->setCurrentVersion( channel, version ); }


void HorizonSection::setDisplayTransformation( Transformation* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
    {
	transformation_->ref();

	HorizonSectionTile** tileptrs = tiles_.getData();
	for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
	{
	    if ( tileptrs[idx] )
		tileptrs[idx]->setDisplayTransformation( nt );
	}
    }
}


Transformation* HorizonSection::getDisplayTransformation()
{ return transformation_; }


void HorizonSection::setZAxisTransform( ZAxisTransform* zt )
{
    if ( zaxistransform_==zt ) 
	return;
    
    removeZTransform();
    if ( !zt ) return;

    zaxistransform_ = zt;
    zaxistransform_->ref();
    if ( geometry_ )
    {
	updateZAxisVOI();
	CBCapsule<const TypeSet<GeomPosID>*> caps( 0, geometry_ );
	surfaceChangeCB( &caps );
    }
}


void HorizonSection::removeZTransform()
{
    if ( !zaxistransform_ )
	return;

    if ( zaxistransformvoi_!=-2 )
	zaxistransform_->removeVolumeOfInterest( zaxistransformvoi_ );

    zaxistransformvoi_ = -2;
    
    zaxistransform_->unRef();
    zaxistransform_ = 0;
}


void HorizonSection::updateZAxisVOI()
{
    if ( !geometry_ || zaxistransformvoi_==-1 )	//transform does not use voi
	return;

    if ( !zaxistransform_ || !zaxistransform_->needsVolumeOfInterest() )
	return;

    CubeSampling cs;
    cs.hrg.set( geometry_->rowRange(), geometry_->colRange() );

    HorSamplingIterator iter( cs.hrg );

    bool first = true;
    BinID curpos;
    while ( iter.next(curpos) )
    {
	const float depth = geometry_->getKnot(curpos,false).z;
	if ( first )
	{
	    cs.zrg.start = cs.zrg.stop = depth;
	    first = false;
	}
	else
	    cs.zrg.include( depth );
    }

    if ( first ) return;

    if ( zaxistransformvoi_==-2 )
	zaxistransformvoi_ = zaxistransform_->addVolumeOfInterest( cs, false );
    else
	zaxistransform_->setVolumeOfInterest( zaxistransformvoi_, cs, false );
}


void HorizonSection::getDataPositions( BinIDValueSet& res, float zoff ) const
{
    if ( !geometry_ ) return;
    
    const int nrknots = geometry_->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const BinID bid = geometry_->getKnotRowCol(idx);
	const Coord3 pos = geometry_->getKnot(bid,false);
	if ( pos.isDefined() ) 
	    res.add( bid, pos.z+zoff );
    }
}


void HorizonSection::setTextureData( int channel, const BinIDValueSet* data )
{
    if ( channel<0 || channel>=cache_.size() || !geometry_ ||
	 !geometry_->getArray() ) 
	return;

    if ( cache_[channel] )
    {
	if ( !data )
	{
	    delete cache_[channel];
	    cache_.replace( channel, 0 );
	}
	else
	{
	    (*cache_[channel]) = *data;
	}
    }
    else if ( data )
    {
	cache_.replace( channel, new BinIDValueSet(*data) );
    	setNrVersions( channel, data->nrVals()-1 );
    }

    updateTexture( channel );
}


void HorizonSection::updateTexture( int channel )
{
    const StepInterval<int>& rrg = geometry_->rowRange();
    const StepInterval<int>& crg = geometry_->colRange();

    const int nrrows = rrg.nrSteps()+1;
    const int nrcols = crg.nrSteps()+1;

    channels_->setSize( 1, nrrows, nrcols );
   
    const int nrversions = channels_->nrVersions( channel );

    ObjectSet<float> versiondata;
    versiondata.allowNull( true );
    const int nrcells = nrrows*nrcols;

    MemSetter<float> memsetter;
    memsetter.setSize( nrcells );
    memsetter.setValue( mUdf(float) );

    const BinIDValueSet* data = getCache( channel );
    if ( !data ) return;

    for ( int idx=0; idx<nrversions; idx++ )
    {
	if ( idx<data->nrVals()-1 )
	{
	    float* vals = new float[nrcells];
	    if ( !vals )
	    {
		deepEraseArr( versiondata );
		return;
	    }

	    memsetter.setTarget( vals );
	    memsetter.execute();

	    versiondata += vals;
	}
	else
	    versiondata += 0;
    }

    const float eps = SI().zRange(true).step*1e-3;

    BinIDValueSet::Pos pos;
    while ( data->next(pos,true) )
    {
	const BinID bid = data->getBinID( pos );
	const float geomzval = geometry_->getKnot(bid, false).z;
	if ( mIsUdf(geomzval) )
	    continue;

	const float* vals = data->getVals(pos);
	const float datazval = vals[0];
	if ( !mIsEqual(datazval,geomzval, eps ) )
	    continue;

	const int inlidx = rrg.nearestIndex(bid.inl);
	const int crlidx = crg.nearestIndex(bid.crl);

	const int offset = inlidx*nrcols + crlidx;

	for ( int idx=0; idx<data->nrVals()-1; idx++ )
	    versiondata[idx][offset] = vals[idx+1]; 
    }

    for ( int idx=0; idx<nrversions; idx++ )
	channels_->setUnMappedData( channel, idx, versiondata[idx],
				    OD::TakeOverPtr, 0 );
}


void HorizonSection::setWireframeColor( Color col )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setWireframeColor( col );
}


void HorizonSection::setColTabSequence(int channel, const ColTab::Sequence& se)
{
    if ( channel>=0 ) channel2rgba_->setSequence( channel, se );
}


const ColTab::Sequence* HorizonSection::getColTabSequence( int channel ) const
{
    return channel<0 ? 0 : channel2rgba_->getSequence( channel );
}


void HorizonSection::setColTabMapperSetup( int channel, 
		       const ColTab::MapperSetup& mapper, TaskRunner* tr )
{
    if ( channel>=0 )
    {
	channels_->setColTabMapperSetup( channel, mapper );
	channels_->reMapData( channel, tr );
    }
}


const ColTab::MapperSetup* HorizonSection::getColTabMapperSetup( int ch ) const
{
    return ch<0 ? 0 : &channels_->getColTabMapperSetup( ch,activeVersion(ch) );
}


void HorizonSection::setTransparency( int ch, unsigned char yn )
{ 
    if ( ch>=0 ) channel2rgba_->setTransparency( ch, yn );
}


unsigned char HorizonSection::getTransparency( int ch ) const
{ 
    return channel2rgba_->getTransparency( ch ); 
}


void HorizonSection::enableChannel( int channel, bool yn )
{
    channel2rgba_->setEnabled( channel, yn ); 
}


bool HorizonSection::isChannelEnabled( int channel ) const
{ 
    return channel2rgba_->isEnabled( channel ); 
}


void HorizonSection::inValidateCache( int channel )
{
    if ( channel==-1 )
    {
	for ( int idx=0; idx<cache_.size(); idx++ )
	    inValidateCache( idx );
    }
    else
    {
    	delete cache_[channel];
    	cache_.replace( channel, 0 );
    }
}


const BinIDValueSet* HorizonSection::getCache( int channel ) const
{ 
    return (channel<0 && channel>=cache_.size()) ? 0 : cache_[channel]; 
}


void HorizonSection::setSurface( Geometry::BinIDSurface* surf, bool connect,
       				 TaskRunner* tr )
{
    if ( !surf ) return;

    origin_.row = surf->rowRange().start;
    origin_.col = surf->colRange().start;
    step_.row = surf->rowRange().step;
    step_.col = surf->colRange().step;
    rowdistance_ = step_.row*SI().inlDistance();
    coldistance_ = step_.col*SI().inlDistance();

    if ( connect )
    {
	geometry_ = surf;
	CallBack cb =  mCB( this, HorizonSection, surfaceChangeCB );
	geometry_->movementnotifier.notify( cb );
	geometry_->nrpositionnotifier.notify( cb );
    }

    surfaceChange( 0, tr );
}


void HorizonSection::surfaceChangeCB( CallBacker* cb )
{
    geometrylock_.lock();
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, gpids, cb );

    surfaceChange( gpids, 0 );
}


void HorizonSection::surfaceChange( const TypeSet<GeomPosID>* gpids,
       				    TaskRunner* tr )
{
    if ( zaxistransform_ && zaxistransformvoi_>=0 )
    {
	updateZAxisVOI();
	if ( !zaxistransform_->loadDataIfMissing(zaxistransformvoi_) )
	{
	    geometrylock_.unLock();
	    return;
	}
    }
    
    const StepInterval<int>& rrg = geometry_->rowRange();
    const StepInterval<int>& crg = geometry_->colRange();
    if ( rrg.width(false)<0 || crg.width(false)<0 )
    {
	geometrylock_.unLock();
	return;
    }

    if ( !origin_.row && !origin_.col )
	origin_ = RowCol( rrg.start, crg.start );
   
    ObjectSet<HorizonSectionTile> newtiles;
    TypeSet<RowCol> tilestarts;

    if ( !gpids || !tiles_.info().getSize(0) || !tiles_.info().getSize(1) )
    {
	origin_ = RowCol( rrg.start, crg.start );
	const int nrrows = nrBlocks( rrg.nrSteps()+1, mNrCoordsPerTileSide, 1 );
	const int nrcols = nrBlocks( crg.nrSteps()+1, mNrCoordsPerTileSide, 1 );

	if ( !tiles_.setSize( nrrows, nrcols ) )
	{
	    geometrylock_.unLock();
	    return;
	}

	tiles_.setAll( 0 );

	for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
	{
	    for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	    {
		newtiles += createTile(tilerowidx, tilecolidx);
		const int startrow = tilerowidx*mTileSideSize + origin_.row;
		const int startcol = tilecolidx*mTileSideSize + origin_.col;
		tilestarts += RowCol( startrow, startcol );
	    }
	}

	updateTileTextureOrigin();
    }
    else
    {
	updateTileArray( rrg, crg );
	const int nrrowsz = tiles_.info().getSize(0);
	const int nrcolsz = tiles_.info().getSize(1);

    	for ( int idx=(*gpids).size()-1; idx>=0; idx-- )
	{
	    const RowCol& absrc( (*gpids)[idx] );
	    RowCol rc = absrc-origin_; rc /= step_;

	    const int tilerowidx = rc.row/mTileSideSize;
	    const int tilerow = rc.row%mTileSideSize;

	    const int tilecolidx = rc.col/mTileSideSize;
	    const int tilecol = rc.col%mTileSideSize;

	    const Coord3 pos = geometry_->getKnot(absrc,false);

	    HorizonSectionTile* tile = tiles_.get( tilerowidx, tilecolidx );
	    if ( !tile ) 
	    {
		tile = createTile( tilerowidx, tilecolidx );
		const int startrow = tilerowidx*mTileSideSize + origin_.row;
		const int startcol = tilecolidx*mTileSideSize + origin_.col;
		tilestarts += RowCol( startrow, startcol );
		newtiles += tile;
	    }
	    else
	    {
    		for ( int res=0; res<=mLowestResIdx; res++ )
    		    tile->setAllNormalsInvalid( res, true );
	    
		tile->setPos( tilerow, tilecol, pos );
	    }

	    for ( int rowidx=-1; rowidx<=1; rowidx++ )
	    {
		const int neighborrow = tilerowidx+rowidx;
		if ( neighborrow<0 || neighborrow>=nrrowsz )
		    continue;

		for ( int colidx=-1; colidx<=1; colidx++ )
		{
		    if ( !rowidx && !colidx ) continue;

		    const int neighborcol = tilecolidx+colidx;
		    if ( neighborcol<0 || neighborcol>=nrcolsz )
			continue;

		    if ( !tiles_.get( neighborrow, neighborcol ) )
			continue;

		    tiles_.get( neighborrow, neighborcol )->setPos(
			    tilerow-rowidx*mTileSideSize,
			    tilecol-colidx*mTileSideSize, pos );
		}
	    }
	}
    }

    HorizonSectionTilePosSetup task( newtiles, tilestarts, *geometry_ );
    if ( tr ) tr->execute( task );
    else task.execute();

    geometrylock_.unLock();
}


void HorizonSection::updateTileArray( const StepInterval<int>& rrg,
				      const StepInterval<int>& crg )
{
    const int rowsteps =  mTileSideSize*step_.row;
    const int colsteps = mTileSideSize*step_.col;
    const int oldrowsize = tiles_.info().getSize(0);
    const int oldcolsize = tiles_.info().getSize(1);
    int newrowsize = oldrowsize;
    int newcolsize = oldcolsize;
    int nrnewrowsbefore = 0;
    int nrnewcolsbefore = 0;
	
    int diff = origin_.row - rrg.start;
    if ( diff>0 ) 
    {
	nrnewrowsbefore = diff/rowsteps + diff%rowsteps ? 1 : 0;
    	newrowsize += nrnewrowsbefore;
    }

    diff = rrg.stop - (origin_.row+oldrowsize*rowsteps);
    if ( diff>0 ) newrowsize += diff/rowsteps + diff%rowsteps ? 1 : 0;
    
    diff = origin_.col-crg.start;
    if ( diff>0 ) 
    {
	nrnewcolsbefore = diff/colsteps + diff%colsteps ? 1 : 0;
    	newcolsize += nrnewcolsbefore;
    }

    diff = crg.stop - (origin_.col+oldcolsize*colsteps);
    if ( diff>0 ) newcolsize += diff/colsteps + diff%colsteps ? 1 : 0;

    if ( newrowsize==oldrowsize && newcolsize==oldcolsize )
	return;

    Array2DImpl<HorizonSectionTile*> newtiles( newrowsize, newcolsize );
    newtiles.setAll( 0 );

    for ( int rowidx=0; rowidx<oldrowsize; rowidx++ )
    {
	const int targetrow = rowidx+nrnewrowsbefore;
	for ( int colidx=0; colidx<oldcolsize; colidx++ )
	{
	    const int targetcol = colidx+nrnewcolsbefore;
	    newtiles.set( targetrow, targetcol, tiles_.get(rowidx,colidx) );
	}
    }

    tiles_.copyFrom( newtiles );
    origin_.row -= nrnewrowsbefore*rowsteps;
    origin_.col -= nrnewcolsbefore*colsteps;
    updateTileTextureOrigin();
}


void HorizonSection::updateTileTextureOrigin()
{
    for ( int rowidx=0; rowidx<tiles_.info().getSize(0); rowidx++ )
    {
	for ( int colidx=0; colidx<tiles_.info().getSize(1); colidx++ )
	{
	    if ( tiles_.get(rowidx,colidx) )
		tiles_.get(rowidx,colidx)->setTextureOrigin( 
			rowidx*mTileSideSize, colidx*mTileSideSize );
	}
    }
}


HorizonSectionTile* HorizonSection::createTile( int tilerowidx, int tilecolidx )
{
    HorizonSectionTile* tile =  new HorizonSectionTile();
    tile->setDisplayTransformation( transformation_ );
    tile->setResolution( desiredresolution_ );
    tile->useShading( channel2rgba_->usesShading() );
    tile->useWireframe( usewireframe_ );
    tile->setWireframeMaterial( visBase::Material::create() );

    tiles_.set( tilerowidx, tilecolidx, tile );
    
    for ( int rowidx=-1; rowidx<=1; rowidx++ )
    {
	const int neighborrow = tilerowidx+rowidx;
	if ( neighborrow<0 || neighborrow>=tiles_.info().getSize(0) )
	    continue;

	for ( int colidx=-1; colidx<=1; colidx++ )
	{
	    if ( !colidx && !rowidx )
		continue;

	    const int neighborcol = tilecolidx+colidx;
	    if ( neighborcol<0 || neighborcol>=tiles_.info().getSize(1) )
		continue;

	    HorizonSectionTile* neighbor = tiles_.get(neighborrow,neighborcol);

	    if ( !neighbor ) 
		continue;

	    char pos;
	    if ( colidx==-1 ) 
		pos = rowidx==-1 ? 0 : (!rowidx ? 3 : 6);
	    else if ( colidx==0 ) 
		pos = rowidx==-1 ? 1 : (!rowidx ? 4 : 7);
	    else 
		pos = rowidx==-1 ? 2 : (!rowidx ? 5 : 8);

	    tile->setNeighbor( pos, neighbor );

	    if ( colidx==1 ) 
		pos = rowidx==1 ? 0 : (!rowidx ? 3 : 6);
	    else if ( colidx==0 ) 
		pos = rowidx==1 ? 1 : (!rowidx ? 4 : 7);
	    else 
		pos = rowidx==1 ? 2 : (!rowidx ? 5 : 8);

	    neighbor->setNeighbor( pos, tile );
	}
    }
	     
    addChild( tile->getNodeRoot() );

    return tile;
}


void HorizonSection::updateAutoResolution( void* clss, SoAction* action )
{
    if ( action->isOfType( SoGLRenderAction::getClassTypeId()) )
    {
	SoState* state = action->getState();
	SoCacheElement::invalidate( state );
    	((HorizonSection*) clss)->updateAutoResolution( state, 0 );
    }
    else if ( action->isOfType( SoGetBoundingBoxAction::getClassTypeId()) ) 
    {
	SoCacheElement::invalidate( action->getState() );
	((HorizonSection*) clss)->updateBBox((SoGetBoundingBoxAction*)action);
    }
}


void HorizonSection::updateBBox( SoGetBoundingBoxAction* action )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();

    SbBox3f bigbox;
    bool didset = false;
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] ) continue;
	SbBox3f bbox = tileptrs[idx]->getBBox();
	if ( bbox.isEmpty() ) continue;

	if ( !didset )
	{
	    bigbox = bbox;
	    didset = true;
	}
	else
	    bigbox.extendBy( bbox );
    }

    if ( didset && !bigbox.isEmpty() )
    {
	action->extendBy( bigbox );
	action->setCenter( bigbox.getCenter(), true );
    }
}


void HorizonSection::updateAutoResolution( SoState* state, TaskRunner* tr )
{
    const int nrrowtiles = tiles_.info().getSize(0);
    const int nrcoltiles = tiles_.info().getSize(1);
    const int tilesz = nrrowtiles*nrcoltiles;
    if ( !tilesz ) return;
 
    for ( int tilerow=0; tilerow<nrrowtiles; tilerow++ )
    {
	for ( int tilecol=0; tilecol<nrcoltiles; tilecol++ )
	{
	    HorizonSectionTile* tile = tiles_.get(tilerow, tilecol);
	    if ( !tile ) continue;

	    tile->updateAutoResolution( state );
	    tile->tesselateActualResolution();
	    int res = tile->getActualResolution();
	    if ( res==-1 ) res = desiredresolution_;
	    updateNormals( *tile, res, tilerow, tilecol );
	}
    }

    HorizonSectionTile** tileptrs = tiles_.getData();
    HorSectTileGlueUpdater gluetask( tileptrs, tilesz );
    if ( tr )
	tr->execute(gluetask);
    else
	gluetask.execute();

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetResolutionChangeFlag();
}


void HorizonSection::updateNormals( HorizonSectionTile& tile, int res,
       				    int tilerowidx, int tilecolidx )
{
    if ( res<0 ) return;

    if ( tile.allNormalsInvalid(res) )
    {
	const int normalstop = 
	    res<mLowestResIdx ? normalstartidx_[res+1]-1 : mTotalNormalSize-1;
	for ( int idx=normalstartidx_[res]; idx<=normalstop; idx++ )
	    setNormal( idx, res, tile, tilerowidx, tilecolidx );
    }
    else
    {
	TypeSet<int> updatelist;
	tile.getNormalUpdateList( res, updatelist );
	for ( int idx=0; idx<updatelist.size(); idx++ )
	    setNormal( updatelist[idx], res, tile, tilerowidx, tilecolidx );
    }

    tile.removeInvalidNormals( res );
    tile.setAllNormalsInvalid( res, false );
}


void HorizonSection::setNormal( int nmidx, int res, HorizonSectionTile& ti,
				int tilerowidx, int tilecolidx )
{
    const int spacing = (int)pow(2.0,res);
    const int normalrow = (nmidx-normalstartidx_[res])/normalsidesize_[res];
    const int normalcol = (nmidx-normalstartidx_[res])%normalsidesize_[res];
    const int row = origin_.row + tilerowidx*mTileSideSize +
	(normalrow==normalsidesize_[res]-1 ? mTileSideSize : normalrow*spacing);
    const int col = origin_.col + tilecolidx*mTileSideSize +
	normalcol==normalsidesize_[res]-1 ? mTileSideSize : normalcol*spacing;

    TypeSet<float> posarray, zarray;
    for ( int idx=-spacing; idx<=spacing; idx++ )
    {
	const Coord3 pos = 
	    geometry_->getKnot( RowCol(row+idx*step_.row,col), false );
	if ( pos.isDefined() )
	{
	    posarray += idx*rowdistance_;
	    zarray += pos.z;
	}
    }
	   
    double drow = 0;
    if ( zarray.size()>1 )
	getGradient( posarray.arr(), zarray.arr(), zarray.size(), 0, 0, &drow );

    posarray.erase(); zarray.erase();    
    for ( int idx=-spacing; idx<=spacing; idx++ )
    {
	const Coord3 pos = 
	    geometry_->getKnot( RowCol(row,col+idx*step_.col), false );
	if ( pos.isDefined() )
	{
	    posarray += idx*coldistance_;
	    zarray += pos.z;
	}
    }

    double dcol = 0;
    if ( zarray.size()>1 )
	getGradient( posarray.arr(), zarray.arr(), zarray.size(), 0, 0, &dcol );

    ti.setNormal( nmidx, Coord3(drow*cosanglexinl_+dcol*sinanglexinl_,
				dcol*cosanglexinl_-drow*sinanglexinl_,-1) );
}


char HorizonSection::currentResolution() const
{ return desiredresolution_; }


void HorizonSection::setResolution( int res, TaskRunner* tr )
{
    desiredresolution_ = res;
    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz ) return;
    
    if ( usewireframe_ ) turnOnWireframe( res, tr );

    HorizonSectionTile** tileptrs = tiles_.getData();
    TypeSet<int> resolutions;
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setResolution( res );

    if ( res==-1 )
	return;

    HorSectTileResolutionTesselator task( tileptrs, res, tilesz );
    if ( tr )
	tr->execute(task);
    else
	task.execute();

    HorSectTileGlueUpdater gluetask( tileptrs, tilesz );
    if ( tr ) 
	tr->execute(gluetask);
    else 
	gluetask.execute();

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetResolutionChangeFlag();
}


void HorizonSection::turnOnWireframe( int res, TaskRunner* tr )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    
    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();
  
    HorSectTileWireframeUpdater task( tileptrs, res, tilesz );
    if ( tr )
	tr->execute( task );
    else
	task.execute();

}


void HorizonSection::useWireframe( bool yn )
{
    if ( usewireframe_==yn )
	return;

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    usewireframe_ = yn;

    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();
    
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->useWireframe( yn );
}


bool HorizonSection::usesWireframe() const
{ return usewireframe_; }



int HorizonSectionTile::spacing_[] = { 1, 2, 4, 8, 16, 32 };
int HorizonSectionTile::normalstartidx_[] = { 0, 4096, 5185, 5474, 5555, 5580 };
int HorizonSectionTile::nrcells_[] = { 4096, 1024, 256, 64, 16, 4 };
int HorizonSectionTile::normalsidesize_[] = { 64, 33, 17, 9, 5, 3 };


HorizonSectionTile::HorizonSectionTile()
    : root_( new SoLockableSeparator )
    , normals_( new SoNormal )  
    , coords_( visBase::Coordinates::create() )
    , texture_( new SoTextureComposer )
    , resswitch_( new SoSwitch )	
    , gluelowdimswitch_( new SoSwitch )					
    , gluetriangles_( new SoIndexedTriangleStripSet )
    , gluelines_( new SoIndexedLineSet )
    , glueneedsretesselation_( true )
    , gluepoints_( new SoDGBIndexedPointSet )
    , desiredresolution_( -1 )
    , resolutionhaschanged_( false )
    , bboxstart_( Coord3::udf() )				    
    , bboxstop_( Coord3::udf() )				   
    , needsupdatebbox_( false )
    , usewireframe_( false )
    , wireframematerial_( 0 )			   
    , wireframetexture_( visBase::Texture2::create() )		  
    , useshading_( false )						  
{
    root_->ref();
    coords_->ref();
    normals_->ref();
    normals_->vector.setNum( mTotalNormalSize );
    for ( int idx=0; idx<mTotalNormalSize; idx++ )
	normals_->vector.set1Value(idx, SbVec3f(0,0,-1) );

    wireframetexture_->ref();

    root_->addChild( coords_->getInventorNode() );
    root_->addChild( normals_ );
    root_->addChild( texture_ );

    root_->addChild( gluetriangles_ );
    root_->addChild( gluepoints_ );
    root_->addChild( gluelowdimswitch_ );
    gluelowdimswitch_->addChild( gluelines_ );
    
    gluetriangles_->coordIndex.deleteValues( 0, -1 );
    gluelines_->coordIndex.deleteValues( 0, -1 );
    gluepoints_->coordIndex.deleteValues( 0, -1 );

    root_->addChild( resswitch_ );
    for ( int idx=0; idx<mHorSectNrRes; idx++ )
    {
	allnormalsinvalid_[idx] = true;
	needsretesselation_[idx] = true;
	resolutions_[idx] = new SoGroup;
	resswitch_->addChild( resolutions_[idx] );

	triangles_[idx] = new SoIndexedTriangleStripSet;
	triangles_[idx]->coordIndex.deleteValues( 0, -1 );
	points_[idx] = new SoDGBIndexedPointSet;
	points_[idx]->coordIndex.deleteValues( 0, -1 );
	wireframeswitch_[idx] = new SoSwitch;
	resolutions_[idx]->addChild( triangles_[idx] );
	resolutions_[idx]->addChild( points_[idx] );
	resolutions_[idx]->addChild( wireframeswitch_[idx] );

	wireframeseparator_[idx] = new SoSeparator;	
	wireframes_[idx] = new SoIndexedLineSet;
	wireframes_[idx]->coordIndex.deleteValues( 0, -1 );
	wireframeseparator_[idx]->addChild( 
		 wireframetexture_->getInventorNode() );
	wireframeseparator_[idx]->addChild( wireframes_[idx] );

	lines_[idx] = new SoIndexedLineSet;
	lines_[idx]->coordIndex.deleteValues( 0, -1 );
	wireframeswitch_[idx]->addChild( wireframeseparator_[idx] );
	wireframeswitch_[idx]->addChild( lines_[idx] );
    }

    for ( int idx=0; idx<9; idx++ )
	neighbors_[idx] = 0;

    setTextureSize( mNrCoordsPerTileSide, mNrCoordsPerTileSide );
    useWireframe( usewireframe_ );
}


HorizonSectionTile::~HorizonSectionTile()
{
    coords_->unRef();
    normals_->unref();
    root_->unref();
    wireframetexture_->unRef();
    if ( wireframematerial_ ) wireframematerial_->unRef();
}


void HorizonSectionTile::useShading( bool yn )
{ 
    if ( useshading_ == yn )
	return;

    useshading_ = yn; 
}


void HorizonSectionTile::setWireframeMaterial( Material* nm )
{
    if ( wireframematerial_ )
    {
	for ( int idx=0; idx<mHorSectNrRes; idx++ )
    	    wireframeseparator_[idx]->removeChild( 
		    wireframematerial_->getInventorNode() );

	wireframematerial_->unRef();
    }
    
    wireframematerial_ = nm;
    
    if ( wireframematerial_ )
    {
	wireframematerial_->ref();
	for ( int idx=0; idx<mHorSectNrRes; idx++ )
    	    wireframeseparator_[idx]->insertChild( 
		    wireframematerial_->getInventorNode(), 0 );
    }
}


void HorizonSectionTile::setWireframeColor( Color col )
{ if ( wireframematerial_ ) wireframematerial_->setColor( col ); }


void HorizonSectionTile::setDisplayTransformation( Transformation* nt )
{
    if ( coords_->getDisplayTransformation()==nt )
	return;

    coords_->setDisplayTransformation( nt );
    needsupdatebbox_ = true;
}


void HorizonSectionTile::setNormal( int index, const Coord3& norm )
{
    normals_->vector.set1Value( index, SbVec3f(norm[0],norm[1],norm[2]) );
}


int HorizonSectionTile::getNormalIdx( int crdidx, int res ) const
{
    //Normals size = 64*64+33*33+17*17+9*9+5*5+3*3
    if ( crdidx<0 || res<0 )
	return -1;

    //Index in the 64*64 tile
    const int row = crdidx/mNrCoordsPerTileSide;
    const int col = crdidx%mNrCoordsPerTileSide;

    int useres = res;
    if ( row==mTileLastIdx || col==mTileLastIdx )
    {
	if ( row==mTileLastIdx && col==mTileLastIdx && neighbors_[8] )
	    useres = neighbors_[8]->getActualResolution();
	else if ( row==mTileLastIdx && neighbors_[7] )
	    useres = neighbors_[7]->getActualResolution();
	else if ( col==mTileLastIdx && neighbors_[5] )
	    useres = neighbors_[5]->getActualResolution();

	if ( useres==-1 )
	    useres = res;
    }
   
    if ( (!(row%spacing_[useres]) || row==mTileLastIdx) &&
         (!(col%spacing_[useres]) || col==mTileLastIdx) )
    {
	if ( row==mTileLastIdx && col==mTileLastIdx && !neighbors_[8] )
	    return useres<5 ? normalstartidx_[useres+1]-1 :normalstartidx_[5]+8;
	else
	{
    	    const int resrow = row/spacing_[useres] + 
		( useres>0 && row==mTileLastIdx ? 1 : 0 );
    	    const int rescol = col/spacing_[useres] +
    		( useres>0 && col==mTileLastIdx ? 1 : 0 );

    	   return normalstartidx_[useres]+resrow*normalsidesize_[useres]+rescol;
	}
    }

    return -1;
}


void HorizonSectionTile::resetResolutionChangeFlag()
{ resolutionhaschanged_= false; }


void HorizonSectionTile::tesselateActualResolution()
{
    const int res = getActualResolution();
    if ( res==-1 || !needsretesselation_[res] )
	return;

    tesselateResolution( res );
}


void HorizonSectionTile::getNormalUpdateList( int res, TypeSet<int>& result )
{ result = invalidnormals_[res]; }


void HorizonSectionTile::removeInvalidNormals( int res )
{ invalidnormals_[res].erase(); }


void HorizonSectionTile::setAllNormalsInvalid( int res, bool yn )
{ allnormalsinvalid_[res] = yn; }


bool HorizonSectionTile::allNormalsInvalid( int res ) const
{ return allnormalsinvalid_[res]; }


void HorizonSectionTile::setResolution( int res )
{ desiredresolution_ = res; }


int HorizonSectionTile::getActualResolution() const
{
    root_->lock.readLock();
    const int res = resswitch_->whichChild.getValue();
    root_->lock.readUnlock();

    return res;
}


void HorizonSectionTile::updateAutoResolution( SoState* state )
{
     int newres = desiredresolution_;
     if ( newres==-1 && state )
     {
	 updateBBox();
	 const SbBox3f bbox = getBBox();
	 if ( bbox.isEmpty() || SoCullElement::cullTest(state, bbox, true ) )
	     newres = -1;
	 else if ( desiredresolution_==-1 )
	     newres = getAutoResolution( state );
     }

     setActualResolution( newres );
}


SbBox3f HorizonSectionTile::getBBox() const
{
    return  (!bboxstart_.isDefined() || !bboxstop_.isDefined()) ? SbBox3f() :
	SbBox3f( bboxstart_.x, bboxstart_.y, bboxstart_.z,
		 bboxstop_.x, bboxstop_.y, bboxstop_.z );
}


void HorizonSectionTile::updateBBox()
{
    if ( !needsupdatebbox_ ) return;

    Interval<float> xrg, yrg, zrg;
    bool first = true;
    for ( int idx=0; idx<coords_->size(); idx++ )
    {
	const Coord3 pos = coords_->getPos( idx, true );
	if ( !pos.isDefined() )
	    continue;

	if ( first )
	{
	    xrg.start = xrg.stop = pos.x;
	    yrg.start = yrg.stop = pos.y;
	    zrg.start = zrg.stop = pos.z;
	    first = false;
	}
	else
	{
	    xrg.include( pos.x );
	    yrg.include( pos.y );
	    zrg.include( pos.z );
	}
    }

    if ( first )
	return;

    bboxstart_ = Coord3( xrg.start, yrg.start, zrg.start );
    bboxstop_ = Coord3( xrg.stop, yrg.stop, zrg.stop );
    needsupdatebbox_ = false;
}


int HorizonSectionTile::getAutoResolution( SoState* state ) 
{
    updateBBox();

    if ( !bboxstart_.isDefined() || !bboxstop_.isDefined() )
	return -1;

    const SbBox3f bbox( bboxstart_.x, bboxstart_.y, bboxstart_.z,
	    	        bboxstop_.x, bboxstop_.y, bboxstop_.z );
    if ( bbox.isEmpty() )
	return -1;
    
    const int32_t camerainfo = SoCameraInfoElement::get(state);
    if ( camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE) )
	return mLowestResIdx;

    SbVec2s screensize;
    SoShape::getScreenSize( state, bbox, screensize );
    const float complexity = SbClamp(SoComplexityElement::get(state),0.0f,1.0f);
    const float wantednumcells = complexity*screensize[0]*screensize[1]/64;

    for ( int desiredres=mLowestResIdx; desiredres>=0; desiredres-- )
    {
	if ( nrcells_[desiredres]>wantednumcells )
	    return desiredres;
    }

    return 0;
}


void HorizonSectionTile::setActualResolution( int resolution )
{
    if ( resolution==getActualResolution() ) 
	return;

    root_->lock.writeLock();
    resswitch_->whichChild.setValue( resolution );
    root_->lock.writeUnlock();

    resolutionhaschanged_ = true;
}


#define mAddInitialTriangle( ci0, ci1, ci2, triangle ) \
{ \
    isstripterminated =  false; \
    mAddIndex( ci0, triangle, stripidx ); \
    mAddIndex( ci1, triangle, stripidx ); \
    mAddIndex( ci2, triangle, stripidx ); \
}


#define mTerminateStrip( triangle ) \
if ( !isstripterminated ) \
{ \
    isstripterminated = true; \
    mAddIndex( -1, triangle, stripidx ); \
}


#define mAddIndex( ci, obj, objidx ) \
{ \
    root_->lock.writeLock(); \
    obj->coordIndex.set1Value( objidx, ci ); \
    obj->textureCoordIndex.set1Value( objidx, ci ); \
    obj->normalIndex.set1Value( objidx, getNormalIdx(ci,res) ); \
    objidx++; \
    root_->lock.writeUnlock(); \
}

#define m00 0
#define m01 1
#define m02 2
#define m10 3
#define m11 4
#define m12 5
#define m20 6
#define m21 7
#define m22 8

#define mNrBlocks( res) \
    mTileSideSize/spacing_[res] + (mTileSideSize%spacing_[res] ? 0 : -1)


void HorizonSectionTile::tesselateResolution( int res )
{
    if ( res<0 ) return;

    const int nrmyblocks = mNrBlocks(res);
    int stripidx = 0, lnidx = 0, ptidx = 0;

    for ( int ridx=0; ridx<=nrmyblocks; ridx++ )
    {
	int ci11 = spacing_[res]*ridx*mNrCoordsPerTileSide;
	int ci21 = ci11 + spacing_[res]*mNrCoordsPerTileSide;

	bool nbdef[] = { false, false, false,		// 00   01     02
	    		 false, false, false,		// 10   11(me) 12
			 false, false, false };		// 20   21     22

	root_->lock.readLock();
	nbdef[m11] = coords_->isDefined(ci11);
	nbdef[m21] = ridx!=nrmyblocks ? coords_->isDefined(ci21)
					: false;
	if ( ridx )
	{
	    int ci01 = ci11 - spacing_[res]*mNrCoordsPerTileSide;
	    nbdef[m01] = coords_->isDefined(ci01);
	}
	root_->lock.readUnlock();

	bool isstripterminated = true;
	for ( int cidx=0; cidx<=nrmyblocks; cidx++ )
	{
	    const int ci12 = ci11 + spacing_[res];
	    const int ci22 = ci21 + spacing_[res];
	    
	    root_->lock.readLock();
	    nbdef[m12] = cidx!=nrmyblocks ? coords_->isDefined(ci12)
					    : false;
	    nbdef[m22] = (cidx==nrmyblocks || ridx==nrmyblocks) ? 
		false : coords_->isDefined(ci22);
	    
	    int ci02 = ci12 - spacing_[res]*mNrCoordsPerTileSide;
	    nbdef[m02] = ridx ? coords_->isDefined(ci02) : false;
	    root_->lock.readUnlock();

	    const int defsum = nbdef[m11]+nbdef[m12]+nbdef[m21]+nbdef[m22];
	    if ( defsum<3 ) 
	    {
		mTerminateStrip( triangles_[res] );
		if ( ridx<nrmyblocks && cidx<nrmyblocks && nbdef[m11] && 
		     (nbdef[m12] || nbdef[m21]) )
		{
		    mAddIndex( ci11, lines_[res], lnidx );
		    mAddIndex( nbdef[m12] ? ci12 : ci21, lines_[res], lnidx );
		    mAddIndex( -1, lines_[res], lnidx );
		}
		else if ( nbdef[m11] && !nbdef[m10] && !nbdef[m12] && 
			 !nbdef[m01] && !nbdef[m21] )
		{
		    mAddIndex( ci11, points_[res], ptidx ) 
		} 
	    }
	    else if ( defsum==3 )
	    {
		mTerminateStrip( triangles_[res] );
		if ( !nbdef[m11] )
		    mAddInitialTriangle( ci12, ci21, ci22, triangles_[res] )
		else if ( !nbdef[m21] )
		    mAddInitialTriangle( ci11, ci22, ci12, triangles_[res] )
		else if ( !nbdef[m12] )
		    mAddInitialTriangle( ci11, ci21, ci22, triangles_[res] )
		else
		    mAddInitialTriangle( ci11, ci21, ci12, triangles_[res] )
		mTerminateStrip( triangles_[res] );
	    }
	    else
	    {
		root_->lock.readLock();
		const float diff0 = coords_->getPos(ci11,true).z-
				    coords_->getPos(ci22,true).z;
		const float diff1 = coords_->getPos(ci12,true).z-
				    coords_->getPos(ci21,true).z;
		root_->lock.readUnlock();

		const bool do11to22 = fabs(diff0) < fabs(diff1);
		if ( do11to22 )
		{
		    mTerminateStrip( triangles_[res] );
		    mAddInitialTriangle( ci21, ci22, ci11, triangles_[res] );
		    mAddIndex( ci12, triangles_[res], stripidx );
		    mTerminateStrip( triangles_[res] );
		}
		else
		{
		    if ( isstripterminated )
		    {
			mAddInitialTriangle(ci11, ci21, ci12, triangles_[res]);
			mAddIndex( ci22, triangles_[res], stripidx );
		    }
		    else
		    {
			mAddIndex( ci12, triangles_[res], stripidx );
			mAddIndex( ci22, triangles_[res], stripidx );
		    }
		}
	    } 
	
	    nbdef[m00] = nbdef[m01]; nbdef[m01] = nbdef[m02];
    	    nbdef[m10] = nbdef[m11]; nbdef[m11] = nbdef[m12];
    	    nbdef[m20] = nbdef[m21]; nbdef[m21] = nbdef[m22];
    	    ci11 = ci12; ci21 = ci22;
	}

	mTerminateStrip( triangles_[res] );
    }

    root_->lock.writeLock();
    triangles_[res]->coordIndex.deleteValues( stripidx, -1 ); 
    triangles_[res]->normalIndex.deleteValues( stripidx, -1 ); 
    triangles_[res]->textureCoordIndex.deleteValues( stripidx, -1 );
    lines_[res]->coordIndex.deleteValues( lnidx, -1 );
    lines_[res]->normalIndex.deleteValues( lnidx, -1 );
    lines_[res]->textureCoordIndex.deleteValues( lnidx, -1 );
    points_[res]->coordIndex.set1Value( ptidx, -1 );
    points_[res]->normalIndex.set1Value( ptidx, -1 );
    points_[res]->textureCoordIndex.set1Value( ptidx, -1 );
    root_->lock.writeUnlock();
    
    needsretesselation_[res] = false;
}


void HorizonSectionTile::useWireframe( bool yn )
{
    usewireframe_ = yn;
    turnOnWireframe( yn ? desiredresolution_ : -1 );
}


void HorizonSectionTile::turnOnWireframe( int res )
{
    for ( int idx=0; idx<mHorSectNrRes; idx++ )
    {
	if ( idx==res && wireframes_[idx]->coordIndex.getNum()<2 )
	    setWireframe( res );

	wireframeswitch_[idx]->whichChild = (usewireframe_ && idx==res) ? 0 : 1;
    }

    gluelowdimswitch_->whichChild = 
	(usewireframe_ && res!=-1) ? SO_SWITCH_NONE : SO_SWITCH_ALL;
}


#define mAddWireframeIndex( ci0, ci1 ) \
{ \
    root_->lock.writeLock(); \
    wireframes_[res]->coordIndex.set1Value( lnidx, ci0 ); \
    wireframes_[res]->normalIndex.set1Value( lnidx++, getNormalIdx(ci0,res) );\
    wireframes_[res]->coordIndex.set1Value( lnidx, ci1 ); \
    wireframes_[res]->normalIndex.set1Value( lnidx++, getNormalIdx(ci1,res) );\
    wireframes_[res]->coordIndex.set1Value( lnidx, -1 ); \
    wireframes_[res]->normalIndex.set1Value( lnidx++, -1 ); \
    root_->lock.writeUnlock(); \
}


void HorizonSectionTile::setWireframe( int res )
{
    if ( res<0 && usewireframe_ )
    {
	turnOnWireframe( res );
	return;
    }

    const int tilesz = mTileSideSize/spacing_[res] + 
		     ( mTileSideSize%spacing_[res] ? 1 : 0 );
    int lnidx = 0;
    for ( int idx=0; idx<=tilesz; idx++ )
    {
	const int rowstartidx = 
	     idx<tilesz ? idx*spacing_[res]*mNrCoordsPerTileSide
			: mNrCoordsPerTileSide*mTileSideSize; 
	for ( int idy=0; idy<=tilesz; idy++ )
	{
	    const int colshift = idy<tilesz ? idy*spacing_[res] : mTileSideSize;
	    const int ci0 = rowstartidx + colshift;
	    if ( !coords_->isDefined(ci0) ) 
		continue;

	    if ( idy<tilesz )
	    {
    		const int nexthorci = idy==tilesz-1 ? rowstartidx+mTileSideSize 
						    : ci0 + spacing_[res];
    		if ( coords_->isDefined( nexthorci ) )
		    mAddWireframeIndex( ci0, nexthorci );
	    }

	    if ( idx<tilesz )
	    {		
    		const int nextvertci = idx==tilesz-1 
    		    ? mNrCoordsPerTileSide*mTileSideSize+colshift 
    		    : ci0 + spacing_[res]*mNrCoordsPerTileSide;
		if ( coords_->isDefined( nextvertci ) )
		    mAddWireframeIndex( ci0, nextvertci );
	    }
	}
    }

    wireframes_[res]->textureCoordIndex.deleteValues( lnidx, -1 );
    wireframes_[res]->coordIndex.deleteValues( lnidx, -1 );
    wireframes_[res]->normalIndex.deleteValues( lnidx, -1 );
}


void HorizonSectionTile::setNeighbor( int nbidx, HorizonSectionTile* nb )
{
    if ( (nbidx==5 || nbidx==7 || nbidx==8 ) && neighbors_[nbidx]!=nb )
	glueneedsretesselation_ = true;

    neighbors_[nbidx] = nb;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    if ( row>=0 && row<mNrCoordsPerTileSide && 
	 col>=0 && col<mNrCoordsPerTileSide )
    {
	const int posidx = row*mNrCoordsPerTileSide+col;
	const char oldnewdefined = 
	    coords_->isDefined(posidx) + pos.isDefined();
	coords_->setPos( posidx, pos );

	if ( !oldnewdefined ) return;

	if ( oldnewdefined==1 )
	{
	    for ( int res=0; res<mHorSectNrRes; res++ )
	    {
		if ( !needsretesselation_[res] )
		{
		    if ( !(row % spacing_[res]) && !(col % spacing_[res]) )
			needsretesselation_[res] = true;
		}
	    }

	    glueneedsretesselation_ = true;
	    needsupdatebbox_ = true;
	}
    }

    setInvalidNormals( row, col );
}


void HorizonSectionTile::setInvalidNormals( int row, int col )
{ 
    for ( int res=0; res<mHorSectNrRes; res++ )
    {
	if ( allnormalsinvalid_[res] )
	{
	    invalidnormals_[res].erase();
	    continue;
	}

	int rowstart = row-spacing_[res];
	if ( rowstart>mTileSideSize ) continue;
	if ( rowstart<0 ) rowstart = 0;

	int rowstop = row+spacing_[res];
	if ( rowstop<0 ) continue;
	if ( rowstop>mTileSideSize ) rowstop = mTileLastIdx;

	for ( int rowidx=rowstart; rowidx<=rowstop; rowidx++ )
	{
	    if ( rowidx%spacing_[res] && rowidx!=mTileLastIdx ) continue;
	    const int nmrow = rowidx==mTileLastIdx ? normalsidesize_[res]-1 
						   : rowidx/spacing_[res];

	    int colstart = col-spacing_[res];
	    if ( colstart>mTileSideSize ) continue;
	    if ( colstart<0 ) colstart = 0;

	    int colstop = col+spacing_[res];
	    if ( colstop<0 ) continue;
	    if ( colstop>mTileSideSize ) colstop = mTileLastIdx;

	    for ( int colidx=colstart; colidx<=colstop; colidx++ )
	    {
		if ( colidx%spacing_[res] && colidx!=mTileLastIdx ) continue;
		const int nmcol = colidx==mTileLastIdx ? normalsidesize_[res]-1
						       : colidx/spacing_[res];
		invalidnormals_[res].addIfNew( 
			normalstartidx_[res]+nmrow*normalsidesize_[res]+nmcol );
	    }
	}
    }
}


void HorizonSectionTile::updateGlue()
{
    if ( glueneedsretesselation_ || resolutionhaschanged_ ||
	 (neighbors_[5] && neighbors_[5]->resolutionhaschanged_) ||
	 (neighbors_[7] && neighbors_[7]->resolutionhaschanged_) ||
	 (neighbors_[8] && neighbors_[8]->resolutionhaschanged_) )
    {
	tesselateGlue();
	glueneedsretesselation_ = false;
    }
}


#define mAddGlueIndices( i0, i1, i2 ) \
{ \
    root_->lock.readLock(); \
    const bool df0 = coords_->isDefined(i0); \
    const bool df1 = coords_->isDefined(i1); \
    const bool df2 = coords_->isDefined(i2); \
    root_->lock.readUnlock(); \
    const int dfsum = df0 + df1 + df2; \
    if ( dfsum==1 ) \
    { \
	int ptidx = df0 ? i0 : ( df1 ? i1 : i2 ); \
	mAddIndex( ptidx, gluepoints_, ptidx ); \
    }\
    else if ( dfsum==2 ) \
    { \
	if ( df0 ) mAddIndex( i0, gluelines_, lnidx ) \
	if ( df1 ) mAddIndex( i1, gluelines_, lnidx ) \
	if ( df2 ) mAddIndex( i2, gluelines_, lnidx ) \
	mAddIndex( -1, gluelines_, lnidx ) \
    } \
    else if ( dfsum==3 ) \
    { \
	mAddIndex( i0, gluetriangles_, stripidx ) \
	mAddIndex( i1, gluetriangles_, stripidx ) \
	mAddIndex( i2, gluetriangles_, stripidx ) \
	mAddIndex( -1, gluetriangles_, stripidx ) \
    } \
}


void HorizonSectionTile::tesselateGlue()
{
    const int res = getActualResolution();
    int stripidx = 0, lnidx = 0, ptidx = 0;
    
    if ( res!=-1 )
    {
	const int nrmyblocks = mNrBlocks( res );	
	TypeSet<int> edgeindices; //Get all my own edge knot indices 
	for ( int idx=0; idx<2*(nrmyblocks+1); idx++ )
	{
	    edgeindices += ( idx<=nrmyblocks 
		? spacing_[res]*(nrmyblocks+idx*mNrCoordsPerTileSide) 
		: spacing_[res]*(mNrCoordsPerTileSide*nrmyblocks+idx-
		    		 nrmyblocks-1) );
	}

	for ( int nb=5; nb<8; nb += 2 )//Only consider neighbor 5 and 7
	{
	    int nbres = neighbors_[nb] ? neighbors_[nb]->getActualResolution() 
				       : res; 
	    if ( nbres==-1 ) nbres = res; 
	    const int nbblocks = mNrBlocks( nbres ); 
	    TypeSet<int> nbindices; 
	    for ( int idx=0; idx<=nbblocks; idx++ ) 
	    {
		nbindices += 
		    (nb==5 ? (1+idx*spacing_[nbres])*mNrCoordsPerTileSide-1
		     : mNrCoordsPerTileSide*mTileSideSize+idx*spacing_[nbres]);
	    }

	    const bool  highres = nrmyblocks >= nbblocks;
	    const int highstartidx = highres ? (nb==5 ? 0 : nrmyblocks+1) : 0;
	    const int highstopidx = highres ? 
		(nb==5 ? nrmyblocks : 2*nrmyblocks+1) : nbblocks;
	    const int nrconns = spacing_[abs(nbres-res)];
	    
	    int lowresidx = highres ? 0 : (nb==5 ? 0 : nrmyblocks+1); 
	    int skipped = 0; 
	    for ( int idx=highstartidx; idx<highstopidx; idx++ ) 
	    {
		int i0 = highres ? edgeindices[idx] : edgeindices[lowresidx]; 
		int i1 = highres ? nbindices[lowresidx] : nbindices[idx]; 
		int i2 = highres ? edgeindices[idx+1] : nbindices[idx+1]; 
		if ( nb==7 ) mAddGlueIndices( i0, i1, i2 ) 
		if ( nb==5 ) 
		{
		    mAddGlueIndices( i0, i2, i1 );
		    if ( !idx )
		    {
			if ( highres && nbblocks )
			    mAddGlueIndices( i1, i2, nbindices[1] )
			else if ( !highres && nrmyblocks )
			    mAddGlueIndices( i0, edgeindices[1], i2 )
		    }
		}

		skipped++;

		if ( skipped%nrconns && idx-highstartidx+1!=nrconns/2 ) 
		    continue; 

		skipped = 0; 
		if ( lowresidx+1<(highres ? nbblocks+1 : edgeindices.size()) ) 
		{ 
		    lowresidx++; 
		    i0 = highres ? edgeindices[idx+1] :edgeindices[lowresidx-1]; 		    i1 = highres ? nbindices[lowresidx-1] : nbindices[idx+1]; 
		    i2 = highres ? nbindices[lowresidx]:edgeindices[lowresidx];
		    if ( nb==7 ) mAddGlueIndices( i0, i1, i2 ) 
		    if ( nb==5 ) mAddGlueIndices( i0, i2, i1 )	
		} 
	    }
	    
	    if ( nb==5 )	    
	    {
		mAddGlueIndices( nbindices[nbblocks], edgeindices[nrmyblocks], 
				 mTotalNrCoordsPerTile-1 );
	    }
	    else if ( nb==7 )
	    {
		mAddGlueIndices( edgeindices[nrmyblocks], nbindices[nbblocks],
				 mTotalNrCoordsPerTile-1 ); 
	    }
	}
    }
   
    root_->lock.writeLock(); 
    gluetriangles_->textureCoordIndex.deleteValues( stripidx, -1 );
    gluetriangles_->coordIndex.deleteValues( stripidx, -1 );
    gluetriangles_->normalIndex.deleteValues( stripidx, -1 );
    gluelines_->textureCoordIndex.deleteValues( lnidx, -1 );
    gluelines_->coordIndex.deleteValues( lnidx, -1 );
    gluelines_->normalIndex.deleteValues( lnidx, -1 );
    gluepoints_->textureCoordIndex.deleteValues( ptidx, -1 );
    gluepoints_->coordIndex.deleteValues( ptidx, -1 );
    gluepoints_->normalIndex.deleteValues( ptidx, -1 );
    root_->lock.writeUnlock();
}


void HorizonSectionTile::setTextureSize( int rowsz, int colsz )
{ texture_->size.setValue( 1, rowsz, colsz ); }


void HorizonSectionTile::setTextureOrigin( int row, int col )
{ texture_->origin.setValue( 0, row, col ); }


}; // namespace visBase

