/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: vishorizonsection.cc,v 1.33 2009-05-28 19:11:22 cvsyuancheng Exp $";

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

#define mTileSideSize	64
#define mTileLastIdx	63
#define mLowestResIdx	5

namespace visBase
{

mClass HorSectTileResolutionTesselator : public SequentialTask
{
public:
			HorSectTileResolutionTesselator( 
				HorizonSectionTile& tile,char res,bool doset)
	    		: tile_( tile )
	    		, doset_( doset )
	    		, resolution_( res )		{}

   int			nextStep()
			{
    			    tile_.tesselateResolution( resolution_ );
    			    if ( doset_ ) 
				tile_.setActualResolution( resolution_ );
    			    return Finished();
			}
protected:

   HorizonSectionTile&	tile_;
   int			resolution_;
   bool			doset_;
};


mClass HorSectTileGlueUpdater : public SequentialTask
{
public:
			HorSectTileGlueUpdater( HorizonSectionTile& tile )
	    		: tile_( tile ) {}

   int			nextStep()
   			{
			    tile_.updateGlue();
			    return Finished();
			}
protected:

   HorizonSectionTile&	tile_;
};


mClass HorSectTileWireframeUpdater : public SequentialTask
{
public:
			HorSectTileWireframeUpdater( HorizonSectionTile& tile, 
						     int res )
			: tile_( tile ), res_( res ) {}
    int			nextStep()
    			{
			    tile_.turnOnWireframe( res_ );
			    return Finished();
			}	
protected:

    HorizonSectionTile&  tile_;
    int			 res_;
};


mClass HorizonSectionTilePosSetup: public ParallelTask
{
public:    
HorizonSectionTilePosSetup( HorizonSectionTile& tile, 
			    const Geometry::BinIDSurface& geo, int res, 
			    int startni, int row, int col, RowCol rc )
    : tile_( tile )
    , geo_( geo )  
    , spacing_( (int)pow(2.0,res) )  
    , startnormalidx_( startni )			   
    , startrow_( row )
    , startcol_( col )
    , center_( rc )
    , rowstep_( geo.rowRange().step )
    , colstep_( geo.colRange().step )		  		     
    , rowdistance_( rowstep_*SI().inlDistance() )
    , coldistance_( colstep_*SI().crlDistance() )		    
{
    nodesz_ = mTileSideSize/spacing_ + ( spacing_==1 ? 0 : 1 );
    const double angle = SI().computeAngleXInl();
    sinanglexinl_ = sin( angle );
    cosanglexinl_ = cos( angle );
}

od_int64 nrIterations() const	{ return mTileSideSize*mTileSideSize; }

protected:

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const bool setall = center_.row==-1 || center_.col==-1;
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int row = idx/mTileSideSize;
	const int col = idx%mTileSideSize;
	if ( (row%spacing_ && row<mTileLastIdx) || 
	     (col%spacing_ && col<mTileLastIdx) ) 
	{
	    addToNrDone(1);
	    continue;
	}

	const RowCol rc( row*rowstep_+startrow_, col*colstep_+startcol_ );
	if ( spacing_==1 )
	{
    	    const bool shouldsetpos = setall || ( !setall && 
    		    (abs(rc.row-center_.row)<=spacing_ || 
		     abs(rc.col-center_.col)<=spacing_) );
	    if ( shouldsetpos )
	    {
    		lock_.lock();
    		tile_.setPos( row, col, geo_.getKnot(rc,false) );
    		lock_.unLock();
	    }
	}

	const bool shouldsetnormal = setall;
        //TODO: should update normal when pos changes	
	//const RowCol diff = rc-center_;
	// shouldsetnormal = shouldsetnormal || 
	// ( !setall && (abs(diff.row)<=(spacing_+32) || 
	// 	         abs(diff.col)<=(spacing_+32)) ); 

	if ( shouldsetnormal )
	{
    	    int ni = idx;
	    if ( spacing_>1 )
	    {
		const int localrow = row/spacing_+(row==mTileLastIdx ? 1 : 0);
		const int localcol = col/spacing_+(col==mTileLastIdx ? 1 : 0);
		ni = localrow*nodesz_+localcol+startnormalidx_;
	    }
		
    	    Coord3 normal;
    	    computeNormal( rc.row, rc.col, normal );
	    
	    lock_.lock();
	    tile_.setNormal( ni, normal);
	    lock_.unLock();
	}
	    
	addToNrDone(1);
    }

