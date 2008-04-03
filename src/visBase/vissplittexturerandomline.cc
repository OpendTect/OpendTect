/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		3-12-2008
 RCS:		$Id: vissplittexturerandomline.cc,v 1.2 2008-04-03 19:13:16 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "vissplittexturerandomline.h"

#include "scaler.h"
#include "simpnumer.h"
#include "viscoord.h"
#include "SoSplitTexture2.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTextureCoordinate3.h>

#define mMaxHorSz 256
#define mMaxVerSz 256

mCreateFactoryEntry( visBase::SplitTextureRandomLine );

namespace visBase
{
   
SplitTextureRandomLine::SplitTextureRandomLine()
    : VisualObjectImpl( false )
    , dosplit_( false )
    , nrzpixels_( 0 )
    , zrg_( 0, 0 )
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


void SplitTextureRandomLine::enableSpliting( bool yn )
{
    if ( dosplit_==yn )
	return;

    dosplit_ = yn;
    updateDisplay();
}


bool SplitTextureRandomLine::isSplitingEnabled() const
{ return dosplit_; }


void SplitTextureRandomLine::setTextureUnits( const TypeSet<int>& units )
{ 
    if ( usedunits_==units )
	return;

    usedunits_ = units;
    if ( dosplit_ )
    	updateDisplay();
}


void SplitTextureRandomLine::setTexturePath( const TypeSet<BinID>& path, 
					     int nrzpixels )
{
    path_ = path;
    nrzpixels_ = nrzpixels;

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


Interval<float> SplitTextureRandomLine::getDepthRange() const
{ return zrg_; }


mVisTrans* SplitTextureRandomLine::getDisplayTransformation()
{ return coords_->getDisplayTransformation(); }


void SplitTextureRandomLine::setDisplayTransformation( mVisTrans* nt )
{ coords_->setDisplayTransformation( nt ); }


void SplitTextureRandomLine::updateDisplay( )
{
    if ( !knots_.size() || !zrg_.width() )
	return;

    const int pathsz = path_.size();
    const bool shouldsplit = dosplit_ && pathsz && usedunits_.size() && 
			     nrzpixels_;

    if ( dosplit_ && !shouldsplit )
	return;

    const int nrhorblocks =  shouldsplit ? nrBlocks( pathsz,mMaxHorSz,1 ) : 1;
    const int nrzblocks = shouldsplit ? nrBlocks( nrzpixels_,mMaxVerSz,1 ) : 1;

    ObjectSet<SoSeparator> unusedseparators = separators_;

    int coordidx = 0;
    for ( int horidx=0; horidx<nrhorblocks; horidx++ )
    {
	const int startpathidx = horidx * (mMaxHorSz-1);
	int lastpathidx = startpathidx + mMaxHorSz-1;
	if ( lastpathidx>=pathsz || nrhorblocks==1 ) 
	    lastpathidx = pathsz-1;

	TypeSet<BinID> knots;
	if ( shouldsplit )
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
	
	for ( int idz=0; idz<nrzblocks; idz++ )
	{
	    SoSeparator* sep = 0;
	    SoTextureCoordinate3* tc = 0;
	    SoSplitTexture2Part* sp = 0;
	    SoIndexedTriangleStripSet* triangle = 0;

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
		triangle = (SoIndexedTriangleStripSet*)
		    sep->getChild( sep->getNumChildren()-1 );
	    }
	    else
	    {
		triangle = new SoIndexedTriangleStripSet;
		sep->addChild( triangle );
	    }

	    if ( pathsz && nrzpixels_ )
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
		    
		    if ( sep->findChild(tc)!=sep->findChild(triangle)-1 )
		    {
			sep->removeChild( tc );
			sep->insertChild( tc, sep->findChild( triangle ) );
		    }
		}

		if ( shouldsplit )
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
	    if ( stopzpixel>=nrzpixels_ || nrzblocks==1 ) 
		stopzpixel = nrzpixels_-1;
	    
	    const int horsz = lastpathidx-startpathidx+1;
	    const int versz = stopzpixel-startzpixel+1;
	    const int texturepathsz = shouldsplit ? nextPower(horsz,2) : horsz;
	    const int texturezsz = shouldsplit ? nextPower(versz,2) : versz;

	    if ( sp )
	    {
		sp->origin.setValue( startzpixel, startpathidx );
		sp->size.setValue( texturezsz, texturepathsz );
		
		const int unitssz = usedunits_.size();
		for ( int idx=0; idx<unitssz; idx++ )
		    sp->textureunits.set1Value( idx, usedunits_[idx] );

		sp->textureunits.deleteValues( unitssz );
	    }

	    if ( tc )
	    {
    		const float tcstart = shouldsplit ? 0.5/texturezsz : 0;
    		const float tcstop = shouldsplit ? (versz-0.5)/texturezsz : 1;
		int textureidx=0;
		for ( int idx=0; idx<knots.size(); idx++ )
    		{
		    const int posid = path_.indexOf(knots[idx]);
		    const float tcrd = (posid-startpathidx+0.5)/texturepathsz;
		    tc->point.set1Value( textureidx, SbVec3f(tcstart,tcrd,0) );
		    textureidx++;

		    tc->point.set1Value( textureidx, SbVec3f(tcstop,tcrd,0) );
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

	    int curknotidx=0;
	    for ( int idx=0; idx<knots.size(); idx++ )
	    {
		const Coord coord( knots[idx].inl, knots[idx].crl );
		coords_->setPos( coordidx, Coord3(coord,blockzrg.start) );
		triangle->coordIndex.set1Value( curknotidx, coordidx );
		triangle->textureCoordIndex.set1Value( curknotidx,curknotidx );	
		curknotidx++; 
		coordidx++;
		
		coords_->setPos( coordidx, Coord3(coord,blockzrg.stop) );
		triangle->coordIndex.set1Value( curknotidx, coordidx );
		triangle->textureCoordIndex.set1Value( curknotidx,curknotidx );
		curknotidx++; 
		coordidx++;
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
    	unusedseparators.remove( idx ); 
    }
}


}; // Namespace
