/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: vishorizonsection.cc,v 1.7 2009-04-07 06:10:46 cvsraman Exp $";

#include "vishorizonsection.h"

#include "binidsurface.h"
#include "binidvalset.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vispolyline.h"
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
#include <Inventor/nodes/SoSwitch.h>

mCreateFactoryEntry( visBase::HorizonSection );

namespace visBase
{
HorizonSection::HorizonSection() 
    : VisualObjectImpl( false )
    , callbacker_( new SoCallback )  
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , geometry_( 0 )
    , wireframelines_( visBase::PolyLine::create() )		   
    , tiles_( 0, 0 )					   
{
    callbacker_->ref();
    callbacker_->setCallback( updateResolution, this );
    addChild( callbacker_ );
	
    wireframelines_->ref();
    wireframelines_->setMaterial( Material::create() );
    addChild( wireframelines_->getInventorNode() );
}


HorizonSection::~HorizonSection()
{
    callbacker_->unref();
    if ( transformation_ ) transformation_->unRef();
    if ( zaxistransform_ ) zaxistransform_->unRef();
    
    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	removeChild( tileptrs[idx]->root_ );
	delete tileptrs[idx];
    }

    if ( wireframelines_ )
    {
	removeChild( wireframelines_->getInventorNode() );
	wireframelines_->unRef();
    }
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
		tileptrs[idx]->coords_->setDisplayTransformation( nt );
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
	removeChild( tileptrs[idx]->root_ );
	delete tileptrs[idx];
    }

    const StepInterval<int>& rrg = geometry_->rowRange();
    const StepInterval<int>& crg = geometry_->colRange();
    const int nrrowtiles = nrBlocks(rrg.width(),mHorizonSectionSideSize,1);
    const int nrcoltiles = nrBlocks(crg.width(),mHorizonSectionSideSize,1);

    if ( !tiles_.setSize(nrrowtiles,nrcoltiles) )
	return;
