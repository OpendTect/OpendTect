/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: vishorizonsection.cc,v 1.12 2009-04-16 04:16:29 cvsranojay Exp $";

#include "vishorizonsection.h"

#include "binidsurface.h"
#include "binidvalset.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "task.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistexturechannels.h"
#include "vistexturechannel2rgba.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include "SoCameraInfo.h"
#include "SoCameraInfoElement.h"
#include "SoIndexedPointSet.h"
#include "SoLockableSeparator.h"
#include "SoTextureComposer.h"
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoSwitch.h>

mCreateFactoryEntry( visBase::HorizonSection );

namespace visBase
{

mClass TileResolutionTesselator: public ParallelTask
{
public:    
TileResolutionTesselator( HorizonSectionTile& tile, int res, int nrblocks, 
	int& stripidx, int& lineidx, int& pointidx )
    : tile_( tile )
    , nrblocks_( nrblocks ) 
    , res_( res )			    
    , spacing_( (int)pow(2.0,res) )
    , stripidx_( stripidx )
    , lineidx_( lineidx )
    , pointidx_( pointidx )
{}

od_int64 totalNr() const { return nrblocks_+1; }

protected:

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    for ( int ridx=start; ridx<=stop && shouldContinue(); ridx++ )
    {
	int chainlength = 0; 
	TypeSet<int> stripindices;
	TypeSet<int> stripsizes;
	for ( int cidx=0; cidx<=nrblocks_; cidx++ )
	{
	    const int idx0 = spacing_*(cidx+ridx*mHorSectNrSideKnots);
	    const bool defpos0 = tile_.getCoords()->getPos(idx0).isDefined();
	    const int idx1 = idx0+mHorSectNrSideKnots*spacing_;

	    if ( defpos0 )
	    {
		if ( cidx && ridx<nrblocks_ && 
		     !tile_.getCoords()->getPos(idx1-spacing_).isDefined() && 
		     tile_.getCoords()->getPos(idx0-spacing_).isDefined() )
		{
		    stripindices += idx0-spacing_;
		    chainlength = 1;
		}

		stripindices += idx0;
		chainlength++;

	    }
	    else if ( chainlength ) 
	    {
		stripsizes += chainlength;
		chainlength = 0;
	    }
	   
	    if ( ridx==nrblocks_ ) continue;

	    if ( tile_.getCoords()->getPos(idx1).isDefined() )
	    {
		if ( cidx && !defpos0 && 
			tile_.getCoords()->getPos(idx1-spacing_).isDefined() )
		{
		    stripindices += idx1-spacing_;
		    chainlength = 1;
		}

		stripindices += idx1;
		chainlength++;
	    }
	    else if ( chainlength )
	    {
		stripsizes += chainlength;		
		chainlength = 0;
	    }
	}
	    
	if ( !stripsizes.size() )
	    stripsizes += chainlength;

    }

    return true;
}

    od_int64			nrblocks_;
    int				spacing_;
    int				res_;
    HorizonSectionTile& 	tile_;
    int&			stripidx_;
    int&			lineidx_;
    int&			pointidx_;
};    


mClass HorizonTileCreater: public ParallelTask
{
public:
HorizonTileCreater( HorizonSection& hor, const StepInterval<int>& rrg, 
	const StepInterval<int>& crg, int nrrows, int nrcols )
    : hor_( hor )
    , rrg_( rrg )
    , crg_( crg )
    , nrrows_( nrrows )
    , nrcols_( nrcols_ )  		       
{}

od_int64	totalNr() const { return nrrows_*nrcols_; }

protected:
bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	int rc[2];
	hor_.tiles_.info().getArrayPos( idx, rc );
	const int startrow = rc[0]*mHorSectSideSize + rrg_.start;
	const int startcol = rc[1]*mHorSectSideSize + crg_.start;

	HorizonSectionTile* tile = new HorizonSectionTile();
	tile->setTextureComposerSize( mHorSectSideSize+2,
				      mHorSectSideSize+2 );
	tile->setTextureComposerOrig( startrow, startcol );
	
	for ( int r=0; r<=mHorSectSideSize; r++ )
	{
	    for ( int c=0; c<=mHorSectSideSize; c++ )
	    {
		if ( startrow+r<=rrg_.stop && startcol+c<=crg_.stop )
		    tile->setPos( r, c, hor_.geometry_->getKnot(
				RowCol(startrow+r,startcol+c), false) );
		else
		    tile->setPos( r, c, Coord3::udf() );
	    }
	}

	tile->setResolution( 5 );
	hor_.tiles_.set( rc[0], rc[1], tile );
	hor_.addChild( tile->getNodeRoot() );
    }

    return true;
}

    HorizonSection&		hor_;
    const StepInterval<int>&	rrg_;
    const StepInterval<int>&	crg_;
    int				nrrows_;
    int				nrcols_;

};



