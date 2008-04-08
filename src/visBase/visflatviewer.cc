/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: visflatviewer.cc,v 1.8 2008-04-08 05:05:08 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "visflatviewer.h"
#include "arraynd.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "flatview.h"
#include "viscolortab.h"
#include "vismultitexture2.h"
#include "vissplittexture2rectangle.h"


mCreateFactoryEntry( visBase::FlatViewer );

namespace visBase
{
   
FlatViewer::FlatViewer()
    : VisualObjectImpl( false )
    , dataChange( this )
    , texture_( MultiTexture2::create() )
    , rectangle_( SplitTexture2Rectangle::create() )
{
    texture_->ref();
    addChild( texture_->getInventorNode() );

    if ( texture_->nrTextures()<1 )
    {
    	texture_->addTexture( "Flat Viewer" );
    	texture_->enableTexture( 0, true );	
    }

    rectangle_->ref();
    rectangle_->setMaterial( 0 );
    rectangle_->removeSwitch();
    addChild( rectangle_->getInventorNode() );
}


FlatViewer::~FlatViewer()
{
    texture_->unRef();
    rectangle_->unRef();
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
		if ( !dp )
		    texture_->turnOn( false );
		else
		{
    		    texture_->splitTexture( true );
    		    texture_->setData( 0, 0, &dp->data(), true );
    		    texture_->turnOn( appearance().ddpars_.vd_.show_ );
    		    
    		    rectangle_->enableSpliting( texture_->canUseShading() );
    		    rectangle_->setUsedTextureUnits( 
    			    texture_->getUsedTextureUnits() );
    		    rectangle_->setOriginalTextureSize( 
			    dp->data().info().getSize(1),
			    dp->data().info().getSize(0) );
		}

		dataChange.trigger();
		if ( dt!=All )
		    break;
	    }
	case VDPars : 	
		const char* ctabname = appearance().ddpars_.vd_.ctab_.buf();
		visBase::VisColorTab& vct = texture_->getColorTab( 0 );
		ColTab::Sequence& ct = vct.colorSeq().colors();
		if ( ColTab::SM().get(ctabname,ct) )
		    vct.colorSeq().colorsChanged();

		const Interval<float>& range = appearance().ddpars_.vd_.rg_;
		
		if ( mIsUdf( range.start ) || mIsUdf( range.stop ) )
		    vct.setClipRate( appearance().ddpars_.vd_.clipperc_.start );
		else 
		    vct.scaleTo( range );
    }			
}


void FlatViewer::setPosition( const Coord3& c00, const Coord3& c01, 
			      const Coord3& c10, const Coord3& c11 )
{
    rectangle_->setPosition( c00, c01, c10, c11 );
}    


void FlatViewer::allowShading( bool yn )
{
    texture_->allowShading( yn ); 
}


void FlatViewer::replaceTexture( MultiTexture2* nt )
{
    if ( !nt )
	return;

    if ( texture_ )
    {
	removeChild( texture_->getInventorNode() );
	texture_->unRef();
    }

    texture_ = nt;
    texture_->ref();
}


}; // Namespace
