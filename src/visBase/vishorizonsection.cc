/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: vishorizonsection.cc,v 1.79 2009-08-26 18:14:15 cvskris Exp $";

#include "vishorizonsection.h"

#include "binidsurface.h"
#include "binidvalset.h"
#include "cubesampling.h"
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
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoSeparator.h>
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
#define mHorSectNrRes		6

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
    void			setDisplayTransformation(Transformation*);
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
    void			updateNormals(char res);
    void			tesselateResolution(char res);
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


protected:

    int				getNormalIdx(int crdidx,int res) const;
    				//!<Gives normal index in normals_

    friend class		HorizonSectionTilePosSetup;
    friend class		HorizonSectionTileUpdater;			
    friend class		TileTesselator;			
    void			bgTesselationFinishCB(CallBacker*);
    void			setActualResolution(int);
    int				getAutoResolution(SoState*);
    void			tesselateGlue();
    void			tesselateWireframe(char,TypeSet<int>& ci,
						   TypeSet<int>& ni) const;
    void			updateBBox();
    void			setWireframe(int res);
    void			setInvalidNormals(int row,int col);
    void			computeNormal(int normidx, int res);

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
    visBase::Coordinates*	coords_;
    SoTextureComposer*		texture_;
    SoSwitch*			resswitch_;
    SoNormal*			normals_;

    int				desiredresolution_;
    bool			resolutionhaschanged_;

    char			needsretesselation_[mHorSectNrRes];
    				//!<0 - updated, 1 - needs update, 2 - dont disp
    ObjectSet<TileTesselator>	tesselationqueue_;
    Threads::ConditionVar	tesselationqueuelock_;

    bool			allnormalsinvalid_[mHorSectNrRes];

    SoSeparator*		resolutions_[mHorSectNrRes];
    SoIndexedTriangleStripSet*	triangles_[mHorSectNrRes];
    SoIndexedLineSet3D*		lines_[mHorSectNrRes];
    SoIndexedLineSet*		wireframes_[mHorSectNrRes];
    SoDGBIndexedPointSet*	points_[mHorSectNrRes];

    TypeSet<int>		invalidnormals_[mHorSectNrRes];
    ObjectSet<TesselationData>	tesselationdata_;
    Threads::Mutex		datalock_;
    SoSwitch*			wireframeswitch_;
    SoSeparator*		wireframeseparator_;

    SoIndexedTriangleStripSet*	gluetriangles_;
    SoIndexedLineSet3D*		gluelines_;
    SoDGBIndexedPointSet*	gluepoints_;
    char			glueneedsretesselation_;
    				//!<0 - updated, 1 - needs update, 2 - dont disp

    CallBack			bgfinished_;

    static const char		spacing_[];
    static const short		nrcells_[];
    static const short		normalstartidx_[];
    static const char		normalsidesize_[];
};


const short HorizonSectionTile::normalstartidx_[] =
{ 0, 4096, 5185, 5474, 5555, 5580 };
const char HorizonSectionTile::normalsidesize_[] = { 64, 33, 17, 9, 5, 3 };
const char HorizonSectionTile::spacing_[] = { 1, 2, 4, 8, 16, 32 };
const short HorizonSectionTile::nrcells_[] = { 4096, 1024, 256, 64, 16, 4 };


class HorizonSectionTileUpdater: public ParallelTask
{
public: 
HorizonSectionTileUpdater( HorizonSection& section, SoState* state, int res )
    : section_( section )
    , state_( state )
    , tiles_( section.tiles_.getData() )
    , nrtiles_( section.tiles_.info().getTotalSz() )
    , nrcoltiles_( section.tiles_.info().getSize(1) )
    , resolution_( res )						     
{}


od_int64 nrIterations() const { return nrtiles_; }
od_int64 totalNr() const { return nrtiles_ * 2; }
const char* message() const { return "Updating Horizon Display"; }
const char* nrDoneText() const { return "Parts completed"; }


bool doPrepare( int nrthreads )
{
    nrthreads_ = nrthreads;
    nrthreadsfinishedwithres_ = 0;

    mAllocVarLenArr( int, arr, nrtiles_ );
    for ( int idx=0; idx<nrtiles_; idx++ )
	arr[idx] = idx;

    std::random_shuffle( mVarLenArr(arr), arr+nrtiles_ );
    for ( int idx=0; idx<nrtiles_; idx++ )
	permutation_ += arr[idx];

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    if ( state_ )
    {
	for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
	{
	    const int realidx = permutation_[idx];
	    if ( tiles_[realidx] ) 
		tiles_[realidx]->updateAutoResolution( state_ );
	}

	controlcond_.lock();
	nrthreadsfinishedwithres_++;
	if ( nrthreadsfinishedwithres_==nrthreads_ )
	    controlcond_.signal( true ); 
	controlcond_.unLock();
    }

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	HorizonSectionTile* tile = tiles_[realidx];
	if ( tile )
	{
	    if ( state_ ) 
	    {
		int res = tile->getActualResolution();
		if ( res==-1 ) res = resolution_;
		tile->updateNormals( res );
	    }

	    if ( state_ )
	    {
		const int actualres = tile->getActualResolution();
		if ( actualres!=-1 )
		    tile->tesselateResolution( actualres );
	    }
	    else
	    {
		tile->tesselateResolution( resolution_ );
		tile->setActualResolution( resolution_ );
	    }
	}

	addToNrDone( 1 );
    }	

