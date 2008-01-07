/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: visflatviewer.cc,v 1.4 2008-01-07 20:49:37 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "visflatviewer.h"
#include "arraynd.h"
#include "colortab.h"
#include "flatview.h"
#include "simpnumer.h"
#include "viscolortab.h"
#include "viscoord.h"
#include "visfaceset.h"
#include "vismaterial.h"
#include "vismultitexture2.h"
#include "vistexturecoords.h"

#include "SoSplitTexture2.h"


#define mMaxRowSz 256 
#define mMaxColSz 256

mCreateFactoryEntry( visBase::FlatViewer );

namespace visBase
{
   
FlatViewer::FlatViewer()
    : VisualObjectImpl( false )
    , dataChange( this )
    , texture_( MultiTexture2::create() )
{
    texture_->ref();
    addChild( texture_->getInventorNode() );

    if ( texture_->nrTextures()<1 )
    {
    	texture_->addTexture( "Flat Viewer" );
    	texture_->enableTexture( 0, true );	
    }

    coords_ = Coordinates::create();
    coords_->ref();
    addChild( coords_->getInventorNode() );
    getMaterial()->setColor( Color( 255,255,255 ) , 0 );
}


FlatViewer::~FlatViewer()
{
    if ( texture_ ) 
	texture_->unRef();

    if ( coords_ )
	coords_->unRef();

    deepUnRef( facesets_ );

    for ( int idx=0; idx<splittextures_.size(); idx++ )
	splittextures_[idx]->unref();
}


void FlatViewer::handleChange( FlatView::Viewer::DataChangeType dt )
{
    switch ( dt )
    {
	case None:	
		break;
	case WVAData:	
	case WVAPars:	
		pErrMsg( "Not supported" );
		break;
	case All:	
	case Annot:	
		pErrMsg( "Not implemented yet" );
		if ( dt!=All )
		    break;
	case VDData:
	    {
		const FlatDataPack* dp = pack( false );
		if ( dp )
		{
		    const int colsz = dp->data().info().getSize(0);
		    const int rowsz = dp->data().info().getSize(1);
		    
		    texture_->setData(0, 0, &dp->data(), true);
		    texture_->turnOn( appearance().ddpars_.vd_.show_ ); 
		    updateFaceSets( rowsz, colsz );
		    texture_->turnOn( true );
		}
		else
		    texture_->turnOn( false );

		dataChange.trigger();
		if ( dt!=All )
		    break;
	    }
	case VDPars : 	
		const char* ctabname = appearance().ddpars_.vd_.ctab_.buf();
		visBase::VisColorTab& vct = texture_->getColorTab( 0 );
		ColorTable& ct = vct.colorSeq().colors();
		
		if ( ColorTable::get( ctabname, ct ) )
		    vct.colorSeq().colorsChanged();

		const Interval<float>& range = appearance().ddpars_.vd_.rg_;
		
		if ( mIsUdf( range.start ) || mIsUdf( range.stop ) )
		    vct.setClipRate( appearance().ddpars_.vd_.clipperc_ ); 
		else 
		    vct.scaleTo( range );
    }			
}


int FlatViewer::nrBlocks( int totalnr, int base, int overlap )
{
    int res = 0;
    while ( totalnr>base )
    {
	res++;
	totalnr = totalnr - base + overlap;
    }

    return res+1;
}


void FlatViewer::updateFaceSets( int rowsz, int colsz )
{
    const bool dosplit =texture_->canUseShading() || texture_->splitsTexture();

    const int nrrowblocks = dosplit ? nrBlocks( rowsz, mMaxRowSz, 1 ) : 1;
    const int nrcolblocks = dosplit ? nrBlocks( colsz, mMaxColSz, 1 ) : 1;
    if ( nrrowblocks > 1 || nrcolblocks > 1 )
	texture_->splitTexture( true );

    ObjectSet<FaceSet> unusedfacesets = facesets_;
    ObjectSet<SoSplitTexture2Part> unusedsplittextures = splittextures_;

    for ( int row=0; row<nrrowblocks; row++ )
    {
	const int firstrow = row * (mMaxRowSz-1);
	int lastrow = firstrow + mMaxRowSz-1;
	if ( lastrow>=rowsz ) lastrow = rowsz-1;

	for ( int col=0; col<nrcolblocks; col++ )
	{
	    const int firstcol = col * (mMaxColSz-1);
	    int lastcol = firstcol + mMaxColSz-1;
	    if ( lastcol>=colsz ) lastcol = colsz-1;
	    
	    FaceSet* fs = 0;
	    SoSplitTexture2Part* sp = 0;

	    if ( unusedfacesets.size() )
	    {
		fs = unusedfacesets.remove( 0 );
		if ( dosplit )
		    sp =  unusedsplittextures.remove( 0 );
	    }
	    else
	    {
		if ( dosplit )
		{
		    sp = new SoSplitTexture2Part;
		    sp->ref();
		    addChild( sp );
		    splittextures_ += sp;
		}

		fs = FaceSet::create();
		fs->ref();
		fs->setCoordinates( 0 );
		fs->setVertexOrdering( visBase::VertexShape::
			cCounterClockWiseVertexOrdering() );
		fs->setShapeType( visBase::VertexShape::cUnknownShapeType() );
		fs->setTextureCoordIndex( 0, 0 );
		fs->setTextureCoordIndex( 1, 1 );
		fs->setTextureCoordIndex( 2, 2 );
		fs->setTextureCoordIndex( 3, 3 );
		fs->removeSwitch();
		if ( !fs->getTextureCoords() )
		    fs->setTextureCoords( TextureCoords::create() );
		addChild( fs->getInventorNode() );
		facesets_ += fs;
	    }

	    const int rowsz = lastrow-firstrow+1;
	    const int colsz = lastcol-firstcol+1;
	    const int texturerowsz = dosplit ? nextPower( rowsz, 2 ) : rowsz; 
	    const int texturecolsz = dosplit ? nextPower( colsz, 2 ) : colsz;

	    if ( sp )
	    {
    		sp->origin.setValue( firstrow, firstcol );
		sp->size.setValue( texturerowsz, texturecolsz );

		const TypeSet<int>& units = texture_->getUsedTextureUnits();
		for ( int idx=0; idx<units.size(); idx++ )
		    sp->textureunits.set1Value( idx, units[idx] );

		sp->textureunits.deleteValues( units.size() );
	    }

	    const float rowstartmargin = 0.5/texturerowsz;
	    const float colstartmargin = 0.5/texturecolsz;
	    const float rowendmargin = (float)rowsz/texturerowsz-rowstartmargin;
	    const float colendmargin = (float)colsz/texturecolsz-colstartmargin;

	    const Coord c00( rowstartmargin, colstartmargin );
	    const Coord c01( rowstartmargin, colendmargin );
	    const Coord c10( rowendmargin, colstartmargin );
	    const Coord c11( rowendmargin, colendmargin );
	    
	    fs->getTextureCoords()->setCoord( 0, c00 );
	    fs->getTextureCoords()->setCoord( 1, c01 );
	    fs->getTextureCoords()->setCoord( 2, c11 );
	    fs->getTextureCoords()->setCoord( 3, c10 );

	    fs->setCoordIndex( 0, row*(nrcolblocks+1) + col );
	    fs->setCoordIndex( 1, row*(nrcolblocks+1) + col + 1 );
	    fs->setCoordIndex( 2, (row+1)*(nrcolblocks+1) + col + 1);
	    fs->setCoordIndex( 3, (row+1)*(nrcolblocks+1) + col );
	}
    }

    c00factors_.erase();
    c01factors_.erase();
    c10factors_.erase();
    c11factors_.erase();

    for ( int idx=0; idx<=nrrowblocks; idx++ )
    {
	const float rowfactor = (float) idx/nrrowblocks;

	for ( int idy=0; idy<=nrcolblocks; idy++ )
	{
	    const float colfactor = (float) idy/nrcolblocks;

	    c00factors_ += (1-rowfactor)*(1-colfactor);
	    c01factors_ += (1-rowfactor)*colfactor;
	    c10factors_ += rowfactor*(1-colfactor);
	    c11factors_ += rowfactor*colfactor;
	}
    }


    for ( int idx=unusedfacesets.size()-1; idx>=0; idx-- )
    {
	facesets_ -= unusedfacesets[idx];
	removeChild( unusedfacesets[idx]->getInventorNode() );
	unusedfacesets[idx]->unRef();
	unusedfacesets.remove( idx );

	splittextures_ -= unusedsplittextures[idx];
	removeChild( unusedsplittextures[idx] );
	unusedsplittextures[idx]->unref();
	unusedsplittextures.remove( idx );
    }

    updateCoordinates();
}



void FlatViewer::updateCoordinates()
{
    Coord3 c00 = c00_;
    Coord3 c10 = c10_;
    Coord3 c11 = c11_;
    Coord3 c01 = c01_;
    if ( appearance().annot_.x1_.reversed_ )
    { 
	Swap( c00, c01 );
	Swap( c10, c11 );
    }

    if ( appearance().annot_.x2_.reversed_ )
    { 
	Swap( c00, c10 );
	Swap( c01, c11 );
    }

    for ( int idx=0; idx<c00factors_.size(); idx++ )
	coords_->setPos( idx, c00factors_[idx]*c00 + c01factors_[idx]*c01 +
	       		      c10factors_[idx]*c10 + c11factors_[idx]*c11 );
    
    coords_->removeAfter( c00factors_.size() );
}


void FlatViewer::setPosition( const Coord3& c00, const Coord3& c01, 
			      const Coord3& c10, const Coord3& c11 )
{
    c00_ = c00; c01_ = c01;
    c10_ = c10; c11_ = c11;

    updateCoordinates();
}    


visBase::Transformation* FlatViewer::getDisplayTransformation()
{ 
    return facesets_.size()>0 ? facesets_[0]->getDisplayTransformation() : 0;
}


void FlatViewer::setDisplayTransformation( mVisTrans* nt )
{ 
    for ( int idx=0; idx<facesets_.size(); idx++ )
	facesets_[idx]->setDisplayTransformation( nt );
}


void FlatViewer::allowShading( bool yn )
{
    texture_->allowShading( yn );  
}


}; // Namespace
