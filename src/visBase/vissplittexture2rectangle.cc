/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		2-28-2008
 RCS:		$Id: vissplittexture2rectangle.cc,v 1.4 2008-04-08 05:05:08 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "vissplittexture2rectangle.h"

#include "simpnumer.h"
#include "viscolortab.h"
#include "viscoord.h"
#include "visfaceset.h"
#include "vismaterial.h"
#include "vistexturecoords.h"
#include "SoSplitTexture2.h"

#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>

#define mMaxRowSz 256 
#define mMaxColSz 256

mCreateFactoryEntry( visBase::SplitTexture2Rectangle );

namespace visBase
{
   
SplitTexture2Rectangle::SplitTexture2Rectangle()
    : VisualObjectImpl( false )
    , dosplit_( false )
    , rowsz_( 0 )
    , colsz_( 0 )
    , nrrowblocks_( 0 )
    , nrcolblocks_( 0 )
{
    coords_ = Coordinates::create();
    coords_->ref();
    addChild( coords_->getInventorNode() );

    SoShapeHints* shapehint = new SoShapeHints;
    addChild( shapehint );
    shapehint->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapehint->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    getMaterial()->setColor( Color(255,255,255) , 0 );
}


SplitTexture2Rectangle::~SplitTexture2Rectangle()
{
    coords_->unRef();

    for ( int idx=0; idx<separators_.size(); idx++ )
	separators_[idx]->unref();
}


void SplitTexture2Rectangle::enableSpliting( bool yn )
{
    dosplit_ = yn;
    updateFaceSets();
}


void SplitTexture2Rectangle::setOriginalTextureSize( int rowsz, int colsz )
{
    rowsz_ = rowsz;
    colsz_ = colsz;
    nrrowblocks_ = dosplit_ ? nrBlocks( rowsz_, mMaxRowSz, 1 ) : 1;
    nrcolblocks_ = dosplit_ ? nrBlocks( colsz_, mMaxColSz, 1 ) : 1;

    updateFaceSets();
}


void SplitTexture2Rectangle::updateFaceSets( )
{
    if ( nrrowblocks_==0 || nrcolblocks_==0 )
	return;

    if ( dosplit_ && usedunits_.size()<0 )
	return;

    ObjectSet<SoSeparator> unusedseparators = separators_;
    
    for ( int row=0; row<nrrowblocks_; row++ )
    {
	const int firstrow = row * (mMaxRowSz-1);
	int lastrow = firstrow + mMaxRowSz-1;
	if ( lastrow>=rowsz_ || nrrowblocks_==1 ) lastrow = rowsz_-1;

	for ( int col=0; col<nrcolblocks_; col++ )
	{
	    const int firstcol = col * (mMaxColSz-1);
	    int lastcol = firstcol + mMaxColSz-1;
	    if ( lastcol>=colsz_ || nrcolblocks_==1 ) lastcol = colsz_-1;
	    
	    SoSeparator* sep = 0;
	    SoIndexedFaceSet* fs = 0;
	    SoTextureCoordinate2* tc = 0;
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
		fs = (SoIndexedFaceSet*)
		    sep->getChild( sep->getNumChildren()-1 );
	    }
	    else
	    {
		fs = new SoIndexedFaceSet;
		sep->addChild( fs );
		const int tindices[] = { 0, 1, 3, 2 };
		fs->textureCoordIndex.setValues( 0, 4, tindices );
	    }
	    
	    if ( sep->getNumChildren()>1 )
	    {
		tc = (SoTextureCoordinate2*)
		    sep->getChild( sep->getNumChildren()-2 );
	    }
	    else
	    {
		tc = new SoTextureCoordinate2;
		sep->insertChild( tc, 0 );
		
		if ( sep->findChild(tc)!=sep->findChild(fs)-1 )
		{
		    sep->removeChild( tc );
		    sep->insertChild( tc, sep->findChild( fs ) );
		}
	    }
		
	    if ( dosplit_ )
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

	    const int rowsz = lastrow-firstrow+1;
	    const int colsz = lastcol-firstcol+1;
	    const int texturerowsz = dosplit_ ? nextPower( rowsz, 2 ) : rowsz; 
	    const int texturecolsz = dosplit_ ? nextPower( colsz, 2 ) : colsz;

	    if ( sp )
	    {
    		sp->origin.setValue( firstrow, firstcol );
		sp->size.setValue( texturerowsz, texturecolsz );

		const int unitssz = usedunits_.size();
		for ( int idx=0; idx<unitssz; idx++ )
		    sp->textureunits.set1Value( idx, usedunits_[idx] );

		sp->textureunits.deleteValues( unitssz );
	    }

	    const float rowstartmargin = 0.5/texturerowsz;
	    const float colstartmargin = 0.5/texturecolsz;
	    const float rowendmargin = (float)rowsz/texturerowsz-rowstartmargin;
	    const float colendmargin = (float)colsz/texturecolsz-colstartmargin;

	    tc->point.set1Value( 0, SbVec2f(rowstartmargin,colstartmargin) );
	    tc->point.set1Value( 1, SbVec2f(rowstartmargin,colendmargin) );
	    tc->point.set1Value( 2, SbVec2f(rowendmargin,colstartmargin) );
	    tc->point.set1Value( 3, SbVec2f(rowendmargin,colendmargin) );

	    fs->coordIndex.set1Value( 0, row*(nrcolblocks_+1) + col );
	    fs->coordIndex.set1Value( 1, row*(nrcolblocks_+1) + col + 1 );
	    fs->coordIndex.set1Value( 2, (row+1)*(nrcolblocks_+1) + col + 1 );
	    fs->coordIndex.set1Value( 3, (row+1)*(nrcolblocks_+1) + col );
	}
    }

