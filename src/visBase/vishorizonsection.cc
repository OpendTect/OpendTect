/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID mUnusedVar = "$Id: vishorizonsection.cc,v 1.128 2012-07-10 08:05:39 cvskris Exp $";

#include "vishorizonsection.h"

#include "binidsurface.h"
#include "binidvalset.h"
#include "cubesampling.h"
#include "coltabmapper.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "mousecursor.h"
#include "posvecdataset.h"
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
#include "SoIndexedLineSet3D.h"
#include "SoTextureComposer.h"
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>

#include <osgGeo/Horizon3D>
#include <osgGeo/LayeredTexture>

mCreateFactoryEntry( visBase::HorizonSection );

#define mMaxLineRadius		20
#define mLineRadius		1.5

#define mMaxNrTiles		15
#define mMaxNrResolutions	10

#define mNoTesselationNeeded	0
#define mShouldRetesselate      1
#define mMustRetesselate        2

#define m00 0
#define m01 1
#define m02 2
#define m10 3
#define m11 4
#define m12 5
#define m20 6
#define m21 7
#define m22 8

namespace visBase
{

class TileTesselator;

//A tile with 64x64 nodes.
class HorizonSectionTile : CallBacker
{
public:
				HorizonSectionTile(const HorizonSection&,
						   const RowCol& origin,
						   SoNode* wfmaterial);
				~HorizonSectionTile();
    void			setNeighbor(char neighbor,HorizonSectionTile*);
    				//!<The neighbor is numbered from 0 to 8
    void			setRightHandSystem(bool yn);

    void			setResolution(int);
    				/*!<Resolution -1 means it is automatic. */
    int				getActualResolution() const;
    void			updateAutoResolution(SoState*);
    				/*<Update only when the resolutionis -1. */

    void			setPos(int row,int col,const Coord3&);
    void			setPositions(const TypeSet<Coord3>&);

    void			setTextureSize(int rowsz,int colsz);
    				//!<Sets size of my part of global texture
    void			setTextureOrigin(int globrow,int globcol);
    				//!<Sets origin in global texture

    				//Calle by the end of each render
    void			resetResolutionChangeFlag();
    void			resetGlueNeedsUpdateFlag();

    				//Makes object ready for render
    void			updateNormals( char res);
    void			tesselateResolution(char,bool onlyifabsness);
    void			applyTesselation(char res);
    				//!<Should be called from rendering thread
    void			updateGlue();

    void			useWireframe(bool);
    void			turnOnWireframe(char res);
    
    const SbBox3f&		getBBox() const; 
    SoNode*			getNodeRoot() const	{ return root_; }

    bool			allNormalsInvalid(int res) const;
    void			setAllNormalsInvalid(int res,bool yn);
    void			emptyInvalidNormalsList(int res);

    bool			isDefined(int row,int col) const;
    				//!<Row/Col is local to this tile
protected:

    int				getNormalIdx(int crdidx,int res) const;
    				//!<Gives normal index in normals_

    friend class		HorizonSectionTilePosSetup;
    friend class		TileTesselator;		
    friend class		HorizonSection;    
    void			setActualResolution(int);
    int				getAutoResolution(SoState*);
    void			tesselateGlue();
    void			tesselateWireframe(char,TypeSet<int>& ci,
						   TypeSet<int>& ni) const;
    void			updateBBox();
    void			setWireframe(int res);
    void			setInvalidNormals(int row,int col);
    void			computeNormal(int normidx, int res,
	    				      SbVec3f& normal) const;
    void			hideFromDisplay();

    struct TesselationData
    {
	TypeSet<int>		pointci_;
	TypeSet<int>		lineci_;
	TypeSet<int>		stripci_;

	TypeSet<int>		pointni_;
	TypeSet<int>		lineni_;
	TypeSet<int>		stripni_;

	TypeSet<int>		wireframeci_;
	TypeSet<int>		wireframeni_;
    };

    bool			usewireframe_;

    HorizonSectionTile*		neighbors_[9];

    SbBox3f			bbox_;	//In display space
    bool			needsupdatebbox_;
    int				nrdefinedpos_;
    const RowCol		origin_;
    const HorizonSection&	section_;

    SoSeparator*		root_;
    SoCoordinate3*		coordnode_;
    SoTextureComposer*		texture_;
    SoSwitch*			resswitch_;
    SoNormal*			normalnode_;
    SbVec3f*			coords_;
    SbVec3f*			normals_;

    int				desiredresolution_;
    bool			resolutionhaschanged_;

    bool			allnormalsinvalid_[mMaxNrResolutions];
    char			needsretesselation_[mMaxNrResolutions];
    				//!<0 - updated, 1 - needs update, 2 - dont disp

    SoSeparator*		resolutions_[mMaxNrResolutions];
    SoIndexedTriangleStripSet*	triangles_[mMaxNrResolutions];
    SoIndexedLineSet3D*		lines_[mMaxNrResolutions];
    SoIndexedLineSet*		wireframes_[mMaxNrResolutions];
    SoDGBIndexedPointSet*	points_[mMaxNrResolutions];

    TypeSet<int>		invalidnormals_[mMaxNrResolutions];
    ObjectSet<TesselationData>	tesselationdata_;
    Threads::Mutex		datalock_;
    SoSwitch*			wireframeswitch_;
    SoSeparator*		wireframeseparator_;

    SoIndexedTriangleStripSet*	gluetriangles_;
    SoIndexedLineSet3D*		gluelines_;
    SoDGBIndexedPointSet*	gluepoints_;
    char			glueneedsretesselation_;
    				//!<0 - updated, 1 - needs update, 2 - dont disp

    int				tesselationqueueid_;
};


class HorizonTileRenderPreparer: public ParallelTask
{
public: 
HorizonTileRenderPreparer( HorizonSection& section, SoState* state, int res )
    : section_( section )
    , state_( state )
    , tiles_( section.tiles_.getData() )
    , nrtiles_( section.tiles_.info().getTotalSz() )
    , nrcoltiles_( section.tiles_.info().getSize(1) )
    , resolution_( res )						
    , permutation_( 0 )    
{}

~HorizonTileRenderPreparer()
{ delete [] permutation_; }

od_int64 nrIterations() const { return nrtiles_; }
od_int64 totalNr() const { return nrtiles_ * 2; }
const char* message() const { return "Updating Horizon Display"; }
const char* nrDoneText() const { return "Parts completed"; }


bool doPrepare( int nrthreads )
{
    if ( !state_ )
	return false;

    barrier_.setNrThreads( nrthreads );
    nrthreadsfinishedwithres_ = 0;

    delete [] permutation_;
    permutation_ = 0;
    mTryAlloc( permutation_, od_int64[nrtiles_] );
	
    for ( int idx=0; idx<nrtiles_; idx++ )
	permutation_[idx] = idx;

    std::random_shuffle( permutation_, permutation_+nrtiles_ );

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( tiles_[realidx] ) 
	    tiles_[realidx]->updateAutoResolution( state_ );
    }

    barrier_.waitForAll();

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	HorizonSectionTile* tile = tiles_[realidx];
	if ( tile )
	{
	    int res = tile->getActualResolution();
	    if ( res==-1 ) res = resolution_;
	    tile->updateNormals( res );
	    const int actualres = tile->getActualResolution();
	    if ( actualres!=-1 )
		tile->tesselateResolution( actualres, true );
	}

	addToNrDone( 1 );
    }	
    
    barrier_.waitForAll();
    if ( !shouldContinue() )
	return false;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( tiles_[realidx] ) 
	    tiles_[realidx]->updateGlue();

	addToNrDone( 1 );
    }

    return true;
}

    od_int64*			permutation_;
    SoState*			state_;
    HorizonSectionTile**	tiles_;
    HorizonSection&		section_;
    int				nrtiles_;
    int				nrcoltiles_;
    int				resolution_;
    int				nrthreads_;
    int				nrthreadsfinishedwithres_;
    Threads::Barrier		barrier_;
};


class TileTesselator : public SequentialTask
{
public:
	TileTesselator( HorizonSectionTile* tile, char res )
	    : tile_( tile ), res_( res ) {}

    int	nextStep()
    {
	tile_->updateNormals( res_ );
	tile_->tesselateResolution( res_, false );
	return SequentialTask::Finished();
    }

    HorizonSectionTile*		tile_;
    unsigned char		res_;
};



class HorizonSectionTilePosSetup: public ParallelTask
{
public:    
    HorizonSectionTilePosSetup( ObjectSet<HorizonSectionTile> tiles, 
	    const Geometry::BinIDSurface& geo,
	    StepInterval<int> rrg, StepInterval<int> crg, ZAxisTransform* zat,
	    int ssz, unsigned char lowresidx )
	: tiles_( tiles )
	, geo_( geo )  
	, rrg_( rrg )
	, crg_( crg )			 	
        , zat_( zat )
	, nrCrdsPerTileSide_( ssz )
	, lowestresidx_( lowresidx )	
    {
	if ( zat_ ) zat_->ref();
    }

    ~HorizonSectionTilePosSetup()
    {
	if ( zat_ ) zat_->unRef();
    }

    od_int64	nrIterations() const { return tiles_.size(); }

    const char*	message() const { return "Creating Horizon Display"; }
    const char*	nrDoneText() const { return "Parts completed"; }

protected:

    bool doWork( od_int64 start, od_int64 stop, int threadid )
    {
	for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
	{
	    const RowCol& origin = tiles_[idx]->origin_;
	    TypeSet<Coord3> positions;
	    positions.setCapacity( nrCrdsPerTileSide_ * nrCrdsPerTileSide_ );
	    for ( int rowidx=0; rowidx<nrCrdsPerTileSide_ ; rowidx++ )
	    {
		const int row = origin.row + rowidx*rrg_.step;
		const bool rowok = rrg_.includes(row, false);
		const StepInterval<int> colrg( 
			mMAX(geo_.colRange(row).start, crg_.start),
		        mMIN(geo_.colRange(row).stop, crg_.stop), crg_.step );

		for ( int colidx=0; colidx<nrCrdsPerTileSide_ ; colidx++ )
		{
		    const int col = origin.col + colidx*colrg.step;
		    Coord3 pos = rowok && colrg.includes(col, false)
			? geo_.getKnot(RowCol(row,col),false) 
			: Coord3::udf();
		    if ( zat_ ) pos.z = zat_->transform( pos );		
	    	    positions += pos;
		}
	    }

	    tiles_[idx]->setPositions( positions );
	    tiles_[idx]->updateNormals( lowestresidx_ );
	    tiles_[idx]->tesselateResolution( lowestresidx_, false );

	    addToNrDone(1);
	}

	return true;
    }

    int					nrCrdsPerTileSide_;
    unsigned char			lowestresidx_;
    ObjectSet<HorizonSectionTile> 	tiles_;
    const Geometry::BinIDSurface&	geo_;
    StepInterval<int>			rrg_, crg_;
    ZAxisTransform*			zat_;
};



HorizonSection::HorizonSection() 
    : VisualObjectImpl( false )
    , callbacker_( new SoCallback ) 
    , shapehints_( new SoShapeHints )				    
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , zaxistransformvoi_( -2 )			  
    , geometry_( 0 )
    , displayrrg_( -1, -1, 0 )
    , displaycrg_( -1, -1, 0 )
    , userchangedisplayrg_( false )			      
    , channels_( TextureChannels::create() )		   
    , channel2rgba_( ColTabTextureChannel2RGBA::create() ) 
    , tiles_( 0, 0 )					  
    , texturecrds_( new SoTextureCoordinate2 )
    , desiredresolution_( -1 )
    , ismoving_( false )
    , usewireframe_( false )
    , cosanglexinl_( cos(SI().computeAngleXInl()) )
    , sinanglexinl_( sin(SI().computeAngleXInl()) )		     
    , wireframematerial_( visBase::Material::create() )
    , tesselationlock_( false )
    , mNrCoordsPerTileSide( 0 )
    , mTotalNrCoordsPerTile( 0 )
    , mTileSideSize( 0 )
    , mTileLastIdx( 0 )
    , mTotalNormalSize( 0 )
    , mLowestResIdx( 0 )
    , mHorSectNrRes( 0 )
    , spacing_( 0 )
    , nrcells_( 0 )
    , normalstartidx_( 0 )
    , normalsidesize_( 0 )
    , osghorizon_( 0 )
{
    setLockable();
    cache_.allowNull( true );
    
    callbacker_->ref();
    callbacker_->setCallback( updateAutoResolution, this );
    addChild( callbacker_ );

    addChild( shapehints_ );

    channel2rgba_->ref();
    channels_->ref();
    addChild( channels_->getInventorNode() );
    channels_->setChannels2RGBA( channel2rgba_ );
    if ( channels_->nrChannels()<1 )
	addChannel();
    else 
	cache_ += 0;

    addChild( texturecrds_ );

    wireframematerial_->ref();

    if ( doOsg() )
    {
	osghorizon_ = new osgGeo::Horizon3DNode;
	addChild( osghorizon_ );
    }
}


HorizonSection::~HorizonSection()
{
    wireframematerial_->unRef();

    channel2rgba_->unRef();
    deepErase( cache_ );

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;

	writeLock();
	removeChild( tileptrs[idx]->getNodeRoot() );
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
    removeZTransform();
   
    removeChild( callbacker_ ); 
    callbacker_->unref();

    removeChild( channels_->getInventorNode() );
    channels_->unRef();
    removeChild( texturecrds_ );

    delete [] spacing_;
    delete [] nrcells_;
    delete [] normalstartidx_;
    delete [] normalsidesize_;
}


void HorizonSection::setTextureCoords()
{
    int idx = 0;
    for ( int irow=0; irow<mNrCoordsPerTileSide; irow++ )
    {
	for ( int icol=0; icol<mNrCoordsPerTileSide; icol++ )
	{
	    texturecrds_->point.set1Value( idx++,
		    (icol+0.5)/mNrCoordsPerTileSide,
		    (irow+0.5)/mNrCoordsPerTileSide );
	}
    }

    texturecrds_->point.deleteValues( idx, -1 );
}


void HorizonSection::setRightHandSystem( bool yn )
{
    VisualObjectImpl::setRightHandSystem( yn );
    
    shapehints_->vertexOrdering = yn ? SoShapeHints::COUNTERCLOCKWISE 
				     : SoShapeHints::CLOCKWISE;
    shapehints_->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setRightHandSystem( yn );
}


void HorizonSection::setChannels2RGBA( TextureChannel2RGBA* t )
{
    channels_->setChannels2RGBA( t );
    if ( channel2rgba_ )
	channel2rgba_->unRef();

    channel2rgba_ = t;

    if ( channel2rgba_ )
	channel2rgba_->ref();
}

TextureChannel2RGBA* HorizonSection::getChannels2RGBA()
{ return channel2rgba_; }


const TextureChannel2RGBA* HorizonSection::getChannels2RGBA() const
{ return channel2rgba_; }


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


void HorizonSection::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();
}


const mVisTrans* HorizonSection::getDisplayTransformation() const
{ return transformation_; }


void HorizonSection::setZAxisTransform( ZAxisTransform* zt, TaskRunner* )
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
    if ( !geometry_ || zaxistransformvoi_==-1 )	
	return;

    if ( !zaxistransform_ || !zaxistransform_->needsVolumeOfInterest() )
	return;

    CubeSampling cs;
    if ( userchangedisplayrg_ )
    	cs.hrg.set( displayrrg_, displaycrg_ );
    else
	cs.hrg.set( geometry_->rowRange(), geometry_->colRange() );

    HorSamplingIterator iter( cs.hrg );

    bool first = true;
    BinID curpos;
    while ( iter.next(curpos) )
    {
	const float depth = geometry_->getKnot(RowCol(curpos),false).z;
	if ( mIsUdf(depth) )
	    continue;

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


void HorizonSection::getDataPositions( DataPointSet& res, double zshift,
				       int sid, TaskRunner* tr ) const 
{
    if ( !geometry_ ) return;

    if ( zaxistransform_ && zaxistransformvoi_>=0 )
    {
	if ( !zaxistransform_->loadDataIfMissing(zaxistransformvoi_) )
		return;
    }

    const DataColDef sidcol( sKeySectionID() );
    if ( res.dataSet().findColDef(sidcol,PosVecDataSet::NameExact)==-1 )
	res.dataSet().add( new DataColDef(sidcol) );

    const int sidcolidx =  res.dataSet().findColDef( 
	    sidcol, PosVecDataSet::NameExact ) - res.nrFixedCols();
 
    BinIDValueSet& bivs = res.bivSet();
    mAllocVarLenArr( float, vals, bivs.nrVals() ); 
    for ( int idx=0; idx<bivs.nrVals(); idx++ )
	vals[idx] = mUdf(float);

    vals[sidcolidx+res.nrFixedCols()] = sid;

    const int nrknots = geometry_->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const BinID bid = geometry_->getKnotRowCol(idx);
	if ( userchangedisplayrg_ &&
	     ( !displayrrg_.includes(bid.inl, false) || 
	       !displaycrg_.includes(bid.crl, false) ||
	       ((bid.inl-displayrrg_.start)%displayrrg_.step) ||
  	       ((bid.crl-displaycrg_.start)%displaycrg_.step) ) )
	    continue;

	const Coord3 pos = geometry_->getKnot(RowCol(bid),false);
	if ( !pos.isDefined() ) 
	    continue;

	float zval = pos.z;
	if ( zshift )
	{
	    if ( zaxistransform_ )
	    {
		zval = zaxistransform_->transform( BinIDValue(bid,zval) );
		if ( mIsUdf(zval) )
		    continue;

		zval += zshift;
		zval = zaxistransform_->transformBack( BinIDValue(bid,zval) );

		if ( mIsUdf(zval) )
		    continue;
	    }
	    else
	    {
		zval += zshift;
	    }
	}

	vals[0] = zval;
	bivs.add( bid, vals );
    }
}


void HorizonSection::setTextureData( int channel, const DataPointSet* dpset,
				     int sid, TaskRunner* tr )
{
    const BinIDValueSet* data = dpset ? &dpset->bivSet() : 0;
    if ( channel<0 || channel>=cache_.size() ) 
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
    }

    updateTexture( channel, dpset, sid );
}


#define mDefineRCRange \
    const StepInterval<int> rrg = \
	userchangedisplayrg_ ? displayrrg_ : geometry_->rowRange(); \
    const StepInterval<int> crg = \
	userchangedisplayrg_ ? displaycrg_ : geometry_->colRange(); 