    if ( state_ )
    {
	controlcond_.lock();
	while ( nrthreadsfinishedwithres_!=nrthreads_ && shouldContinue() )
	    controlcond_.wait();
	controlcond_.unLock();
    }

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( tiles_[realidx] ) 
	    tiles_[realidx]->updateGlue();

	addToNrDone( 1 );
    }

    return true;
}

    TypeSet<int>		permutation_;
    SoState*			state_;
    HorizonSectionTile**	tiles_;
    HorizonSection&		section_;
    int				nrtiles_;
    int				nrcoltiles_;
    int				resolution_;
    int				nrthreads_;
    int				nrthreadsfinishedwithres_;
    Threads::ConditionVar	controlcond_;
};


class TileTesselator : public SequentialTask
{
public:
	TileTesselator( HorizonSectionTile* tile, char res )
	    : tile_( tile ), res_( res ) {}

    int	nextStep()
    {
	tile_->tesselateResolution( res_ );
	return SequentialTask::Finished();
    }

    HorizonSectionTile*		tile_;
    char			res_;
};



class HorizonSectionTilePosSetup: public ParallelTask
{
public:    
    HorizonSectionTilePosSetup( ObjectSet<HorizonSectionTile> tiles, 
	    const Geometry::BinIDSurface& geo,
	    StepInterval<int> rrg, StepInterval<int> crg, ZAxisTransform* zat )
	: tiles_( tiles )
	, geo_( geo )  
	, rrg_( rrg )
	, crg_( crg )			 	
        , zat_( zat )						
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
	TypeSet<Coord3> positions( mNrCoordsPerTileSide*mNrCoordsPerTileSide,
	       			   Coord3::udf() );
	for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
	{
	    const RowCol& origin = tiles_[idx]->origin_;
	    int coordidx = 0;
	    for ( int rowidx=0; rowidx<mNrCoordsPerTileSide; rowidx++ )
	    {
		const int row = origin.row + rowidx*rrg_.step;
		const bool rowok = rrg_.includes(row);
		const StepInterval<int> colrg( 
			mMAX(geo_.colRange(row).start, crg_.start),
		        mMIN(geo_.colRange(row).stop, crg_.stop), crg_.step );
		for ( int colidx=0; colidx<mNrCoordsPerTileSide;
		      colidx++, coordidx++ )
		{
		    const int col = origin.col + colidx*colrg.step;
		    Coord3 pos = rowok && colrg.includes(col)
			? geo_.getKnot(RowCol(row,col),false) 
			: Coord3::udf();
		    if ( zat_ ) pos.z = zat_->transform( pos );
		
	    	    positions[coordidx] = pos;
		}
	    }

	    tiles_[idx]->setPositions( positions );
	    tiles_[idx]->updateNormals( mLowestResIdx );
	    tiles_[idx]->tesselateResolution( mLowestResIdx );

	    addToNrDone(1);
	}

	return true;
    }

    ObjectSet<HorizonSectionTile> 	tiles_;
    const Geometry::BinIDSurface&	geo_;
    StepInterval<int>			rrg_, crg_;
    ZAxisTransform*			zat_;
};



ArrPtrMan<SbVec2f> HorizonSection::texturecoordptr_ = 0;

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
    , userchangeddisplayrg_( false )			      
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
    wireframematerial_->ref();
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
   
    removeChild( callbacker_ ); 
    callbacker_->unref();

    removeChild( channels_->getInventorNode() );
    channels_->unRef();
    removeChild( texturecrds_ );
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


void HorizonSection::setChannel2RGBA( TextureChannel2RGBA* t )
{
    channels_->setChannels2RGBA( t );
    if ( channel2rgba_ )
	channel2rgba_->unRef();

    channel2rgba_ = t;

    if ( channel2rgba_ )
	channel2rgba_->ref();
}

TextureChannel2RGBA* HorizonSection::getChannel2RGBA()
{ return channel2rgba_; }


