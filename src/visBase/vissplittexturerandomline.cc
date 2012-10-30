/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		3-12-2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vissplittexturerandomline.h"

#include "scaler.h"
#include "simpnumer.h"
#include "SoTextureComposer.h"
#include "viscoord.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>


#define mMaxHorSz 256
#define mMaxVerSz 256

mCreateFactoryEntry( visBase::SplitTextureRandomLine );

namespace visBase
{
   
SplitTextureRandomLine::SplitTextureRandomLine()
    : VisualObjectImpl( false )
    , nrzpixels_( 0 )
    , zrg_( 0, 0 )
    , pathpixelscale_( 1 )		  
{
    coords_ = visBase::Coordinates::create();
    coords_->ref();
    addChild( coords_->getInventorNode() );
    
    SoShapeHints* shapehint = new SoShapeHints;
    addChild( shapehint );
    shapehint->vertexOrdering = SoShapeHints::CLOCKWISE;
    shapehint->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    
    setMaterial( 0 );
}


SplitTextureRandomLine::~SplitTextureRandomLine()
{
    coords_->unRef();

    for ( int idx=0; idx<separators_.size(); idx++ )
	separators_[idx]->unref();
}


void SplitTextureRandomLine::setTexturePathAndPixels( 
	const TypeSet<BinID>& path, int pathsizescale, int nrzpixels )
{
    path_ = path;
    nrzpixels_ = nrzpixels;
    pathpixelscale_ = pathsizescale;

    updateDisplay();
}


void SplitTextureRandomLine::setDepthRange( const Interval<float>& intv )
{
    if ( zrg_==intv )
	return;

    zrg_ = intv;
    updateDisplay();
}


void SplitTextureRandomLine::setLineKnots( const TypeSet<BinID>& knots )
{
    if ( knots_==knots )
	return;

    knots_ = knots;
    updateDisplay();
}


const Coordinates* SplitTextureRandomLine::getCoordinates() const
{ return coords_; }


const Interval<float>& SplitTextureRandomLine::getDepthRange() const
{ return zrg_; }


const mVisTrans* SplitTextureRandomLine::getDisplayTransformation() const
{ return coords_->getDisplayTransformation(); }


void SplitTextureRandomLine::setDisplayTransformation( const mVisTrans* nt )
{ coords_->setDisplayTransformation( nt ); }


void SplitTextureRandomLine::updateSeparator( SoSeparator* sep,
	SoIndexedTriangleStripSet*& triangle, SoTextureCoordinate2*& tc,
	SoTextureComposer*& tcomp, bool hastexture ) const
{
    if ( sep->getNumChildren() )
    {
	triangle = (SoIndexedTriangleStripSet*)
	    sep->getChild( sep->getNumChildren()-1 );
    }
    else
    {
	triangle = new SoIndexedTriangleStripSet;
	sep->addChild( triangle );
    }

    if ( hastexture )
    {
	if ( sep->getNumChildren()>1 )
	    tc = (SoTextureCoordinate2*) sep->getChild(sep->getNumChildren()-2);
	else
	{
	    tc = new SoTextureCoordinate2;
	    sep->insertChild( tc, 0 );
	    
	    if ( sep->findChild(tc)!=sep->findChild(triangle)-1 )
	    {
		sep->removeChild( tc );
		sep->insertChild( tc, sep->findChild( triangle ) );
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
    else while ( sep->getNumChildren()>1 )
	sep->removeChild( 0 );
}


void SplitTextureRandomLine::updateDisplay( )
{
    if ( !knots_.size() || !zrg_.width() )
	return;

    const int pathsz = path_.size();
    const int nrhorblocks =  nrBlocks( pathsz, mMaxHorSz, 1 );
    const int nrverblocks = nrBlocks( nrzpixels_, mMaxVerSz, 1 );

    ObjectSet<SoSeparator> unusedseparators = separators_;

    int coordidx = 0;
    TypeSet<Coord3> usedpts;
    for ( int horidx=0; horidx<nrhorblocks; horidx++ )
    {
	const int startpathidx = horidx * (mMaxHorSz-1);
	int lastpathidx = startpathidx + mMaxHorSz-1;
	if ( lastpathidx>=pathsz || nrhorblocks==1 ) 
	    lastpathidx = pathsz ? pathsz-1 : 0;

	TypeSet<BinID> knots;
	if ( pathsz )
	{
    	    knots += path_[startpathidx];
    	    for ( int idx=startpathidx+1; idx<lastpathidx; idx++ )
    	    {
    		const BinID bid = path_[idx];
    		if ( knots_.indexOf( bid )>=0 && knots.indexOf(bid)<0 )
    		    knots += bid;
    	    }
	    
    	    knots += path_[lastpathidx];
	}
	else
	    knots = knots_;
	
	for ( int veridx=0; veridx<nrverblocks; veridx++ )
	{
	    SoSeparator* sep = 0;
	    SoTextureComposer* tcomp = 0;
	    SoTextureCoordinate2* tc = 0;
	    SoIndexedTriangleStripSet* triangle = 0;

	    if ( unusedseparators.size() )
		sep = unusedseparators.removeSingle( 0 );
	    else
	    {
		sep =new SoSeparator;
		sep->ref();
		addChild( sep );
		separators_ += sep;
	    }

	    updateSeparator( sep, triangle, tc, tcomp, pathsz && nrzpixels_ );

	    const int startzpixel = veridx * (mMaxVerSz-1);
	    int stopzpixel = startzpixel + mMaxVerSz-1;
	    if ( stopzpixel>=nrzpixels_ || nrverblocks==1 ) 
		stopzpixel = nrzpixels_ ? nrzpixels_-1 : 0;
	    
	    const int horsz = (lastpathidx-startpathidx+1) * pathpixelscale_;
	    const int versz = stopzpixel-startzpixel+1;
	    const int texturepathsz = nextPower(horsz,2);
	    const int texturezsz = nextPower(versz,2);

	    if ( tcomp )
	    {
		tcomp->origin.setValue( 0, startpathidx * pathpixelscale_, 
					startzpixel );
		tcomp->size.setValue( 1, texturepathsz, texturezsz );
	    }

	    if ( tc )
	    {
    		const float tcstart = 0.5f/texturezsz;
    		const float tcstop = (versz-0.5f)/texturezsz;
		int textureidx=0;
		for ( int idx=0; idx<knots.size(); idx++ )
    		{
		    const int posid = path_.indexOf(knots[idx]);
		    const float tcrd = ((posid-startpathidx) * pathpixelscale_
			    		+ 0.5f)/texturepathsz;
		    tc->point.set1Value( textureidx, SbVec2f(tcstart,tcrd) );
		    textureidx++;

		    tc->point.set1Value( textureidx, SbVec2f(tcstop,tcrd) );
		    textureidx++;
		}
		
		tc->point.deleteValues( textureidx, -1 );
	    }

	    Interval<float> blockzrg = zrg_;
	    if ( nrzpixels_ )
	    {
		const float zstep =  zrg_.width()/(nrzpixels_-1);
		blockzrg.start += startzpixel * zstep;
		blockzrg.stop = (stopzpixel==nrzpixels_-1)
			      ? zrg_.stop : blockzrg.start+(versz-1)*zstep;
	    }

	    int curknotidx = 0;
	    int curknot = 0;
	    bool repeated = false;
	    for ( int idx=0; idx<knots.size(); idx++ )
	    {
		const Coord coord( knots[idx].inl, knots[idx].crl );
		const Coord3 start(coord,blockzrg.start);
		if ( usedpts.indexOf(start)==-1 )
		{
		    usedpts += start;
		    coords_->setPos( coordidx, start );
		    coordidx++;		
		}

		triangle->coordIndex.set1Value( curknotidx, 
			usedpts.indexOf(start) );
		triangle->textureCoordIndex.set1Value( curknotidx,curknot);	
		curknotidx++;
		curknot++;

		const Coord3 stop = Coord3(coord,blockzrg.stop);
		if ( usedpts.indexOf(stop)==-1 )
		{
		    usedpts += stop;
		    coords_->setPos( coordidx, stop );
		    coordidx++;		
		}

		triangle->coordIndex.set1Value( curknotidx,
			usedpts.indexOf(stop) );
		triangle->textureCoordIndex.set1Value( curknotidx,curknot );
		curknotidx++; 
		curknot++;

		if ( idx!=0 && idx!=knots.size()-1 && !repeated )
		{
		    triangle->coordIndex.set1Value( curknotidx, -1 );
		    triangle->textureCoordIndex.set1Value( curknotidx, -1 );
		    curknotidx++;
		    curknot -= 2;
		    idx--;
		    repeated = true;
		}
		else
		    repeated = false;
	    }

	    triangle->coordIndex.deleteValues( curknotidx, -1 );
	    triangle->textureCoordIndex.deleteValues( curknotidx, -1 );
	}
    }

    coords_->removeAfter( coordidx-1 );
    for ( int idx=unusedseparators.size()-1; idx>=0; idx-- )
    { 
    	separators_ -= unusedseparators[idx]; 
    	removeChild( unusedseparators[idx] ); 
    	unusedseparators[idx]->unref(); 
    	unusedseparators.removeSingle( idx ); 
    }
}


}; // Namespace
