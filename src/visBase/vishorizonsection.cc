/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: vishorizonsection.cc,v 1.18 2009-04-27 04:36:07 cvsranojay Exp $";

#include "vishorizonsection.h"

#include "binidsurface.h"
#include "binidvalset.h"
#include "coltabmapper.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "task.h"
#include "viscolortab.h"
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
#include <Inventor/elements/SoViewportRegionElement.h>
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
#include <Inventor/nodes/SoTextureCoordinate2.h>

mCreateFactoryEntry( visBase::HorizonSection );

namespace visBase
{

ArrPtrMan<SbVec2f> HorizonSection::texturecoordptr_ = 0;

HorizonSection::HorizonSection() 
    : VisualObjectImpl( false )
    , callbacker_( new SoCallback )  
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , geometry_( 0 )
    , channels_( TextureChannels::create() )		   
    , channel2rgba_( ColTabTextureChannel2RGBA::create() ) 
    , wireframelines_( visBase::IndexedPolyLine::create() )		   
    , tiles_( 0, 0 )					  
    , sinanglexinl_( sin(SI().computeAngleXInl()) )
    , cosanglexinl_( cos(SI().computeAngleXInl()) )
    , rowdistance_( 0 )
    , coldistance_( 0 )
    , texturecrds_( new SoTextureCoordinate2 )
    , desiredresolution_( -1 )
{
    cache_.allowNull( true );
    
    callbacker_->ref();
    callbacker_->setCallback( updateResolution, this );
    addChild( callbacker_ );

    channel2rgba_->ref();
    channel2rgba_->allowShading( true );

    channels_->ref();
    addChild( channels_->getInventorNode() );
    channels_->setChannels2RGBA( channel2rgba_ );
    if ( channels_->nrChannels()<1 )
	addChannel();
    else 
	cache_ += 0;

    addChild( texturecrds_ );
    
    wireframelines_->ref();
    wireframelines_->setMaterial( Material::create() );
    addChild( wireframelines_->getInventorNode() );

    const int totalnrpts = mHorSectSideSize*mHorSectSideSize;
    if ( !texturecoordptr_ )
    {
	mTryAlloc( texturecoordptr_, SbVec2f[totalnrpts] );
	int idx = 0;
	for ( int irow=0; irow<mHorSectSideSize; irow++ )
	{
	    for ( int icol=0; icol<mHorSectSideSize; icol++ )
	    {
		texturecoordptr_[idx] = SbVec2f((icol+0.5)/mHorSectSideSize,
						(irow+0.5)/mHorSectSideSize);
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

    callbacker_->unref();
    if ( transformation_ ) transformation_->unRef();
    if ( zaxistransform_ ) zaxistransform_->unRef();
    
    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	removeChild( tileptrs[idx]->getNodeRoot() );
	delete tileptrs[idx];
    }

    removeChild( wireframelines_->getInventorNode() );
    wireframelines_->unRef();
}


void HorizonSection::allowShading( bool yn )
{ channel2rgba_->allowShading( yn ); }


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

    const int nrrows = rrg.width()+1;
    const int nrcols = crg.width()+1;

    channels_->setSize( 1, nrrows, nrcols );
   
    const int nrversions = channels_->nrVersions( channel );

    ObjectSet<float> versiondata;
    versiondata.allowNull( true );
    const int nrcells = nrrows*nrcols;

    MemSetter<float> memsetter;
    memsetter.setSize( nrcells );
    memsetter.setValue( mUdf(float) );

    const BinIDValueSet* data = getCache( channel );

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
				    OD::TakeOverPtr );
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
					   const ColTab::MapperSetup& mapper )
{
    if ( channel>=0 ) channels_->setColTabMapperSetup( channel, mapper );
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
    return channel<0 ? 0 : cache_[channel]; 
}


void HorizonSection::setSurface( Geometry::BinIDSurface* ng )
{
    if ( ng && geometry_!=ng ) 
    {
    	geometry_ = ng;
    	updateGeometry();
    }
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
    const int nrrowtiles = nrBlocks(rrg.width()+1,mHorSectSideSize,1);
    const int nrcoltiles = nrBlocks(crg.width()+1,mHorSectSideSize,1);

    rowdistance_ = rrg.step*SI().inlDistance();
    coldistance_ = crg.step*SI().crlDistance();

    if ( !tiles_.setSize(nrrowtiles,nrcoltiles) )
	return;

    for ( int row=0; row<nrrowtiles; row++ )
    {
	const int startrow = row*(mHorSectSideSize-1) + rrg.start;
	for ( int col=0; col<nrcoltiles; col++ )
	{
	    const int startcol = col*(mHorSectSideSize-1) + crg.start;
	    HorizonSectionTile* tile = new HorizonSectionTile();
	    tile->setTextureSize( mHorSectSideSize, mHorSectSideSize );
	    tile->setTextureOrigion( startrow-rrg.start, startcol-crg.start );
	    setTileNormals( *tile, startrow, startcol );

	    for ( int r=0; r<mHorSectSideSize; r++ )
	    {
		for ( int c=0; c<mHorSectSideSize; c++ )
		{
		    if ( startrow+r<=rrg.stop && startcol+c<=crg.stop )
    			tile->setPos( r, c, geometry_->getKnot(
    				    RowCol(startrow+r,startcol+c), false) );
		    else
			tile->setPos( r, c, Coord3::udf() );
		}
	    }

	    tiles_.set( row, col, tile );
	    addChild( tile->getNodeRoot() );
	}
    }

    for ( int row=0; row<nrrowtiles; row++ )
    {
	for ( int col=0; col<nrcoltiles; col++ )
	{
	    HorizonSectionTile* tile = tiles_.get(row,col);
	    if ( !tile ) continue;

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
		    else if ( j==1 ) 
			pos = i==-1 ? 2 : (!i ? 5 : 8);

		    if ( (!r && !c) || (r<0 || r>=nrrowtiles) ||
				       (c<0 || c>=nrcoltiles) ) 
			continue;

		    tile->setNeighbor( pos, tiles_.get(r,c) );
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
	if ( !tileptrs[idx] ) continue;
	tileptrs[idx]->updateResolution( state );
	tileptrs[idx]->tesselateActualResolution();
    }

    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->updateGlue();

    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->resetResolutionChangeFlag();
}


char HorizonSection::currentResolution() const
{
    return desiredresolution_;
}


void HorizonSection::setResolution( char res )
{
    if ( desiredresolution_==res )
	return;

    desiredresolution_ = res;

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( tileptrs[idx] ) tileptrs[idx]->setResolution( res );
    }
}


void HorizonSection::updateWireFrame( char res )
{
    return;
    const int spacing = (int)pow( 2.0, res );
    int coordidx = 0, ciidx = 0;
    visBase::Coordinates* coords = wireframelines_->getCoordinates();
    
    const StepInterval<int>& rrg = geometry_->rowRange();
    const StepInterval<int>& crg = geometry_->colRange();
    const int nrrowtiles = nrBlocks(rrg.width()+1,mHorSectSideSize,1);
    const int nrcoltiles = nrBlocks(crg.width()+1,mHorSectSideSize,1);
    
    for ( int rowidx=0; rowidx<nrrowtiles; rowidx++ )
    {
	const int startrow = rowidx*(mHorSectSideSize-1) + rrg.start;
	for ( int colidx=0; colidx<nrcoltiles; colidx++ )
	{
	    const int startcol = colidx*(mHorSectSideSize-1) + crg.start;
	    const int tilesz = (mHorSectSideSize-1)/spacing + 
			       ( (mHorSectSideSize-1)%spacing ? 1 : 0 );
	    for ( int idx=0; idx<tilesz; idx++ )
	    {
		const int row = startrow+mMIN(idx*spacing,mHorSectSideSize-1);
		const int col = startcol+mMIN(idx*spacing,mHorSectSideSize-1);

		RowCol rc(row,-1);
		RowCol cr(-1,col);
		for ( int idy=0; idy<tilesz; idy++ )
		{
		    rc.col = startcol+mMIN(idy*spacing,mHorSectSideSize-1);
		    Coord3 p0 = geometry_->getKnot( rc, false );
		    rc.col = mMIN( rc.col+spacing, geometry_->colRange().stop );
		    Coord3 p1 = geometry_->getKnot( rc, false );

		    if ( row<=rrg.stop && p0.isDefined() && p1.isDefined() )
		    {
			coords->setPos( coordidx, p0 );
			wireframelines_->setCoordIndex( ciidx++, coordidx++ );

			coords->setPos( coordidx, p1 );
			wireframelines_->setCoordIndex( ciidx++, coordidx++ );
			wireframelines_->setCoordIndex( ciidx++, -1 );
		    }
		    
		    cr.row = startrow + mMIN(idy*spacing, mHorSectSideSize-1);
		    p0 = geometry_->getKnot( cr, false );

		    cr.row = mMIN( cr.row+spacing, geometry_->rowRange().stop );
		    p1 = geometry_->getKnot( cr, false );

		    if ( col<=crg.stop && p0.isDefined() && p1.isDefined() )
		    {
			coords->setPos( coordidx, p0 );
			wireframelines_->setCoordIndex( ciidx++, coordidx++ );

			coords->setPos( coordidx, p1 );
			wireframelines_->setCoordIndex( ciidx++, coordidx++ );
			wireframelines_->setCoordIndex( ciidx++, -1 );
		    }
		}
	    }
	}
    }

    wireframelines_->removeCoordIndexAfter( ciidx-1 );
}


void HorizonSection::useWireframe( bool yn )
{
    wireframelines_->turnOn( yn );
    if ( yn && !wireframelines_->getCoordinates()->size() )
	updateWireFrame( currentResolution() );
}


bool HorizonSection::usesWireframe() const
{
    return wireframelines_->isOn();
}


void HorizonSection::setTileNormals( HorizonSectionTile& tile, 
				     int startrow, int startcol )
{
    int index = 0;
    for ( int res = 0; res<mHorSectNrRes; res++ )
    {
	const int spacing = (int)pow( 2.0, res );
	for ( int row=0; row<mHorSectSideSize; row++ )
	{
	    if ( row%spacing && row<mHorSectSideSize-1 ) continue;

    	    for ( int col=0; col<mHorSectSideSize; col++ )
	    {
		if ( col%spacing && col<mHorSectSideSize-1 ) continue;

		Coord3 normal;
		computeNormal( row+startrow, startcol+col, spacing, normal );
    		tile.setNormal( index, normal );
		index++;
	    }
	}
    }
}


void HorizonSection::computeNormal( int row, int col, int spacing, Coord3& nm )
{
    TypeSet<float> posarray, zarray;
    for ( int idx=-spacing; idx<=spacing; idx++ )
    {
	const Coord3 pos = geometry_->getKnot( RowCol(row+idx,col), false );
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
	const Coord3 pos = geometry_->getKnot( RowCol(row,col+idx), false );
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
    , desiredresolution_( -1 )
    , resolutionhaschanged_( false )
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
	const int sz = (mHorSectSideSize-1)/spacing[res]+(res==0 ? 1 : 2);
	normalsum += sz*sz;
    }
}


HorizonSectionTile::~HorizonSectionTile()
{
    coords_->unRef();
    normals_->unref();
    root_->unref();
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
    const int row = crdidx/mHorSectSideSize;
    const int col = crdidx%mHorSectSideSize;
    if ( row>mHorSectSideSize-1 )
    return -1;

    int useres = res;
    if ( row==mHorSectSideSize-1 && col==mHorSectSideSize-1 && neighbors_[8] )
	useres = neighbors_[8]->getActualResolution();
    else if ( row==mHorSectSideSize-1 && neighbors_[7] )
	useres = neighbors_[7]->getActualResolution();
    else if ( col==mHorSectSideSize-1 && neighbors_[5] )
	useres = neighbors_[5]->getActualResolution();
    
    if ( (!(row%spacing[useres]) || row==mHorSectSideSize-1) &&
         (!(col%spacing[useres]) || col==mHorSectSideSize-1) )
    {
	if ( row==mHorSectSideSize-1 && col==mHorSectSideSize-1 && 
	     !neighbors_[8] )
	    return useres<5 ? normalstartidx[useres+1]-1 : normalstartidx[5]+8;
	
    	const int nrsideknots = (mHorSectSideSize-1)/spacing[useres] +
	    ( useres==0 ? 1 : 2 );
	const int resrow = row/spacing[useres] +
	    ( useres>0 && row==mHorSectSideSize-1 ? 1 : 0 );
	const int rescol = col/spacing[useres] +
	    ( useres>0 && col==mHorSectSideSize-1 ? 1 : 0 );
	
	return normalstartidx[useres]+resrow*nrsideknots+rescol;
    }

    return -1;
}


void HorizonSectionTile::resetResolutionChangeFlag()
{ resolutionhaschanged_= false; }


void HorizonSectionTile::tesselateActualResolution()
{
    const int res = getActualResolution();
    if ( !needsretesselation_[res] )
	return;

    tesselateResolution( res );
}


void HorizonSectionTile::setResolution( int rs )
{
    desiredresolution_ = rs;
}


int HorizonSectionTile::getActualResolution() const
{
    return resswitch_->whichChild.getValue();
}


void HorizonSectionTile::updateResolution( SoState* st )
{
    int newres = desiredresolution_;
    if ( desiredresolution_==-1 && st )
	newres = getAutoResolution( st );

    setActualResolution( newres );
}


int HorizonSectionTile::getAutoResolution( SoState* state ) const
{
    SbViewportRegion vp;
    SoGetBoundingBoxAction action( vp );
    action.setViewportRegion(SoViewportRegionElement::get(state));
    action.apply( root_ );
    const SbBox3f bbox = action.getBoundingBox();
    if ( bbox.isEmpty() )
	mHorSectNrRes-1;
    
    const int32_t camerainfo = SoCameraInfoElement::get(state);
    if ( camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE) )
	return mHorSectNrRes-1;