const TextureChannel2RGBA* HorizonSection::getChannel2RGBA() const
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
    if ( !geometry_ || zaxistransformvoi_==-1 )	
	return;

    if ( !zaxistransform_ || !zaxistransform_->needsVolumeOfInterest() )
	return;

    CubeSampling cs;
    cs.hrg.set( displayrrg_, displaycrg_ );

    HorSamplingIterator iter( cs.hrg );

    bool first = true;
    BinID curpos;
    while ( iter.next(curpos) )
    {
	const float depth = geometry_->getKnot(curpos,false).z;
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
	if ( !displayrrg_.includes(bid.inl) || !displaycrg_.includes(bid.crl) )
	    continue;

	const Coord3 pos = geometry_->getKnot(bid,false);
	if ( !pos.isDefined() ) 
	    continue;

	float zval = pos.z;
	if ( zaxistransform_ && zshift )
	{
	    zval = zaxistransform_->transform( BinIDValue(bid,zval) );
	    if ( mIsUdf(zval) )
		continue;

	    zval += zshift;
	    zval = zaxistransform_->transformBack( BinIDValue(bid,zval) );

	    if ( mIsUdf(zval) )
		continue;
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

    const int nrrows = displayrrg_.nrSteps()+1;
    const int nrcols = displaycrg_.nrSteps()+1;

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
	if ( sidcol!=-1 && sid!=mNINT(ptr[sidcol]) )
	    continue;

	const BinID bid = data->getBinID( pos );

	const int inlidx = displayrrg_.nearestIndex(bid.inl);
	const int crlidx = displaycrg_.nearestIndex(bid.crl);

	const int offset = inlidx*nrcols + crlidx;

	for ( int idx=0; idx<nrversions; idx++ )
	    versiondata[idx][offset] = ptr[idx+startsourceidx];
    }

    for ( int idx=0; idx<nrversions; idx++ )
	channels_->setUnMappedData( channel, idx, versiondata[idx],
				    OD::TakeOverPtr, 0 );

    updateTileTextureOrigin( RowCol(displayrrg_.start-origin_.row,
				    displaycrg_.start-origin_.col) );
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
	channels_->setColTabMapperSetup( channel, mapper );
	channels_->reMapData( channel, tr );
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


void HorizonSection::setSurface( Geometry::BinIDSurface* surf, bool connect,
       				 TaskRunner* tr )
{
    if ( !surf ) return;
   
    if ( !userchangeddisplayrg_ )
    {	
    	displayrrg_ = surf->rowRange();
    	displaycrg_ = surf->colRange();
    }

    origin_.row = displayrrg_.start;
    origin_.col = displaycrg_.start;
    rowdistance_ = displayrrg_.step*SI().inlDistance();
    coldistance_ = displaycrg_.step*SI().crlDistance();

    if ( connect )
    {
	geometry_ = surf;
	CallBack cb =  mCB( this, HorizonSection, surfaceChangeCB );
	geometry_->movementnotifier.notify( cb );
	geometry_->nrpositionnotifier.notify( cb );
    }

    surfaceChange( 0, tr );
}


void HorizonSection::setDisplayRange( const StepInterval<int>& rrg,
	const StepInterval<int>& crg, bool userchange )
{
    const bool usegeo = !userchange && geometry_;
    if ( (!usegeo && displayrrg_==rrg && displaycrg_==crg) || 
	 (usegeo && displayrrg_==geometry_->rowRange() && 
	  	    displaycrg_==geometry_->colRange()) )
	return;

    displayrrg_ = usegeo ? geometry_->rowRange() : rrg;
    displaycrg_ = usegeo ? geometry_->colRange() : crg;
    origin_.row = displayrrg_.start;
    origin_.col = displayrrg_.start;
    userchangeddisplayrg_ = userchange;

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( tileptrs[idx] )
	{
    	    removeChild( tileptrs[idx]->getNodeRoot() );
    	    delete tileptrs[idx];
	}
    }

    surfaceChange( 0, 0 );
    setResolution( desiredresolution_, 0 );
}


const StepInterval<int>& HorizonSection::displayedRowRange() const
{ return displayrrg_; }


const StepInterval<int>& HorizonSection::displayedColRange() const
{ return displaycrg_; }


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
  
    if ( !userchangeddisplayrg_ )
    {
	displayrrg_ = geometry_->rowRange();
	displaycrg_ = geometry_->colRange();
    }

    if ( displayrrg_.width(false)<0 || displaycrg_.width(false)<0 )
	return;

    const RowCol step( displayrrg_.step, displaycrg_.step );
    ObjectSet<HorizonSectionTile> fullupdatetiles;

    const bool updatewireframe = usewireframe_ && desiredresolution_!=-1;

    tesselationlock_ = true;
    if ( !gpids || !tiles_.info().getSize(0) || !tiles_.info().getSize(1) )
    {
	origin_ = RowCol( displayrrg_.start, displaycrg_.start );
	const int nrrows = 
	    nrBlocks( displayrrg_.nrSteps()+1, mNrCoordsPerTileSide, 1 );
	const int nrcols = 
	    nrBlocks( displaycrg_.nrSteps()+1, mNrCoordsPerTileSide, 1 );

	if ( !tiles_.setSize( nrrows, nrcols ) )
	{
	    tesselationlock_ = false;
	    return;
	}

	tiles_.setAll( 0 );

	for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
	{
	    for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	    {
		fullupdatetiles += createTile(tilerowidx, tilecolidx);
	    }
	}
    }
    else
    {
	updateTileArray();
	const int nrrowsz = tiles_.info().getSize(0);
	const int nrcolsz = tiles_.info().getSize(1);

    	for ( int idx=(*gpids).size()-1; idx>=0; idx-- )
	{
	    const RowCol& absrc( (*gpids)[idx] );
	    RowCol rc = absrc-origin_; rc /= step;

	    const int tilerowidx = rc.row/mTileSideSize;
	    const int tilerow = rc.row%mTileSideSize;

	    const int tilecolidx = rc.col/mTileSideSize;
	    const int tilecol = rc.col%mTileSideSize;

	    const Coord3 pos = geometry_->getKnot(absrc,false);

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

		    HorizonSectionTile* nbtile = 
			tiles_.get( neighborrow,neighborcol );
		    if ( !nbtile || fullupdatetiles.indexOf(nbtile)!=-1)
			continue;

		    nbtile->setPos( tilerow-rowidx*mTileSideSize,
			    	    tilecol-colidx*mTileSideSize, pos );
		}
	    }
	}
    }

    HorizonSectionTilePosSetup task( fullupdatetiles, *geometry_,
	   displayrrg_, displaycrg_, zaxistransform_ );
    if ( tr ) tr->execute( task );
    else task.execute();
  
    tesselationlock_ = false;
}


