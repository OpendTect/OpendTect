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

using namespace visBase;


HorizonSectionTileGlue::HorizonSectionTileGlue()
    : gluevtxcoords_( new osg::Vec3Array )
    , gluetxcoords_( new osg::Vec2Array )
    , gluegeode_( new osg::Geode )
    , gluegeom_( new osg::Geometry )
    , gluenormals_ ( new osg::Vec3Array )
    , glueps_(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, 0))
    , glueosgps_( 0 )
{
    glueps_->ref();
    buildOsgGeometry();
}


HorizonSectionTileGlue::~HorizonSectionTileGlue()
{
    removeGlue();
    glueps_->unref();
}


void HorizonSectionTileGlue::removeGlue()
{
    if ( gluegeom_ ) 
	gluegeom_->removePrimitiveSet( 0,gluegeom_->getNumPrimitiveSets() );
    if ( mGetOsgVec3Arr( gluevtxcoords_ )->size() )
	mGetOsgVec3Arr( gluevtxcoords_ )->clear();
    if ( mGetOsgVec3Arr( gluenormals_ )->size() )
	mGetOsgVec3Arr( gluenormals_ )->clear();
    if ( mGetOsgVec2Arr( gluetxcoords_ )->size() )
	 mGetOsgVec2Arr( gluetxcoords_ )->clear();
    if ( glueps_->size() ) glueps_->clear();
}


void HorizonSectionTileGlue::buildOsgGeometry()
{
    gluegeode_->addDrawable( gluegeom_ );
    gluegeom_->setVertexArray( gluevtxcoords_ );
    gluegeom_->setNormalArray( gluenormals_ );
    gluegeom_->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
    gluegeom_->setDataVariance( osg::Object::DYNAMIC );
    gluegeom_->removePrimitiveSet( 0 , gluegeom_->getNumPrimitiveSets() );
}


void HorizonSectionTileGlue::buildGlue( HorizonSectionTile* thistile, 
		    HorizonSectionTile* neighbortile, bool rightneighbor )
{
    osg::Vec3Array* coordsarr = mGetOsgVec3Arr( gluevtxcoords_ );
    osg::Vec3Array* normalarr = mGetOsgVec3Arr( gluenormals_ );
    osg::Vec2Array* texturesarr = mGetOsgVec2Arr( gluetxcoords_ );

    datalock_.lock();
    removeGlue();
    datalock_.unLock();

    if ( !neighbortile || !thistile ) return;

    const static int cTriangleEndIdx = 3;

    const char thisres = thistile->getActualResolution();
    const char neighborres = neighbortile->getActualResolution();

    const HorizonSectionTile* gluetile = 
	thisres <= neighborres ? thistile : neighbortile ;
    const char highestres = thisres <= neighborres ? thisres : neighborres;

    if ( thisres == neighborres ) return;

    if( highestres == cNoneResolution ) return;

    osg::StateSet* stateset = gluetile->stateset_;

    const HorizonSection& hrsection = gluetile->hrsection_;
    const int spacing = hrsection.spacing_[highestres];
    const int nrblocks = spacing == 1 ? (int)hrsection.nrcoordspertileside_
	/spacing : (int)hrsection.nrcoordspertileside_/spacing +1 ;
    const Coordinates* coords = 
	gluetile->tileresolutiondata_[highestres]->vertices_;

    const osg::Vec3Array* arr = mGetOsgVec3Arr( 
	gluetile->tileresolutiondata_[highestres]->vertices_->osgArray() );
    const osg::Vec3Array* normals = mGetOsgVec3Arr(
	gluetile->tileresolutiondata_[highestres]->normals_ );
    const osg::Vec2Array* tcoords = mGetOsgVec2Arr(
	gluetile->tileresolutiondata_[highestres]->txcoords_ );

    if ( arr->size()<=0  || normals->size()<=0  || tcoords->size()<=0 )
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

	if( coords->isDefined( coordidx ) )
	{
	    if ( coordsarr ) coordsarr->push_back((*arr)[coordidx] ) ; 
	    if ( gluetxcoords_  ) texturesarr->push_back((*tcoords)[coordidx]);
	    if ( normalarr  ) 	normalarr->push_back((*normals)[coordidx]);

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
	gluegeom_->setTexCoordArray( gluetile->txunit_, gluetxcoords_ );
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

