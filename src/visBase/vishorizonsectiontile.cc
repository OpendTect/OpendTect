 /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vishorizonsectiontile.h"
#include "vishortileresolutiondata.h"
#include "vishorizonsectiontileglue.h"


#include "threadwork.h"
#include "viscoord.h"
#include "vishorthreadworks.h"
#include "binidsurface.h"

#include <osgGeo/LayeredTexture>

#include <osg/Switch>
#include <osg/PrimitiveSet>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LightModel>
#include <osg/LineWidth>
#include <osg/Point>
#include <osgUtil/CullVisitor>
#include <osg/UserDataContainer>

using namespace visBase;


HorizonSectionTile::HorizonSectionTile( const visBase::HorizonSection& section,
					const RowCol& origin )
    : osgswitchnode_( new osg::Switch )
    , desiredresolution_( cNoneResolution )
    , resolutionhaschanged_( false )
    , needsupdatebbox_( false )
    , nrdefinedvertices_( 0 )
    , origin_( origin )
    , hrsection_( section )
    , updatenewpoint_( false )
    , wireframedisplayed_( false )
    , righttileglue_( new HorizonSectionTileGlue )
    , bottomtileglue_( new HorizonSectionTileGlue )
    , stateset_( new osg::StateSet )
    , tesselationqueueid_( Threads::WorkManager::twm().addQueue(
    Threads::WorkManager::MultiThread, "Tessalation" ) )
{
    refOsgPtr( stateset_ );
    refOsgPtr( osgswitchnode_ );
    tileresolutiondata_.allowNull();
    buildOsgGeometries();
    bbox_.init();
}

void HorizonSectionTile::buildOsgGeometries()
{
    for ( int i = LEFTUPTILE; i <= RIGHTBOTTOMTILE; i++ )
	neighbors_[i] = 0;

    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
    {
	tileresolutiondata_ += new TileResolutionData( this, res );
	osgswitchnode_->addChild( tileresolutiondata_[res]->osgswitch_ );
    }

    osgswitchnode_->addChild( righttileglue_->getGeode() );
    osgswitchnode_->addChild( bottomtileglue_->getGeode() );

}


HorizonSectionTile::~HorizonSectionTile()
{
    osgswitchnode_->removeChildren( 0, osgswitchnode_->getNumChildren() );
    unRefOsgPtr( osgswitchnode_ );
    unRefOsgPtr( stateset_ );

    if ( righttileglue_ ) delete righttileglue_;
    if ( bottomtileglue_ ) delete bottomtileglue_;

    Threads::WorkManager::twm().removeQueue( tesselationqueueid_, false );
    deepErase( tileresolutiondata_ );
}


void HorizonSectionTile::updateNormals( char res )
{
    if ( !tileresolutiondata_.validIdx( res ) ) 
	return;

    datalock_.lock();
    tileresolutiondata_[res]->calcNormals( 
	tileresolutiondata_[res]->allnormalsinvalid_  );
    datalock_.unLock();

    emptyInvalidNormalsList( res );
    tileresolutiondata_[res]->allnormalsinvalid_ = false; 

}


void HorizonSectionTile::emptyInvalidNormalsList( char res )
{
    datalock_.lock();
    tileresolutiondata_[res]->invalidnormals_.erase();
    datalock_.unLock();
}


void HorizonSectionTile::setAllNormalsInvalid( char res, bool yn )
{ 
    if ( !tileresolutiondata_.validIdx( res ) ) 
	return;

    tileresolutiondata_[res]->allnormalsinvalid_ = yn; 

    if ( yn ) emptyInvalidNormalsList( res );
}


bool HorizonSectionTile::allNormalsInvalid( char res ) const
{ 
    if ( !tileresolutiondata_.validIdx( res ) ) 
	return false;

    return tileresolutiondata_[res]->allnormalsinvalid_; 
}


void HorizonSectionTile::setResolution( char res )
{ 
    desiredresolution_ = res; 
}


char HorizonSectionTile::getActualResolution() const
{
    const int numchildren = osgswitchnode_->getNumChildren();
    for ( int i=0; i<numchildren-2; i++ ) // the last two children are glues
    {
	if ( osgswitchnode_->getValue( i ) )
	    return  tileresolutiondata_[i]->resolution_;
    }
    return cNoneResolution;
}