void HorizonSection::updateTileArray()
{
    const int rowsteps =  mTileSideSize*displayrrg_.step;
    const int colsteps = mTileSideSize*displaycrg_.step;
    const int oldrowsize = tiles_.info().getSize(0);
    const int oldcolsize = tiles_.info().getSize(1);
    int newrowsize = oldrowsize;
    int newcolsize = oldcolsize;
    int nrnewrowsbefore = 0;
    int nrnewcolsbefore = 0;
	
    int diff = origin_.row - displayrrg_.start;
    if ( diff>0 ) 
    {
	nrnewrowsbefore = diff/rowsteps + (diff%rowsteps ? 1 : 0);
    	newrowsize += nrnewrowsbefore;
    }

    diff = displayrrg_.stop - (origin_.row+oldrowsize*rowsteps);
    if ( diff>0 ) newrowsize += diff/rowsteps + (diff%rowsteps ? 1 : 0);
    
    diff = origin_.col-displaycrg_.start;
    if ( diff>0 ) 
    {
	nrnewcolsbefore = diff/colsteps + (diff%colsteps ? 1 : 0);
    	newcolsize += nrnewcolsbefore;
    }

    diff = displaycrg_.stop - (origin_.col+oldcolsize*colsteps);
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

    tiles_.copyFrom( newtiles );
    origin_.row -= nrnewrowsbefore*rowsteps;
    origin_.col -= nrnewcolsbefore*colsteps;
}


void HorizonSection::updateTileTextureOrigin( const RowCol& textureorigin )
{
    const int nrrows = tiles_.info().getSize(0);
    const int nrcols = tiles_.info().getSize(1);
    const RowCol step( displayrrg_.step, displaycrg_.step );

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
    const RowCol step( displayrrg_.step, displaycrg_.step );
    const RowCol tileorigin( origin_.row+tilerowidx*mTileSideSize*step.row,
			     origin_.col+tilecolidx*mTileSideSize*step.col );
    HorizonSectionTile* tile = new HorizonSectionTile( *this, tileorigin,
	    			wireframematerial_->getInventorNode() );

    tile->setDisplayTransformation( transformation_ );
    tile->setResolution( desiredresolution_ );
    tile->useWireframe( usewireframe_ );
    tile->setRightHandSystem( righthandsystem_ );

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

#define mApplyTesselation \
{ \
    for ( int idx=0; idx<tilesz; idx++ ) \
    { \
	if ( tileptrs[idx] ) \
	{ \
	    tileptrs[idx]->applyTesselation( \
	    tileptrs[idx]->getActualResolution() ); \
	} \
    } \
}


void HorizonSection::updateAutoResolution( SoState* state, TaskRunner* tr )
{
    if ( tesselationlock_ )
	return;

    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz || !state ) return;

    HorizonSectionTile** tileptrs = tiles_.getData();
    if ( desiredresolution_!=-1 )
    {
	mApplyTesselation;
	return;
    }

    const int32_t camerainfo = SoCameraInfoElement::get(state);
    bool ismoving = camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE);
    if ( ismoving_ && ismoving )
	return;

    ismoving_ = ismoving;

    HorizonSectionTileUpdater task( *this, state, desiredresolution_ );
    if ( tr )
	tr->execute(task);
    else
	task.execute();

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetGlueNeedsUpdateFlag();

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetResolutionChangeFlag();

    mApplyTesselation;
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

    HorizonSectionTileUpdater task( *this, 0, res );
    if ( tr )
	tr->execute(task);
    else
	task.execute();

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetGlueNeedsUpdateFlag();

    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetResolutionChangeFlag();
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
    , normals_( new SoNormal )  
    , coords_( visBase::Coordinates::create() )
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
    , bgfinished_( mCB( this, HorizonSectionTile, bgTesselationFinishCB ) )
    , origin_( origin )
    , section_( section )
{
    tesselationdata_.allowNull();
    root_->ref();
    coords_->ref();
    normals_->ref();
    normals_->vector.setNum( mTotalNormalSize );
    for ( int idx=0; idx<mTotalNormalSize; idx++ )
	normals_->vector.set1Value(idx, SbVec3f(0,0,-1) );


    root_->addChild( coords_->getInventorNode() );
    root_->addChild( normals_ );
    root_->addChild( texture_ );

    root_->addChild( gluetriangles_ );
    root_->addChild( gluepoints_ );
    
    gluetriangles_->coordIndex.deleteValues( 0, -1 );
    gluelines_->coordIndex.deleteValues( 0, -1 );
    gluepoints_->coordIndex.deleteValues( 0, -1 );

    gluelines_->rightHandSystem = true ;
    gluelines_->radius = 2;
    gluelines_->screenSize = false;

    wireframeseparator_ = new SoSeparator;
    root_->addChild( wireframeseparator_ );
    wireframeseparator_->addChild( wfmat );
    wireframeswitch_ = new SoSwitch;
    wireframeseparator_->addChild( wireframeswitch_ );

    root_->addChild( resswitch_ );
    for ( int idx=0; idx<mHorSectNrRes; idx++ )
    {
	allnormalsinvalid_[idx] = true;
	needsretesselation_[idx] = false;
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
	lines_[idx]->rightHandSystem = true; 
	lines_[idx]->radius = 2;
	lines_[idx]->screenSize = false;
	lines_[idx]->coordIndex.deleteValues( 0, -1 );
	resolutions_[idx]->addChild( lines_[idx] );

	wireframes_[idx] = new SoIndexedLineSet;
	wireframes_[idx]->coordIndex.deleteValues( 0, -1 );
	wireframeswitch_->addChild( wireframes_[idx] );

	tesselationdata_ += 0;
    }

    for ( int idx=0; idx<9; idx++ )
	neighbors_[idx] = 0;

    setTextureSize( mNrCoordsPerTileSide, mNrCoordsPerTileSide );
    bbox_.makeEmpty();
}


