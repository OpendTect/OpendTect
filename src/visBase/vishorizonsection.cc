/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: vishorizonsection.cc,v 1.1 2009-03-12 20:41:53 cvsyuancheng Exp $";

#include "vishorizonsection.h"

#include "binidsurface.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "vispolyline.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include "SoLockableSeparator.h"
#include "SoTextureComposer.h"
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoSwitch.h>

mCreateFactoryEntry( visBase::HorizonSection );

namespace visBase
{
HorizonSection::HorizonSection() 
    : VisualObjectImpl( false )
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , geometry_( 0 )
    , wireframelines_( 0 )		   
    , tiles_( 0, 0 )					   
{}


HorizonSection::~HorizonSection()
{
    if ( transformation_ ) transformation_->unRef();
    if ( zaxistransform_ ) zaxistransform_->unRef();
}


void HorizonSection::setDisplayTransformation( Transformation* nt )
{
    if ( transformation_ )
    {
	for ( int idx=0; geometry_ && idx<geometry_->nrKnots(); idx++ )
	{
	    const Coord3& pos = geometry_->getKnot(
		    geometry_->getKnotRowCol(idx), false );
	    transformation_->transformBack( pos );
	}

	transformation_->unRef();
	transformation_ = 0;
    }

    transformation_ = nt;
    if ( transformation_ )
    {
	transformation_->ref();
	for ( int idx=0; geometry_ && idx<geometry_->nrKnots(); idx++ )
	{
	    const Coord3& pos = geometry_->getKnot(
		    geometry_->getKnotRowCol(idx), false );
	    transformation_->transform( pos );
	}

	if ( wireframelines_ )
	    wireframelines_->setDisplayTransformation( nt );
    }
}


Transformation* HorizonSection::getDisplayTransformation()
{ return transformation_; }


void  HorizonSection::setZAxisTransform( ZAxisTransform* zt )
{
    if ( !zt || zt==zaxistransform_ )
	return;

    if ( zaxistransform_ )
	zaxistransform_->unRef();

    zaxistransform_ = zt;
    zaxistransform_->ref();
}


void HorizonSection::setGeometry( Geometry::BinIDSurface* ng )
{
    if ( !ng || geometry_==ng ) 
	return;

    geometry_ = ng;
    geomChangeCB( 0 );
}


void HorizonSection::geomChangeCB( CallBacker* )
{
    if ( !geometry_ )
	return;

    const StepInterval<int>& rrg = geometry_->rowRange();
    const int nrrowblocks = nrBlocks(rrg.width(),mHorizonSectionSideSize+2,1);
    int nrcolblocks = 0;
    for ( int row=rrg.start; row<rrg.stop; row=row+rrg.step )
    {
	const StepInterval<int>& crg = geometry_->colRange( row );
	const int nr = nrBlocks( crg.width(), mHorizonSectionSideSize+2, 1 );
	if ( nrcolblocks<nr )
	    nrcolblocks = nr;
    }

    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	int p[2];
	tiles_.info().getArrayPos( idx, p );
	delete tiles_.get( p[0], p[1] );
    }

    if ( !tiles_.setSize(nrrowblocks,nrcolblocks) )
	return;

    const char res = currentResolution();
    for ( int row=0; row<nrrowblocks; row++ )
    {
	int startrow = row ? row*(mHorizonSectionSideSize+2) : 1;
	for ( int col=0; col<nrcolblocks; col++ )
	{
	    int startcol = col ? col*(mHorizonSectionSideSize+2) : 1;
	    HorizonSectionTile* tile = new HorizonSectionTile();
	    tile->setResolution( res );
	    for ( int r=0; r<mHorizonSectionSideSize; r++ )
	    {
		for ( int c=0; c<mHorizonSectionSideSize; c++ )
		{
		    tile->setPos( r, c, geometry_->getKnot(
				RowCol(startrow+r,startcol+c), false ) );
		}
	    }

	    tiles_.set( row, col, tile );
	}
    }
    