void HorizonSectionTile::updateAutoResolution( const osg::CullStack* cs )
{
    char newres = desiredresolution_;
    if ( newres==cNoneResolution )
    {
	updateBBox();
	if ( !bbox_.valid() )
	    newres = cNoneResolution;
	else if ( desiredresolution_==cNoneResolution )
	{
	    const char wantedres = getAutoResolution( cs );

	    if ( !tileresolutiondata_.validIdx( wantedres ) ) 
		return;
	    newres = wantedres;
	    datalock_.lock();
	    for ( ; newres<hrsection_.nrhorsectnrres_-1; newres++ )
	    {
		if ( tileresolutiondata_[newres]->needsretesselation_ < 
		    cMustRetesselate )
		    break;
	    }
	    datalock_.unLock();

	    if ( !hrsection_.tesselationlock_ && ( wantedres!=newres || 
		tileresolutiondata_[newres]->needsretesselation_ ) )
	    {
		addTileTesselator( wantedres );
	    }
	}
    }

    setActualResolution( newres );
}


#define mIsOsgDef( pos ) (pos[2]<9.9e29)


void HorizonSectionTile::addTileTesselator( int res )
{
    TileTesselator* tt = new TileTesselator( this, res );
    Threads::WorkManager::twm().addWork(
	Threads::Work( *tt, true ),
	0, tesselationqueueid_, true );
}


void HorizonSectionTile::addTileGlueTesselator()
{
    if ( !glueneedsretesselation_ ) return;

    TileGlueTesselator* tt = new TileGlueTesselator( this );
    Threads::WorkManager::twm().addWork(
	Threads::Work( *tt, true ),
	0, tesselationqueueid_, true );
}


void HorizonSectionTile::updateBBox()
{
    if ( !needsupdatebbox_ )
	return;
    if ( tileresolutiondata_[0] )
    {
	bbox_.init();
	bbox_.expandBy( tileresolutiondata_[0]->updateBBox() );
    }
    needsupdatebbox_ = false;
}


void HorizonSectionTile::enableGeometryTypeDisplay( GeometryType type, bool yn )
{ 
    wireframedisplayed_ = ( ( type== WireFrame ) && yn ) ? true : false; 
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	tileresolutiondata_[res]->enableGeometryTypeDisplay( type, yn );
}

void HorizonSectionTile::setWireframeColor( Color& color)
{
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	tileresolutiondata_[res]->setWireframeColor( color );
}


void HorizonSectionTile::ensureGlueTesselated()
{
    bool needgluetesselation =
	glueneedsretesselation_ || resolutionhaschanged_ ||
	(neighbors_[RIGHTTILE] && 
	neighbors_[RIGHTTILE]->resolutionhaschanged_) ||
	(neighbors_[BOTTOMTILE] && 
	neighbors_[BOTTOMTILE]->resolutionhaschanged_);
    
    if ( !needgluetesselation )
	return;

    const char res = getActualResolution();
    if( res==cNoneResolution || 
	( !neighbors_[RIGHTTILE] && !neighbors_[BOTTOMTILE] ) ) 
	return;

    for ( int nb=RIGHTTILE; nb<RIGHTBOTTOMTILE; nb += 2 )
    {
	bool isright = nb == RIGHTTILE ? true : false;

	HorizonSectionTileGlue* tileglue = 
	    isright ? righttileglue_ : bottomtileglue_;
	if ( tileglue )
	{
	    datalock_.lock();
	    tileglue->setDisplayTransformation( hrsection_.transformation_ );
	    tileglue->buildGlue( this, neighbors_[nb], isright );
	    datalock_.unLock();
	}

    }

    glueneedsretesselation_ = false;

}


