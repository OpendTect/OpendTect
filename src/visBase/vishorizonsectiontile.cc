 /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vishorizonsectiontile.h"
#include "vishorizonsection.h"
#include "vishortileresolutiondata.h"
#include "vishorizonsectiondef.h"
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
    , usewireframe_( false )
    , nrdefinedvertices_( 0 )
    , origin_( origin )
    , hrsection_( section )
    , dispgeometrytype_( Triangle )
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
    if ( !needsupdatebbox_ ) return;

    bbox_.init();
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	bbox_.expandBy( tileresolutiondata_[res]->updateBBox() );

    needsupdatebbox_ = false;
}


void HorizonSectionTile::setDisplayGeometryType( unsigned int dispgeometrytype )
{ 
    dispgeometrytype_ = dispgeometrytype; 
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	tileresolutiondata_[res]->setDisplayGeometryType( dispgeometrytype_ );
}

void HorizonSectionTile::setLineColor( Color& color)
{
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	tileresolutiondata_[res]->setLineColor( color );
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
	    tileglue->buildGlue( this, neighbors_[nb], isright );
	    datalock_.unLock();
	}

    }

    glueneedsretesselation_ = false;

}


char HorizonSectionTile::getAutoResolution( const osg::CullStack* cs ) 
{
    const static int cIdealNrPixelsPerCell = 32;

    updateBBox();
    if ( !bbox_.valid() || !cs) return cNoneResolution;

    const float screensize = cs->clampedPixelSize( osg::BoundingSphere(bbox_));
    const float nrpixels = screensize*screensize;
    const int wantednumcells = (int)( nrpixels / cIdealNrPixelsPerCell);

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
	tileresolutiondata_[resolution]->setDisplayGeometryType( 
	    ( GeometryType ) dispgeometrytype_ );
	if ( dispgeometrytype_ == Triangle )
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


void HorizonSectionTile::useWireframe( bool yn )
{ usewireframe_=yn; }


void HorizonSectionTile::setPositions( const TypeSet<Coord3>& pos )
{
    const RefMan<const Transformation> trans = hrsection_.transformation_;
    nrdefinedvertices_ = 0;
    bbox_.init();

    datalock_.lock();

    tileresolutiondata_[0]->initVertices();
    tileresolutiondata_[0]->setDisplayTransformation( trans );
    tileresolutiondata_[0]->setAllVertices( pos );
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
    bool dohide( false );
    const int spacing = hrsection_.spacing_[res];

    if ( row%spacing || col%spacing ) return;

    const int tilerow = row/spacing;
    const int tilecol = col/spacing;

    datalock_.lock();
    tileresolutiondata_[res]->setSingleVertex( tilerow, tilecol, pos,dohide );
    datalock_.unLock();

    if ( dohide )
	setActualResolution( -1 );

    glueneedsretesselation_ = true;
   
}


void HorizonSectionTile::setTexture( const Coord& origincrd, 
    const Coord& oppositecrd )
{
    
    osg::Vec2f origin = Conv::to<osg::Vec2f>( origincrd );  
    osg::Vec2f opposite = Conv::to<osg::Vec2f>( oppositecrd ); 

    std::vector<osgGeo::LayeredTexture::TextureCoordData> tcdata;
    const osgGeo::LayeredTexture* texture = hrsection_.getOsgTexture();

    unRefOsgPtr( stateset_ );
	stateset_  = texture->createCutoutStateSet( origin,opposite, tcdata );
    refOsgPtr( stateset_ );

    const std::vector<osgGeo::LayeredTexture::TextureCoordData>::iterator tcit= 
	tcdata.begin();

    if ( !tcdata.size() ) return;    

    const osg::Vec2 texturetilestep = tcit->_tc11 - tcit->_tc00;

    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++)
    {
	const int spacing = hrsection_.spacing_[res];
	const int size = spacing == 1 ? 
	    (int)hrsection_.nrcoordspertileside_/spacing :
	    (int)hrsection_.nrcoordspertileside_/spacing +1 ;

	const osg::Vec2f diff = ( opposite-origin )/spacing;
	osg::Vec2Array* txcoords = 
	    mGetOsgVec2Arr( tileresolutiondata_[res]->txcoords_ );

	for ( int r=0; r<size; r++ )
	{
	    for ( int c=0; c<size; c++)
	    {
		(*txcoords)[r*size+c][0] = 
		    tcit->_tc00[0] + ( float(c)/diff.x() )*texturetilestep.x();
		(*txcoords)[r*size+c][1] = 
		    tcit->_tc00[1] + ( float(r)/diff.y() )*texturetilestep.y();
	    }
	}

	tileresolutiondata_[res]->setTexture( tcit->_textureUnit,
	    txcoords,stateset_ );
    }

    txunit_ = tcit->_textureUnit ;
}

