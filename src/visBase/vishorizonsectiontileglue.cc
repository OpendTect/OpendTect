/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vishorizonsectiontileglue.h"
#include "vishorizonsectiontile.h"
#include "vishorizonsection.h"
#include "vishorizonsectiondef.h"
#include "vishortileresolutiondata.h"
#include "viscoord.h"
#include "visosg.h"

#include <osg/PrimitiveSet>
#include <osg/Geometry>
#include <osg/Geode>

namespace visBase
{

HorizonSectionTileGlue::HorizonSectionTileGlue()
    : gluegeode_( new osg::Geode )
    , gluegeom_( new osg::Geometry )
    , glueps_(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, 0))
    , gluevtexarr_( Coordinates::create() )
    , gluenormalarr_( Coordinates::create() )
    , glueosgps_( 0 )
    , transformation_( 0 )
{
    gluegeode_->ref();
    glueps_->ref();
    gluevtexarr_->ref();
    gluenormalarr_->ref();
    buildOsgGeometry();
}


HorizonSectionTileGlue::~HorizonSectionTileGlue()
{
    removeGlue();
    glueps_->unref();
    if ( glueosgps_ )
	glueosgps_->unref();
    gluegeode_->unref();
    gluevtexarr_->unRef();
    gluenormalarr_->unRef();
    setNrTexCoordLayers( 0 );
}


void HorizonSectionTileGlue::removeGlue()
{
    if ( gluegeom_ )
	gluegeom_->removePrimitiveSet( 0,gluegeom_->getNumPrimitiveSets() );

    gluevtexarr_->setEmpty();
    gluenormalarr_->setEmpty();

    for ( int idx=0; idx<gluetxcoords_.size(); idx++ )
	mGetOsgVec2Arr( gluetxcoords_[idx] )->clear();

    if ( glueps_->size() ) glueps_->clear();
}


void HorizonSectionTileGlue::setNrTexCoordLayers( int nrlayers )
{
    while ( nrlayers > gluetxcoords_.size() )
    {
	gluetxcoords_.push_back( new osg::Vec2Array );
	gluetxcoords_.back()->ref();
    }
    while ( nrlayers < gluetxcoords_.size() )
    {
	gluetxcoords_.back()->unref();
	gluetxcoords_.pop_back();
    }
}


void HorizonSectionTileGlue::buildOsgGeometry()
{
    gluegeode_->addDrawable( gluegeom_ );
    gluegeom_->setVertexArray( gluevtexarr_->osgArray() );
    gluegeom_->setNormalArray( gluenormalarr_->osgArray() );
    gluegeom_->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
    gluegeom_->setDataVariance( osg::Object::DYNAMIC );
    gluegeom_->removePrimitiveSet( 0 , gluegeom_->getNumPrimitiveSets() );
}