HorizonSection::HorizonSection() 
    : VisualObjectImpl( false )
    , callbacker_( new SoCallback )  
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , geometry_( 0 )
    , channels_( TextureChannels::create() )		   
    , channel2rgba_( ColTabTextureChannel2RGBA::create() ) 
    , wireframelines_( visBase::PolyLine::create() )		   
    , tiles_( 0, 0 )					   
{
    callbacker_->ref();
    callbacker_->setCallback( updateResolution, this );
    addChild( callbacker_ );

    channel2rgba_->ref();
    channel2rgba_->allowShading( true );

    channels_->ref();
    addChild( channels_->getInventorNode() );
    channels_->setChannels2RGBA( channel2rgba_ );
    if ( channels_->nrChannels()<1 )
    {
	channels_->addChannel();
	channel2rgba_->setEnabled( 0, true );
    }

    wireframelines_->ref();
    wireframelines_->setMaterial( Material::create() );
    addChild( wireframelines_->getInventorNode() );
}


HorizonSection::~HorizonSection()
{
    channels_->unRef();
    channel2rgba_->unRef();

    callbacker_->unref();
    if ( transformation_ ) transformation_->unRef();
    if ( zaxistransform_ ) zaxistransform_->unRef();
    
    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	removeChild( tileptrs[idx]->getNodeRoot() );
	delete tileptrs[idx];
    }

    if ( wireframelines_ )
    {
	removeChild( wireframelines_->getInventorNode() );
	wireframelines_->unRef();
    }
}


void HorizonSection::replaceChannels( TextureChannels* nt )
{
    if ( !nt ) return;
    
    if ( channels_ )
    {
	removeChild( channels_->getInventorNode() );
	channels_->unRef();
    }
    
    channels_ = nt;
    channels_->ref();
}


void HorizonSection::setDisplayTransformation( Transformation* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
    {
	transformation_->ref();

	if ( wireframelines_ )
	    wireframelines_->setDisplayTransformation( nt );

	HorizonSectionTile** tileptrs = tiles_.getData();
	for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
	{
	    if ( tileptrs[idx] )
		tileptrs[idx]->getCoords()->setDisplayTransformation( nt );
	}
    }
}


Transformation* HorizonSection::getDisplayTransformation()
{ return transformation_; }