    c00factors_.erase();
    c01factors_.erase();
    c10factors_.erase();
    c11factors_.erase();

    for ( int idx=0; idx<=nrrowblocks_; idx++ )
    {
	const float pixelrow = idx*(mMaxRowSz-1);
	float rowfactor = idx==nrrowblocks_ ? 1 : pixelrow/(rowsz_-1);

	for ( int idy=0; idy<=nrcolblocks_; idy++ )
	{
	    const float pixelcol = idy*(mMaxColSz-1);
	    float colfactor = idy==nrcolblocks_ ? 1 : pixelcol/(colsz_-1);

	    c00factors_ += (1-rowfactor)*(1-colfactor);
	    c01factors_ += (1-rowfactor)*colfactor;
	    c10factors_ += rowfactor*(1-colfactor);
	    c11factors_ += rowfactor*colfactor;
	}
    }

    for ( int idx=unusedseparators.size()-1; idx>=0; idx-- )
    {
	separators_ -= unusedseparators[idx];
	unusedseparators[idx]->unref();
	removeChild( unusedseparators[idx] );
	unusedseparators.remove( idx );
    }

    updateCoordinates();
}


void SplitTexture2Rectangle::setUsedTextureUnits( const TypeSet<int>& units )
{ 
    usedunits_ = units;
    if ( dosplit_ )
       updateFaceSets();	
}


void SplitTexture2Rectangle::updateCoordinates()
{
    for ( int idx=0; idx<c00factors_.size(); idx++ )
	coords_->setPos( idx, c00factors_[idx]*c00_ + c01factors_[idx]*c01_ +
	       		      c10factors_[idx]*c10_ + c11factors_[idx]*c11_ );
    
    coords_->removeAfter( c00factors_.size()-1 );
}


void SplitTexture2Rectangle::setPosition( const Coord3& c00, const Coord3& c01, 
			      		 const Coord3& c10, const Coord3& c11 )
{
    c00_ = c00; c01_ = c01;
    c10_ = c10; c11_ = c11;

    updateCoordinates();
}    


const Coord3& SplitTexture2Rectangle::getPosition( bool dim0, bool dim1 ) const
{
    if ( !dim0 )
	return dim1 ? c01_ : c00_;

    return dim1 ? c11_ : c10_;
}


mVisTrans* SplitTexture2Rectangle::getDisplayTransformation()
{ 
    return coords_->getDisplayTransformation();
}


void SplitTexture2Rectangle::setDisplayTransformation( mVisTrans* nt )
{ 
    coords_->setDisplayTransformation( nt );
}


}; // Namespace