    SbVec2s screensize;
    SoShape::getScreenSize( state, bbox, screensize );
    const float complexity = SbClamp(SoComplexityElement::get(state),0.0f,1.0f);
    const float wantednumcells = complexity*screensize[0]*screensize[1]/32;

    int nrcells = (mHorSectSideSize-1)*(mHorSectSideSize-1);
    int desiredres;
    for ( desiredres=mHorSectNrRes-1; desiredres>=0; desiredres-- )
    {
	const int nextnumcells = nrcells/4;
	if ( nextnumcells<wantednumcells )
	    break;

	nrcells = nextnumcells;
    }

    if ( desiredres<0 )
	return mHorSectNrRes-1;
    
    return desiredres;
}


void HorizonSectionTile::setActualResolution( int resolution )
{
    if ( resolution==-1 || resolution>=mHorSectNrRes ||
	 resolution==getActualResolution() ) 
	return;

    resswitch_->whichChild.setValue( resolution );
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
    obj->coordIndex.set1Value( objidx, ci ); \
    obj->textureCoordIndex.set1Value( objidx, ci ); \
    obj->normalIndex.set1Value( objidx, getNormalIdx(ci,res) ); \
    objidx++; \
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
    const int nrsideblocks = (mHorSectSideSize-1)/spacing[res] + 
			    ((mHorSectSideSize-1)%spacing[res] ? 0 : -1);
    int stripidx = 0, lnidx = 0, ptidx = 0;
    for ( int ridx=0; ridx<=nrsideblocks; ridx++ )
    {
	int ci11 = spacing[res]*ridx*mHorSectSideSize;
	int ci21 = ci11 + spacing[res]*mHorSectSideSize;

	bool nbdef[] = { false, false, false,		// 00   01     02
	    		 false, false, false,		// 10   11(me) 12
			 false, false, false };		// 20   21     22
	nbdef[m11] = coords_->getPos(ci11).isDefined();
	nbdef[m21] = ridx!=nrsideblocks ? coords_->getPos(ci21).isDefined()
					: false;
	if ( ridx )
	{
	    int ci01 = ci11 - spacing[res]*mHorSectSideSize;
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
	    
	    int ci02 = ci12 - spacing[res]*mHorSectSideSize;
	    nbdef[m02] = ridx ? coords_->getPos(ci02).isDefined() : false;

	    const int defsum = nbdef[m11]+nbdef[m12]+nbdef[m21]+nbdef[m22];
	    if ( defsum<3 ) 
	    {
		mTerminateStrip( triangles_[res] );
		if ( nbdef[m11] && ( nbdef[m12] || nbdef[m21]) )
		{
		    mAddIndex( ci11, lines_[res], lnidx );
		    mAddIndex( nbdef[m12] ? ci12 : ci21, lines_[res], lnidx );
		    mAddIndex( -1, lines_[res], lnidx );
		}
		else if ( nbdef[m11] && !nbdef[m10] && !nbdef[m12] && 
			 !nbdef[m01] && !nbdef[m21] )
		{
		    // mAddIndex( c11, points_[res], ptidx ) 
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

    if ( (nbidx==5 || nbidx==7 || nbidx==8 ) && neighbors_[nbidx]!=nb )
	glueneedsretesselation_ = true;

    neighbors_[nbidx] = nb;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    if ( row<0 || row>mHorSectSideSize-1 || 
	 col<0 || col>mHorSectSideSize-1 )
	return;

    const int posidx = row*mHorSectSideSize+col;

    const bool posdefined = pos.isDefined();
    const bool oldposdefined = coords_->isDefined( posidx );

    if ( (posdefined+oldposdefined) == 1 )
    {
	for ( int res=0; res<mHorSectNrRes; res++ )
	{
	    if ( !needsretesselation_[res] )
	    {
		if ( (!row % spacing[res]) && (!col % spacing[res]) )
		    needsretesselation_[res] = true;
	    }
	}

	glueneedsretesselation_ = true;
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
    }

    glueneedsretesselation_ = false;
}


#define mMakeConnection( nb ) \
{ \
    int nbres = -1; \
    if ( nb==5 && neighbors_[5] ) \
	nbres = neighbors_[5]->getActualResolution(); \
    else if ( nb==7 && neighbors_[7] ) \
	nbres = neighbors_[7]->getActualResolution(); \
    const int nbblocks = nbres==-1 ? nrsideblocks \
    				   : (mHorSectSideSize-1)/spacing[nbres] + \
    			((mHorSectSideSize-1)%spacing[nbres] ? 0 : -1); \
    TypeSet<int> nbindices; \
    for ( int idx=0; idx<=nbblocks; idx++ ) \
    { \
	const int usespacing = nbres==-1 ? spacing[res] : spacing[nbres]; \
	if ( nb==7 ) \
	    nbindices += mHorSectSideSize*(mHorSectSideSize-1)+idx*usespacing;\
	else if ( nb==5 ) \
	    nbindices += (1+idx*usespacing)*mHorSectSideSize-1; \
    } \
    const bool finer = nrsideblocks >= nbblocks; \
    const int nrconns = nbres==-1 ? 1 : (int)pow(2.0, abs(nbres-res)); \
    const int startidx = finer ? (nb==5 ? 0 : nrsideblocks+1) : 0; \
    const int stopidx = finer ? (nb==5 ? nrsideblocks : 2*nrsideblocks+1) \
    			      : nbblocks; \
    int coarseidx = finer ? 0 : (nb==5 ? 0 : nrsideblocks+1); \
    int skipped = 0; \
    for ( int idx=startidx; idx<stopidx; idx++ ) \
    { \
	int i0 = finer ? edgeindices[idx] : edgeindices[coarseidx]; \
	int i1 = finer ? nbindices[coarseidx] : nbindices[idx]; \
	int i2 = finer ? edgeindices[idx+1] : nbindices[idx+1]; \
	mAddGlueIndices( i0, i1, i2 ) \
	skipped++; \
	if ( skipped%nrconns && idx-startidx+1!=nrconns/2 ) continue; \
	skipped = 0; \
	if ( coarseidx+1<(finer ? nbblocks+1 : edgeindices.size()) ) \
	{ \
	    coarseidx++; \
	    i0 = finer ? edgeindices[idx+1] : edgeindices[coarseidx-1]; \
	    i1 = finer ? nbindices[coarseidx-1] : nbindices[idx+1]; \
	    i2 = finer ? nbindices[coarseidx] : edgeindices[coarseidx]; \
	    mAddGlueIndices( i0, i1, i2 ) \
	} \
    } \
}

#define mAddGlueIndices( i0, i1, i2 ) \
{ \
    const bool df0 = coords_->getPos(i0).isDefined(); \
    const bool df1 = coords_->getPos(i1).isDefined(); \
    const bool df2 = coords_->getPos(i2).isDefined(); \
    const int dfsum = df0 + df1 + df2; \
    if ( dfsum==1 ) \
    { \
	int ptidx = df0 ? i0 : ( df1 ? i1 : i2 ); \
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
//mAddIndex( ptidx, gluepoints_, ptidx ); \


void HorizonSectionTile::tesselateGlue()
{
    const int res = getActualResolution();
    if ( res==-1 ) return;
   
    const int nrsideblocks = (mHorSectSideSize-1)/spacing[res] + 
			    ((mHorSectSideSize-1)%spacing[res] ? 0 : -1);
    
    TypeSet<int> edgeindices; //Get all my own edge knot indices 
    for ( int ridx=0; ridx<=nrsideblocks; ridx++ )
	edgeindices += spacing[res]*(nrsideblocks+ridx*mHorSectSideSize);

    for ( int cidx=0; cidx<=nrsideblocks; cidx++ )
	edgeindices += spacing[res]*(mHorSectSideSize*nrsideblocks+cidx);

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


void HorizonSectionTile::setTextureSize( int rowsz, int colsz )
{ texture_->size.setValue( 1, rowsz, colsz ); }


void HorizonSectionTile::setTextureOrigion( int row, int col )
{ texture_->origin.setValue( 0, row, col ); }


}; // namespace visBase

