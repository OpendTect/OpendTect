/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		3-8-2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vissplittextureseis2d.cc,v 1.4 2009-02-05 22:12:18 cvsyuancheng Exp $";

#include "vissplittextureseis2d.h"

#include "idxable.h"
#include "posinfo.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vistexturecoords.h"
#include "SoSplitTexture2.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoNormalBinding.h>
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
    for ( int idx=0; idx<separators_.size(); idx++ )
	separators_[idx]->unref();

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
    path_.erase();
    if ( !path.size() )
	return;

    firsttrcnr_ = path[0].nr_;
    const int size = path[path.size()-1].nr_ - firsttrcnr_ + 1;
    path_.setSize( size, Coord::udf() );
    
    for ( int idx=0; idx<path.size()-1; idx++ )
    {
	const int start = path[idx].nr_ - firsttrcnr_;
	const int stop = path[idx+1].nr_ - firsttrcnr_;
	path_[start] = path[idx].coord_;
	path_[stop] = path[idx+1].coord_;

	const int nrinsertpts = stop-start;
	if ( nrinsertpts>1 )
	{
	    for ( int idy=1; idy<nrinsertpts; idy++ )
	    {
		const Coord diff = path_[stop] - path_[start];
		path_[start+idy] = path_[start] + diff*(float)idy/nrinsertpts;
	    }
	}
    }
    
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
    if ( !trcrg_.width() || !path_.size() ) 
	return;

    deepErase( horblocktrcindices_ );
    const int diff = trcrg_.start - firsttrcnr_;
    if ( diff<0 )
	return;

    const int nrhorpixels = trcrg_.width()+1;
    const int nrhorblocks = nrBlocks( nrhorpixels, mMaxHorSz, 1 );

    for ( int idx=0; idx<nrhorblocks; idx++ )
    {
	Interval<int> blockidxrg( idx*(mMaxHorSz-1), (idx+1)*(mMaxHorSz-1) );
	if ( blockidxrg.stop>=nrhorpixels || nrhorblocks==1 ) 
	    blockidxrg.stop = nrhorpixels-1;

	TypeSet<double> x, y;
	for ( int idy=blockidxrg.start; idy<=blockidxrg.stop; idy++ )
	{
	    x += path_[idy+diff].x;
	    y += path_[idy+diff].y;
	}

	TypeSet<int> totalbps;
	IdxAble::getBendPoints( x, y, x.size(), 0.5, totalbps );

	TypeSet<int>* trcindices = new TypeSet<int>;
	trcindices->setCapacity( totalbps.size() );
	for ( int idy=0; idy<totalbps.size(); idy++ )
	    (*trcindices) += totalbps[idy] + blockidxrg.start + diff;

	horblocktrcindices_ += trcindices;
    }
  
    updateDisplay();
}


void SplitTextureSeis2D::updateDisplay( )
{
    if ( !zrg_.width() || !trcrg_.width() )
	return;

    if ( splittexture_ && (!nrzpixels_ || !usedunits_.size()) )
	return;

    const int verblocks = splittexture_ ? nrBlocks(nrzpixels_,mMaxVerSz,1) : 1;
    ObjectSet<SoSeparator> unusedseparators = separators_;

    int coordidx = 0;
    const float inithorpos = (*horblocktrcindices_[0])[0];
    for ( int horidx=0; horidx<horblocktrcindices_.size(); horidx++ )
    {
	TypeSet<int>* horblockrg = horblocktrcindices_[ horidx ];
	for ( int idz=0; idz<verblocks; idz++ )
	{
	    SoSeparator* sep = 0;
	    SoTextureCoordinate3* tc = 0;
	    SoIndexedTriangleStripSet* tristrip = 0;
	    SoSplitTexture2Part* sp = 0;

	    if ( unusedseparators.size() )
		sep = unusedseparators.remove( 0 );
	    else
	    {
		sep =new SoSeparator;
		sep->ref();
		addChild( sep );
		separators_ += sep;
	    }

	    if ( sep->getNumChildren() )
	    {
		tristrip = (SoIndexedTriangleStripSet*)
		    sep->getChild( sep->getNumChildren()-1 );
	    }
	    else
	    {
		tristrip = new SoIndexedTriangleStripSet;
		sep->addChild( tristrip );
	    }

	    if ( nrzpixels_ )
	    {
		if ( sep->getNumChildren()>1 )
		{
		    tc = (SoTextureCoordinate3*)
			sep->getChild( sep->getNumChildren()-2 );
		}
		else
		{
		    tc = new SoTextureCoordinate3;
		    sep->insertChild( tc, 0 );
		    
		    if ( sep->findChild(tc)!=sep->findChild(tristrip)-1 )
		    {
			sep->removeChild( tc );
			sep->insertChild( tc, sep->findChild( tristrip ) );
		    }
		}

		if ( splittexture_ )
		{
		    if ( sep->getNumChildren()>2 )
			sp = (SoSplitTexture2Part*)
			    sep->getChild( sep->getNumChildren()-3 );
		    else
		    {
			sp = new SoSplitTexture2Part;
			sep->insertChild( sp, 0 );
			
			if ( sep->findChild(sp)!=sep->findChild(tc)-1 )
			{
			    sep->removeChild( sp );
			    sep->insertChild( sp, sep->findChild( tc ) );
			}
		    }
		}
		else while ( sep->getNumChildren()>2 )
		    sep->removeChild( 0 );
	    }
	    else while ( sep->getNumChildren()>1 )
		sep->removeChild( 0 );
	    
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
		const int firstpathidx = trcrg_.start - firsttrcnr_; 
		sp->origin.setValue( (*horblockrg)[0]-firstpathidx,startzpixel);
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
		    const float dist = (*horblockrg)[idx]-(*horblockrg)[0];
		    const float tcrd = splittexture_ ? (0.5+dist)/texturehorsz
			: ((*horblockrg)[idx]-inithorpos)/trcrg_.width();
	
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
		const int ti = (*horblockrg)[idx];
		tristrip->textureCoordIndex.set1Value( curknotidx,curknotidx );
		tristrip->coordIndex.set1Value( curknotidx, coordidx ); 
		coords_->setPos( coordidx, Coord3(path_[ti], blockzrg.start) );
		curknotidx++; 
		coordidx++;
		
		tristrip->textureCoordIndex.set1Value( curknotidx,curknotidx );
		tristrip->coordIndex.set1Value( curknotidx, coordidx ); 
		coords_->setPos( coordidx, Coord3(path_[ti], blockzrg.stop) );
		curknotidx++;
		coordidx++;
	    }
	    
	    tristrip->coordIndex.deleteValues( curknotidx, -1 );
	    tristrip->textureCoordIndex.deleteValues( curknotidx, -1 );
	}
    }

    coords_->removeAfter( coordidx-1 );
    
    for ( int idx=unusedseparators.size()-1; idx>=0; idx-- ) 
    { 
    	separators_ -= unusedseparators[idx]; 
    	removeChild( unusedseparators[idx] );
    	unusedseparators[idx]->unref();
    	unusedseparators.remove( idx ); 
    }
}

}; // Namespace