void HorizonSection::updateTexture( int channel, const DataPointSet* dpset, 
				    int sid )
{
    const BinIDValueSet* data = getCache( channel );
    if ( !geometry_ || !geometry_->getArray() || !dpset || !data )
	return;

    const int nrfixedcols = dpset->nrFixedCols();
    const DataColDef sidcoldef( sKeySectionID() );
    const int sidcol = 
	dpset->dataSet().findColDef(sidcoldef,PosVecDataSet::NameExact);
    const int shift = sidcol==-1 ?  nrfixedcols : nrfixedcols+1;

    const int nrversions = data->nrVals()-shift;
    setNrVersions( channel, nrversions );

    mDefineRCRange;
    const int nrrows = rrg.nrSteps()+1;
    const int nrcols = crg.nrSteps()+1;

    channels_->setSize( 1, nrrows, nrcols );
   
    ObjectSet<float> versiondata;
    versiondata.allowNull( true );
    const int nrcells = nrrows*nrcols;

    MemSetter<float> memsetter;
    memsetter.setSize( nrcells );
    memsetter.setValue( mUdf(float) );

    for ( int idx=0; idx<nrversions; idx++ )
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

    BinIDValueSet::Pos pos;
    const int startsourceidx = nrfixedcols + (nrfixedcols==sidcol ? 1 : 0);
    while ( data->next(pos,true) )
    {
	const float* ptr = data->getVals(pos);
	if ( sidcol!=-1 && sid!=mNINT32(ptr[sidcol]) )
	    continue;

	const BinID bid = data->getBinID( pos );
	if ( userchangedisplayrg_ && (!rrg.includes(bid.inl, false) ||
		    		      !crg.includes(bid.crl, false)) )
	    continue;

	const int inlidx = rrg.nearestIndex(bid.inl);
	const int crlidx = crg.nearestIndex(bid.crl);

	const int offset = inlidx*nrcols + crlidx;
	if ( offset>=nrcells )
	    continue;

	for ( int idx=0; idx<nrversions; idx++ )
	    versiondata[idx][offset] = ptr[idx+startsourceidx];
    }

    for ( int idx=0; idx<nrversions; idx++ )
	channels_->setUnMappedData( channel, idx, versiondata[idx],
				    OD::TakeOverPtr, 0 );

    updateTileTextureOrigin( RowCol( rrg.start - origin_.row,
				     crg.start - origin_.col ) );
}


void HorizonSection::setWireframeColor( Color col )
{ wireframematerial_->setColor( col ); }


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
	const bool needsclip =
	    channels_->getColTabMapperSetup( channel, 0 ).needsReClip( mapper );
	channels_->setColTabMapperSetup( channel, mapper );
	channels_->reMapData( channel, !needsclip, tr );
    }
}


const ColTab::MapperSetup* HorizonSection::getColTabMapperSetup( int ch ) const
{
    return ch<0 ? 0 : &channels_->getColTabMapperSetup( ch,activeVersion(ch) );
}


const TypeSet<float>* HorizonSection::getHistogram( int ch ) const
{ return channels_->getHistogram( ch ); }


void HorizonSection::setTransparency( int ch, unsigned char yn )
{ 
    mDynamicCastGet( ColTabTextureChannel2RGBA*, ct, channel2rgba_ );
    if ( ct && ch>=0 ) ct->setTransparency( ch, yn );
}


unsigned char HorizonSection::getTransparency( int ch ) const
{ 
    mDynamicCastGet( ColTabTextureChannel2RGBA*, ct, channel2rgba_ );
    if ( !ct )
	return 0;
    
    return ct->getTransparency( ch ); 
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
    return cache_.validIdx(channel) ? cache_[channel] : 0; 
}


void HorizonSection::setSizeParameters()
{
    mDefineRCRange; 
    const int maxsz = mMAX( rrg.nrSteps()+1, crg.nrSteps()+1 ); 

    mNrCoordsPerTileSide = 64;
    mHorSectNrRes = 6;
    while ( maxsz > (mNrCoordsPerTileSide * mMaxNrTiles) &&
	    mHorSectNrRes < mMaxNrResolutions )
    {
	mNrCoordsPerTileSide *= 2;
	mHorSectNrRes++;
    }

    mTotalNrCoordsPerTile = mNrCoordsPerTileSide * mNrCoordsPerTileSide;
    mTileSideSize = mNrCoordsPerTileSide - 1;
    mTileLastIdx = mNrCoordsPerTileSide - 1;
    mLowestResIdx = mHorSectNrRes-1;

    delete [] spacing_;
    spacing_ = new int[mHorSectNrRes];

    delete [] nrcells_;
    nrcells_ = new int[mHorSectNrRes];

    delete [] normalstartidx_;
    normalstartidx_ = new int[mHorSectNrRes];

    delete [] normalsidesize_;
    normalsidesize_ = new int[mHorSectNrRes];

    mTotalNormalSize = 0;
    for ( int idx=0; idx<mHorSectNrRes; idx++ )
    {
	spacing_[idx] = !idx ? 1 : 2 * spacing_[idx-1];
       	
	normalsidesize_[idx] = mTileSideSize / spacing_[idx] + ( idx ? 2 : 1 ); 
	nrcells_[idx] = ( normalsidesize_[idx] - (idx ? 1 : 0) ) * 
	    		( normalsidesize_[idx] - (idx ? 1 : 0) );
	normalstartidx_[idx] = mTotalNormalSize;

	mTotalNormalSize += normalsidesize_[idx] * normalsidesize_[idx];
    }

    setTextureCoords();
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

    setSizeParameters();
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
    	    removeChild( tileptrs[idx]->getNodeRoot() );
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

    setSizeParameters();
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

    updatelock_.lock();
    surfaceChange( gpids, 0 );
    updatelock_.unLock();
}


void HorizonSection::surfaceChange( const TypeSet<GeomPosID>* gpids,
       				    TaskRunner* tr )
{
    if ( !geometry_ || !geometry_->getArray() )
	return;

    if ( zaxistransform_ && zaxistransformvoi_!=-1 )
    {
	updateZAxisVOI();
	if ( !zaxistransform_->loadDataIfMissing(zaxistransformvoi_,tr) )
	    return;
    }

    if ( osghorizon_ )
    {
	const Interval<int> rowrg = geometry_->rowRange();
	const Interval<int> colrg = geometry_->colRange();
	std::vector<osg::Vec2d> cornerpts;
	Coord crd00 = geometry_->getKnotCoord( RowCol( rowrg.start, colrg.start ) );
	Coord crd01 = geometry_->getKnotCoord( RowCol( rowrg.start, colrg.stop ) );
	Coord crd10 = geometry_->getKnotCoord( RowCol( rowrg.start, colrg.stop ) );
	std::vector<osg::Vec2d> cornerptr;
	cornerpts.push_back( osg::Vec2d( crd00.x, crd00.y ) );
	cornerpts.push_back( osg::Vec2d( crd01.x, crd01.y ) );
	cornerpts.push_back( osg::Vec2d( crd10.x, crd10.y ) );

	osg::ref_ptr<osg::FloatArray> deptharr =
	    static_cast<osg::FloatArray*>( osghorizon_->getDepthArray() );

	if ( !deptharr )
	{
	    deptharr = new osg::FloatArray;
	    osghorizon_->setDepthArray( deptharr );
	    gpids = 0; //Force full update
	}

	const int newsize = geometry_->getArray()->info().getTotalSz();

	if ( deptharr->size()!=newsize )
	    deptharr->resize( newsize, mUdf(float) );

	float* depthptr = (float*) deptharr->getDataPointer();

	if ( !gpids )
	{
	    if ( !zaxistransform_ )
		geometry_->getArray()->getAll( depthptr );
	    else
	    {
		PtrMan<Geometry::Iterator> iter = geometry_->createIterator();
		GeomPosID posid;
		while ( (posid=iter->next())!=-1 )
		{
		    float z = geometry_->getPosition( posid ).z;
		    const BinID bid( posid );
		    if ( !mIsUdf(z) )
		    {
			if ( zaxistransform_ )
			    z = zaxistransform_->transform( BinIDValue( bid, z ) );
		    }

		    depthptr[geometry_->getKnotIndex( bid )] = z;
		}
	    }
	}
	else
    	{
	    const GeomPosID* gpidptr = gpids->arr();
	    const GeomPosID* stopptr = gpidptr+gpids->size();

	    while ( gpidptr!=stopptr )
	    {
		float z = geometry_->getPosition( *gpidptr ).z;
		const BinID bid( *gpidptr );
		if ( !mIsUdf(z) )
		    z = zaxistransform_->transform( BinIDValue( bid, z ) );

		depthptr[geometry_->getKnotIndex( bid )] = z;
		gpidptr++;
	    }
	}
    }

    if ( !gpids || !tiles_.info().getSize(0) || !tiles_.info().getSize(1) )
	resetAllTiles( tr );
    else
	updateNewPoints( gpids, tr );
}


