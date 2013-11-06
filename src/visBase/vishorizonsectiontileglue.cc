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
    : gluetxcoords_( new osg::Vec2Array )
    , gluegeode_( new osg::Geode )
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
    gluegeode_->unref();
    gluevtexarr_->unRef();
    gluenormalarr_->unRef();
}


void HorizonSectionTileGlue::removeGlue()
{
    if ( gluegeom_ ) 
	gluegeom_->removePrimitiveSet( 0,gluegeom_->getNumPrimitiveSets() );

    gluevtexarr_->setEmpty();
    gluenormalarr_->setEmpty();

    if ( mGetOsgVec2Arr( gluetxcoords_ )->size() )
	 mGetOsgVec2Arr( gluetxcoords_ )->clear();

    if ( glueps_->size() ) glueps_->clear();
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

    const Coordinates* vtxarr = 
	gluetile->tileresolutiondata_[highestres]->vertices_;

    const osg::Vec3Array* normals = mGetOsgVec3Arr(
	gluetile->tileresolutiondata_[highestres]->normals_ );
    const osg::Vec2Array* tcoords = mGetOsgVec2Arr(
	gluetile->tileresolutiondata_[highestres]->txcoords_ );

    if ( vtxarr->size()<=0  || normals->size()<=0  || tcoords->size()<=0 )
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

	if( vtxarr->isDefined( coordidx ) )
	{
	    if ( vtxarr ) gluevtexarr_->addPos( vtxarr->getPos( coordidx ) );
	    if ( gluetxcoords_  ) texturesarr->push_back((*tcoords)[coordidx]);
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
    gluegeom_->dirtyDisplayList();
}