HorizonSectionTile::~HorizonSectionTile()
{
    root_->removeChild( coords_->getInventorNode() );
    coords_->unRef();
    normals_->unref();
    root_->unref();
    
    tesselationqueuelock_.lock();
    for ( int idx=tesselationqueue_.size()-1; idx>=0; idx-- )
    {
	if ( ParallelTask::twm().removeWork( tesselationqueue_[idx] ) )
	    delete tesselationqueue_.remove( idx );
    }

    while ( tesselationqueue_.size() )
	tesselationqueuelock_.wait();

    tesselationqueuelock_.unLock();

    deepErase( tesselationdata_ );
}


void HorizonSectionTile::setRightHandSystem( bool yn )
{
    gluelines_->rightHandSystem = yn;
    for ( int idx=0; idx<mHorSectNrRes; idx++ )
    	lines_[idx]->rightHandSystem = yn;
}


void HorizonSectionTile::setDisplayTransformation( Transformation* nt )
{
    if ( coords_->getDisplayTransformation()==nt )
	return;

    coords_->setDisplayTransformation( nt );
    needsupdatebbox_ = true;
}


void HorizonSectionTile::updateNormals( char res )
{
    if ( res<0 ) return;

    const bool oldstatus = normals_->vector.enableNotify( false );
    bool change = false;

    if ( allnormalsinvalid_[res] )
    {
	const int normalstop = res<mLowestResIdx ? 
	    normalstartidx_[res+1]-1 : mTotalNormalSize-1;
	for ( int idx=normalstartidx_[res]; idx<=normalstop; idx++ )
	    computeNormal( idx, res );
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
	    computeNormal( invnormals[idx], res );
	}
    }

    emptyInvalidNormalsList( res );
    allnormalsinvalid_[res] = false; 
    normals_->vector.enableNotify( oldstatus );
    if ( change )
	normals_->vector.touch();
}


#define mGetGradient( rc, arrpos ) \
    beforefound = false; afterfound = false; \
    for ( int idx=spacing_[res]; idx>=0; idx-- ) \
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



void HorizonSectionTile::computeNormal( int nmidx, int res )
{
    const RowCol step( section_.displayrrg_.step, section_.displaycrg_.step );

    const int normalrow = (nmidx-normalstartidx_[res])/normalsidesize_[res];
    const int normalcol = (nmidx-normalstartidx_[res])%normalsidesize_[res];

    const int row = origin_.row + step.row * normalrow*spacing_[res];
    const int col = origin_.col + step.col * normalcol*spacing_[res];

    const StepInterval<int> rowrange = section_.geometry_->rowRange();
    const StepInterval<int> colrange = section_.geometry_->colRange();
    bool beforefound, afterfound;
    Coord beforepos, afterpos;

    mGetGradient( row, RowCol(currow,col) );
    mGetGradient( col, RowCol(row,curcol) );
 
    const SbVec3f normal(
	drow*section_.cosanglexinl_+dcol*section_.sinanglexinl_,
	dcol*section_.cosanglexinl_-drow*section_.sinanglexinl_, -1 );
    normals_->vector.set1Value( nmidx, normal );
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
   
    if ( (!(row%spacing_[useres]) || (row==mTileLastIdx)) &&
         (!(col%spacing_[useres]) || (col==mTileLastIdx)) )
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
		  SoCullElement::cullTest(state, bbox, true ) )
	     newres = -1;
	 else if ( desiredresolution_==-1 )
	 {
	     const int wantedres = getAutoResolution( state );
	     newres = wantedres;
	     datalock_.lock();
	     for ( ; newres<mHorSectNrRes-1; newres++ )
	     {
		 if ( !needsretesselation_[newres] &&
		      !allnormalsinvalid_[newres] &&
		      !invalidnormals_[newres].size() )
		     break;
	     }
	     datalock_.unLock();

	     if ( wantedres!=newres )
	     {
		 tesselationqueuelock_.lock();
		 bool found = false;
		 for ( int idx=0; idx<tesselationqueue_.size(); idx++ )
		 {
		     if ( tesselationqueue_[idx]->res_==wantedres )
		     {
			 found = true;
			 break;
		     }
		 }

		 if ( !found )
		 {
		     TileTesselator* tt = new TileTesselator( this, wantedres );
		     tesselationqueue_ += tt;
		     ParallelTask::twm().addWork( tt, &bgfinished_, true );
		 }

		 tesselationqueuelock_.unLock();
		 newres = wantedres;
	     }
	 }
     }

     setActualResolution( newres );
}