/*
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
*/

    for ( int row=0; row<nrrowtiles; row++ )
    {
	const int startrow = row*mHorizonSectionSideSize + rrg.start;
	for ( int col=0; col<nrcoltiles; col++ )
	{
	    const int startcol = col*mHorizonSectionSideSize + crg.start;
	    HorizonSectionTile* tile = new HorizonSectionTile();
	    tile->setTextureComposerSize( mHorizonSectionSideSize+2,
		    			  mHorizonSectionSideSize+2 );
	    tile->setTextureComposerOrig( startrow, startcol );

	    for ( int r=0; r<=mHorizonSectionSideSize; r++ )
	    {
		for ( int c=0; c<=mHorizonSectionSideSize; c++ )
		{
		    if ( startrow+r<=rrg.stop && startcol+c<=crg.stop )
    			tile->setPos( r, c, geometry_->getKnot(
    				    RowCol(startrow+r,startcol+c), false) );
		    else
			tile->setPos( r, c, Coord3::udf() );
		}
	    }

	    for ( int idx=0; idx<9; idx++ )
		tile->setNeighbor( idx, 0 );

	    tile->setResolution( 5 );
	    tiles_.set( row, col, tile );
	    addChild( tile->root_ );
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
    
    for ( int row=0; row<nrrowtiles; row++ )
    {
	for ( int col=0; col<nrcoltiles; col++ )
	{
	    tiles_.get(row,col)->updateGlue();
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
{ return mHorizonSectionNrRes; }


char HorizonSection::currentResolution() const
{
    return tiles_.info().getTotalSz() ? tiles_.get(0,0)->getActualResolution()
				      : -1;
}


void HorizonSection::setResolution( char res )
{
    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( tileptrs[idx] ) 
	    tileptrs[idx]->setResolution( res );
    }
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( tileptrs[idx] ) 
	    tileptrs[idx]->updateGlue();
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

    root_->addChild( coords_->getInventorNode() );
    root_->addChild( texture_ );
    root_->addChild( resswitch_ );
    root_->addChild( gluetriangles_ );
    root_->addChild( gluelines_ );
    //root_->addChild( gluepoints_ );

    for ( int idx=0; idx<mHorizonSectionNrRes; idx++ )
    {
	needsretesselation_[idx] = true;
	resolutions_[idx] = new SoGroup;
	resswitch_->addChild( resolutions_[idx] );

	triangles_[idx] = new SoIndexedTriangleStripSet;
	resolutions_[idx]->addChild( triangles_[idx] );

	lines_[idx] = new SoIndexedLineSet;
	resolutions_[idx]->addChild( lines_[idx] );

	//points_[idx] = new SoIndexedPointSet;
	//resolutions_[idx]->addChild( points_[idx] );
    }
}


HorizonSectionTile::~HorizonSectionTile()
{
    coords_->unRef();
    root_->unref();
}


void HorizonSectionTile::setResolution( int rs )
{
    if ( getActualResolution()==rs )
	return;

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
    if ( curres==-1 || curres>=mHorizonSectionNrRes )
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
	return mHorizonSectionNrRes-1;

    SbVec2s screensize;
    SoShape::getScreenSize( state, bbox, screensize );
    const float complexity = SbClamp(SoComplexityElement::get(state),0.0f,1.0f);
    const float wantednumcells = complexity*screensize[0]*screensize[1]/32;

    int nrcells = mHorizonSectionSideSize*mHorizonSectionSideSize;
    int desiredres = mHorizonSectionNrRes-1;
    for ( desiredres=mHorizonSectionNrRes-1; desiredres>=0; desiredres-- )
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
    if ( resolution==-1 || resolution>=mHorizonSectionNrRes || 
	 getActualResolution()==resolution )
	return;

    resswitch_->whichChild.setValue( resolution );
    needsretesselation_[resolution] = true;
    tesselateResolution( resolution );
}


void HorizonSectionTile::tesselateResolution( int res )
{
    if ( res==-1 || !needsretesselation_[res] ) return;

    const int resstep = (int)pow( 2, res ); //Nr of blocks between two points.
    const int nrsideblocks = mHorizonSectionSideSize/resstep + 
	(mHorizonSectionSideSize%resstep ? 0 : -1);

    triangles_[res]->textureCoordIndex.deleteValues( 0, -1 );
    triangles_[res]->coordIndex.deleteValues( 0, -1 );
    lines_[res]->textureCoordIndex.deleteValues( 0, -1 );
    lines_[res]->coordIndex.deleteValues( 0, -1 );
    //points_[res]->textureCoordIndex.deleteValues( 0, -1 );
    //points_[res]->coordIndex.deleteValues( 0, -1 );
    
    int chainlength;
    int stripidx = 0;
    int lineidx = 0;
    int pointidx = 0;
     
    for ( int ridx=0; ridx<=nrsideblocks; ridx++ )
    {
	chainlength = 0; 
    	TypeSet<int> stripindices;
    	TypeSet<int> stripsizes;
	for ( int cidx=0; cidx<=nrsideblocks; cidx++ )
	{
	    const int crdidx0 = resstep*(cidx+ridx*(1+mHorizonSectionSideSize));
	    const bool defpos0 = coords_->getPos(crdidx0).isDefined();
	    const int crdidx1 = crdidx0+(1+mHorizonSectionSideSize)*resstep;

	    if ( defpos0 )
	    {
		if ( cidx && ridx<nrsideblocks && 
			!coords_->getPos(crdidx1-resstep).isDefined() && 
			coords_->getPos(crdidx0-resstep).isDefined() )
		{
		    stripindices += crdidx0-resstep;
		    chainlength = 1;
		}

		stripindices += crdidx0;
		chainlength++;

	    }
	    else if ( chainlength ) 
	    {
		stripsizes += chainlength;
		chainlength = 0;
	    }
	   
	    if ( ridx==nrsideblocks ) continue;

	    if ( coords_->getPos(crdidx1).isDefined() )
	    {
		if ( cidx && !defpos0 && 
			coords_->getPos(crdidx1-resstep).isDefined() )
		{
		    stripindices += crdidx1-resstep;
		    chainlength = 1;
		}

		stripindices += crdidx1;
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

	setCoordIndices( *triangles_[res], *lines_[res], *points_[res],
		stripidx, lineidx, pointidx, stripsizes, stripindices );
    }
    
    triangles_[res]->textureCoordIndex.deleteValues( stripidx, -1 );
    triangles_[res]->coordIndex.deleteValues( stripidx, -1 ); 
    lines_[res]->textureCoordIndex.deleteValues( lineidx, -1 );
    lines_[res]->coordIndex.deleteValues( lineidx, -1 );
    //points_[res]->textureCoordIndex.set1Value( pointidx, -1 );
   // points_[res]->coordIndex.set1Value( pointidx, -1 );
    
    needsretesselation_[res] = false;
    glueneedsretesselation_ = true;
}


void HorizonSectionTile::setCoordIndices( SoIndexedTriangleStripSet& triangles,
	SoIndexedLineSet& lines, SoIndexedPointSet& points, 
	int& stripidx, int& lineidx, int& pointidx, 
	const TypeSet<int>& stripsizes, const TypeSet<int>& stripindices )
{
    int sum = 0;
    for ( int idx=0; idx<stripsizes.size(); idx++ )
    {
	if ( stripsizes[idx]>2 )
	{
	    for ( int s=0; s<stripsizes[idx]; s++ )
	    {
		triangles.textureCoordIndex.set1Value( stripidx, stripidx ); 
		triangles.coordIndex.set1Value( stripidx,stripindices[sum+s] ); 
		stripidx++;
	    }
	    
	    triangles.textureCoordIndex.set1Value( stripidx, -1 ); 
	    triangles.coordIndex.set1Value( stripidx, -1 ); 
	    stripidx++;
	}	
	else if ( stripsizes[idx]==2 )
	{
	    /*
	    lines.textureCoordIndex.set1Value( lineidx, lineidx );
	    lines.coordIndex.set1Value( lineidx, stripindices[sum] );
	    lineidx++;

	    lines.textureCoordIndex.set1Value( lineidx, lineidx );
	    lines.coordIndex.set1Value( lineidx, stripindices[sum+1] );
	    lineidx++;
	
	    lines.textureCoordIndex.set1Value( lineidx, -1 );
	    lines.coordIndex.set1Value( lineidx, -1 );
	    lineidx++;
	    */
	}
	else
	{
	    /*
	    points.textureCoordIndex.set1Value( pointidx, pointidx );
	    points.coordIndex.set1Value( pointidx, stripindices[sum] );
	    pointidx++;
	    */
	}
	
	sum += stripsizes[idx];
    }
}


void HorizonSectionTile::setNeighbor( int nbidx, HorizonSectionTile* nb )
{
    if ( nbidx<0 || nbidx>8 || nbidx==4 )
	return;

    neighbors_[nbidx] = nb;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    if ( row<0 || row>mHorizonSectionSideSize || 
	 col<0 || col>mHorizonSectionSideSize )
	return;

    const int res = getActualResolution();  
    if ( !needsretesselation_[res] && res>-1 && res<mHorizonSectionNrRes )
    {
	const int resstep = (int)pow( 2, res );
	if ( (row % resstep) && (col % resstep) )
    	    needsretesselation_[res] = true;
    }

    coords_->setPos( row*(mHorizonSectionSideSize+1)+col, pos );
}


void HorizonSectionTile::updateGlue()
{
    //if ( glueneedsretesselation_ )
	tesselateGlue();
}


#define mMakeDefaultConnection( startidx, stopidx ) \
{ \
    for ( int idx=startidx; idx<=stopidx; idx++ ) \
    { \
	const bool def0 = coords_->getPos(edgeindices[idx]).isDefined(); \
	const int shift = \
	(idx<=nrsideblocks ? gluesz : (1+mHorizonSectionSideSize)*gluesz); \
	if ( def0 ) \
	{ \
	    if ( ( (idx<=nrsideblocks && idx) || idx>nrsideblocks+1 ) && \
		    !coords_->getPos(edgeindices[idx-1]+shift).isDefined() && \
		    coords_->getPos(edgeindices[idx-1]).isDefined() ) \
	    { \
		stripindices += edgeindices[idx-1]; \
		chainlength = 1; \
	    } \
	    stripindices += edgeindices[idx]; \
	    chainlength++; \
	} \
	else if ( chainlength )  \
	{ \
	    stripsizes += chainlength; \
	    chainlength = 0; \
	} \
	if ( coords_->getPos(shift+edgeindices[idx]).isDefined() ) \
	{ \
	    if ( !def0 && ((idx<=nrsideblocks && idx) || idx>nrsideblocks+1) \
		    && coords_->getPos(edgeindices[idx-1]+shift).isDefined() )\
	    { \
		stripindices += edgeindices[idx-1]+shift; \
		chainlength = 1; \
	    } \
	    stripindices += edgeindices[idx]+shift; \
	    chainlength++; \
	} \
	else if ( chainlength ) \
	{ \
	    stripsizes += chainlength; \
	    chainlength = 0; \
	} \
	if ( idx==nrsideblocks && chainlength && idx>startidx ) \
	{ \
	    stripsizes += chainlength; \
	    chainlength = 0; \
	} \
    } \
    if ( chainlength ) \
    { \
	stripsizes += chainlength; \
	chainlength = 0; \
    } \
}


#define mSetConnection( nbnr ) \
{ \
    const int startidx = nbnr==5 ? 0 : nrsideblocks+1; \
    const int diff = nrsideblocks - nrsideblocks##nbnr; \
    if ( firstdfidx##nbnr==-1 ) \
    { \
	mMakeDefaultConnection( startidx, startidx+nrsideblocks ); \
    } \
    else \
    { \
	int nrconnected = 0; \
	int lastdefidx = -1; \
	for ( int idx=startidx; idx<=startidx+nrsideblocks; idx++ ) \
	{ \
	    if ( coords_->getPos(edgeindices[idx]).isDefined() ) \
	    { \
		stripindices += edgeindices[idx]; \
		nrconnected++; \
		chainlength++; \
		lastdefidx = edgeindices[idx]; \
		if ( nrconnected%2 ) \
		{ \
		    stripindices += nbindices##nbnr[firstdfidx##nbnr]; \
		    chainlength++; \
		} \
	    } \
	    else if ( chainlength )  \
	    { \
		stripsizes += chainlength; \
		chainlength = 0; \
		nrconnected = 0; \
	    } \
	} \
	if ( chainlength ) \
	{ \
	    stripsizes += chainlength; \
	    chainlength = 0; \
	} \
	nrconnected = 0; \
	for ( int idx=firstdfidx##nbnr; idx<nbindices##nbnr.size(); idx++ ) \
	{ \
	    if ( coords_->getPos(nbindices##nbnr[idx]).isDefined() ) \
	    { \
		stripindices += nbindices##nbnr[idx]; \
		nrconnected++; \
		chainlength++; \
		if ( nrconnected%2 ) \
		{ \
		    stripindices += lastdefidx; \
		    chainlength++; \
		} \
	    } \
	    else if ( chainlength )  \
	    { \
		stripsizes += chainlength; \
		chainlength = 0; \
		nrconnected = 0; \
	    } \
	} \
    } \
}


#define mMakeConnection( nb ) \
{ \
    const bool finer = nrsideblocks > nrsideblocks##nb; \
    const int nrconns = finer ? (nrsideblocks+1)/(nrsideblocks##nb+1) \
    			      : (nrsideblocks##nb+1)/(nrsideblocks+1); \
    const int startidx = finer ? (nb==5 ? 0 : nrsideblocks+1) : 0; \
    const int stopidx = finer ? (nb==5 ? nrsideblocks : 2*nrsideblocks+1) \
    			      : nrsideblocks##nb; \
    const int coarsesz = finer ? nbindices##nb.size() : edgeindices.size();\
    int curidx = finer ? 0 : (nb==5 ? 0 : nrsideblocks+1); \
    int skipped = 0; \
    int i0, i1, i2; \
    bool c0, c1, c2; \
    for ( int idx=startidx; idx<stopidx; idx++ ) \
    { \
	i0 = finer ? edgeindices[idx] : edgeindices[curidx]; \
	i1 = finer ? nbindices##nb[curidx] : nbindices##nb[idx]; \
	i2 = finer ? edgeindices[idx+1] : nbindices##nb[idx+1]; \
	c0 = coords_->getPos( i0 ).isDefined(); \
	c1 = coords_->getPos( i1 ).isDefined(); \
	c2 = coords_->getPos( i2 ).isDefined(); \
	if ( c0 ) stripindices += i0; \
	if ( c1 ) stripindices += i1; \
	if ( c2 ) stripindices += i2; \
	if ( c0 || c1 || c2 ) stripsizes += (c0 + c1 + c2); \
	skipped++; \
	if ( !(skipped%nrconns) ) \
	{ \
	    if ( curidx+1<coarsesz ) \
	    { \
		curidx++; \
		i0 = finer ? edgeindices[idx+1] : edgeindices[curidx-1]; \
		i1 = finer ? nbindices##nb[curidx-1] : nbindices##nb[idx+1]; \
		i2 = finer ? nbindices##nb[curidx] : edgeindices[curidx]; \
		c0 = coords_->getPos( i0 ).isDefined(); \
		c1 = coords_->getPos( i1 ).isDefined(); \
		c2 = coords_->getPos( i2 ).isDefined(); \
		if ( c0 ) stripindices += i0; \
		if ( c1 ) stripindices += i1; \
		if ( c2 ) stripindices += i2; \
		if ( c0 || c1 || c2 ) stripsizes += (c0 + c1 + c2); \
	    } \
	    skipped = 0; \
	} \
    } \
    i0 = finer ? edgeindices[stopidx] : nbindices##nb[stopidx]; \
    i1 = finer ? nbindices##nb[curidx] : edgeindices[curidx]; \
    i2 = skipnr+nrsideblocks*resstep; \
    c0 = coords_->getPos( i0 ).isDefined(); \
    c1 = coords_->getPos( i1 ).isDefined(); \
    c2 = coords_->getPos( i2 ).isDefined(); \
    if ( c0 ) stripindices += i0; \
    if ( c1 ) stripindices += i1; \
    if ( c2 ) stripindices += i2; \
    if ( c0 || c1 || c2 ) stripsizes += (c0 + c1 + c2); \
}

void HorizonSectionTile::tesselateGlue()
{
    if ( !glueneedsretesselation_ || getActualResolution()==-1 )
	return;
   
    gluetriangles_->textureCoordIndex.deleteValues( 0, -1 );
    gluetriangles_->coordIndex.deleteValues( 0, -1 );
    gluelines_->textureCoordIndex.deleteValues( 0, -1 );
    gluelines_->coordIndex.deleteValues( 0, -1 );
    //gluepoints_->textureCoordIndex.deleteValues( 0, -1 );
    //gluepoints_->coordIndex.deleteValues( 0, -1 );

    const int resstep = (int)pow( 2, getActualResolution() );
    const int nrsideblocks = mHorizonSectionSideSize/resstep + 
	(mHorizonSectionSideSize%resstep ? 0 : -1);
    const int gluesz = mHorizonSectionSideSize - resstep*nrsideblocks;
    
    //Get all knots to be connected.
    /*	       !---------->My knot indices	
      	*......0.....
	.      1    nb5	
	.      2    .
	4.5.6..3.....
	.      .    .	
	.      .    .  --Last col: Neighbor 5 indices
	...nb7.......  --Last row: Neighbor 7 indices
     */

    TypeSet<int> edgeindices; 
    const int startcolidx = resstep*nrsideblocks;
    for ( int ridx=0; ridx<=nrsideblocks; ridx++ )
	edgeindices += startcolidx+ridx*(mHorizonSectionSideSize+1)*resstep;

    const int startrowidx = (mHorizonSectionSideSize+1)*resstep*nrsideblocks;
    for ( int cidx=0; cidx<=nrsideblocks; cidx++ )
	edgeindices += startrowidx+cidx*resstep;

    const int nbstep5 = 
	(int)pow(2, neighbors_[5] ? neighbors_[5]->getActualResolution() : -1);
    const int nrsideblocks5 = (!neighbors_[5] || !nbstep5) ? -1 : 
    mHorizonSectionSideSize/nbstep5+(mHorizonSectionSideSize%nbstep5 ? 0 : -1);
    TypeSet<int> nbindices5;
    int firstdfidx5 = -1;
    for ( int idx=0; idx<=nrsideblocks5; idx++ )
    {
	nbindices5 += (1+idx*nbstep5)*mHorizonSectionSideSize+idx*nbstep5; 
	if ( firstdfidx5==-1 && coords_->getPos(nbindices5[idx]).isDefined() )
	    firstdfidx5 = idx;
    }

    const int nbstep7 = 
	(int)pow(2, neighbors_[7] ? neighbors_[7]->getActualResolution() : -1);
    const int nrsideblocks7 = (!neighbors_[7] || !nbstep7) ? -1 :
    mHorizonSectionSideSize/nbstep7+(mHorizonSectionSideSize%nbstep7 ? 0 : -1);
    const int skipnr = (mHorizonSectionSideSize+1)*mHorizonSectionSideSize;
    int firstdfidx7 = -1;
    TypeSet<int> nbindices7;
    for ( int idx=0; idx<=nrsideblocks7; idx++ )
    {
	nbindices7 += skipnr + idx*nbstep7;
	if ( firstdfidx7==-1 && coords_->getPos(nbindices7[idx]).isDefined() )
	    firstdfidx7 = idx;
    }

    int chainlength = 0;
    int stripidx = 0;
    int lineidx = 0;
    int pointidx = 0;
    TypeSet<int> stripindices;
    TypeSet<int> stripsizes;

    /*Get strip indices chain for the glue. */
    if ( (!neighbors_[5] && !neighbors_[7]) || 
	 (resstep==nbstep5 && resstep==nbstep7) ||
         (!neighbors_[5] && (resstep==nbstep7 || firstdfidx7==-1)) ||
         (!neighbors_[7] && (resstep==nbstep5 || firstdfidx5==-1)) ||
	 (firstdfidx7==-1 && firstdfidx5==-1) )
    {
	mMakeDefaultConnection( 0, 2*nrsideblocks+1 );
    }
    else if ( !neighbors_[5] )
    {
	mMakeDefaultConnection( 0, nrsideblocks );
	mMakeConnection( 7 );
	/*
	const bool finer = nrsideblocks > nrsideblocks7;
    	const int nrconns = finer ? (nrsideblocks+1)/(nrsideblocks7+1) 
	    			   : (nrsideblocks7+1)/(nrsideblocks+1);
	const int startidx = finer ? nrsideblocks+1 : 0;
	const int stopidx = finer ? 2*nrsideblocks+1 : nrsideblocks7;
	const int coarsesz = finer ? nbindices7.size() : edgeindices.size();

	int curidx = finer ? 0 : nrsideblocks+1;
	int skipped = 0;
	for ( int idx=startidx; idx<stopidx; idx++ )
	{
	    int i0 = finer ? edgeindices[idx] : edgeindices[curidx];
	    int i1 = finer ? nbindices7[curidx] : nbindices7[idx];
	    int i2 = finer ? edgeindices[idx+1] : nbindices7[idx+1];

	    bool c0 = coords_->getPos( i0 ).isDefined();
	    bool c1 = coords_->getPos( i1 ).isDefined();
	    bool c2 = coords_->getPos( i2 ).isDefined();

	    if ( c0 )
		stripindices += i0;
	    
	    if ( c1 )
		stripindices += i1;

	    if ( c2 )
		stripindices += i2;

	    char nrdefined = c0 + c1 + c2;
	    if ( nrdefined )
		stripsizes += nrdefined;

	    skipped++;
	    if ( !(skipped%nrconns) )
	    {
		if ( curidx+1<coarsesz )
		{
		    curidx++;

		    int j0 = finer ? edgeindices[idx+1] : edgeindices[curidx-1];
		    int j1 = finer ? nbindices7[curidx-1] : nbindices7[idx+1];
		    int j2 = finer ?  nbindices7[curidx] : edgeindices[curidx];
		    bool d0 = coords_->getPos( j0 ).isDefined();
		    bool d1 = coords_->getPos( j1 ).isDefined();
		    bool d2 = coords_->getPos( j2 ).isDefined();
		    if ( d0 )
			stripindices += j0;
		    
		    if ( d1 )
			stripindices += j1;

		    if ( d2 )
			stripindices += j2;

		    char nrdefined = d0 + d1 + d2;
		    if ( nrdefined )
			stripsizes += nrdefined;
		}

		skipped = 0;
	    }
	}

	int n0 = finer ? edgeindices[stopidx] : nbindices7[stopidx];
	int n1 = finer ? nbindices7[curidx] : edgeindices[curidx];
	bool cn0 = coords_->getPos( n0 ).isDefined();
	bool cn1 = coords_->getPos( n1 ).isDefined();
	bool cn2 = coords_->getPos(skipnr+nrsideblocks*resstep).isDefined();
	if ( cn0 )
	    stripindices += n0;
	if ( cn1 )
	    stripindices += n1;
	if ( cn2 )
	    stripindices += skipnr+nrsideblocks*resstep;
	if ( cn0 || cn1 || cn2 )
	    stripsizes += cn0 + cn1 + cn2;*/
    }
    else if ( !neighbors_[7] )
    {
	mMakeDefaultConnection( nrsideblocks+1, 2*nrsideblocks+1 );
	mMakeConnection( 5 );
    }
    else
    {
	mMakeConnection( 5 );
	mMakeConnection( 7 );
    }
    
    setCoordIndices( *gluetriangles_, *gluelines_, *gluepoints_,
	    stripidx, lineidx, pointidx, stripsizes, stripindices );
    gluetriangles_->textureCoordIndex.deleteValues( stripidx, -1 );
    gluetriangles_->coordIndex.deleteValues( stripidx, -1 );
    gluelines_->textureCoordIndex.deleteValues( lineidx, -1 );
    gluelines_->coordIndex.deleteValues( lineidx, -1 );
    //gluepoints_->textureCoordIndex.deleteValues( pointidx, -1 );
    //gluepoints_->coordIndex.deleteValues( pointidx, -1 );
}


void HorizonSectionTile::setTextureComposerSize( int rowsz, int colsz )
{ texture_->size.setValue( rowsz, colsz, 0 ); }


void HorizonSectionTile::setTextureComposerOrig( int globrow, int globcol )
{ texture_->origin.setValue( globrow, globcol, 0 ); }


}; // namespace visBase