void HorizonSection::updateNewPoints( const TypeSet<GeomPosID>* gpids,
				      TaskRunner* tr )
{
    mDefineRCRange;
    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return;
    
    tesselationlock_ = true;
    updateTileArray();
    
    const int nrrowsz = tiles_.info().getSize(0);
    const int nrcolsz = tiles_.info().getSize(1);
    
    ObjectSet<HorizonSectionTile> fullupdatetiles;
    ObjectSet<HorizonSectionTile> oldupdatetiles;

    for ( int idx=(*gpids).size()-1; idx>=0; idx-- )
    {
	const RowCol& absrc( (*gpids)[idx] );
	RowCol rc = absrc - origin_; 
	rc.row /= rrg.step; rc.col /= crg.step;

	int tilerowidx = rc.row/mTileSideSize;
	int tilerow = rc.row%mTileSideSize;
	if ( tilerowidx==nrrowsz && !tilerow )
	{
	    tilerowidx--;
	    tilerow = mTileLastIdx;
	}

	int tilecolidx = rc.col/mTileSideSize;
	int tilecol = rc.col%mTileSideSize;
	if ( tilecolidx==nrcolsz && !tilecol )
	{
	    tilecolidx--;
	    tilecol = mTileLastIdx;
	}

	/*If we already set work area and the position is out of the area,
	  we will skip the position. */
	if ( tilerowidx>=nrrowsz || tilecolidx>=nrcolsz )
	    continue;

	const Coord3 pos = geometry_->getKnot(absrc,false);
	
	bool addoldtile = false;
	HorizonSectionTile* tile = tiles_.get( tilerowidx, tilecolidx );
	if ( !tile ) 
	{
	    tile = createTile( tilerowidx, tilecolidx );
	    fullupdatetiles += tile;
	}
	else if ( fullupdatetiles.indexOf(tile)==-1 )
	{
	    for ( int res=0; res<=mLowestResIdx; res++ )
		tile->setAllNormalsInvalid( res, false );
	
	    tile->setPos( tilerow, tilecol, pos );
	    if ( desiredresolution_!=-1 )
	    {
		addoldtile = true;
		if ( oldupdatetiles.indexOf(tile)==-1 )
		    oldupdatetiles += tile;		    
	    }
	}

	for ( int rowidx=-1; rowidx<=1; rowidx++ ) //Update neighbors
	{
	    const int nbrow = tilerowidx+rowidx;
	    if ( nbrow<0 || nbrow>=nrrowsz ) continue;

	    for ( int colidx=-1; colidx<=1; colidx++ )
	    {
		const int nbcol = tilecolidx+colidx;
		if ( (!rowidx && !colidx) || nbcol<0 || nbcol>=nrcolsz )
		    continue;

		HorizonSectionTile* nbtile = tiles_.get( nbrow, nbcol );
		if ( !nbtile || fullupdatetiles.indexOf(nbtile)!=-1)
		    continue;

		nbtile->setPos( tilerow-rowidx*mTileSideSize,
				tilecol-colidx*mTileSideSize, pos );
		if ( !addoldtile || rowidx+colidx>=0 || desiredresolution_==-1 
				 || oldupdatetiles.indexOf(nbtile)!=-1 )
		    continue;
	    
		if ( (!tilecol && !rowidx && colidx==-1) || 
			(!tilerow && rowidx==-1 && 
			 ((!tilecol && colidx==-1) || !colidx)) )
		    oldupdatetiles += nbtile;
	    }
	}
    }

    HorizonSectionTilePosSetup task( fullupdatetiles, *geometry_, rrg, crg, 
	    zaxistransform_, mNrCoordsPerTileSide, mLowestResIdx );
    if ( tr ) tr->execute( task );
    else task.execute();

    //Only for fixed resolutions, which won't be tesselated at render.
    if ( oldupdatetiles.size() )
    {
	TypeSet<Threads::Work> work;
	for ( int idx=0; idx<oldupdatetiles.size(); idx++ )
	{
	    TileTesselator* tt =
		new TileTesselator( oldupdatetiles[idx], desiredresolution_ );
	    work += Threads::Work( *tt, true );
	}

	Threads::WorkManager::twm().addWork( work,
	       Threads::WorkManager::cDefaultQueueID() );
    }
  
    tesselationlock_ = false;
    shapehints_->touch(); //trigger rerender
}


void HorizonSection::resetAllTiles( TaskRunner* tr )
{
    mDefineRCRange;
    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return;
    
    tesselationlock_ = true;
    origin_ = RowCol( rrg.start, crg.start );
    const int nrrows = nrBlocks( rrg.nrSteps()+1, mNrCoordsPerTileSide, 1 );
    const int nrcols = nrBlocks( crg.nrSteps()+1, mNrCoordsPerTileSide, 1 );

    writeLock();
    if ( !tiles_.setSize( nrrows, nrcols ) )
    {
	tesselationlock_ = false;
	writeUnLock();
	return;
    }

    tiles_.setAll( 0 );
    writeUnLock();

    ObjectSet<HorizonSectionTile> fullupdatetiles;
    for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
    {
	for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	{
	    fullupdatetiles += createTile(tilerowidx, tilecolidx);
	}
    }

    updateTileTextureOrigin( RowCol(0,0) );
    
    HorizonSectionTilePosSetup task( fullupdatetiles, *geometry_, rrg, crg, 
	    zaxistransform_, mNrCoordsPerTileSide, mLowestResIdx );
    if ( tr ) tr->execute( task );
    else task.execute();
    
    tesselationlock_ = false;
    shapehints_->touch(); 
}


void HorizonSection::updateTileArray()
{
    mDefineRCRange;
    const int rowsteps =  mTileSideSize * rrg.step;
    const int colsteps = mTileSideSize * crg.step;
    const int oldrowsize = tiles_.info().getSize(0);
    const int oldcolsize = tiles_.info().getSize(1);
    int newrowsize = oldrowsize;
    int newcolsize = oldcolsize;
    int nrnewrowsbefore = 0;
    int nrnewcolsbefore = 0;
	
    int diff = origin_.row - rrg.start;
    if ( diff>0 ) 
    {
	nrnewrowsbefore = diff/rowsteps + (diff%rowsteps ? 1 : 0);
    	newrowsize += nrnewrowsbefore;
    }

    diff = rrg.stop - (origin_.row+oldrowsize*rowsteps);
    if ( diff>0 ) newrowsize += diff/rowsteps + (diff%rowsteps ? 1 : 0);
    
    diff = origin_.col - crg.start;
    if ( diff>0 ) 
    {
	nrnewcolsbefore = diff/colsteps + (diff%colsteps ? 1 : 0);
    	newcolsize += nrnewcolsbefore;
    }

    diff = crg.stop - (origin_.col+oldcolsize*colsteps);
    if ( diff>0 ) newcolsize += diff/colsteps + (diff%colsteps ? 1 : 0);

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

    writeLock();
    tiles_.copyFrom( newtiles );
    origin_.row -= nrnewrowsbefore*rowsteps;
    origin_.col -= nrnewcolsbefore*colsteps;
    writeUnLock();
}


void HorizonSection::updateTileTextureOrigin( const RowCol& textureorigin )
{
    const int nrrows = tiles_.info().getSize(0);
    const int nrcols = tiles_.info().getSize(1);
    mDefineRCRange;
    const RowCol step( rrg.step, crg.step );

    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	for ( int colidx=0; colidx<nrcols; colidx++ )
	{
	    HorizonSectionTile* tile = tiles_.get(rowidx,colidx);
	    if ( !tile )
		continue;

	    const RowCol tilestart( rowidx*mTileSideSize, colidx*mTileSideSize);
	    const RowCol texturestart = tilestart - textureorigin/step;

	    tile->setTextureOrigin( texturestart.row, texturestart.col );
	}
    }
}


HorizonSectionTile* HorizonSection::createTile( int tilerowidx, int tilecolidx )
{
    mDefineRCRange;
    const RowCol step( rrg.step, crg.step );
    const RowCol tileorigin( origin_.row+tilerowidx*mTileSideSize*step.row,
			     origin_.col+tilecolidx*mTileSideSize*step.col );
    HorizonSectionTile* tile = new HorizonSectionTile( *this, tileorigin,
	    			wireframematerial_->getInventorNode() );

    tile->setResolution( desiredresolution_ );
    tile->useWireframe( usewireframe_ );
    tile->setRightHandSystem( righthandsystem_ );

    writeLock();
    tiles_.set( tilerowidx, tilecolidx, tile );
    writeUnLock();
    
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
		pos = rowidx==-1 ? m00 : (!rowidx ? m10 : m20);
	    else if ( colidx==0 ) 
		pos = rowidx==-1 ? m01 : (!rowidx ? m11 : m21);
	    else 
		pos = rowidx==-1 ? m02 : (!rowidx ? m12 : m22);

	    tile->setNeighbor( pos, neighbor );

	    if ( colidx==1 ) 
		pos = rowidx==1 ? m00 : (!rowidx ? m10 : m20);
	    else if ( colidx==0 ) 
		pos = rowidx==1 ? m01 : (!rowidx ? m11 : m21);
	    else 
		pos = rowidx==1 ? m02 : (!rowidx ? m12 : m22);

	    neighbor->setNeighbor( pos, tile );
	}
    }

    writeLock();
    addChild( tile->getNodeRoot() );
    writeUnLock();

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
    /*
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
    */
}

#define mApplyTesselation( extra ) \
{ \
    for ( int idx=0; idx<tilesz; idx++ ) \
    { \
	if ( tileptrs[idx] ) \
	{ \
	    tileptrs[idx]->applyTesselation( \
	    tileptrs[idx]->getActualResolution() ); \
	    extra; \
	} \
    } \
}


void HorizonSection::updateAutoResolution( SoState* state, TaskRunner* tr )
{
    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz || !state ) return;

    HorizonSectionTile** tileptrs = tiles_.getData();
    if ( desiredresolution_!=-1 )
    {
	mApplyTesselation( tileptrs[idx]->updateGlue() );
    }
    else
    {
	const int32_t camerainfo = SoCameraInfoElement::get(state);
	bool ismoving = 
	    camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE);

	ismoving_ = ismoving;

	if ( ismoving )
	    return;

	HorizonTileRenderPreparer task( *this, state, desiredresolution_ );
	if ( tr )
	    tr->execute(task);
	else
	    task.execute();
    
	mApplyTesselation();
    }

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetGlueNeedsUpdateFlag();

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetResolutionChangeFlag();
}


char HorizonSection::currentResolution() const
{ return desiredresolution_; }


char HorizonSection::nrResolutions() const
{ return mHorSectNrRes; }


void HorizonSection::setResolution( int res, TaskRunner* tr )
{
    desiredresolution_ = res;
    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz ) return;
    
    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setResolution( res );

    if ( res==-1 )
	return;

    TypeSet<Threads::Work> work;
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;
	
	tileptrs[idx]->setActualResolution( res );
	work += Threads::Work(
		*new TileTesselator( tileptrs[idx], res ), true );
    }
    
    Threads::WorkManager::twm().addWork( work,
	       Threads::WorkManager::cDefaultQueueID() );
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