void HorizonSection::setZAxisTransform( ZAxisTransform* zt )
{
    if ( zaxistransform_ )
	zaxistransform_->unRef();

    zaxistransform_ = zt;
    if ( zaxistransform_ )
    	zaxistransform_->ref();
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


void HorizonSection::setDisplayData( const BinIDValueSet* data )
{
    if ( !data || !geometry_ ) 
	return;

    for ( int idx=0; idx<data->totalSize(); idx++ )
    {
	const BinIDValueSet::Pos& bp = data->getPos(idx);
	BinID bid = data->getBinID( bp );
	geometry_->setKnot( bid,
		Coord3( SI().transform(bid), data->getVal(bp,idx) ) );
    }
}


void HorizonSection::setGeometry( Geometry::BinIDSurface* ng )
{
    if ( !ng || geometry_==ng ) 
	return;

    geometry_ = ng;
    updateGeometry();
}


void HorizonSection::updateGeometry()
{
    if ( !geometry_ )
	return;

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	removeChild( tileptrs[idx]->getNodeRoot() );
	delete tileptrs[idx];
    }

    const StepInterval<int>& rrg = geometry_->rowRange();
    const StepInterval<int>& crg = geometry_->colRange();
    const int nrrowtiles = nrBlocks(rrg.width(),mHorSectSideSize,1);
    const int nrcoltiles = nrBlocks(crg.width(),mHorSectSideSize,1);

    if ( !geometry_->getArray() )
	channels_->turnOn( false );
    else
    {
	const Array2D<float>& arr = *geometry_->getArray();
	channels_->setSize( 1, arr.info().getSize(0), arr.info().getSize(1) );

	if ( !arr.getData() )
	{
	    const int bufsz = arr.info().getTotalSz();
	    mDeclareAndTryAlloc(float*,ptr, float[bufsz]);
	    if ( !ptr )
		channels_->turnOn( false );
	    else
	    {
		arr.getAll( ptr );
		channels_->setUnMappedData(0,0,ptr,TextureChannels::TakeOver);
	    }
	}
	else
	{
   	    channels_->setUnMappedData( 0, 0, arr.getData(),
		    			TextureChannels::Cache );
	}

	channels_->turnOn( true );
    }

    if ( !tiles_.setSize(nrrowtiles,nrcoltiles) )
	return;

    while ( wireframelines_->size() )
	wireframelines_->removePoint( 0 );

    wireframelines_->addPoint( 
	    geometry_->getKnot(RowCol(rrg.start,crg.start),false) );
    wireframelines_->addPoint( 
	    geometry_->getKnot(RowCol(rrg.start,crg.stop),false) );
    wireframelines_->addPoint( 
	    geometry_->getKnot(RowCol(rrg.stop,crg.stop),false) );
    wireframelines_->addPoint( 
	    geometry_->getKnot(RowCol(rrg.stop,crg.start),false) );
    wireframelines_->addPoint( 
	    geometry_->getKnot(RowCol(rrg.start,crg.start),false) );

    for ( int row=0; row<nrrowtiles; row++ )
    {
	const int startrow = row*mHorSectSideSize + rrg.start;
	for ( int col=0; col<nrcoltiles; col++ )
	{
	    const int startcol = col*mHorSectSideSize + crg.start;
	    HorizonSectionTile* tile = new HorizonSectionTile();
	    tile->setTextureComposerSize( mHorSectSideSize+2,
		    			  mHorSectSideSize+2 );
	    tile->setTextureComposerOrig( startrow, startcol );

	    for ( int r=0; r<=mHorSectSideSize; r++ )
	    {
		for ( int c=0; c<=mHorSectSideSize; c++ )
		{
		    if ( startrow+r<=rrg.stop && startcol+c<=crg.stop )
    			tile->setPos( r, c, geometry_->getKnot(
    				    RowCol(startrow+r,startcol+c), false) );
		    else
			tile->setPos( r, c, Coord3::udf() );
		}
	    }

	    tile->computeAllNormals();
	    tile->setResolution( 5 );
	    tiles_.set( row, col, tile );
	    addChild( tile->getNodeRoot() );
	}
    }
    
    for ( int row=0; row<nrrowtiles; row++ )
    {
	for ( int col=0; col<nrcoltiles; col++ )
	{
	    for ( int i=-1; i<2; i++ )
	    {
		const int r = row+i;
		for ( int j=-1; j<2; j++ )
		{
		    const int c = col+j;
		    char pos;
		    if ( j==-1 )
			pos = i==-1 ? 0 : (!i ? 3 : 6);
		    else if ( j==0 )
			pos = i==-1 ? 1 : (!i ? 4 : 7);
		    else
			pos = i==-1 ? 2 : (!i ? 5 : 8);

		    if ( ( (!r && !c) || (r<0 || r>=nrrowtiles) ||
		         (c<0 || c>=nrcoltiles) ) )
			continue;

		    tiles_.get(row,col)->setNeighbor(pos,tiles_.get(r,c));
		}
	    }
	}
    }
}


void HorizonSection::updateResolution( void* clss, SoAction* action )
{
    if ( action->isOfType(SoGLRenderAction::getClassTypeId()) )
    {
    	SoState* state = action->getState();
    	SoCacheElement::invalidate( state );
    	((HorizonSection*) clss)->updateResolution( state );
    }
}


void HorizonSection::updateResolution( SoState* state )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( tileptrs[idx] )
	    tileptrs[idx]->updateResolution( state );
    }
}


char HorizonSection::nrResolutions() const
{ return mHorSectNrRes; }


char HorizonSection::currentResolution() const
{
    return tiles_.info().getTotalSz() ? tiles_.get(0,0)->getActualResolution()
				      : -1;
}


