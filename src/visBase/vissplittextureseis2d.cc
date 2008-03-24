/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		3-8-2008
 RCS:		$Id: vissplittextureseis2d.cc,v 1.1 2008-03-24 15:49:51 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "vissplittextureseis2d.h"

#include "idxable.h"
#include "posinfo.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vistexturecoords.h"
#include "SoSplitTexture2.h"

#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoTextureCoordinate3.h>

#define mMaxHorSz 256
#define mMaxVerSz 256

mCreateFactoryEntry( visBase::SplitTextureSeis2D );

namespace visBase
{
   
SplitTextureSeis2D::SplitTextureSeis2D()
    : VisualObjectImpl( false )
    , splittexture_( false )
    , zrg_( 0, 0 )
    , trcrg_( 0, 0 )	
    , nrzpixels_( 0 )
{
    coords_ = visBase::Coordinates::create();
    coords_->ref();
    addChild( coords_->getInventorNode() );

    SoShapeHints* shapehint = new SoShapeHints;
    addChild( shapehint );
    shapehint->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapehint->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    SoNormalBinding* nb = new SoNormalBinding;
    addChild( nb );
    nb->value = SoNormalBinding::PER_FACE_INDEXED;

    setMaterial( 0 );
}


SplitTextureSeis2D::~SplitTextureSeis2D()
{
    coords_->unRef();
    for ( int idx=0; idx<splittextures_.size(); idx++ )
	splittextures_[idx]->unref();

    for ( int idx=0; idx<tristrips_.size(); idx++ )
	tristrips_[idx]->unref();

    for ( int idx=0; idx<texturecoords_.size(); idx++ )
	texturecoords_[idx]->unref();

    deepErase( horblocktrcindices_ );
}


void SplitTextureSeis2D::enableSpliting( bool yn )
{
    if ( splittexture_==yn )
	return;

    splittexture_ = yn;
    updateDisplay();
}


bool SplitTextureSeis2D::isSplitingEnabled() const
{
    return splittexture_;
}


void SplitTextureSeis2D::setTextureUnits( const TypeSet<int>& units )
{ 
    if ( usedunits_ == units )
	return;

    usedunits_ = units;
    if ( splittexture_ )
	updateDisplay();
}


void SplitTextureSeis2D::setPath( const TypeSet<PosInfo::Line2DPos>& path )
{
    if ( path_== &path )
	return;

    path_ = &path;
    updateHorSplit();
}


void SplitTextureSeis2D::setDisplayedGeometry( const Interval<int>& trcrg,
					       const Interval<float>& zrg )
{
    if ( trcrg_ == trcrg && zrg_ == zrg )
	return;

    trcrg_ = trcrg;
    zrg_ = zrg;
    updateHorSplit();
}


void SplitTextureSeis2D::setTextureZPixels( int zsize )
{
    if ( nrzpixels_ == zsize )
	return;

    nrzpixels_ = zsize;
    updateDisplay();
}


mVisTrans* SplitTextureSeis2D::getDisplayTransformation()
{
    return coords_->getDisplayTransformation(); 
}


void SplitTextureSeis2D::setDisplayTransformation( mVisTrans* nt )
{
    coords_->setDisplayTransformation( nt );
}


void SplitTextureSeis2D::updateHorSplit()
{
    if ( !trcrg_.width() || !(*path_).size() ) 
	return;

    deepErase( horblocktrcindices_ );
    const int nrcrds = trcrg_.width()+1;
    const int nrhorblocks = nrBlocks( nrcrds, mMaxHorSz, 1 );
    for ( int idx=0; idx<nrhorblocks; idx++ )
    {
	Interval<int> blocktrcrg( idx*(mMaxHorSz-1), (idx+1)*(mMaxHorSz-1) );
	if ( blocktrcrg.stop>trcrg_.stop || nrhorblocks==1 ) 
	    blocktrcrg.stop = nrcrds-1;

	TypeSet<double> x, y;
	for ( int idx=blocktrcrg.start; idx<=blocktrcrg.stop; idx++ )
	{
	    const Coord& coord = (*path_)[idx].coord_;
	    x += coord.x;
	    y += coord.y;
	}

	TypeSet<int> totalbps;
	IdxAble::getBendPoints( x, y, x.size(), 0.5, totalbps );

	TypeSet<int>* trcindices = new TypeSet<int>;
	trcindices->setCapacity( totalbps.size() );
	for ( int idy=0; idy<totalbps.size(); idy++ )
	    (*trcindices) += totalbps[idy] + blocktrcrg.start;

	horblocktrcindices_ += trcindices;
    }
  
    updateDisplay();
}


#define mRemoveUnused( unused, object ) \
for ( int idx=unused.size()-1; idx>=0; idx-- ) \
{ \
    object -= unused[idx]; \
    removeChild( unused[idx] ); \
    unused[idx]->unref(); \
    unused.remove( idx ); \
}


void SplitTextureSeis2D::updateDisplay( )
{
    if ( !zrg_.width() || !trcrg_.width() )
	return;

    if ( splittexture_ && (!nrzpixels_ || !usedunits_.size()) )
	return;

    const int nrcrds = trcrg_.width()+1; 
    const int verblocks = splittexture_ ? nrBlocks(nrzpixels_,mMaxVerSz,1) : 1;
    
    ObjectSet<SoIndexedTriangleStripSet> unusedtristrips = tristrips_;
    ObjectSet<SoTextureCoordinate3> unusedtexturecoords = texturecoords_;
    ObjectSet<SoSplitTexture2Part> unusedsplittextures = splittextures_;

    int coordidx = 0;
    for ( int horidx=0; horidx<horblocktrcindices_.size(); horidx++ )
    {
	TypeSet<int>* horblockrg = horblocktrcindices_[ horidx ];
	for ( int idz=0; idz<verblocks; idz++ )
	{
	    SoTextureCoordinate3* tc = 0;
	    SoIndexedTriangleStripSet* tristrip = 0;
	    SoSplitTexture2Part* sp = 0;
	    
	    if ( unusedtristrips.size() )
		tristrip = unusedtristrips.remove( 0 );
	    else
	    {
		tristrip = new SoIndexedTriangleStripSet;
		tristrip->ref();
		addChild( tristrip );
		tristrips_ += tristrip;
	    }
	    
	    if ( nrzpixels_ )
	    {
		if ( unusedtexturecoords.size() )
		    tc = unusedtexturecoords.remove( 0 );
		else
		{
		    tc = new SoTextureCoordinate3;
		    tc->ref();
		    addChild( tc );
		    texturecoords_ += tc;
		}
		
		if ( childIndex(tc)!=childIndex(tristrip)-1 )
		{
		    removeChild( tc );
		    insertChild( childIndex( tristrip ), tc );
		}
		
		if ( splittexture_ )
		{
		    if ( unusedsplittextures.size() )
			sp = unusedsplittextures.remove( 0 );
		    else
		    {
			sp = new SoSplitTexture2Part;
			sp->ref();
			insertChild( childIndex( tc ), sp );
			splittextures_ += sp;
		    }
		    
		    if ( childIndex(sp)!=childIndex(tc)-1 )
		    {
			removeChild( sp );
			insertChild( childIndex( tc ), sp );
		    }
		}
	    }
	    
	    const int startzpixel = idz * (mMaxVerSz-1);
	    int stopzpixel = startzpixel + mMaxVerSz-1;
	    if ( stopzpixel>=nrzpixels_ || verblocks==1 ) 
		stopzpixel = nrzpixels_-1;
	    
	    const int bpsz = (*horblockrg).size();
	    const int horsz = (*horblockrg)[bpsz-1]-(*horblockrg)[0]+1;
	    const int versz = stopzpixel-startzpixel+1;
	    const int texturehorsz = splittexture_ ? nextPower(horsz,2) : horsz;
	    const int textureversz = splittexture_ ? nextPower(versz,2) : versz;
	    
	    if ( sp )
	    {
		sp->origin.setValue( (*horblockrg)[0], startzpixel );
		sp->size.setValue( texturehorsz, textureversz );

		const int unitssz = usedunits_.size();
		for ( int idx=0; idx<unitssz; idx++ )
		    sp->textureunits.set1Value( idx, usedunits_[idx] );

		sp->textureunits.deleteValues( unitssz );
	    }
	 
	    if ( tc )
	    {
		const float tcstart = splittexture_ ? 0.5/textureversz : 0;
		const float tcstop = splittexture_ ? (versz-0.5)/textureversz 
						   : 1;
		int tcidx = 0;
		for ( int idx=0; idx<bpsz; idx++ )
		{
		    float tcrd = splittexture_ ? 
			(0.5+(*horblockrg)[idx]-(*horblockrg)[0])/texturehorsz 
			: (float)(*horblockrg)[idx]/(nrcrds-1);
	
		    tc->point.set1Value( tcidx, SbVec3f(tcrd,tcstart,0) );
		    tcidx++;
		    
		    tc->point.set1Value( tcidx, SbVec3f(tcrd,tcstop,0) );
		    tcidx++;
		}
		
		tc->point.deleteValues( tcidx, -1 );
	    }
	    
	    Interval<float> blockzrg = zrg_;
	    if ( nrzpixels_ )
	    {
		const float zstep = zrg_.width()/(nrzpixels_-1);
		blockzrg.start += zstep*startzpixel; 
		blockzrg.stop = (stopzpixel==nrzpixels_-1)
		    ? zrg_.stop : blockzrg.start+(versz-1)*zstep;
	    }
	    
	    int curknotidx=0;
	    for ( int idx=0; idx<bpsz; idx++ )
	    {
		tristrip->textureCoordIndex.set1Value( curknotidx,curknotidx );
		tristrip->coordIndex.set1Value( curknotidx, coordidx ); 
		coords_->setPos( coordidx, 
			Coord3( (*path_)[(*horblockrg)[idx]].coord_,
			    blockzrg.start) );
		curknotidx++; 
		coordidx++;
		
		tristrip->textureCoordIndex.set1Value( curknotidx,curknotidx );
		tristrip->coordIndex.set1Value( curknotidx, coordidx ); 
		coords_->setPos( coordidx, 
			Coord3( (*path_)[(*horblockrg)[idx]].coord_,
			    blockzrg.stop) ); 
		curknotidx++;
		coordidx++;
	    }
	    
	    tristrip->coordIndex.deleteValues( curknotidx, -1 );
	    tristrip->textureCoordIndex.deleteValues( curknotidx, -1 );
	}
    }

    coords_->removeAfter( coordidx-1 );
    mRemoveUnused( unusedtristrips, tristrips_ );
    mRemoveUnused( unusedtexturecoords, texturecoords_ );
    mRemoveUnused( unusedsplittextures, splittextures_ );
}

}; // Namespace
