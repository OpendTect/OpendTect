/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		3-8-2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vissplittextureseis2d.cc,v 1.6 2009-07-02 22:03:02 cvsyuancheng Exp $";

#include "vissplittextureseis2d.h"

#include "idxable.h"
#include "posinfo.h"
#include "simpnumer.h"
#include "SoTextureComposer.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vistexturecoords.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>

#define mMaxHorSz 256
#define mMaxVerSz 256

mCreateFactoryEntry( visBase::SplitTextureSeis2D );

namespace visBase
{
   
SplitTextureSeis2D::SplitTextureSeis2D()
    : VisualObjectImpl( false )
    , zrg_( 0, 0 )
    , trcrg_( 0, 0, 1 )
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


void SplitTextureSeis2D::setPath( const TypeSet<PosInfo::Line2DPos>& path, 
				  int tracestep )
{
    path_.erase();
    
    const int nrtraces = path.size();
    if ( !nrtraces )
	return;

    firsttrcnr_ = path[0].nr_;
    if ( tracestep ) trcrg_.step = tracestep;

    if ( nrtraces==1 )
    {
	path_.setSize( 1, path[0].coord_ );
	return;
    }

    path_.setSize( nrtraces, Coord::udf() );
    
    for ( int idx=0; idx<nrtraces; idx++ )
	path_[idx] = path[idx].coord_;
    
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
    const int diff = (trcrg_.start - firsttrcnr_)/trcrg_.step;
    if ( diff<0 )
	return;

    const int nrhorpixels = trcrg_.nrSteps() + 1;
    const int nrhorblocks = nrBlocks( nrhorpixels, mMaxHorSz, 1 );

    for ( int idx=0; idx<nrhorblocks; idx++ )
    {
	Interval<int> blockidxrg( idx*(mMaxHorSz-1), (idx+1)*(mMaxHorSz-1) );
	if ( blockidxrg.stop>=nrhorpixels || nrhorblocks==1 ) 
	    blockidxrg.stop = nrhorpixels-1;

	const int pathsize = blockidxrg.width()+1;
	mAllocVarLenArr( double, x, pathsize );
	mAllocVarLenArr( double, y, pathsize );

	for ( int idy=0; idy<pathsize; idy++ )
	{
	    const int pathidx = idy+blockidxrg.start+diff;
	    x[idy] = path_[pathidx].x;
	    y[idy] = path_[pathidx].y;
	}

	TypeSet<int> totalbps;
	IdxAble::getBendPoints<double*,double*,double>( x, y, pathsize,
							0.5, totalbps );

	TypeSet<int>* trcindices = new TypeSet<int>;
	trcindices->setCapacity( totalbps.size() );
	for ( int idy=0; idy<totalbps.size(); idy++ )
	    (*trcindices) += totalbps[idy] + blockidxrg.start + diff;

	horblocktrcindices_ += trcindices;
    }
  
    updateDisplay();
}


void SplitTextureSeis2D::updateSeparator( SoSeparator* sep,
	SoIndexedTriangleStripSet*& tristrip, SoTextureCoordinate2*& tc,
	SoTextureComposer*& tcomp, bool hastexture ) const
{
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

    if ( hastexture )
    {
	if ( sep->getNumChildren()>1 )
	    tc = (SoTextureCoordinate2*) sep->getChild(sep->getNumChildren()-2);
	else
	{
	    tc = new SoTextureCoordinate2;
	    sep->insertChild( tc, 0 );
	    
	    if ( sep->findChild(tc)!=sep->findChild(tristrip)-1 )
	    {
		sep->removeChild( tc );
		sep->insertChild( tc, sep->findChild( tristrip ) );
	    }
	}

	if ( sep->getNumChildren()>2 )
	    tcomp = (SoTextureComposer*) sep->getChild(sep->getNumChildren()-3);
	else
	{
	    tcomp = new SoTextureComposer;
	    sep->insertChild( tcomp, 0 );
	    
	    if ( sep->findChild(tcomp)!=sep->findChild(tc)-1 )
	    {
		sep->removeChild( tcomp );
		sep->insertChild( tcomp, sep->findChild( tc ) );
	    }
	}
    }
    else 
    {
	while ( sep->getNumChildren()>1 )
    	    sep->removeChild( 0 );
    }
}


void SplitTextureSeis2D::updateDisplay( )
{
    if ( !zrg_.width() || !trcrg_.width() )
	return;

    const int verblocks = nrBlocks( nrzpixels_, mMaxVerSz, 1 );
    ObjectSet<SoSeparator> unusedseparators = separators_;

    int coordidx = 0;
    const float inithorpos = (*horblocktrcindices_[0])[0];
    for ( int horidx=0; horidx<horblocktrcindices_.size(); horidx++ )
    {
	TypeSet<int>* horblockrg = horblocktrcindices_[ horidx ];
	for ( int idz=0; idz<verblocks; idz++ )
	{
	    SoSeparator* sep = 0;
	    SoTextureComposer* tcomp = 0;
	    SoTextureCoordinate2* tc = 0;
	    SoIndexedTriangleStripSet* tristrip = 0;

	    if ( unusedseparators.size() )
		sep = unusedseparators.remove( 0 );
	    else
	    {
		sep = new SoSeparator;
		sep->ref();
		addChild( sep );
		separators_ += sep;
	    }

	    updateSeparator( sep, tristrip, tc, tcomp, nrzpixels_ );
	    
	    const int startzpixel = idz * (mMaxVerSz-1);
	    int stopzpixel = startzpixel + mMaxVerSz-1;
	    if ( stopzpixel>=nrzpixels_ || verblocks==1 ) 
		stopzpixel = nrzpixels_-1;
	    
	    const int bpsz = (*horblockrg).size();
	    const int horsz = (*horblockrg)[bpsz-1]-(*horblockrg)[0]+1;
	    const int versz = stopzpixel-startzpixel+1;
	    const int texturehorsz = nextPower( horsz, 2 );
	    const int textureversz = nextPower( versz, 2 );
	    
	    if ( tcomp )
	    {
		const int firstpathidx = 
		    (trcrg_.start - firsttrcnr_)/trcrg_.step; 
		tcomp->origin.setValue( 0, 
			(*horblockrg)[0]-firstpathidx, startzpixel );
		tcomp->size.setValue( 1, texturehorsz, textureversz );
	    }
	 
	    if ( tc )
	    {
		const float tcstart = 0.5/textureversz;
		const float tcstop = (versz-0.5)/textureversz;
		int tcidx = 0;
		for ( int idx=0; idx<bpsz; idx++ )
		{
		    const float dist = (*horblockrg)[idx]-(*horblockrg)[0];
		    const float tcrd = (0.5+dist)/texturehorsz;
	
		    tc->point.set1Value( tcidx, SbVec2f(tcstart,tcrd) );
		    tcidx++;
		    
		    tc->point.set1Value( tcidx, SbVec2f(tcstop,tcrd) );
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