void HorizonSectionTile::bgTesselationFinishCB( CallBacker* cb )
{
    mDynamicCastGet( const TileTesselator*, tt,ParallelTask::twm().getWork(cb));
    if ( !tt )
	return;

    tesselationqueuelock_.lock();

    const int idx = tesselationqueue_.indexOf( tt );
    delete tesselationqueue_.remove( idx );

    if ( !tesselationqueue_.size() )
	tesselationqueuelock_.signal( true );

    tesselationqueuelock_.unLock();

    resswitch_->touch();
}


const SbBox3f& HorizonSectionTile::getBBox() const
{ return bbox_; }


void HorizonSectionTile::updateBBox()
{
    if ( !needsupdatebbox_ ) return;

    bbox_.makeEmpty();

    int id = coords_->nextID( -1 );
    while ( id>=0 )
    {
	const Coord3 pos = coords_->getPos( id, true );
	if ( pos.isDefined() )
	{
	    const SbVec3f vec( pos.x, pos.y, pos.z );
	    bbox_.extendBy( vec );
	}

	id = coords_->nextID( id );
    }

    if ( bbox_.isEmpty() )
	return;

    needsupdatebbox_ = false;
}


int HorizonSectionTile::getAutoResolution( SoState* state ) 
{
    updateBBox();

    if ( bbox_.isEmpty() )
	return -1;

    if ( section_.ismoving_ )
	return mLowestResIdx;

    SbVec2s screensize;
    SoShape::getScreenSize( state, bbox_, screensize );
    const float complexity = SbClamp(SoComplexityElement::get(state),0.0f,1.0f);
    const int wantednumcells = (int)(complexity*screensize[0]*screensize[1]/16);
    if ( !wantednumcells )
	return mLowestResIdx;

    if ( nrdefinedpos_<=wantednumcells )
	return 0;

    int maxres = nrdefinedpos_/wantednumcells;
    if ( nrdefinedpos_%wantednumcells ) maxres++;

    for ( int desiredres=mLowestResIdx; desiredres>=0; desiredres-- )
    {
	if ( nrcells_[desiredres]>=wantednumcells )
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

#define mStrip 3
#define mLine 2
#define mPoint 1

#define mAddInitialTriangle( ci0, ci1, ci2 ) \
{ \
    isstripterminated =  false; \
    mAddIndex( ci0, mStrip ); \
    mAddIndex( ci1, mStrip ); \
    mAddIndex( ci2, mStrip ); \
}


#define mTerminateStrip \
if ( !isstripterminated ) \
{ \
    isstripterminated = true; \
    mAddIndex( -1, mStrip ); \
}


#define mAddIndex( ci, obj ) \
{ \
    if ( obj==mStrip ) \
    { \
	stripci += ci; \
	stripni += getNormalIdx(ci,res); \
    }\
    else if ( obj==mLine ) \
    { \
	lineci += ci; \
	lineni += getNormalIdx(ci,res); \
    }\
    else if ( obj==mPoint ) \
    { \
	pointci += ci; \
	pointni += getNormalIdx(ci,res); \
    }\
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


void HorizonSectionTile::tesselateResolution( char res )
{
    if ( res<0 || !needsretesselation_[res] ) return;

    TesselationData* td = new TesselationData;

    TypeSet<int>& pointci = td->pointci_, &lineci = td->lineci_,
		    &stripci = td->stripci_, &wireframeci = td->wireframeci_;
    TypeSet<int>& pointni = td->pointni_, &lineni = td->lineni_,
		    &stripni = td->stripni_, &wireframeni = td->wireframeni_;

    const int nrmyblocks = mNrBlocks(res);
    
    for ( int ridx=0; ridx<=nrmyblocks; ridx++ )
    {
	int ci11 = spacing_[res]*ridx*mNrCoordsPerTileSide;
	int ci21 = ci11 + spacing_[res]*mNrCoordsPerTileSide;

	bool nbdef[] = { false, false, false,		// 00   01     02
	    		 false, false, false,		// 10   11(me) 12
			 false, false, false };		// 20   21     22

	nbdef[m11] = coords_->isDefined(ci11);
	nbdef[m21] = ridx!=nrmyblocks ? coords_->isDefined(ci21) : false;
	if ( ridx )
	{
	    int ci01 = ci11 - spacing_[res]*mNrCoordsPerTileSide;
	    nbdef[m01] = coords_->isDefined(ci01);
	}

	bool isstripterminated = true;
	for ( int cidx=0; cidx<=nrmyblocks; cidx++ )
	{
	    const int ci12 = ci11 + spacing_[res];
	    const int ci22 = ci21 + spacing_[res];
	    
	    nbdef[m12] = cidx!=nrmyblocks ? coords_->isDefined(ci12) : false;
	    nbdef[m22] = (cidx==nrmyblocks || ridx==nrmyblocks) ? 
		false : coords_->isDefined(ci22);
	    
	    int ci02 = ci12 - spacing_[res]*mNrCoordsPerTileSide;
	    nbdef[m02] = ridx ? coords_->isDefined(ci02) : false;

	    const int defsum = nbdef[m11]+nbdef[m12]+nbdef[m21]+nbdef[m22];
	    if ( defsum<3 ) 
	    {
		mTerminateStrip;
		if ( ridx<nrmyblocks && cidx<nrmyblocks && nbdef[m11] )
		{
		    const bool con12 = nbdef[m12] &&
			(!ridx || (!nbdef[m01] && !nbdef[m02]) );
		    const bool con21 = nbdef[m21] &&
			(!cidx || (!nbdef[m10] && !nbdef[m20]) );

		    if ( con12 || con21 )
    		    {
			const int lastci = lineci.size()
			    ? lineci[lineci.size()-1] : -1;

			if ( lastci!=ci11 )
			{
			    if ( lastci!=-1 )
				mAddIndex( -1, mLine );

			    mAddIndex( ci11, mLine );
			}

			mAddIndex( con12 ? ci12 : ci21, mLine );

		    }
		}
		else if ( nbdef[m11] && !nbdef[m10] && !nbdef[m12] && 
			 !nbdef[m01] && !nbdef[m21] )
		{
		    mAddIndex( ci11, mPoint );
		} 
	    }
	    else if ( defsum==3 )
	    {
		mTerminateStrip;
		if ( !nbdef[m11] )
		    mAddInitialTriangle( ci12, ci21, ci22 )
		else if ( !nbdef[m21] )
		    mAddInitialTriangle( ci11, ci22, ci12 )
		else if ( !nbdef[m12] )
		    mAddInitialTriangle( ci11, ci21, ci22 )
		else
		    mAddInitialTriangle( ci11, ci21, ci12 )
		mTerminateStrip;
	    }
	    else
	    {
		const float diff0 = coords_->getPos(ci11,true).z-
				    coords_->getPos(ci22,true).z;
		const float diff1 = coords_->getPos(ci12,true).z-
				    coords_->getPos(ci21,true).z;

		const bool do11to22 = fabs(diff0) < fabs(diff1);
		if ( do11to22 )
		{
		    mTerminateStrip;
		    mAddInitialTriangle( ci21, ci22, ci11 );
		    mAddIndex( ci12, mStrip );
		    mTerminateStrip;
		}
		else
		{
		    if ( isstripterminated )
		    {
			mAddInitialTriangle( ci11, ci21, ci12 );
			mAddIndex( ci22, mStrip );
		    }
		    else
		    {
			mAddIndex( ci12, mStrip );
			mAddIndex( ci22, mStrip );
		    }
		}
	    } 
	
	    nbdef[m00] = nbdef[m01]; nbdef[m01] = nbdef[m02];
    	    nbdef[m10] = nbdef[m11]; nbdef[m11] = nbdef[m12];
    	    nbdef[m20] = nbdef[m21]; nbdef[m21] = nbdef[m22];
    	    ci11 = ci12; ci21 = ci22;
	}

	mTerminateStrip;
    }

    tesselateWireframe( res, wireframeci, wireframeni );

    datalock_.lock();
    delete tesselationdata_.replace( res, td );
    datalock_.unLock();
    
    needsretesselation_[res] = false;
}


#define mSetTesselationDataImpl( nodes, field, var ) \
     nodes[res]->field.setValues( 0, tesselationdata_[res]->var.size(),        \
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
    for ( int idx=0; idx<mHorSectNrRes; idx++ )
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
    const int tilesz = mTileSideSize/spacing_[res] + 
			( mTileSideSize%spacing_[res] ? 1 : 0 );
    int lnidx = 0;
    for ( int idx=0; idx<=tilesz; idx++ )
    {
	const int rowstartidx = idx<tilesz
	    ? idx*spacing_[res]*mNrCoordsPerTileSide
	    : mNrCoordsPerTileSide*mTileSideSize; 
	for ( int idy=0; idy<=tilesz; idy++ )
	{
	    const int colshift = idy<tilesz ? idy*spacing_[res] : mTileSideSize;
	    const int ci0 = rowstartidx + colshift;
	    if ( !coords_->isDefined(ci0) ) 
		continue;

	    if ( idy<tilesz )
	    {
		const int nexthorci = idy==tilesz-1
		    ? rowstartidx+mTileSideSize 
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
}


void HorizonSectionTile::setPositions( const TypeSet<Coord3>& pos )
{
    coords_->setPositions( pos.arr(), pos.size(), 0 );

    nrdefinedpos_ = 0;
    for ( int idx=0; idx<pos.size(); idx++ )
    {
	if ( pos[idx].isDefined() )
	    nrdefinedpos_++;
    }

    datalock_.lock();
    for ( int idx=0; idx<mHorSectNrRes; idx++ )
    {
	needsretesselation_[idx] = true;
	allnormalsinvalid_[idx] = true;
	invalidnormals_[idx].erase();
    }
    datalock_.unLock();

    needsupdatebbox_ = true;
}
 
 
void HorizonSectionTile::setNeighbor( char nbidx, HorizonSectionTile* nb )
{
    if ( (nbidx==5 || nbidx==7 || nbidx==8 ) && neighbors_[nbidx]!=nb )
	glueneedsretesselation_ = true;

    neighbors_[nbidx] = nb;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    if ( row>=0 && row<=mTileLastIdx && col>=0 && col<=mTileLastIdx )
    {
	const int posidx = row*mNrCoordsPerTileSide+col;
	const bool olddefined = coords_->isDefined(posidx);
	const bool newdefined = pos.isDefined();
	const char oldnewdefined = ( olddefined ) + ( newdefined );
	coords_->setPos( posidx, pos );

	if ( !oldnewdefined ) 
	    return;
	else if ( oldnewdefined==1 )
	{
	    nrdefinedpos_ += (newdefined ? 1 : -1);
	    for ( int res=0; res<mHorSectNrRes; res++ )
	    {
		if ( !needsretesselation_[res] && !(row%spacing_[res]) && 
						  !(col%spacing_[res]) )
			needsretesselation_[res] = true;
	    }
	   
	    if ( !glueneedsretesselation_ ) 
    		glueneedsretesselation_ = true;
	}
	    
	if ( !needsupdatebbox_ ) needsupdatebbox_ = true;
    }

    setInvalidNormals( row, col );
}


void HorizonSectionTile::setInvalidNormals( int row, int col )
{ 
    for ( int res=0; res<mHorSectNrRes; res++ )
    {
	if ( allnormalsinvalid_[res] )
	    continue;

	int rowstart = row-spacing_[res];
	if ( rowstart>mTileSideSize ) continue;
	if ( rowstart<0 ) rowstart = 0;

	int rowstop = row+spacing_[res];
	if ( rowstop<0 ) continue;
	if ( rowstop>mTileSideSize ) rowstop = mTileLastIdx;

	datalock_.lock();
	for ( int rowidx=rowstart; rowidx<=rowstop; rowidx++ )
	{
	    if ( (rowidx%spacing_[res]) && (rowidx!=mTileLastIdx) ) continue;
	    const int nmrow = rowidx==mTileLastIdx ? normalsidesize_[res]-1 
						   : rowidx/spacing_[res];

	    int colstart = col-spacing_[res];
	    if ( colstart>mTileSideSize ) continue;
	    if ( colstart<0 ) colstart = 0;

	    int colstop = col+spacing_[res];
	    if ( colstop<0 ) continue;
	    if ( colstop>mTileSideSize ) colstop = mTileLastIdx;

	    int colstartni = normalstartidx_[res]+nmrow*normalsidesize_[res];
	    for ( int colidx=colstart; colidx<=colstop; colidx++ )
	    {
		if ( (colidx%spacing_[res]) && (colidx!=mTileLastIdx) )
		    continue;
		const int nmcol = colidx==mTileLastIdx ? normalsidesize_[res]-1
						       : colidx/spacing_[res];
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


#define mAddGlueIndices( i0, i1, i2 ) \
{ \
    const bool df0 = coords_->isDefined(i0); \
    const bool df1 = coords_->isDefined(i1); \
    const bool df2 = coords_->isDefined(i2); \
    const int dfsum = df0 + df1 + df2; \
    if ( df0 ) mAddIndex( i0, dfsum ) \
    if ( df1 ) mAddIndex( i1, dfsum ) \
    if ( df2 ) mAddIndex( i2, dfsum ) \
    if ( dfsum>1 ) mAddIndex( -1, dfsum ) \
}


void HorizonSectionTile::tesselateGlue()
{
    const int res = getActualResolution();
    TypeSet<int> pointci, lineci, stripci;
    TypeSet<int> pointni, lineni, stripni;
    
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
		if ( nb==5 ) mAddGlueIndices( i0, i2, i1 );

		skipped++;

		if ( (skipped%nrconns) && (idx-highstartidx+1!=nrconns/2) ) 
		    continue; 

		skipped = 0; 
		if ( lowresidx+1<(highres ? nbblocks+1 : edgeindices.size()) ) 
		{ 
		    lowresidx++; 
		    i0 = highres ? edgeindices[idx+1] :edgeindices[lowresidx-1];
		    i1 = highres ? nbindices[lowresidx-1] : nbindices[idx+1]; 
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