HorizonSectionTile::HorizonSectionTile( const HorizonSection& section,
					const RowCol& origin, SoNode* wfmat )
    : root_( new SoSeparator )
    , normalnode_( new SoNormal )  
    , coordnode_( new SoCoordinate3 )
    , texture_( new SoTextureComposer )
    , resswitch_( new SoSwitch )	
    , gluetriangles_( new SoIndexedTriangleStripSet )
    , gluelines_( new SoIndexedLineSet3D )
    , glueneedsretesselation_( false )
    , gluepoints_( new SoDGBIndexedPointSet )
    , desiredresolution_( -1 )
    , resolutionhaschanged_( false )
    , needsupdatebbox_( false )
    , usewireframe_( false )
    , nrdefinedpos_( 0 )
    , origin_( origin )
    , section_( section )
    , tesselationqueueid_( Threads::WorkManager::twm().addQueue(
		Threads::WorkManager::MultiThread ) )
{
    coords_ = new SbVec3f[section_.mTotalNrCoordsPerTile];
    normals_ = new SbVec3f[section_.mTotalNormalSize];
   
    tesselationdata_.allowNull();
    root_->ref();

    MemSetter<SbVec3f> coordsetter( normals_, SbVec3f(0,0,-1),
	    			    section_.mTotalNormalSize );
    coordsetter.execute();

    normalnode_->vector.setValuesPointer( section_.mTotalNormalSize, normals_ );
    root_->addChild( normalnode_ );

    coordsetter.setSize( section_.mTotalNrCoordsPerTile );
    coordsetter.setValue( SbVec3f(1e30,1e30,1e30 ) );
    coordsetter.setTarget( coords_ );
    coordsetter.execute();
    coordnode_->point.setValuesPointer(section_.mTotalNrCoordsPerTile,coords_);

    root_->addChild( coordnode_ );
    root_->addChild( texture_ );

    root_->addChild( gluetriangles_ );
    root_->addChild( gluepoints_ );
    
    gluetriangles_->coordIndex.deleteValues( 0, -1 );
    gluelines_->coordIndex.deleteValues( 0, -1 );
    gluepoints_->coordIndex.deleteValues( 0, -1 );

    //gluelines_->rightHandSystem = true ;
    gluelines_->radius = mLineRadius;
    gluelines_->maxRadius = mMaxLineRadius;
    gluelines_->screenSize = true;

    wireframeseparator_ = new SoSeparator;
    root_->addChild( wireframeseparator_ );
    wireframeseparator_->addChild( wfmat );
    wireframeswitch_ = new SoSwitch;
    wireframeseparator_->addChild( wireframeswitch_ );

    root_->addChild( resswitch_ );
    for ( int idx=0; idx<section_.mHorSectNrRes; idx++ )
    {
	allnormalsinvalid_[idx] = true;
	needsretesselation_[idx] = mNoTesselationNeeded;
	resolutions_[idx] = new SoSeparator;
	resolutions_[idx]->renderCaching = SoSeparator::ON;
	resswitch_->addChild( resolutions_[idx] );

	triangles_[idx] = new SoIndexedTriangleStripSet;
	triangles_[idx]->coordIndex.deleteValues( 0, -1 );
	resolutions_[idx]->addChild( triangles_[idx] );

	points_[idx] = new SoDGBIndexedPointSet;
	points_[idx]->coordIndex.deleteValues( 0, -1 );
	resolutions_[idx]->addChild( points_[idx] );

	lines_[idx] = new SoIndexedLineSet3D;
	//lines_[idx]->rightHandSystem = true; 
	lines_[idx]->radius = mLineRadius;
	lines_[idx]->maxRadius = mMaxLineRadius;
	lines_[idx]->screenSize = true;
	lines_[idx]->coordIndex.deleteValues( 0, -1 );
	resolutions_[idx]->addChild( lines_[idx] );

	wireframes_[idx] = new SoIndexedLineSet;
	wireframes_[idx]->coordIndex.deleteValues( 0, -1 );
	wireframeswitch_->addChild( wireframes_[idx] );

	tesselationdata_ += 0;
    }

    for ( int idx=0; idx<9; idx++ )
	neighbors_[idx] = 0;

    setTextureSize(section_.mNrCoordsPerTileSide,section_.mNrCoordsPerTileSide);
    bbox_.makeEmpty();
}


HorizonSectionTile::~HorizonSectionTile()
{
    root_->removeChild( coordnode_ );
    root_->unref();
    
    Threads::WorkManager::twm().removeQueue( tesselationqueueid_, false );

    deepErase( tesselationdata_ );

    delete [] coords_;
    delete [] normals_;
}


void HorizonSectionTile::setRightHandSystem( bool yn )
{
    //gluelines_->rightHandSystem = yn;
    //for ( int idx=0; idx<section_.mHorSectNrRes; idx++ )
    	//lines_[idx]->rightHandSystem = yn;
}


void HorizonSectionTile::updateNormals( char res )
{
    if ( res<0 ) return;

    const bool oldstatus = normalnode_->vector.enableNotify( false );
    bool change = false;

    if ( allnormalsinvalid_[res] )
    {
	const int normalstop = res < section_.mLowestResIdx ? 
	    section_.normalstartidx_[res+1]-1 : section_.mTotalNormalSize-1;
	for ( int idx=section_.normalstartidx_[res]; idx<=normalstop; idx++ )
	    computeNormal( idx, res, normals_[idx] );
	change = true;
    }
    else
    {
	datalock_.lock();
	const int sz = invalidnormals_[res].size();
	mAllocVarLenArr( int, invnormals, sz );
	memcpy( invnormals, invalidnormals_[res].arr(), sz*sizeof(int) );
	datalock_.unLock();
	 
	for ( int idx=0; idx<sz; idx++ )
	{
	    change = true;
	    computeNormal( invnormals[idx], res, normals_[invnormals[idx]] );
	}
    }

    emptyInvalidNormalsList( res );
    allnormalsinvalid_[res] = false; 
    normalnode_->vector.enableNotify( oldstatus );
    if ( change )
	normalnode_->vector.touch();
}