void HorizonSection::setResolution( char res )
{
    if ( res<0 ) return;

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	//if ( tileptrs[idx] ) tileptrs[idx]->setResolution( res );
    }

    //For test only
    tileptrs[0]->setResolution( res );
    tileptrs[1]->setResolution( res<4 ? res+2 : res-2 );

    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( tileptrs[idx] ) tileptrs[idx]->updateGlue();
    }
}


void HorizonSection::useWireframe( bool yn )
{
    if ( wireframelines_ )
	wireframelines_->turnOn( yn );
}


bool HorizonSection::usesWireframe() const
{
    return wireframelines_ && wireframelines_->isOn();
}


HorizonSectionTile::HorizonSectionTile()
    : root_( new SoLockableSeparator )
    , normals_( new SoNormal )  
    , coords_( visBase::Coordinates::create() )
    , texture_( new SoTextureComposer )
    , resswitch_( new SoSwitch )		
    , gluetriangles_( new SoIndexedTriangleStripSet )
    , gluelines_( new SoIndexedLineSet )
    , glueneedsretesselation_( false )
    , gluepoints_( 0 ) //new SoIndexedPointSet* )
{
    root_->ref();
    coords_->ref();
    normals_->ref();

    root_->addChild( coords_->getInventorNode() );
    root_->addChild( normals_ );
    root_->addChild( texture_ );
    root_->addChild( resswitch_ );
    root_->addChild( gluetriangles_ );
    root_->addChild( gluelines_ );
    //root_->addChild( gluepoints_ );

    for ( int idx=0; idx<mHorSectNrRes; idx++ )
    {
	needsretesselation_[idx] = false;
	resolutions_[idx] = new SoGroup;
	resswitch_->addChild( resolutions_[idx] );

	triangles_[idx] = new SoIndexedTriangleStripSet;
	resolutions_[idx]->addChild( triangles_[idx] );

	lines_[idx] = new SoIndexedLineSet;
	resolutions_[idx]->addChild( lines_[idx] );

	//points_[idx] = new SoIndexedPointSet;
	//resolutions_[idx]->addChild( points_[idx] );
    }

    for ( int idx=0; idx<9; idx++ )
	neighbors_[idx] = 0;

    int normalsum = 0;
    for ( int res=0; res<mHorSectNrRes; res++ )
    {
	spacing[res] = (int)pow( 2.0, res );
	normalstartidx[res] = normalsum;	
	const int sz = mHorSectSideSize/spacing[res]+1;
	normalsum += sz*sz;
    }
}


HorizonSectionTile::~HorizonSectionTile()
{
    coords_->unRef();
    normals_->unref();
    root_->unref();
}


void HorizonSectionTile::computeAllNormals()
{
    int index = 0;
    for ( int res = 0; res<mHorSectNrRes; res++ )
    {
	for ( int row=0; row<=mHorSectSideSize; row++ )
	{
	    if ( row%spacing[res] )
		continue;

    	    for ( int col=0; col<=mHorSectSideSize; col++ )
	    {
		if ( col%spacing[res] )
		    continue;

		Coord3 normal;
		computeNormal(row,col,spacing[res],normal);
    		normals_->vector.set1Value( index,
			SbVec3f(normal[0], normal[1], normal[2] ) );
		index++;
	    }
	}
    }
}


int HorizonSectionTile::getNormalIdx( int crdidx, int res ) const
{
    return crdidx<0 ? -1 : getNormalIdx( crdidx/mHorSectNrSideKnots,  
					 crdidx%mHorSectNrSideKnots, res );
}


int HorizonSectionTile::getNormalIdx( int row, int col, int res ) const
{
    //Normals size = 63*63+32*32+16*16+8*8+4*4+2*2
    if ( res<0 || res>=mHorSectNrRes || row<0 || col<0 ||
	 row>mHorSectSideSize || col>mHorSectSideSize ||
	 row%spacing[res] || col%spacing[res] )
    return -1;

    const int nrsideknots = mHorSectSideSize/spacing[res] + 1;
    return normalstartidx[res]+(row*nrsideknots+col)/spacing[res];
}