    for ( int row=0; row<nrrowblocks; row++ )
    {
	for ( int col=0; col<nrcolblocks; col++ )
	{
	    for ( int i=-1; i<2; i++ )
	    {
		const int r = row+i;
		for ( int j=-1; j<2; j++ )
		{
		    char pos;
		    if ( j==1 )
			pos = i==-1 ? 0 : (!i ? 1 : 2);
		    else if ( j==0 )
			pos = i==-1 ? 3 : (!i ? 4 : 5);
		    else
			pos = i==-1 ? 6 : (!i ? 7 : 8);

		    const int c = col+j;
		    if ( (!r && !c) || (r<0 || r>=nrrowblocks) ||
		         (c<0 || c>=nrcolblocks) ) 
			tiles_.get(row,col)->setNeighbor( pos, 0 );
		    else
			tiles_.get(row,col)->setNeighbor(pos,tiles_.get(r,c));
		}
	    }
	}
    }
}


void HorizonSection::updateResolutions( CallBacker* )
{
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	int p[2];
	tiles_.info().getArrayPos( idx, p );
	if ( tiles_.get(p[0],p[1]) )
	    tiles_.get(p[0],p[1])->updateResolution( 0 );
    }
}


char HorizonSection::nrResolutions() const
{ return mHorizonSectionNrRes; }


char HorizonSection::currentResolution() const
{
   if ( tiles_.info().validPos(0,0) && tiles_.get(0,0) )
      return tiles_.get(0,0)->getActualResolution();
   else return -1;
}