#define mGetGradient( rc, arrpos ) \
    beforefound = false; afterfound = false; \
    for ( int idx=section_.spacing_[res]; idx>=0; idx-- ) \
    { \
        if ( !beforefound ) \
        { \
            const int cur##rc = rc-idx*step.rc; \
            if ( cur##rc>=rc##range.start ) \
            { \
                const Coord3 pos =  \
                    section_.geometry_->getKnot(arrpos,false); \
                if ( pos.isDefined() ) \
                { \
                    beforepos.x = -idx*section_.rc##distance_; \
                    beforepos.y = pos.z; \
                    beforefound = true; \
                } \
            } \
        } \
 \
        if ( idx && !afterfound ) \
        { \
            const int cur##rc = rc+idx*step.rc; \
            if ( cur##rc<=rc##range.stop ) \
            { \
                const Coord3 pos =  \
                    section_.geometry_->getKnot(arrpos,false); \
                if ( pos.isDefined() ) \
                { \
                    afterpos.x = idx*section_.rc##distance_; \
                    afterpos.y = pos.z; \
                    afterfound = true; \
                } \
            } \
        } \
 \
        if ( afterfound && beforefound ) \
            break; \
    } \
 \
    const double d##rc = afterfound && beforefound \
        ? (afterpos.y-beforepos.y)/(afterpos.x-beforepos.x) \
        : 0;



void HorizonSectionTile::computeNormal( int nmidx, int res,
					SbVec3f& normal ) const
{
    if ( !section_.geometry_ )
	return;

    RowCol step;
    if ( section_.userchangedisplayrg_ )
	step = RowCol( section_.displayrrg_.step, section_.displaycrg_.step );
    else
	step = RowCol( section_.geometry_->rowRange().step,
		       section_.geometry_->colRange().step );

    const int normalrow = 
	(nmidx-section_.normalstartidx_[res])/section_.normalsidesize_[res];
    const int normalcol = 
	(nmidx-section_.normalstartidx_[res])%section_.normalsidesize_[res];

    const int row = origin_.row + step.row * normalrow*section_.spacing_[res];
    const int col = origin_.col + step.col * normalcol*section_.spacing_[res];

    const StepInterval<int> rowrange = section_.geometry_->rowRange();
    const StepInterval<int> colrange = section_.geometry_->colRange();
    bool beforefound, afterfound;
    Coord beforepos, afterpos;

    mGetGradient( row, RowCol(currow,col) );
    mGetGradient( col, RowCol(row,curcol) );

    normal[0] = drow*section_.cosanglexinl_+dcol*section_.sinanglexinl_;
    normal[1] = dcol*section_.cosanglexinl_-drow*section_.sinanglexinl_;
    //res[2] = -1; Allready set
}


int HorizonSectionTile::getNormalIdx( int crdidx, int res ) const
{
    if ( crdidx<0 || res<0 )
	return -1;

    //Index in the tile
    const int row = crdidx / section_.mNrCoordsPerTileSide;
    const int col = crdidx % section_.mNrCoordsPerTileSide;

    int useres = res;
    if ( row==section_.mTileLastIdx || col==section_.mTileLastIdx )
    {
	if ( row==section_.mTileLastIdx && col==section_.mTileLastIdx 
					&& neighbors_[8] )
	    useres = neighbors_[8]->getActualResolution();
	else if ( row==section_.mTileLastIdx && neighbors_[7] )
	    useres = neighbors_[7]->getActualResolution();
	else if ( col==section_.mTileLastIdx && neighbors_[5] )
	    useres = neighbors_[5]->getActualResolution();

	if ( useres==-1 )
	    useres = res;
    }

    const short spacing = section_.spacing_[useres];
   
    if ( (!(row%spacing) || (row==section_.mTileLastIdx)) &&
         (!(col%spacing) || (col==section_.mTileLastIdx)) )
    {
	if ( row==section_.mTileLastIdx && col==section_.mTileLastIdx 
					&& !neighbors_[8] )
	{
	    return useres<5 ? section_.normalstartidx_[useres+1]-1 
			    : section_.normalstartidx_[5]+8;
	}
	else
	{
    	    const int resrow = row/spacing + 
		( useres>0 && row==section_.mTileLastIdx ? 1 : 0 );
    	    const int rescol = col/spacing +
    		( useres>0 && col==section_.mTileLastIdx ? 1 : 0 );

    	   return section_.normalstartidx_[useres] + 
	       resrow * section_.normalsidesize_[useres] + rescol;
	}
    }

    return -1;
}


void HorizonSectionTile::resetResolutionChangeFlag()
{ resolutionhaschanged_= false; }


void HorizonSectionTile::resetGlueNeedsUpdateFlag()
{ glueneedsretesselation_ = false; }


void HorizonSectionTile::emptyInvalidNormalsList( int res )
{
    datalock_.lock();
    invalidnormals_[res].erase();
    datalock_.unLock();
}


void HorizonSectionTile::setAllNormalsInvalid( int res, bool yn )
{ 
    allnormalsinvalid_[res] = yn; 
    
    if ( yn ) emptyInvalidNormalsList( res );
}


bool HorizonSectionTile::allNormalsInvalid( int res ) const
{ return allnormalsinvalid_[res]; }


void HorizonSectionTile::setResolution( int res )
{ desiredresolution_ = res; }


int HorizonSectionTile::getActualResolution() const
{
    return resswitch_->whichChild.getValue();
}


void HorizonSectionTile::updateAutoResolution( SoState* state )
{
    int newres = desiredresolution_;
    if ( newres==-1 && state )
    {
	updateBBox();
	const SbBox3f bbox = getBBox();
	if ( bbox.isEmpty() )
	    newres = -1;
	else if ( !section_.ismoving_ &&
		SoCullElement::cullTest( state, bbox, true ) )
	    newres = -1;
	else if ( desiredresolution_==-1 )
	{
	    const int wantedres = getAutoResolution( state );
	    newres = wantedres;
	    datalock_.lock();
	    for ( ; newres<section_.mHorSectNrRes-1; newres++ )
	    {
		if ( needsretesselation_[newres]<mMustRetesselate )
		    break;
	    }
	    datalock_.unLock();

	    if ( !section_.tesselationlock_ &&
		(wantedres!=newres || needsretesselation_[newres] ) )
	    {
		TileTesselator* tt = new TileTesselator( this, wantedres );
		Threads::WorkManager::twm().addWork(
		    Threads::Work( *tt, true ),
		    0, tesselationqueueid_, true );
	    }
	}
    }

    setActualResolution( newres );
}


const SbBox3f& HorizonSectionTile::getBBox() const
{ return bbox_; }
#define mIsDef( pos ) (pos[2]<9.9e29)

void HorizonSectionTile::updateBBox()
{
    if ( !needsupdatebbox_ ) return;

    bbox_.makeEmpty();

    for ( int idx=0; idx<section_.mTotalNrCoordsPerTile; idx++ )
    {
	const SbVec3f& pos = coords_[idx];
	if ( mIsDef(pos) )
	    bbox_.extendBy( pos );
    }

    needsupdatebbox_ = false;
}


int HorizonSectionTile::getAutoResolution( SoState* state ) 
{
    updateBBox();

    if ( bbox_.isEmpty() )
	return -1;

    if ( section_.ismoving_ )
	return section_.mLowestResIdx;

    SbVec2s screensize;
    SoShape::getScreenSize( state, bbox_, screensize );
    const float complexity = SbClamp(SoComplexityElement::get(state),0.0f,1.0f);
    const int wantednumcells = (int)(complexity*screensize[0]*screensize[1]/16);
    if ( !wantednumcells )
	return section_.mLowestResIdx;

    if ( nrdefinedpos_<=wantednumcells )
	return 0;

    int maxres = nrdefinedpos_/wantednumcells;
    if ( nrdefinedpos_%wantednumcells ) maxres++;

    for ( int desiredres=section_.mLowestResIdx; desiredres>=0; desiredres-- )
    {
	if ( section_.nrcells_[desiredres]>=wantednumcells )
	    return mMIN(desiredres,maxres);
    }

    return 0;
}


void HorizonSectionTile::setActualResolution( int resolution )
{
    if ( resolution!=getActualResolution() )
    {
	resswitch_->whichChild.setValue( resolution );
	resolutionhaschanged_ = true;
    }

    const int newwfres = usewireframe_ ? resolution : -1;
    if ( newwfres!=wireframeswitch_->whichChild.getValue() )
	wireframeswitch_->whichChild.setValue( newwfres );
}


void HorizonSectionTile::hideFromDisplay()
{
    setActualResolution( -1 );
    gluetriangles_->coordIndex.deleteValues( 0, -1 );
    gluelines_->coordIndex.deleteValues( 0, -1 );
    gluepoints_->coordIndex.deleteValues( 0, -1 );
}


#define mStrip 3
#define mLine 2
#define mPoint 1

#define mAddInitialTriangle( ci0, ci1, ci2 ) \
{ \
    isstripterminated =  false; \
    mAddIndex( ci0, strip ); \
    mAddIndex( ci1, strip ); \
    mAddIndex( ci2, strip ); \
}


#define mTerminateStrip \
if ( !isstripterminated ) \
{ \
    isstripterminated = true; \
    mAddIndex( -1, strip ); \
}


#define mAddIndex( coordindex, obj ) \
{ \
    obj##ci += coordindex; \
    obj##ni += getNormalIdx( coordindex, res );  \
}



#define mNrBlocks(spacing) \
    section_.mTileSideSize/spacing+(section_.mTileSideSize % spacing ? 0 : -1)


void HorizonSectionTile::tesselateResolution( char res, bool onlyifabsness )
{
    if ( res < 0 || needsretesselation_[res]==mNoTesselationNeeded ||
	(needsretesselation_[res]==mShouldRetesselate && onlyifabsness) )
	return;

    TesselationData* td = new TesselationData;
    const short spacing = section_.spacing_[res];
    const short nrcoordspertile = section_.mNrCoordsPerTileSide;

    TypeSet<int>& pointci = td->pointci_, &lineci = td->lineci_,
		    &stripci = td->stripci_, &wireframeci = td->wireframeci_;
    TypeSet<int>& pointni = td->pointni_, &lineni = td->lineni_,
		    &stripni = td->stripni_, &wireframeni = td->wireframeni_;

    const short nrmyblocks = mNrBlocks(spacing);
    
    for ( int ridx=0; ridx<=nrmyblocks; ridx++ )
    {
	const bool islastrow = ridx==nrmyblocks;
	const int currow = spacing*ridx;
	int ci11 = currow*nrcoordspertile;
	int ci21 = ci11 + spacing*nrcoordspertile;
	if ( islastrow && res ) ci21 -= nrcoordspertile;

	bool nbdef00 = false, nbdef01 = false, nbdef02= false;
	bool nbdef10 = false, nbdef11 = false, nbdef12= false;
	bool nbdef20 = false, nbdef21 = false, nbdef22= false;

	nbdef11 = mIsDef(coords_[ci11]);
	nbdef21 = mIsDef(coords_[ci21]);
	if ( ridx )
	{
	    int ci01 = ci11 - spacing * nrcoordspertile;
	    nbdef01 = mIsDef(coords_[ci01]);
	}
	else if ( neighbors_[m01] )
	{
	    nbdef01 =
		neighbors_[m01]->isDefined( currow-spacing+nrcoordspertile,0);
	}

	if ( neighbors_[m10] )
	{
	    const int neighborcol = 0-spacing+nrcoordspertile;
	    nbdef10 = neighbors_[m10]->isDefined( currow, neighborcol );
	    int neighborrow = currow + spacing;
	    if ( islastrow && res )
		neighborrow--; 
	    nbdef20 = neighbors_[m10]->isDefined( neighborrow, neighborcol );
	}

	bool isstripterminated = true;
	for ( int cidx=0; cidx<=nrmyblocks; cidx++ )
	{
	    const int curcol = spacing*cidx;
	    const bool islastcol = cidx==nrmyblocks;
	    int ci12 = ci11 + spacing;
	    int ci22 = ci21 + spacing;
	    if ( islastcol && res )
	    {
		ci12--;
		ci22--;
	    }
	    
	    nbdef12 = mIsDef(coords_[ci12]);
	    nbdef22 = mIsDef(coords_[ci22]);

	    if ( ridx )	    
	    {
		const int ci02 = ci12 - spacing*nrcoordspertile;
		nbdef02 = mIsDef(coords_[ci02]);
	    }
	    else if ( neighbors_[m01] )
	    {
		int neighborcol = curcol+spacing;
		if ( islastcol && res )
		    neighborcol--;

		nbdef02 = neighbors_[m01]->isDefined(
		    currow-spacing+nrcoordspertile,
		    neighborcol );
	    }

	    const int defsum = nbdef11+nbdef12+nbdef21+nbdef22;
	    if ( defsum<3 ) 
	    {
		mTerminateStrip;
		if ( defsum==2 && nbdef11 )
		{
		    const bool con12 = nbdef12 &&
			!nbdef01 && !nbdef02 &&
			!nbdef21 && !nbdef22;
		    const bool con21 = nbdef21 &&
			!nbdef10 && !nbdef20 &&
			!nbdef12 && !nbdef22;

		    if ( con12 || con21 )
    		    {
			const int lastci = lineci.size()
			    ? lineci[lineci.size()-1] : -1;

			if ( lastci!=ci11 )
			{
			    if ( lastci!=-1 )
				mAddIndex( -1, line );

			    mAddIndex( ci11, line );
			}

			mAddIndex( con12 ? ci12 : ci21, line );

		    }
		}
		else if ( defsum==1 && nbdef11 && !nbdef10 && 
			 !nbdef12 && !nbdef01 && !nbdef21 )
		{
		    mAddIndex( ci11, point );
		} 
	    }
	    else if ( !islastrow && !islastcol )
	    {
		if ( defsum==3 )
		{
		    mTerminateStrip;
		    if ( !nbdef11 )
			mAddInitialTriangle( ci12, ci21, ci22 )
		    else if ( !nbdef21 )
			mAddInitialTriangle( ci11, ci22, ci12 )
		    else if ( !nbdef12 )
			mAddInitialTriangle( ci11, ci21, ci22 )
		    else
			mAddInitialTriangle( ci11, ci21, ci12 )
		    mTerminateStrip;
		}
		else
		{
		    const float diff0 = coords_[ci11][2]- coords_[ci22][2];
		    const float diff1 = coords_[ci12][2]- coords_[ci21][2];

		    const bool do11to22 = fabs(diff0) < fabs(diff1);
		    if ( do11to22 )
		    {
			mTerminateStrip;
			mAddInitialTriangle( ci21, ci22, ci11 );
			mAddIndex( ci12, strip );
			mTerminateStrip;
		    }
		    else
		    {
			if ( isstripterminated )
			{
			    mAddInitialTriangle( ci11, ci21, ci12 );
			    mAddIndex( ci22, strip );
			}
			else
			{
			    mAddIndex( ci12, strip );
			    mAddIndex( ci22, strip );
			}
		    }
		} 
	    }
	    else
	    {
		mTerminateStrip;
	    }
	
	    nbdef00 = nbdef01; nbdef01 = nbdef02;
    	    nbdef10 = nbdef11; nbdef11 = nbdef12;
    	    nbdef20 = nbdef21; nbdef21 = nbdef22;
    	    ci11 = ci12; ci21 = ci22;
	}

	mTerminateStrip;
    }

    tesselateWireframe( res, wireframeci, wireframeni );

    datalock_.lock();
    delete tesselationdata_.replace( res, td );
    datalock_.unLock();
    
    needsretesselation_[res] = mNoTesselationNeeded;
}


bool HorizonSectionTile::isDefined( int row, int col ) const
{
    return mIsDef(coords_[row*section_.mNrCoordsPerTileSide+col]);
}



#define mSetTesselationDataImpl( nodes, field, var ) \
    nodes[res]->field.setValues( 0, tesselationdata_[res]->var.size(), \
	    tesselationdata_[res]->var.arr() );                         \
     nodes[res]->field.deleteValues( tesselationdata_[res]->var.size(), -1 )
#define mSetTesselationData( nodes, varprefix ) \
     mSetTesselationDataImpl( nodes, coordIndex, varprefix##ci_ ); \
     mSetTesselationDataImpl( nodes, textureCoordIndex, varprefix##ci_ ); \
     mSetTesselationDataImpl( nodes, normalIndex, varprefix##ni_ )


void HorizonSectionTile::applyTesselation( char res )
{
    if ( !tesselationdata_.validIdx( res ) )
       return;
 
    Threads::MutexLocker lock( datalock_ );
    if ( tesselationdata_[res] )
    {
	mSetTesselationData( triangles_, strip );
	mSetTesselationData( lines_, line );
	mSetTesselationData( points_, point );
	mSetTesselationData( wireframes_, wireframe );

	delete tesselationdata_.replace( res, 0 );
    }
}


void HorizonSectionTile::useWireframe( bool yn )
{
    if ( usewireframe_ == yn )
	return;

    usewireframe_ = yn;
    for ( int idx=0; idx<section_.mHorSectNrRes; idx++ )
	needsretesselation_[idx] = 1; //TODO
}



#define mAddWireframeIndex( ci0, ci1 ) \
{ \
    ci += ci0; \
    ni += getNormalIdx(ci0,res); \
    ci += ci1; \
    ni += getNormalIdx(ci1,res); \
    ci += -1; \
    ni += -1; \
}

void HorizonSectionTile::tesselateWireframe( char res, TypeSet<int>& ci,
	                                     TypeSet<int>& ni ) const
{
    const short sidesize = section_.mTileSideSize;
    const short spacing = section_.spacing_[res];
    const int tilesz = sidesize/spacing + 
	( sidesize%spacing ? 1 : 0 );
    int lnidx = 0;
    for ( int idx=0; idx<=tilesz; idx++ )
    {
	const int rowstartidx = idx<tilesz
	    ? idx*spacing*section_.mNrCoordsPerTileSide
	    : section_.mNrCoordsPerTileSide*sidesize; 
	for ( int idy=0; idy<=tilesz; idy++ )
	{
	    const int colshift = idy<tilesz ? idy*spacing 
					    : sidesize;
	    const int ci0 = rowstartidx + colshift;
	    if ( !mIsDef(coords_[ci0]) ) 
		continue;

	    if ( idy<tilesz )
	    {
		const int nexthorci = idy==tilesz-1
		    ? rowstartidx+sidesize 
		    : ci0 + spacing;
		if ( mIsDef(coords_[ nexthorci ]) )
		    mAddWireframeIndex( ci0, nexthorci );
	    }

	    if ( idx<tilesz )
	    {           
		const int nextvertci = idx==tilesz-1 ? 
  		  section_.mNrCoordsPerTileSide*sidesize+colshift 
		: ci0 + spacing*section_.mNrCoordsPerTileSide;
		if ( mIsDef(coords_[ nextvertci ]) )
		{
		    mAddWireframeIndex( ci0, nextvertci );
		}
	    }
	}
    }
}


void HorizonSectionTile::setPositions( const TypeSet<Coord3>& pos )
{
    RefMan<const Transformation> trans = section_.transformation_;
    nrdefinedpos_ = 0;
    bbox_.makeEmpty();

    for ( int idx=0; idx<section_.mTotalNrCoordsPerTile; idx++ )
    {
	if ( idx>=pos.size() || !pos[idx].isDefined() )
	    coords_[idx][2] = 1e30;
	else
	{
	    Coord3 crd = pos[idx];
	    if ( trans )
		crd = trans->transform( crd );

	    coords_[idx] = SbVec3f( crd[0], crd[1], crd[2] );
	    bbox_.extendBy( coords_[idx] );
	    nrdefinedpos_++;
	}
    }

    datalock_.lock();
    for ( int idx=0; idx<section_.mHorSectNrRes; idx++ )
    {
	needsretesselation_[idx] = mMustRetesselate;
	allnormalsinvalid_[idx] = true;
	invalidnormals_[idx].erase();
    }

    //Prevent anything to be sent in this shape to Coin
    hideFromDisplay();

    needsupdatebbox_ = false;

    datalock_.unLock();
}
 
 
void HorizonSectionTile::setNeighbor( char nbidx, HorizonSectionTile* nb )
{
    if ( (nbidx==5 || nbidx==7 || nbidx==8 ) && neighbors_[nbidx]!=nb )
	glueneedsretesselation_ = true;

    neighbors_[nbidx] = nb;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    bool dohide = false;
    if ( row>=0 && row<=section_.mTileLastIdx && 
	 col>=0 && col<=section_.mTileLastIdx )
    {
	const int posidx = row*section_.mNrCoordsPerTileSide+col;
	const bool olddefined = mIsDef(coords_[posidx]);
	const bool newdefined = pos.isDefined();

	if ( !olddefined && !newdefined )
	    return;

	if ( !newdefined )
	    coords_[posidx][2] = 1e30;
	else
	{
	    Coord3 crd = section_.transformation_
		? section_.transformation_->transform( pos )
		: pos;

	    coords_[posidx] = SbVec3f( crd[0], crd[1], crd[2] );
	    if ( !needsupdatebbox_ )
		bbox_.extendBy(coords_[posidx]);
	}

	char newstatus = mShouldRetesselate;
	if ( olddefined && !newdefined )
	{
	    newstatus = mMustRetesselate;
	    dohide = true;
	}

	if ( newdefined && !olddefined )
	    nrdefinedpos_ ++;
	else if ( !newdefined && olddefined )
	    nrdefinedpos_--;

	for ( int res=0; res<section_.mHorSectNrRes; res++ )
	{
	    if ( newstatus>needsretesselation_[res] &&
		!(row%section_.spacing_[res]) && !(col%section_.spacing_[res]))
		needsretesselation_[res] = newstatus;
	}

	glueneedsretesselation_ = true;

	if ( olddefined && !newdefined )
	    needsupdatebbox_ = true;
    }

    if ( dohide )
	hideFromDisplay();

    setInvalidNormals( row, col );
}


void HorizonSectionTile::setInvalidNormals( int row, int col )
{ 
    for ( int res=0; res<section_.mHorSectNrRes; res++ )
    {
	if ( allnormalsinvalid_[res] )
	    continue;

	int rowstart = row-section_.spacing_[res];
	if ( rowstart>section_.mTileSideSize ) continue;
	if ( rowstart<0 ) rowstart = 0;

	int rowstop = row+section_.spacing_[res];
	if ( rowstop<0 ) continue;
	if ( rowstop>section_.mTileSideSize ) rowstop = section_.mTileLastIdx;

	datalock_.lock();
	for ( int rowidx=rowstart; rowidx<=rowstop; rowidx++ )
	{
	    if ( (rowidx%section_.spacing_[res]) && 
		 (rowidx!=section_.mTileLastIdx) ) 
		continue;
	    const int nmrow = rowidx==section_.mTileLastIdx 
		? section_.normalsidesize_[res]-1 
		: rowidx/section_.spacing_[res];

	    int colstart = col-section_.spacing_[res];
	    if ( colstart>section_.mTileSideSize ) continue;
	    if ( colstart<0 ) colstart = 0;

	    int colstop = col+section_.spacing_[res];
	    if ( colstop<0 ) continue;
	    if ( colstop>section_.mTileSideSize ) 
		colstop = section_.mTileLastIdx;

	    int colstartni = section_.normalstartidx_[res] + 
		nmrow * section_.normalsidesize_[res];
	    for ( int colidx=colstart; colidx<=colstop; colidx++ )
	    {
		if ( (colidx%section_.spacing_[res]) && 
		     (colidx!=section_.mTileLastIdx) )
		    continue;
		const int nmcol = colidx==section_.mTileLastIdx 
		    ? section_.normalsidesize_[res]-1
		    : colidx/section_.spacing_[res];
		invalidnormals_[res].addIfNew( colstartni + nmcol );
	    }
	}

	datalock_.unLock();
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
    }
}


//old code for glue tesselation.
#define mAddGlueIndices( i0, i1, i2, cornpos, adjedgeidx, adjnbidx, inverse ) \
{ \
    const bool df0 =  mIsDef(coords_[i0]); \
    const bool df1 =  mIsDef(coords_[i1]); \
    const bool df2 =  mIsDef(coords_[i2]); \
    const int dfsum = df0 + df1 + df2; \
    if ( dfsum==mStrip ) \
    { \
	mAddIndex( i0, strip ) \
	mAddIndex( nb==7 ? i1 : i2, strip ) \
	mAddIndex( nb==7 ? i2 : i1, strip ) \
	mAddIndex( -1, strip ) \
    } \
    else if ( dfsum==mPoint ) \
    { \
	const int ptidx= df0 ? i0 : (df1 ? i1 : i2); \
	if ( pointci.indexOf(ptidx)==-1 ) mAddIndex( ptidx, point ) \
    } \
    else if ( dfsum==mLine && (cornpos<0 || !mIsDef(coords_[cornpos])) ) \
    { \
	dfadjedge = (adjedgeidx<0 || adjedgeidx>nrmyblocks) ?  \
	false :  mIsDef(coords_[edgeindices[adjedgeidx]]); \
	dfadjnb = (adjnbidx<0 || adjnbidx>nbblocks) ?  \
	false :  mIsDef(coords_[nbindices[adjnbidx]] ); \
	if ( df0 && df2 ) \
	{ \
	    if ( (!inverse && !finer) || (inverse && !dfadjedge && !dfadjnb) )\
	    { \
		mAddIndex(i0,line); mAddIndex(i2,line); mAddIndex(-1,line);\
	    }\
	} \
	else if ( df1 && df2 ) \
	{ \
	    if ( (!inverse && (finer || (!finer && !neighbors_[nb]))) || \
		    (inverse && ((!finer && !dfadjedge && !dfadjnb) || \
				 (finer && !neighbors_[nb]))) ) \
	    { \
		mAddIndex(i1,line); mAddIndex(i2,line); mAddIndex(-1,line);\
	    } \
	} \
	else if ( df0 && df1 ) \
	{ \
	    if ( (!inverse && !dfadjedge && !dfadjnb) || inverse ) \
	    { \
		mAddIndex(i1,line); mAddIndex(i2,line); mAddIndex(-1,line);\
	    } \
	} \
    } \
}


void HorizonSectionTile::tesselateGlue()
{
    const int res = getActualResolution();
    TypeSet<int> pointci, lineci, stripci;
    TypeSet<int> pointni, lineni, stripni;

    const short nrcoordspertile = section_.mNrCoordsPerTileSide;
    
    if ( res!=-1 )
    {
	const short spacing = section_.spacing_[res];
	const short nrmyblocks = mNrBlocks( spacing );	

	for ( int nb=5; nb<8; nb += 2 )
	{	
	    TypeSet<int> edgeindices; 
	    for ( int idx=0; idx<=nrmyblocks; idx++ )
	    {
		edgeindices += ( nb==5 
		    ? spacing * (nrmyblocks+idx*nrcoordspertile) 
		    : spacing * (nrcoordspertile*nrmyblocks+idx));
    	    }

	    int nbres = neighbors_[nb] ? neighbors_[nb]->getActualResolution() 
				       : res; 
	    if ( nbres==-1 ) nbres = res; 
	    const short nbspacing = section_.spacing_[nbres];
	    const short nbblocks = mNrBlocks( nbspacing ); 
	    TypeSet<int> nbindices; 
	    for ( int idx=0; idx<=nbblocks; idx++ ) 
	    {
		nbindices += (nb==5 
		    ? (1+idx*section_.spacing_[nbres]) * nrcoordspertile-1 
		    : nrcoordspertile*section_.mTileSideSize+
		      idx*section_.spacing_[nbres]);
	    }

	    bool  finer = nrmyblocks >= nbblocks;
	    const int highstopidx = finer ? nrmyblocks : nbblocks;
	    const int nrconns = section_.spacing_[abs(nbres-res)];
	    
	    int adjedgeidx, adjnbidx, cornpos;
	    bool dfadjedge, dfadjnb, dfcorner;

	    int lowresidx = 0, skipped = 0; 
	    for ( int idx=0; idx<highstopidx; idx++ ) 
	    {
		int i0 = finer ? edgeindices[idx] : edgeindices[lowresidx]; 
		int i1 = finer ? nbindices[lowresidx] : nbindices[idx]; 
		int i2 = finer ? edgeindices[idx+1] : nbindices[idx+1]; 
		cornpos = finer ?
		    (lowresidx>=nbblocks ? -1 : nbindices[lowresidx+1]) :
		    (lowresidx>=nrmyblocks ? -1 : edgeindices[lowresidx+1]);
		adjedgeidx = finer ? idx-1 : lowresidx-1;
		adjnbidx = finer ? lowresidx-1 : idx-1;
		mAddGlueIndices(i0,i1,i2,cornpos,adjedgeidx,adjnbidx,false)

		skipped++;

		if ( (skipped%nrconns) && (idx+1!=nrconns/2) ) 
		    continue; 

		skipped = 0; 
		if ( lowresidx<(finer ? nbblocks : nrmyblocks) ) 
		{ 
		    lowresidx++; 
		    i0 = finer ? edgeindices[idx+1] : edgeindices[lowresidx-1];
		    i1 = finer ? nbindices[lowresidx-1] : nbindices[idx+1]; 
		    i2 = finer ? nbindices[lowresidx]: edgeindices[lowresidx];
		    cornpos = finer ? edgeindices[idx] : nbindices[idx];
		    adjedgeidx = finer ? idx+2 : lowresidx+1;
		    adjnbidx = finer ? lowresidx+1 : idx+2;
		    
		    mAddGlueIndices(i0,i1,i2,cornpos,adjedgeidx,adjnbidx,true)
		} 
	    }

	    finer = false;
	    mAddGlueIndices( edgeindices[nrmyblocks], nbindices[nbblocks],
 			     section_.mTotalNrCoordsPerTile-1, -1,
			     nrmyblocks-1, nbblocks-1, false );
	}
    }
   
    const int stripsz = stripci.size();
    const int linesz = lineci.size();
    const int pointsz = pointci.size();
    
    gluetriangles_->coordIndex.setValues( 0, stripsz, stripci.arr() );
    gluetriangles_->coordIndex.deleteValues( stripsz, -1 ); 
    gluetriangles_->textureCoordIndex.setValues( 0, stripsz, stripci.arr() );
    gluetriangles_->textureCoordIndex.deleteValues( stripsz, -1 );
    gluetriangles_->normalIndex.setValues( 0, stripni.size(), stripni.arr() );
    gluetriangles_->normalIndex.deleteValues( stripni.size(), -1 ); 
    gluelines_->coordIndex.setValues( 0, linesz, lineci.arr() );
    gluelines_->coordIndex.deleteValues( linesz, -1 ); 
    gluelines_->textureCoordIndex.setValues( 0, linesz, lineci.arr() );
    gluelines_->textureCoordIndex.deleteValues( linesz, -1 );
    gluelines_->normalIndex.setValues( 0, lineni.size(), lineni.arr() );
    gluelines_->normalIndex.deleteValues( lineni.size(), -1 ); 
    gluepoints_->coordIndex.setValues( 0, pointsz, pointci.arr() );
    gluepoints_->coordIndex.deleteValues( pointsz, -1 ); 
    gluepoints_->textureCoordIndex.setValues( 0, pointsz, pointci.arr() );
    gluepoints_->textureCoordIndex.deleteValues( pointsz, -1 );
    gluepoints_->normalIndex.setValues( 0, pointni.size(), pointni.arr() );
    gluepoints_->normalIndex.deleteValues( pointni.size(), -1 ); 
}


void HorizonSectionTile::setTextureSize( int rowsz, int colsz )
{ texture_->size.setValue( 1, rowsz, colsz ); }


void HorizonSectionTile::setTextureOrigin( int row, int col )
{ texture_->origin.setValue( 0, row, col ); }


}; // namespace visBase