    return true;
}


void computeNormal( int row, int col, Coord3& nm )
{
    TypeSet<float> posarray, zarray;
    for ( int idx=-spacing_; idx<=spacing_; idx++ )
    {
	const Coord3 pos = geo_.getKnot( RowCol(row+idx*rowstep_,col), false );
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
    for ( int idx=-spacing_; idx<=spacing_; idx++ )
    {
	const Coord3 pos = geo_.getKnot( RowCol(row,col+idx*colstep_), false );
	if ( pos.isDefined() )
	{
	    posarray += idx*coldistance_;
	    zarray += pos.z;
	}
    }

    double dcol = 0;
    if ( zarray.size()>1 )
	getGradient( posarray.arr(), zarray.arr(), zarray.size(), 0, 0, &dcol );

    nm = Coord3( drow*cosanglexinl_ + dcol*sinanglexinl_,
	         dcol*cosanglexinl_ - drow*sinanglexinl_, -1 );
}

    HorizonSectionTile& 		tile_;
    const Geometry::BinIDSurface&	geo_;
    int					startnormalidx_;
    int					nodesz_;
    int					spacing_;
    int					startrow_;
    int					startcol_;
    double				sinanglexinl_;
    double				cosanglexinl_;
    double				rowdistance_;
    double				coldistance_;
    Threads::Mutex			lock_;
    RowCol				center_;
    int					rowstep_;
    int					colstep_;
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

    const int totalnrpts = mTileSideSize*mTileSideSize;
    if ( !texturecoordptr_ )
    {
	mTryAlloc( texturecoordptr_, SbVec2f[totalnrpts] );
	int idx = 0;
	for ( int irow=0; irow<mTileSideSize; irow++ )
	{
	    for ( int icol=0; icol<mTileSideSize; icol++ )
	    {
		texturecoordptr_[idx] = SbVec2f((icol+0.5)/mTileSideSize,
						(irow+0.5)/mTileSideSize);
		idx++;
	    }
	}
    }

    texturecrds_->point.setValuesPointer( totalnrpts, texturecoordptr_ );
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
	updateZAxisVOI( geometry_ );
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


void HorizonSection::updateZAxisVOI( const Geometry::BinIDSurface* surf )
{
    if ( !surf || zaxistransformvoi_==-1 )	//transform does not use voi
	return;

    if ( !zaxistransform_ || !zaxistransform_->needsVolumeOfInterest() )
	return;

    CubeSampling cs;
    cs.hrg.set( surf->rowRange(), surf->colRange() );

    HorSamplingIterator iter( cs.hrg );

    bool first = true;
    BinID curpos;
    while ( iter.next(curpos) )
    {
	const float depth = surf->getKnot(curpos,false).z;
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
    channels_->turnOn( yn ); 
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


void HorizonSection::setSurface( Geometry::BinIDSurface* surf, bool connect )
{
    if ( !surf ) return;

    origin_.row = surf->rowRange().start;
    origin_.col = surf->colRange().start;
    step_.row = surf->rowRange().step;
    step_.col = surf->colRange().step;

    if ( connect )
    {
	geometry_ = surf;
	CallBack cb =  mCB( this, HorizonSection, surfaceChangeCB );
	geometry_->movementnotifier.notify( cb );
	geometry_->nrpositionnotifier.notify( cb );
    }

    CBCapsule<const TypeSet<GeomPosID>*> caps( 0, surf );
    surfaceChangeCB( &caps );
}


void HorizonSection::surfaceChangeCB( CallBacker* cb )
{
    geometrylock_.lock();
    mCBCapsuleUnpackWithCaller( const TypeSet<GeomPosID>*, gpids, caller, cb );
    const Geometry::BinIDSurface* changedsurface =
	reinterpret_cast<const Geometry::BinIDSurface*>(caller);

    if ( zaxistransform_ && zaxistransformvoi_>=0 )
    {
	updateZAxisVOI( changedsurface );
	if ( !zaxistransform_->loadDataIfMissing(zaxistransformvoi_) )
	{
	    geometrylock_.unLock();
	    return;
	}
    }
    
    const StepInterval<int> rrg = changedsurface->rowRange();
    const StepInterval<int> crg = changedsurface->colRange();
    if ( rrg.width(false)<0 || crg.width(false)<0 )
    {
	geometrylock_.unLock();
	return;
    }

    int startni[] = { 0, 4096, 5185, 5474, 5555, 5580 };
    int nrrows = tiles_.info().getSize( 0 );
    int nrcols = tiles_.info().getSize( 1 );
    if ( !origin_.row && !origin_.col )
	origin_ = RowCol( rrg.start, crg.start );
   
    bool neighborchanged = false; 
    if ( !gpids || !nrrows || !nrcols )
    {
	origin_ = RowCol( rrg.start, crg.start );
	nrrows = nrBlocks( rrg.nrSteps()+1, mTileSideSize, 1 );
	nrcols = nrBlocks( crg.nrSteps()+1, mTileSideSize, 1 );

	if ( !tiles_.setSize( nrrows, nrcols ) )
	{
	    geometrylock_.unLock();
	    return;
	}

	for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
	{
	    const int startrow = tilerowidx*mTileLastIdx + origin_.row;
	    for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	    {
		const int startcol = tilecolidx*mTileLastIdx + origin_.col;
		HorizonSectionTile* tile = createTile(tilerowidx, tilecolidx);
		for ( int res=0; res<mHorSectNrRes; res++ )
		{
		    HorizonSectionTilePosSetup tsp( *tile, *changedsurface, res,
			   startni[res], startrow, startcol, RowCol(-1,-1) );
		    tsp.execute();
		}
	    }
	}

	neighborchanged = true;
    }
    else
    {
	RowCol oldorigin = origin_;
	int nrinsertbef = 0, nrinsertaft = 0;
	const int rowsteps =  mTileLastIdx*step_.row;

	while ( rrg.start<origin_.row )
	{
	    nrinsertbef++;
	    origin_.row -= rowsteps;
	}

	if ( nrinsertbef ) 
	{
	    insertRowColTilesArray( true, true, nrinsertbef );
	    neighborchanged = true;
	}

	while ( oldorigin.row+(nrrows+nrinsertaft)*rowsteps<rrg.stop )
	    nrinsertaft++;

	if ( nrinsertaft ) 
	{
	    insertRowColTilesArray( true, false, nrinsertaft );
	    neighborchanged = true;
	}

	nrinsertbef = 0; nrinsertaft = 0;
	const int colsteps = mTileLastIdx*step_.col;
        while ( crg.start<origin_.col )
	{
	    nrinsertbef++;
	    origin_.col -= colsteps;
	}

	if ( nrinsertbef ) 
	{
	    insertRowColTilesArray( false, true, nrinsertbef );
	    neighborchanged = true;
	}

        while ( oldorigin.col+(nrcols+nrinsertaft)*colsteps<crg.stop )
	    nrinsertaft++; 
    
	if ( nrinsertaft ) 
	{
	    insertRowColTilesArray( false, false, nrinsertaft );
	    neighborchanged = true;
	}
    
	nrrows = tiles_.info().getSize( 0 );
	nrcols = tiles_.info().getSize( 1 );

    	for ( int idx=(*gpids).size()-1; idx>=0; idx-- )
	{
	    RowCol absrc( (*gpids)[idx] );
	    RowCol rc = absrc-origin_; rc /= step_;

	    int tilerowidx = rc.row/mTileLastIdx;
	    if ( tilerowidx && !(rc.row%mTileLastIdx) ) tilerowidx--;
	    int tilecolidx = rc.col/mTileLastIdx;
	    if ( tilecolidx && !(rc.col%mTileLastIdx) ) tilecolidx--;
	    const int startrow = tilerowidx*mTileLastIdx + origin_.row;
	    const int startcol = tilecolidx*mTileLastIdx + origin_.col;

	    HorizonSectionTile* tile = tiles_.get( tilerowidx, tilecolidx );
	    bool newtile = false;
	    if ( !tile )
	    {
		newtile = true;
		tile =  createTile( tilerowidx, tilecolidx );
		neighborchanged = true;
	    }

	    //Update pos, normals
	    for ( int res = 0; res<mHorSectNrRes; res++ )
	    {
		HorizonSectionTilePosSetup tsp( *tile, *changedsurface, res,
					startni[res], startrow, startcol, 
					newtile ? RowCol(-1,-1) : absrc );
		tsp.execute();
	    }

	    TypeSet<RowCol> updatedtiles;
	    updatedtiles += RowCol(tilerowidx, tilecolidx);
	    const int maxsz = mTileSideSize/2;
	    for ( int rstep=-maxsz; rstep<=maxsz; rstep += maxsz )
	    {
		for ( int cstep=-maxsz; cstep<=maxsz; cstep += maxsz )
		{
		    RowCol absnbrc( absrc.row+rstep, absrc.col+cstep );
		    if ( absnbrc.row<rrg.start || absnbrc.row>rrg.stop ||
			 absnbrc.col<crg.start || absnbrc.col>crg.stop )
			continue;

		    RowCol relativenbrc = absnbrc-origin_;
		    relativenbrc /= step_;
		    int trowidx = relativenbrc.row/mTileLastIdx;
		    if ( trowidx && !(relativenbrc.row%mTileLastIdx) ) 
			trowidx--;
		    
		    int tcolidx = relativenbrc.col/mTileLastIdx;
		    if ( tcolidx && !(relativenbrc.col%mTileLastIdx) )
			tcolidx--;
		    
		    RowCol tileidx( trowidx, tcolidx );
		    if ( updatedtiles.indexOf(tileidx)!=-1 )
			continue;

		    updatedtiles += tileidx;
		    HorizonSectionTile* nb = tiles_.get(trowidx,tcolidx);
		    if ( !nb ) continue;

		    const int nbrow = trowidx*mTileLastIdx + origin_.row;
		    const int nbcol = tcolidx*mTileLastIdx + origin_.col;
		    for ( int res = 0; res<mHorSectNrRes; res++ )
		    {
			HorizonSectionTilePosSetup tsp( *nb, 
				*changedsurface, res, startni[res], 
				nbrow, nbcol, absnbrc );
			tsp.execute();
		    }
		}
	    }
	}
    }

    if ( neighborchanged )
    {
	updateTileNeighbors( nrrows, nrcols );
	for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
	{
	    for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	    {
		if ( tiles_.get(tilerowidx,tilecolidx) )
		    tiles_.get(tilerowidx,tilecolidx)->setTextureOrigin( 
			    tilerowidx*mTileLastIdx, tilecolidx*mTileLastIdx );
	    }
	}
    }

    geometrylock_.unLock();
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
    addChild( tile->getNodeRoot() );

    return tile;
}


void HorizonSection::insertRowColTilesArray( bool torow, bool before, int nr )
{
    if ( !nr ) return;

    const int rowsz = tiles_.info().getSize(0) + ( torow ? nr : 0 );
    const int colsz = tiles_.info().getSize(1) + ( torow ? 0 : nr );

    Array2DImpl<HorizonSectionTile*> temp( rowsz, colsz );
    for ( int row=0; row<rowsz; row++ )
    {
	for ( int col=0; col<colsz; col++ )
	{
	    HorizonSectionTile* tile = 0;
	    if ( torow && before && row>=nr )
		tile = tiles_.get(row-nr,col);
	    else if ( torow && !before && row<rowsz-nr )
		tile = tiles_.get(row,col);
	    else if ( !torow && before && col>=nr )
		tile = tiles_.get(row,col-nr);
	    else if ( !torow && !before && col<colsz-nr )
		tile = tiles_.get(row,col);

	    temp.set( row, col, tile );
	}
    }

    tiles_.copyFrom( temp );
}

void HorizonSection::updateTileNeighbors( int nrrowtiles, int nrcoltiles )
{
    for ( int row=0; row<nrrowtiles; row++ )
    {
	for ( int col=0; col<nrcoltiles; col++ )
	{
	    HorizonSectionTile* tile = tiles_.get(row,col);
	    if ( !tile ) continue;
	    
	    for ( int i=-1; i<2; i++ )
	    {
		const int r = row+i;
		if ( r<0 || r>=nrrowtiles ) continue;

		for ( int j=-1; j<2; j++ )
		{
		    const int c = col+j;
		    if ( (!r && !c) || (c<0 || c>=nrcoltiles) ) 
			continue;
		    
		    char pos;
		    if ( j==-1 ) 
			pos = i==-1 ? 0 : (!i ? 3 : 6);
		    else if ( j==0 ) 
			pos = i==-1 ? 1 : (!i ? 4 : 7);
		    else if ( j==1 ) 
			pos = i==-1 ? 2 : (!i ? 5 : 8);
		    tile->setNeighbor( pos, tiles_.get(r,c) );
		}
	    }
	}
    }
}


void HorizonSection::updateAutoResolution( void* clss, SoAction* action )
{
    if ( action->isOfType( SoGLRenderAction::getClassTypeId()) )
    {
	SoState* state = action->getState();
	SoCacheElement::invalidate( state );
    	((HorizonSection*) clss)->updateAutoResolution( state );
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


void HorizonSection::updateAutoResolution( SoState* state )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz ) return;
    
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] ) continue;
	tileptrs[idx]->updateAutoResolution( state );
	tileptrs[idx]->tesselateActualResolution();
    }

    ObjectSet<SequentialTask> tasks;
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] ) continue;
	tasks += new HorSectTileGlueUpdater ( *tileptrs[idx] );
    }

    ParallelTask::twm().addWork( tasks );
    deepErase( tasks );

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetResolutionChangeFlag();
}