char HorizonSectionTile::getAutoResolution( const osg::CullStack* cs ) 
{
    const int cIdealNrPixelsPerCell = 32;

    updateBBox();
    if ( !bbox_.valid() || !cs ) return cNoneResolution;

    osg::Vec2 screensize;
    const float boxwidth = bbox_.xMax()-bbox_.xMin();
    const float boxheight = bbox_.yMax()-bbox_.yMin();

    screensize[0] = cs->clampedPixelSize( bbox_.center(),boxwidth );
    screensize[1] = cs->clampedPixelSize( bbox_.center(),boxheight );

    const int wantednumcells = 
	(int)( screensize[0]*screensize[1]/cIdealNrPixelsPerCell );

    if ( !wantednumcells )
	return hrsection_.lowestresidx_;

    if ( nrdefinedvertices_<=wantednumcells )
	return 0;

    char maxres = nrdefinedvertices_/wantednumcells;
    if ( nrdefinedvertices_%wantednumcells ) maxres++;

    for ( int desiredres=hrsection_.lowestresidx_; desiredres>=0; desiredres-- )
    {
	if ( hrsection_.nrcells_[desiredres]>=wantednumcells )
	    return mMIN(desiredres,maxres);
    }

    return 0;
}


void HorizonSectionTile::setActualResolution( char resolution )
{
    if ( resolution != getActualResolution() )
    {
	if ( resolution == cNoneResolution ) 
	    resolution = hrsection_.lowestresidx_;
	osgswitchnode_->setAllChildrenOff();
	osgswitchnode_->setValue( resolution, true );

	if ( !wireframedisplayed_ )
	{
	    osgswitchnode_->setValue(osgswitchnode_->getNumChildren()-1, true);
	    osgswitchnode_->setValue(osgswitchnode_->getNumChildren()-2, true);
	}
	else
	{
	    osgswitchnode_->setValue(osgswitchnode_->getNumChildren()-1, false);
	    osgswitchnode_->setValue(osgswitchnode_->getNumChildren()-2, false);
	}

	resolutionhaschanged_ = true;
    }
    
}


void HorizonSectionTile::tesselateResolution( char res, bool onlyifabsness )
{
    if ( !tileresolutiondata_.validIdx( res ) )  
	return;

    datalock_.lock();
    if ( updatenewpoint_ )
    {
	tileresolutiondata_[res]->tesselateResolution( false );
	updatenewpoint_ =  false;
    }
    else 
        tileresolutiondata_[res]->tesselateResolution( onlyifabsness );

    datalock_.unLock();
}


void HorizonSectionTile::applyTesselation( char res )
{
    if ( !tileresolutiondata_.validIdx( res ) ) 
	return;
    osgswitchnode_->setChildValue( osgswitchnode_->getChild( res ), true );
    resolutionhaschanged_ = false;
}


void HorizonSectionTile::setDisplayTransformation( const mVisTrans* nt )
{
    if ( getHighestResolutionCoordinates()->size() == 0 )
	return;

    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
    {
	tileresolutiondata_[res]->setDisplayTransformation( nt );
	updateNormals( res );
    }
    righttileglue_->setDisplayTransformation( nt );
    bottomtileglue_->setDisplayTransformation( nt );
}


void HorizonSectionTile::dirtyGeometry()
{
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
    {
	tileresolutiondata_[res]->dirtyGeometry();
    }

}



void HorizonSectionTile::setPositions( const TypeSet<Coord3>& pos )
{
    const RefMan<const Transformation> trans = hrsection_.transformation_;
    nrdefinedvertices_ = 0;
    bbox_.init();

    datalock_.lock();

    tileresolutiondata_[0]->initVertices();
    tileresolutiondata_[0]->setVerticesPositions( 
	const_cast<TypeSet<Coord3>*>(&pos) );
    bbox_.expandBy( tileresolutiondata_[0]->bbox_ );
    tileresolutiondata_[0]->needsretesselation_ = cMustRetesselate;
    tileresolutiondata_[0]->allnormalsinvalid_ = true;
    tileresolutiondata_[0]->invalidnormals_.erase();
    tileresolutiondata_[0]->needsetposition_ = false;

    for ( char res=1; res< hrsection_.nrhorsectnrres_; res++)
	tileresolutiondata_[res]->needsetposition_ = true;

    nrdefinedvertices_ = tileresolutiondata_[Triangle]->nrdefinedvertices_;
    datalock_.unLock();

    needsupdatebbox_ = false;

}


const visBase::Coordinates*
HorizonSectionTile::getHighestResolutionCoordinates()
{
    return tileresolutiondata_[0]->getCoordinates();
}

void HorizonSectionTile::updatePrimitiveSets()
{
    const char res = getActualResolution();
    if ( !tileresolutiondata_.validIdx( res ) )
	return;
    datalock_.lock();
    tileresolutiondata_[res]->updatePrimitiveSets();
    datalock_.unLock();
}