void HorizonSectionTileGlue::buildGlue( HorizonSectionTile* thistile,
		    HorizonSectionTile* neighbortile, bool rightneighbor )
{
    datalock_.lock();
    removeGlue();
    datalock_.unLock();

    if ( !neighbortile || !thistile ) return;

    const int cTriangleEndIdx = 3;

    const char thisres = thistile->getActualResolution();
    const char neighborres = neighbortile->getActualResolution();

    const HorizonSectionTile* gluetile =
			    thisres <= neighborres ? thistile : neighbortile;

    if ( gluetile->txunits_.isEmpty() )
	return;

    const char highestres = thisres <= neighborres ? thisres : neighborres;

    if ( thisres == neighborres ) return;

    if( highestres == cNoneResolution ) return;

    osg::StateSet* stateset = gluetile->stateset_;

    const HorizonSection& hrsection = gluetile->hrsection_;
    const int spacing = hrsection.spacing_[highestres];
    const int nrblocks = spacing == 1 ?
			 (int)hrsection.nrcoordspertileside_/spacing :
			 (int)hrsection.nrcoordspertileside_/spacing + 1;

    HorizonSectionTile* tile = const_cast<HorizonSectionTile*>( gluetile );

    const osg::Vec3Array* vtxarr = mGetOsgVec3Arr( tile->getOsgVertexArray() );

    const osg::Vec3Array* normals = mGetOsgVec3Arr( tile->getNormals() );

    setNrTexCoordLayers( gluetile->txcoords_.size() );

    if ( vtxarr->size()<=0 || normals->size()<=0 || gluetxcoords_.size()<=0 )
	return;

    int coordidx = 0;
    int gluepsidx = 0;

    TypeSet<int> triangleidxs;
    TypeSet<Coord3> trianglecoords;
    TypeSet<Coord3> trianglenormals;

    for ( int idx=0; idx<nrblocks; idx++ )
    {
	if ( rightneighbor && gluetile==thistile )
	{
	    coordidx = ( idx+1 )*nrblocks-1;
	}
	else if ( rightneighbor && gluetile== neighbortile )
	{
	    coordidx = idx == 0 ? 0 : idx*nrblocks;
	}
	else if ( !rightneighbor && gluetile == thistile )
	{
	    const int baseidx = (nrblocks - 1)*nrblocks;
	    coordidx = baseidx + idx ;
	}
	else if ( !rightneighbor && gluetile==neighbortile )
	{
	    coordidx = idx;
	}

	if ( mIsOsgVec3Def((*vtxarr)[coordidx]) )
	{
	    Coord3 pos = Conv::to<Coord3>( (*vtxarr)[coordidx]);
	    gluevtexarr_->getDisplayTransformation()->transformBack( pos );

	    if ( vtxarr ) gluevtexarr_->addPos( pos );

	    for ( int tcidx=0; tcidx<gluetxcoords_.size(); tcidx++ )
	    {
		const osg::Vec2Array* tcoords = mGetOsgVec2Arr(
		gluetile->txcoords_[tcidx] );

		mGetOsgVec2Arr( gluetxcoords_[tcidx] )->push_back(
							(*tcoords)[coordidx] );
	    }

	    if ( normals  )
	    {
		const osg::Vec3f osgnmcrd = (*normals)[coordidx];
		Coord3 nmcrd = Conv::to<Coord3>( osgnmcrd );
		if ( transformation_ )
		    transformation_->transformBack( nmcrd );
		gluenormalarr_->addPos( nmcrd );
	    }

	    triangleidxs += gluepsidx;
	    gluepsidx++;

	    if( triangleidxs.size() == cTriangleEndIdx )
		addGlueTrianglePrimitiveSet( triangleidxs );
	}
    }

    if ( gluegeode_ && gluegeom_ && glueps_->size()>=3 )
    {
	datalock_.lock();
	if ( glueosgps_ )
	    glueosgps_->unref();
	glueosgps_ = new osg::DrawElementsUShort( *glueps_ );
	glueosgps_->ref();
	gluegeom_->addPrimitiveSet( glueosgps_ ) ;

	for ( int layeridx=0; layeridx<gluetile->txunits_.size(); layeridx++ )
	{
	    const int tcidx = mMIN( layeridx, gluetxcoords_.size()-1 );
	    gluegeom_->setTexCoordArray( gluetile->txunits_[layeridx],
					 gluetxcoords_[tcidx] );
	}

	gluegeode_->setStateSet( stateset );
	datalock_.unLock();
    }
    else
    {
	removeGlue();
    }

}


void HorizonSectionTileGlue::addGlueTrianglePrimitiveSet(TypeSet<int>& idxs)
{
    if ( !glueps_ ) return;
    if ( idxs.size() !=3 ) return;

    const int pssize = glueps_->size();
    if ( pssize )
	glueps_->push_back( (*glueps_)[pssize-1] );

    for ( int idx = 0; idx<idxs.size(); idx++ )
	glueps_->push_back( idxs[idx] );

    idxs.setEmpty();
}

void HorizonSectionTileGlue::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();

    gluevtexarr_->setDisplayTransformation( nt );
    gluenormalarr_->setDisplayTransformation( nt );
    gluegeom_->dirtyBound();
    gluegeom_->dirtyGLObjects();
}

} // namespace visBase