bool HorizonSectionTile::computeNormal( int row, int col, int spacing, 
					Coord3& normal )
{
    TypeSet<float> rvals, cvals, zvals, xvals, yvals;
    Coord3 pos;
    BinID bid;
    for ( int idx=-spacing; idx<=spacing; idx++ )
    {
	const int nbrow = row+idx;
	if ( nbrow>=0 && nbrow<=mHorSectSideSize ) 
	{
	    pos = coords_->getPos( nbrow*mHorSectNrSideKnots+col );
	    if ( pos.isDefined() )
	    {
		xvals += pos.x; yvals += pos.y;
		bid = SI().transform( pos );
		rvals += bid.inl; cvals += bid.crl; 
		zvals += pos.z*SI().zFactor();
	    }
	}
    }
	   
    int sz = zvals.size(); 
    const float theta = SI().computeAngleXInl();
    const double sinetheta = sin( theta );
    const double cosinetheta = cos( theta );
    double drow, dcol;
    getGradient( rvals, zvals, sz, 0, 0, &drow );
    
    //double xval, yval;
    //getGradient( xvals, zvals, sz, 0, 0, &xval );

    xvals.erase(); yvals.erase();
    rvals.erase(); cvals.erase(); zvals.erase();
    for ( int idx=-spacing; idx<=spacing; idx++ )
    {
	const int nbcol = col+idx; 	
	if ( nbcol>=0 && nbcol<=mHorSectSideSize )
	{
	    pos = coords_->getPos( row*mHorSectNrSideKnots+nbcol );
	    if ( pos.isDefined() )
	    {
		xvals += pos.x; yvals += pos.y;
		bid = SI().transform( pos );
		rvals += bid.inl; cvals += bid.crl; 
		zvals += pos.z*SI().zFactor();
	    }
	}
    }

    sz = zvals.size();
    //getGradient( yvals, zvals, sz, 0, 0, &yval );
    //normal = Coord3( xval, yval, -1 );
    //return true;

    getGradient( cvals, zvals, sz, 0, 0, &dcol );
    
    if ( mIsUdf(drow) || mIsUdf(dcol) )
	normal = Coord3(0,0,-1);
    else
	normal = Coord3( drow*cosinetheta+dcol*sinetheta,
			 dcol*cosinetheta-drow*sinetheta, -1 );
    return true;
}


void HorizonSectionTile::setResolution( int rs )
{
    if ( getActualResolution() != rs )
    	setActualResolution( rs );
}


int HorizonSectionTile::getActualResolution() const
{
    return resswitch_->whichChild.getValue();
}


void HorizonSectionTile::updateResolution( SoState* st )
{
    if ( !st ) return;

    const int curres = getActualResolution();
    if ( curres==-1 || curres>=mHorSectNrRes )
    	setActualResolution( getAutoResolution(st) );
}


int HorizonSectionTile::getAutoResolution( SoState* state )
{
    SbViewportRegion vp;
    SoGetBoundingBoxAction action( vp );
    action.apply( root_ );
    const SbBox3f bbox = action.getBoundingBox();
    if ( bbox.isEmpty() ) return -1;
    
    const int32_t camerainfo = SoCameraInfoElement::get(state);
    if ( camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE) )
	return mHorSectNrRes-1;

    SbVec2s screensize;
    SoShape::getScreenSize( state, bbox, screensize );
    const float complexity = SbClamp(SoComplexityElement::get(state),0.0f,1.0f);
    const float wantednumcells = complexity*screensize[0]*screensize[1]/32;

    int nrcells = mHorSectSideSize*mHorSectSideSize;
    int desiredres = mHorSectNrRes-1;
    for ( desiredres=mHorSectNrRes-1; desiredres>=0; desiredres-- )
    {
	const int nextnumcells = nrcells/4;
	if ( nextnumcells<wantednumcells )
	    break;

	nrcells = nextnumcells;
    }
    
    return desiredres;
}


void HorizonSectionTile::setActualResolution( int resolution )
{
    if ( resolution==-1 || resolution>=mHorSectNrRes || 
	 getActualResolution()==resolution )
	return;

    resswitch_->whichChild.setValue( resolution );
    needsretesselation_[resolution] = true;
    glueneedsretesselation_ = true;
    
    tesselateResolution( resolution );
}


#define mAddInitialTriangle( ci0, ci1, ci2, triangle ) \
{ \
    isstripterminated =  false; \
    mAddTriangle( ci0, triangle ); \
    mAddTriangle( ci1, triangle ); \
    mAddTriangle( ci2, triangle ); \
}