void HorizonSection::setResolution( char res )
{
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	int p[2];
	tiles_.info().getArrayPos( idx, p );
	if ( tiles_.get(p[0],p[1]) )
	    tiles_.get(p[0],p[1])->setResolution( res );
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
    , coords_( new SoCoordinate3 )
    , texture_( new SoTextureComposer )
    , resswitch_( new SoSwitch )		
    , gluetriangles_( new SoIndexedTriangleStripSet )
    , gluelines_( new SoIndexedLineSet )
    , glueneedsretesselation_( false )
    , resolution_( -1 )	
{
    root_->ref();
    coords_->ref();
    resswitch_->ref();
    gluetriangles_->ref();
    gluelines_->ref();
    texture_->ref();

    root_->addChild( resswitch_ );
    root_->addChild( coords_ );
    root_->addChild( texture_ );
    root_->addChild( gluetriangles_ );
    root_->addChild( gluelines_ );

    for ( int idx=0; idx<mHorizonSectionNrRes; idx++ )
    {
	needsretesselation_[idx] = false;
	resolutions_[idx] = new SoSeparator;
	resolutions_[idx]->ref();
	
	root_->addChild( resolutions_[idx] );
	resswitch_->addChild( resolutions_[idx] );

	triangles_[idx] = new SoIndexedTriangleStripSet;
	triangles_[idx]->ref();
	resolutions_[idx]->addChild( triangles_[idx] );

	lines_[idx] = new SoIndexedLineSet;
	lines_[idx]->ref();
	resolutions_[idx]->addChild(lines_[idx] );
    }
}


HorizonSectionTile::~HorizonSectionTile()
{}


void HorizonSectionTile::setResolution( int rs )
{
    if ( resolution_==rs || rs<-1 || rs>=mHorizonSectionNrRes )
	return;

    resolution_ = rs;
    updateResolution( 0 ); 
}


int HorizonSectionTile::getActualResolution() const
{ return resolution_; }


void HorizonSectionTile::updateResolution( SoState* state )
{
    if ( resolution_==-1 || !triangles_[resolution_] )
	return;

    const int resstep = (int)pow( 2, resolution_ ); 
    const int blocksz = resolution_ != 0 ? mHorizonSectionSideSize/resstep 
					 : mHorizonSectionSideSize-1;
    const int gluenr = mHorizonSectionSideSize-resstep*blocksz;
    
    triangles_[resolution_]->textureCoordIndex.deleteValues( 0, -1 );
    triangles_[resolution_]->coordIndex.deleteValues( 0, -1 );
    lines_[resolution_]->textureCoordIndex.deleteValues( 0, -1 );
    lines_[resolution_]->coordIndex.deleteValues( 0, -1 );
    
    for ( int blkr=0; blkr<blocksz-1; blkr++ )
    {
	int knotidx = 0;
	int chainlength = 0; 
	int crdidx[2]; 
	crdidx[0] = crdidx[1] = mUdf(int);
	for ( int blkc=0; blkc<blocksz; blkc++ )
	{
	    int coordidx = blkr*mHorizonSectionNrRes+blkc;
	    float x, y, z;
	    coords_->point.getValues(coordidx)->getValue(x,y,z);
	    const bool undefinedpt0 = mIsUdf(x) || mIsUdf(y) || mIsUdf(z);

	    if ( !undefinedpt0 )
	    {
	       triangles_[resolution_]->textureCoordIndex.set1Value( knotidx,
								     knotidx );
		triangles_[resolution_]->coordIndex.set1Value(knotidx,coordidx);
		knotidx++;
		chainlength++;
		if ( chainlength<3 )
		    crdidx[ mIsUdf(crdidx[0]) ? 0 : 1 ] = coordidx;
	    }
	    
	    coordidx += mHorizonSectionNrRes;
	    coords_->point.getValues(coordidx)->getValue(x,y,z);
	    const bool undefinedpt1 = mIsUdf(x) || mIsUdf(y) || mIsUdf(z);
	    if ( undefinedpt1 )
	    {
		if ( !undefinedpt0 ) continue;

		triangles_[resolution_]->textureCoordIndex.set1Value( 
			knotidx, -1 );
		triangles_[resolution_]->coordIndex.set1Value( knotidx, -1 );
		if ( chainlength==2 )
		{
		    lines_[resolution_]->textureCoordIndex.set1Value( 
			    knotidx-2, knotidx-2 );
		    lines_[resolution_]->coordIndex.set1Value(
			    knotidx-2, crdidx[0] );

		    lines_[resolution_]->textureCoordIndex.set1Value( 
			    knotidx-1, knotidx-1 );
		    lines_[resolution_]->coordIndex.set1Value(
			    knotidx-1, crdidx[1] );

		    lines_[resolution_]->textureCoordIndex.set1Value( 
			    knotidx, -1 );
		    lines_[resolution_]->coordIndex.set1Value( knotidx, -1 );
		}

		chainlength = 0;
		crdidx[0] = crdidx[1] = mUdf(int);
		knotidx++;
		continue;
	    }
	    else
	    {
    		triangles_[resolution_]->textureCoordIndex.set1Value(
    			knotidx, knotidx );
    		triangles_[resolution_]->coordIndex.set1Value( 
			knotidx, coordidx );
    		knotidx++;
    		chainlength++;
    		if ( chainlength<3 )
    		    crdidx[ mIsUdf(crdidx[0]) ? 0 : 1 ] = coordidx;
	    }
	}

	triangles_[resolution_]->textureCoordIndex.deleteValues( knotidx, -1 );
	triangles_[resolution_]->coordIndex.deleteValues( knotidx, -1 );
    }
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
    if ( row<0 || row>=mHorizonSectionSideSize || col<0 ||
	    col>=mHorizonSectionSideSize )
	return;
    
    if ( resolution_!=-1 )
    {
	const int resstep = (int)pow( 2, resolution_ );
	if ( !mHorizonSectionSideSize % resstep )
    	    needsretesselation_[resolution_] = true;
    }

    coords_->point.set1Value( row*mHorizonSectionSideSize+col, 
	    		      SbVec3f(pos.x,pos.y,pos.z) );
}


void HorizonSectionTile::updateGlue()
{}


void HorizonSectionTile::tesselateGlue()
{}


void HorizonSectionTile::tesselateResolution( int rs )
{
    if ( !needsretesselation_[rs] ) 
	return;

    int tmp = resolution_;
    setResolution( rs );
    resolution_ = tmp;

    needsretesselation_[rs] = false;
}



}; // namespace visBase
