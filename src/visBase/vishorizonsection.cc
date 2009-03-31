/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: vishorizonsection.cc,v 1.4 2009-03-31 16:42:27 cvsyuancheng Exp $";

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
			pos = i==-1 ? 0 : (!i ? 1 : 2);
		    else if ( j==0 )
			pos = i==-1 ? 3 : (!i ? 4 : 5);
		    else
			pos = i==-1 ? 6 : (!i ? 7 : 8);

		    if ( (!r && !c) || (r<0 || r>=nrrowtiles) ||
		         (c<0 || c>=nrcoltiles) ) 
			tiles_.get(row,col)->setNeighbor( pos, 0 );
		    else
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
	{
	    tileptrs[idx]->updateResolution( state );
    	    tileptrs[idx]->updateGlue(); 
	}
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
    const int autores = getAutoResolution( st );
    if ( curres==-1 || curres>=mHorizonSectionNrRes )
    	setActualResolution( autores );
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
    TypeSet<int> stripindices;
    TypeSet<int> stripsizes;
     
    for ( int ridx=0; ridx<=nrsideblocks; ridx++ )
    {
	chainlength = 0; 
	stripindices.erase();
	stripsizes.erase();
	for ( int cidx=0; cidx<=nrsideblocks; cidx++ )
	{
	    const int crdidx0 = resstep*(cidx+ridx*mHorizonSectionSideSize);
	    const bool defpos0 = coords_->getPos(crdidx0).isDefined();
	    const int crdidx1 = crdidx0+mHorizonSectionSideSize*resstep;

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

	int sum = 0;
	for ( int idx=0; idx<stripsizes.size(); idx++ )
	{
	    if ( stripsizes[idx]>2 )
	    {
		for ( int s=0; s<stripsizes[idx]; s++ )
		{
		    triangles_[res]->textureCoordIndex.set1Value(
			    stripidx, stripidx ); 
	    	    triangles_[res]->coordIndex.set1Value( 
			    stripidx, stripindices[sum+s]  ); 
		    stripidx++;
		}
		
		triangles_[res]->textureCoordIndex.set1Value( stripidx, -1 ); 
		triangles_[res]->coordIndex.set1Value( stripidx, -1 ); 
		stripidx++;
	    }	
	    else if ( stripsizes[idx]==2 )
	    {
		lines_[res]->textureCoordIndex.set1Value( lineidx, lineidx );
		lines_[res]->coordIndex.set1Value(lineidx,stripindices[sum]);
		lineidx++;

		lines_[res]->textureCoordIndex.set1Value( lineidx, lineidx );
		lines_[res]->coordIndex.set1Value(lineidx,stripindices[sum+1]);
		lineidx++;
	    
		lines_[res]->textureCoordIndex.set1Value( lineidx, -1 );
    		lines_[res]->coordIndex.set1Value( lineidx, -1 );
    		lineidx++;
	    }
	    else
	    {
		/*
		points_[res]->textureCoordIndex.set1Value(pointidx,pointidx);
		points_[res]->coordIndex.set1Value(pointidx,stripindices[sum]);
		pointidx++;
		*/
	    }
	    
	    sum += stripsizes[idx];
	}
    }

    triangles_[res]->textureCoordIndex.deleteValues( stripidx, -1 );
    triangles_[res]->coordIndex.deleteValues( stripidx, -1 ); 
    lines_[res]->textureCoordIndex.deleteValues( lineidx, -1 );
    lines_[res]->coordIndex.deleteValues( lineidx, -1 );
    //points_[res]->textureCoordIndex.set1Value( pointidx, -1 );
   // points_[res]->coordIndex.set1Value( pointidx, -1 );
    
    needsretesselation_[res] = false;
    tesselateGlue();	
}


void HorizonSectionTile::setNeighbor( int nbidx, HorizonSectionTile* nb )
{
    if ( nbidx<0 || nbidx>8 || nbidx==4 )
	return;

    neighbors_[nbidx] = nb;
    neighborresolutions_[nbidx] = nb ? nb->getActualResolution() : -1;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    if ( row<0 || row>=mHorizonSectionSideSize || 
	 col<0 || col>=mHorizonSectionSideSize )
	return;

    const int res = getActualResolution();  
    if ( !needsretesselation_[res] && res>-1 && res<mHorizonSectionNrRes )
    {
	const int resstep = (int)pow( 2, res );
	if ( (row % resstep) && (col % resstep) )
    	    needsretesselation_[res] = true;
    }

    coords_->setPos( row*mHorizonSectionSideSize+col, pos );
}


void HorizonSectionTile::updateGlue()
{
    if ( glueneedsretesselation_ )
	tesselateGlue();
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
    const int skipidxsz = mHorizonSectionSideSize*resstep;
    const int nrsideblocks = mHorizonSectionSideSize/resstep + 
	(mHorizonSectionSideSize%resstep ? 0 : -1);
    const int gluesz = mHorizonSectionSideSize-resstep*nrsideblocks;


    int knot = 0;
    for ( int nbidx=0; nbidx<9; nbidx++ )
    {
	if ( nbidx==4 || !neighbors_[nbidx] )
	    continue;
    }
    
    gluetriangles_->textureCoordIndex.deleteValues( knot, -1 );
    gluetriangles_->coordIndex.deleteValues( knot, -1 );
}


void HorizonSectionTile::setTextureComposerSize( int rowsz, int colsz )
{ texture_->size.setValue( rowsz, colsz, 0 ); }


void HorizonSectionTile::setTextureComposerOrig( int globrow, int globcol )
{ texture_->origin.setValue( globrow, globcol, 0 ); }


}; // namespace visBase