#define mAddTriangle( ci, triangle ) \
{ \
    triangle->coordIndex.set1Value( stripidx, ci ); \
    triangle->textureCoordIndex.set1Value( stripidx, ci ); \
    triangle->normalIndex.set1Value( stripidx, getNormalIdx(ci,res) ); \
    stripidx++; \
}


#define mTerminateStrip( triangle ) \
if ( !isstripterminated ) \
{ \
    isstripterminated = true; \
    mAddTriangle( -1, triangle ); \
}

#define mAddSinglePoint( ci, point ) \
{ \
    point->coordIndex.set1Value( ptidx, ci ); \
    point->textureCoordIndex.set1Value( ptidx, ci ); \
    point->normalIndex.set1Value( ptidx, getNormalIdx(ci,res) ); \
    ptidx++; \
}


#define mAddLinePoint( ci, line ) \
{ \
    line->coordIndex.set1Value( lnidx, ci ); \
    line->textureCoordIndex.set1Value( lnidx, ci ); \
    line->normalIndex.set1Value( lnidx, getNormalIdx(ci,res) ); \
    lnidx++; \
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

void HorizonSectionTile::tesselateResolution( int res )
{
    if ( res==-1 || !needsretesselation_[res] ) return;
    
    const int nrsideblocks = mHorSectSideSize/spacing[res] + 
			    (mHorSectSideSize%spacing[res] ? 0 : -1);
    int stripidx = 0, lnidx = 0, ptidx = 0;
    for ( int ridx=0; ridx<=nrsideblocks; ridx++ )
    {
	int ci11 = spacing[res]*ridx*mHorSectNrSideKnots;
	int ci21 = ci11 + spacing[res]*mHorSectNrSideKnots;

	bool nbdef[] = { false, false, false,		// 00   01     02
	    		 false, false, false,		// 10   11(me) 12
			 false, false, false };		// 20   21     22
	nbdef[m11] = coords_->getPos(ci11).isDefined();
	nbdef[m21] = ridx!=nrsideblocks ? coords_->getPos(ci21).isDefined()
					: false;
	if ( ridx )
	{
	    int ci01 = ci11 - spacing[res]*mHorSectNrSideKnots;
	    nbdef[m01] = coords_->getPos(ci01).isDefined();
	}

	bool isstripterminated = true;
	for ( int cidx=0; cidx<=nrsideblocks; cidx++ )
	{
	    const int ci12 = ci11 + spacing[res];
	    const int ci22 = ci21 + spacing[res];
	    
	    nbdef[m12] = cidx!=nrsideblocks ? coords_->getPos(ci12).isDefined()
					    : false;
	    nbdef[m22] = (cidx==nrsideblocks || ridx==nrsideblocks) ? 
		false : coords_->getPos(ci22).isDefined();
	    
	    int ci02 = ci12 - spacing[res]*mHorSectNrSideKnots;
	    nbdef[m02] = ridx ? coords_->getPos(ci02).isDefined() : false;

	    const int defsum = nbdef[m11]+nbdef[m12]+nbdef[m21]+nbdef[m22];
	    if ( defsum<3 ) 
	    {
		mTerminateStrip( triangles_[res] );
		if ( nbdef[m11] && ( nbdef[m12] || nbdef[m21]) )
		{
		    mAddLinePoint( ci11, lines_[res] );
		    mAddLinePoint( nbdef[m12] ? ci12 : ci21, lines_[res] );
		    mAddLinePoint( -1, lines_[res] );
		}
		else if ( nbdef[m11] && !nbdef[m10] && !nbdef[m12] && 
			 !nbdef[m01] && !nbdef[m21] )
		{
		    // mAddSinglePoint( c11, points_[res] ) 
		}
	    }
	    else if ( defsum==3 )
	    {
		mTerminateStrip( triangles_[res] );
		if ( !nbdef[11] )
		    mAddInitialTriangle( ci21, ci12, ci22, triangles_[res] )
		else if ( !nbdef[21] )
		    mAddInitialTriangle( ci11, ci12, ci22, triangles_[res] )
		else if ( !nbdef[22] )
		    mAddInitialTriangle( ci11, ci21, ci12, triangles_[res] )
		else
		    mAddInitialTriangle( ci11, ci21, ci22, triangles_[res] )
		mTerminateStrip( triangles_[res] );
	    }
	    else
	    {
		float diff0 = coords_->getPos(ci11).z-coords_->getPos(ci22).z;
		float diff1 = coords_->getPos(ci12).z-coords_->getPos(ci21).z;
		bool do11to22 = fabs(diff0) < fabs(diff1);
		if ( !do11to22 )
		{
		    mTerminateStrip( triangles_[res] );
		    mAddInitialTriangle( ci21, ci22, ci11, triangles_[res] );
		    mAddTriangle( ci12, triangles_[res] );
		    mTerminateStrip( triangles_[res] );
		}
		else
		{
		    if ( isstripterminated )
		    {
			mAddInitialTriangle( ci11, ci21, ci12, triangles_[res]);
			mAddTriangle( ci22, triangles_[res] );
		    }
		    else
		    {
			mAddTriangle( ci12, triangles_[res] );
			mAddTriangle( ci22, triangles_[res] );
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

    triangles_[res]->coordIndex.deleteValues( stripidx, -1 ); 
    triangles_[res]->normalIndex.deleteValues( stripidx, -1 ); 
    triangles_[res]->textureCoordIndex.deleteValues( stripidx, -1 );
    lines_[res]->coordIndex.deleteValues( lnidx, -1 );
    lines_[res]->normalIndex.deleteValues( lnidx, -1 );
    lines_[res]->textureCoordIndex.deleteValues( lnidx, -1 );
   // points_[res]->coordIndex.set1Value( ptidx, -1 );
   // points_[res]->normalIndex.set1Value( ptidx, -1 );
    //points_[res]->textureCoordIndex.set1Value( ptidx, -1 );
    
    needsretesselation_[res] = false;
}


void HorizonSectionTile::setNeighbor( int nbidx, HorizonSectionTile* nb )
{
    if ( nbidx<0 || nbidx>8 || nbidx==4 )
	return;

    if ( (nbidx==5 || nbidx==7) && neighbors_[nbidx]!=nb )
	glueneedsretesselation_ = true;

    neighbors_[nbidx] = nb;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    if ( row<0 || row>mHorSectSideSize || 
	 col<0 || col>mHorSectSideSize )
	return;

    const int posidx = row*mHorSectNrSideKnots+col;
    const Coord3& oldpos = coords_->getPos( posidx );
    if ( oldpos.isDefined() && !pos.isDefined() || 
	 !oldpos.isDefined() && pos.isDefined() )
    {
    	const int res = getActualResolution();  
    	if ( !needsretesselation_[res] && res>-1 && res<mHorSectNrRes )
    	{
    	    if ( (!row % spacing[res]) && (!col % spacing[res]) )
    		needsretesselation_[res] = true;
    	}
    }

    coords_->setPos( posidx, pos );
}


void HorizonSectionTile::updateGlue()
{
    tesselateGlue();
    glueneedsretesselation_ = false;
}


#define mAddGlueIndices() \
{ \
    const int sz = stripindices.size(); \
    for ( int pt=0; pt<sz; pt++ ) \
    { \
	if ( sz>2 ) \
	{ \
	    mAddTriangle( stripindices[pt], gluetriangles_ ) \
	    if ( pt==sz-1 ) \
	    { \
		mAddTriangle( stripindices[0], gluetriangles_ ) \
		mAddTriangle( -1, gluetriangles_ ) \
	    }\
	}\
	else if ( sz==2 ) \
	{ \
	    mAddLinePoint( stripindices[pt], gluelines_ ) \
	    if ( pt==sz-1 ) mAddLinePoint( -1, gluelines_ ) \
	}\
    } \
    stripindices.erase(); \
}
//else if ( sz==1 ) mAddSinglePoint( stripindices[s], gluepoints_ ) \


#define mMakeConnection( nb ) \
{ \
    int nbres = -1; \
    if ( nb==5 && neighbors_[5] ) \
	nbres = neighbors_[5]->getActualResolution(); \
    else if ( nb==7 && neighbors_[7] ) \
	nbres = neighbors_[7]->getActualResolution(); \
    const int nbblocks = nbres==-1 ? nrsideblocks \
    				   : mHorSectSideSize/spacing[nbres] + \
    				   (mHorSectSideSize%spacing[nbres] ? 0 : -1);\
    TypeSet<int> nbindices; \
    const int shift = nb==5 ? 0 : nrsideblocks+1; \
    for ( int idx=0; idx<=nbblocks; idx++ ) \
    { \
	nbindices += (nbres==-1) ? edgeindices[idx+shift]+gluesz : \
	(nb==5 ? (1+idx*spacing[nbres]) : mHorSectNrSideKnots)* \
	mHorSectSideSize + idx*spacing[nbres]; \
    } \
    const bool finer = nrsideblocks >= nbblocks; \
    const int nrconns = nbres==-1 ? 1 : (int)pow(2.0, abs(nbres-res)); \
    const int startidx = finer ? (nb==5 ? 0 : nrsideblocks+1) : 0; \
    const int stopidx = finer ? (nb==5 ? nrsideblocks : 2*nrsideblocks+1) \
    			      : nbblocks; \
    int coarseidx = finer ? 0 : (nb==5 ? 0 : nrsideblocks+1); \
    int i0, i1, i2; \
    int skipped = 0; \
    TypeSet<int> stripindices; \
    for ( int idx=startidx; idx<stopidx; idx++ ) \
    { \
	i0 = finer ? edgeindices[idx] : edgeindices[coarseidx]; \
	i1 = finer ? nbindices[coarseidx] : nbindices[idx]; \
	i2 = finer ? edgeindices[idx+1] : nbindices[idx+1]; \
	if ( coords_->getPos(i0).isDefined() ) stripindices += i0; \
	if ( coords_->getPos(i1).isDefined() ) stripindices += i1; \
	if ( coords_->getPos(i2).isDefined() ) stripindices += i2; \
	mAddGlueIndices() \
	skipped++; \
	if ( skipped%nrconns && idx-startidx+1!=nrconns/2 ) continue; \
	skipped = 0; \
	if ( coarseidx+1<(finer ? nbblocks+1 : edgeindices.size()) ) \
	{ \
	    coarseidx++; \
	    i0 = finer ? edgeindices[idx+1] : edgeindices[coarseidx-1]; \
	    i1 = finer ? nbindices[coarseidx-1] : nbindices[idx+1]; \
	    i2 = finer ? nbindices[coarseidx] : edgeindices[coarseidx]; \
	    if ( coords_->getPos(i0).isDefined() ) stripindices += i0; \
	    if ( coords_->getPos(i1).isDefined() ) stripindices += i1; \
	    if ( coords_->getPos(i2).isDefined() ) stripindices += i2; \
	    mAddGlueIndices() \
	} \
    } \
}


void HorizonSectionTile::tesselateGlue()
{
    const int res = getActualResolution();
    if ( !glueneedsretesselation_ || res==-1 )
	return;
   
    const int nrsideblocks = mHorSectSideSize/spacing[res] + 
			    (mHorSectSideSize%spacing[res] ? 0 : -1);
    const int gluesz = mHorSectSideSize - spacing[res]*nrsideblocks;
    
    TypeSet<int> edgeindices; //Get all my own edge knot indices 
    for ( int ridx=0; ridx<=nrsideblocks; ridx++ )
	edgeindices += spacing[res]*(nrsideblocks+ridx*mHorSectNrSideKnots);

    for ( int cidx=0; cidx<=nrsideblocks; cidx++ )
	edgeindices += spacing[res]*(mHorSectNrSideKnots*nrsideblocks+cidx);

    int stripidx = 0, lnidx = 0, ptidx = 0;
    mMakeConnection( 5 );
    mMakeConnection( 7 );
    
    gluetriangles_->textureCoordIndex.deleteValues( stripidx, -1 );
    gluetriangles_->coordIndex.deleteValues( stripidx, -1 );
    gluetriangles_->normalIndex.deleteValues( stripidx, -1 );
    gluelines_->textureCoordIndex.deleteValues( lnidx, -1 );
    gluelines_->coordIndex.deleteValues( lnidx, -1 );
    gluelines_->normalIndex.deleteValues( lnidx, -1 );
    //gluepoints_->textureCoordIndex.deleteValues( pointidx, -1 );
    //gluepoints_->coordIndex.deleteValues( pointidx, -1 );
    //gluepoints_->normalIndex.deleteValues( pointidx, -1 );
}


void HorizonSectionTile::setTextureComposerSize( int rowsz, int colsz )
{ texture_->size.setValue( rowsz, colsz, 0 ); }


void HorizonSectionTile::setTextureComposerOrig( int globrow, int globcol )
{ texture_->origin.setValue( globrow, globcol, 0 ); }


}; // namespace visBase