void HorizonSectionTile::setNeighbor( int nbidx, HorizonSectionTile* nb )
{ 
    neighbors_[nbidx] = nb;
    glueneedsretesselation_ = true;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos, int res )
{
    if ( row >=0 && row <= hrsection_.nrcoordspertileside_ &&
	 col>=0 && col <= hrsection_.nrcoordspertileside_ )
    {
	const int spacing = hrsection_.spacing_[res];

	if ( ( row%spacing ) && ( col%spacing ) )
	{
	    tileresolutiondata_[res]->needsetposition_ = true;
	    tileresolutiondata_[res]->needsretesselation_ = cMustRetesselate;
	    return;
	}

	if( res !=0 )
	    return;

	const int resrow = (int)( row/spacing );
	const int rescol = (int)( col/spacing );

	bool dohide( false );
	datalock_.lock();

	if ( res == 0 )
	{
	    tileresolutiondata_[res]->setSingleVertex(
		resrow, rescol, pos, dohide);
	}
	
	
	datalock_.unLock();

	if ( dohide )
	    setActualResolution( -1 );

	needsupdatebbox_ = true;
	glueneedsretesselation_ = true;
	updatenewpoint_ = true;
    }
   
}


void HorizonSectionTile::setTexture( const Coord& origincrd, 
    const Coord& oppositecrd )
{
    osg::Vec2f origin = Conv::to<osg::Vec2f>( origincrd );  
    osg::Vec2f opposite = Conv::to<osg::Vec2f>( oppositecrd ); 

    if ( (opposite - origin) == osg::Vec2f(0.0, 0.0) )
	return;

    std::vector<osgGeo::LayeredTexture::TextureCoordData> tcdata;
    const osgGeo::LayeredTexture* texture = hrsection_.getOsgTexture();

    unRefOsgPtr( stateset_ );
    stateset_ = texture->createCutoutStateSet( origin, opposite, tcdata );
    refOsgPtr( stateset_ );

    const bool eachlayersametexcoords = true;	/* Performance shortcut */

    int nrtexcoordlayers = tcdata.size();
    if ( eachlayersametexcoords && nrtexcoordlayers>1 )
	nrtexcoordlayers = 1;

    txunits_.erase();
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	tileresolutiondata_[res]->setNrTexCoordLayers( nrtexcoordlayers );

    std::vector<osgGeo::LayeredTexture::TextureCoordData>::iterator tcit =
								tcdata.begin();
    for ( int layeridx=0; tcit!=tcdata.end(); tcit++, layeridx++ )
    {
	const int tcidx = mMIN( layeridx, nrtexcoordlayers-1 );
	const osg::Vec2 texturetilestep = tcit->_tc11 - tcit->_tc00;

	for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	{
	    osg::Vec2Array* txcoords =
		mGetOsgVec2Arr( tileresolutiondata_[res]->txcoords_[tcidx] );

	    if ( layeridx == tcidx )
	    {
		const int spacing = hrsection_.spacing_[res];
		const int size = spacing == 1 ?
			    (int)hrsection_.nrcoordspertileside_/spacing :
			    (int)hrsection_.nrcoordspertileside_/spacing + 1;

		const osg::Vec2f diff = (opposite-origin) / spacing;

		for ( int r=0; r<size; r++ )
		{
		    for ( int c=0; c<size; c++ )
		    {
			(*txcoords)[r*size+c][0] = tcit->_tc00[0] +
				    ( float(c)/diff.x() )*texturetilestep.x();
			(*txcoords)[r*size+c][1] = tcit->_tc00[1] +
				    ( float(r)/diff.y() )*texturetilestep.y();
		    }
		}
	    }

	    tileresolutiondata_[res]->setTexture( tcit->_textureUnit,
						  txcoords, stateset_ );
	}

	txunits_ += tcit->_textureUnit;
    }
}


bool HorizonSectionTile::hasDefinedCoordinates( int idx ) const
{
    if ( tileresolutiondata_[0] )
	return tileresolutiondata_[0]->hasDefinedCoordinates( idx );
    return false;
}
 

const HorizonSectionTile* HorizonSectionTile::getNeighborTile(int idx) const
{
    if ( neighbors_ && idx>=0 && idx < 9 )
	return neighbors_[idx];
    return 0;
}