char HorizonSection::currentResolution() const
{
    return desiredresolution_;
}


void HorizonSection::setResolution( int res )
{
    //if ( desiredresolution_==res && res!=-1 ) return;

    desiredresolution_ = res;
    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz ) return;
    
    if ( usewireframe_ ) turnOnWireframe( res );

    MouseCursorChanger cursorlock( MouseCursor::Wait );

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setResolution( res );

    //if ( res==-1 ) return;

    ObjectSet<SequentialTask> tasks;
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] ) continue;
	tasks += new HorSectTileResolutionTesselator(*tileptrs[idx],res,true);
    }

    ParallelTask::twm().addWork( tasks );
    deepErase( tasks );

    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] ) continue;
	tasks += new HorSectTileGlueUpdater ( *tileptrs[idx] );
    }

    ParallelTask::twm().addWork( tasks );
    deepErase( tasks );

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetResolutionChangeFlag();
}


void HorizonSection::turnOnWireframe( int res )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    
    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();
   
    ObjectSet<SequentialTask> tasks; 
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( tileptrs[idx] ) 
	    tasks += new HorSectTileWireframeUpdater( *tileptrs[idx], res );
    }

    ParallelTask::twm().addWork( tasks );
    deepErase( tasks );
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


HorizonSectionTile::HorizonSectionTile()
    : root_( new SoLockableSeparator )
    , normals_( new SoNormal )  
    , coords_( visBase::Coordinates::create() )
    , texture_( new SoTextureComposer )
    , resswitch_( new SoSwitch )	
    , gluelowdimswitch_( new SoSwitch )					
    , gluetriangles_( new SoIndexedTriangleStripSet )
    , gluelines_( new SoIndexedLineSet )
    , glueneedsretesselation_( false )
    , gluepoints_( new SoDGBIndexedPointSet )
    , desiredresolution_( -1 )
    , resolutionhaschanged_( false )
    , bboxstart_( Coord3::udf() )				    
    , bboxstop_( Coord3::udf() )				   
    , needsupdatebbox_( false )
    , maxspacing_( 32 )			     
    , usewireframe_( false )
    , wireframematerial_( 0 )			   
    , wireframetexture_( visBase::Texture2::create() )		  
    , useshading_( false )						  
{
    root_->ref();
    coords_->ref();
    normals_->ref();
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
	needsretesselation_[idx] = false;
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

    setTextureSize( mTileSideSize, mTileSideSize );
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
    if ( crdidx<0 || res<0 || res>=mHorSectNrRes )
	return -1;

    //Index in the 64*64 tile
    const int row = crdidx/mTileSideSize;
    const int col = crdidx%mTileSideSize;
    if ( row>mTileLastIdx )
    	return -1;

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
	
    	const int nrsideknots = mTileLastIdx/spacing_[useres] +
	    ( useres==0 ? 1 : 2 );
	const int resrow = row/spacing_[useres] +
	    ( useres>0 && row==mTileLastIdx ? 1 : 0 );
	const int rescol = col/spacing_[useres] +
	    ( useres>0 && col==mTileLastIdx ? 1 : 0 );
	
	return normalstartidx_[useres]+resrow*nrsideknots+rescol;
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


void HorizonSectionTile::setResolution( int rs )
{
    desiredresolution_ = rs;
}


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

    if ( state )
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
    mTileLastIdx/spacing_[res] + (mTileLastIdx%spacing_[res] ? 0 : -1)


void HorizonSectionTile::tesselateResolution( int res )
{
    if ( res<0 ) return;

    const int nrmyblocks = mNrBlocks(res);
    int stripidx = 0, lnidx = 0, ptidx = 0;

    for ( int ridx=0; ridx<=nrmyblocks; ridx++ )
    {
	int ci11 = spacing_[res]*ridx*mTileSideSize;
	int ci21 = ci11 + spacing_[res]*mTileSideSize;

	bool nbdef[] = { false, false, false,		// 00   01     02
	    		 false, false, false,		// 10   11(me) 12
			 false, false, false };		// 20   21     22

	root_->lock.readLock();
	nbdef[m11] = coords_->getPos(ci11).isDefined();
	nbdef[m21] = ridx!=nrmyblocks ? coords_->getPos(ci21).isDefined()
					: false;
	if ( ridx )
	{
	    int ci01 = ci11 - spacing_[res]*mTileSideSize;
	    nbdef[m01] = coords_->getPos(ci01).isDefined();
	}
	root_->lock.readUnlock();

	bool isstripterminated = true;
	for ( int cidx=0; cidx<=nrmyblocks; cidx++ )
	{
	    const int ci12 = ci11 + spacing_[res];
	    const int ci22 = ci21 + spacing_[res];
	    
	    root_->lock.readLock();
	    nbdef[m12] = cidx!=nrmyblocks ? coords_->getPos(ci12).isDefined()
					    : false;
	    nbdef[m22] = (cidx==nrmyblocks || ridx==nrmyblocks) ? 
		false : coords_->getPos(ci22).isDefined();
	    
	    int ci02 = ci12 - spacing_[res]*mTileSideSize;
	    nbdef[m02] = ridx ? coords_->getPos(ci02).isDefined() : false;
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

    gluelowdimswitch_->whichChild = (usewireframe_ && res!=-1) ? 
				    SO_SWITCH_NONE : SO_SWITCH_ALL;
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

    const int tilesz = mTileLastIdx/spacing_[res] + 
		     ( mTileLastIdx%spacing_[res] ? 1 : 0 );
    int lnidx = 0;
    for ( int idx=0; idx<=tilesz; idx++ )
    {
	const int rowstartidx = idx<tilesz ? idx*spacing_[res]*mTileSideSize
	    				   : mTileSideSize*mTileLastIdx; 
	for ( int idy=0; idy<=tilesz; idy++ )
	{
	    const int colshift = idy<tilesz ? idy*spacing_[res] : mTileLastIdx;
	    const int ci0 = rowstartidx + colshift;
	    if ( !coords_->getPos(ci0).isDefined() ) 
		continue;

	    if ( idy<tilesz )
	    {
    		const int nexthorci = idy==tilesz-1 ? rowstartidx+mTileLastIdx 
						    : ci0 + spacing_[res];
    		if ( coords_->getPos( nexthorci ).isDefined() )
		    mAddWireframeIndex( ci0, nexthorci );
	    }

	    if ( idx<tilesz )
	    {		
    		const int nextvertci = idx==tilesz-1 
    		    ? mTileSideSize*mTileLastIdx+colshift 
    		    : ci0 + spacing_[res]*mTileSideSize;
		if ( coords_->getPos( nextvertci ).isDefined() )
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
    if ( nbidx<0 || nbidx>8 || nbidx==4 )
	return;

    if ( (nbidx==5 || nbidx==7 || nbidx==8 ) && neighbors_[nbidx]!=nb )
	glueneedsretesselation_ = true;

    neighbors_[nbidx] = nb;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    if ( row<0 || row>mTileLastIdx || 
	 col<0 || col>mTileLastIdx )
	return;

    const int posidx = row*mTileSideSize+col;

    const bool posdefined = pos.isDefined();
    const bool oldposdefined = coords_->isDefined( posidx );

    if ( (posdefined && !oldposdefined) || (!posdefined && oldposdefined) )
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

    coords_->setPos( posidx, pos );
}


void HorizonSectionTile::updateGlue()
{
    if ( glueneedsretesselation_ ||
	 resolutionhaschanged_ ||
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
    const bool df0 = coords_->getPos(i0).isDefined(); \
    const bool df1 = coords_->getPos(i1).isDefined(); \
    const bool df2 = coords_->getPos(i2).isDefined(); \
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
	    edgeindices += ( idx<=nrmyblocks 
		? spacing_[res]*(nrmyblocks+idx*mTileSideSize) 
		: spacing_[res]*(mTileSideSize*nrmyblocks+idx-nrmyblocks-1) );

	for ( int nb=5; nb<8; nb += 2 )
	{
	    int nbres = neighbors_[nb] ? neighbors_[nb]->getActualResolution() 
				       : res; 
	    if ( nbres==-1 ) nbres = res; 
	    const int nbblocks = mNrBlocks( nbres ); 
	    TypeSet<int> nbindices; 
	    for ( int idx=0; idx<=nbblocks; idx++ ) 
		nbindices += (nb==5 ? (1+idx*spacing_[nbres])*mTileSideSize-1
			   : mTileSideSize*mTileLastIdx+idx*spacing_[nbres]);

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
		if ( nb==5 ) mAddGlueIndices( i0, i2, i1 )     
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
				 mTileSideSize*mTileSideSize-1 ); 
	    }
	    else if ( nb==7 )
	    {
		mAddGlueIndices( edgeindices[nrmyblocks], nbindices[nbblocks],
				 mTileSideSize*mTileSideSize-1 ); 
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


void HorizonSectionTile::setMaxSpacing( int spacing )
{ maxspacing_ = spacing; }


void HorizonSectionTile::setTextureSize( int rowsz, int colsz )
{ texture_->size.setValue( 1, rowsz, colsz ); }


void HorizonSectionTile::setTextureOrigin( int row, int col )
{ texture_->origin.setValue( 0, row, col ); }


}; // namespace visBase

