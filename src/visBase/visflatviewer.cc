/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: visflatviewer.cc,v 1.1 2007-09-10 07:54:57 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "visflatviewer.h"
#include "arraynd.h"
#include "colortab.h"
#include "flatview.h"
#include "viscolortab.h"
#include "viscoord.h"
#include "visfaceset.h"
#include "vismaterial.h"
#include "vismultitexture2.h"
#include "vistexturecoords.h"

mCreateFactoryEntry( visBase::FlatViewer );

namespace visBase
{
   
FlatViewer::FlatViewer()
    : VisualObjectImpl( false )
    , faceset_( FaceSet::create() )
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

    faceset_->ref();
    faceset_->setVertexOrdering( 
	    visBase::VertexShape::cCounterClockWiseVertexOrdering() );
    faceset_->setShapeType( visBase::VertexShape::cUnknownShapeType() );
    faceset_->removeSwitch();
    
    getMaterial()->setColor( Color( 255,255,255 ) , 0 );
    addChild( faceset_->getInventorNode() );
}


FlatViewer::~FlatViewer()
{
    faceset_->unRef();
    texture_->unRef();
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
		if ( data().vdArr() )
		{
		    texture_->setData( 0, 0, data().vdArr(), true ); 
		    texture_->turnOn( appearance().ddpars_.vd_.show_ );
		    updateTextureCoords( data().vdArr() );
		    texture_->turnOn( true );
		}
		else
		    texture_->turnOn( false );

		dataChange.trigger();
		if ( dt!=All )
		    break;	
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


void FlatViewer::updateTextureCoords( const Array2D<float>* array )
{
    if ( array )
    {
	const int x1sz =  array->info().getSize( 0 );
	const int x2sz =  array->info().getSize( 1 );

	const float x1translation = 0.5/( x1sz );
	const float x2translation = 0.5/( x2sz );

	const Coord c00( x1translation, x2translation );
	const Coord c01( x1translation, 1-x2translation );
	const Coord c10( 1-x1translation, x2translation );
	const Coord c11( 1-x1translation, 1-x2translation );
	if ( !faceset_->getTextureCoords() )
	    faceset_->setTextureCoords( TextureCoords::create() );

	if ( !appearance().annot_.x1_.reversed_ &&
	     !appearance().annot_.x2_.reversed_ )
	{
	    faceset_->getTextureCoords()->setCoord( 0, c10 );
	    faceset_->getTextureCoords()->setCoord( 1, c11 );
	    faceset_->getTextureCoords()->setCoord( 2, c01 );
	    faceset_->getTextureCoords()->setCoord( 3, c00 );
	}
	else if ( appearance().annot_.x1_.reversed_ &&
		  !appearance().annot_.x2_.reversed_ )
	{
	    faceset_->getTextureCoords()->setCoord( 0, c11 );
	    faceset_->getTextureCoords()->setCoord( 1, c10 );
	    faceset_->getTextureCoords()->setCoord( 2, c00 );
	    faceset_->getTextureCoords()->setCoord( 3, c01 );
	}
	else if ( !appearance().annot_.x1_.reversed_ &&
		  appearance().annot_.x2_.reversed_ )
	{
	    faceset_->getTextureCoords()->setCoord( 0, c00 );
	    faceset_->getTextureCoords()->setCoord( 1, c01 );
	    faceset_->getTextureCoords()->setCoord( 2, c11 );
	    faceset_->getTextureCoords()->setCoord( 3, c10 );
	}	    
	else
	{
	    faceset_->getTextureCoords()->setCoord( 0, c01 );
	    faceset_->getTextureCoords()->setCoord( 1, c00 );
	    faceset_->getTextureCoords()->setCoord( 2, c10 );
	    faceset_->getTextureCoords()->setCoord( 3, c11 );
	}
    }
}

	
void FlatViewer::setPosition( const Coord3& c00,const Coord3& c01, 
			      const Coord3& c10,const Coord3& c11 )
{
    faceset_->getCoordinates()->setPos( 0, c00 );
    faceset_->getCoordinates()->setPos( 1, c01 );
    faceset_->getCoordinates()->setPos( 2, c11 );
    faceset_->getCoordinates()->setPos( 3, c10 );
    
    if ( faceset_->nrCoordIndex()<4 )
    {
	faceset_->setCoordIndex( 0, 0 );
	faceset_->setCoordIndex( 1, 1 );
	faceset_->setCoordIndex( 2, 2 );
	faceset_->setCoordIndex( 3, 3 );
    }
}    


visBase::Transformation* FlatViewer::getDisplayTransformation()
{ return faceset_->getDisplayTransformation(); }


void FlatViewer::setDisplayTransformation( mVisTrans* nt )
{ faceset_->setDisplayTransformation( nt ); }


void FlatViewer::allowShading( bool yn )
{ texture_->allowShading( yn ); }

}; // Namespace
