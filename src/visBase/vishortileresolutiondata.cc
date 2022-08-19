/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vishortileresolutiondata.h"
#include "vishorizonsection.h"
#include "vishorizonsectiontile.h"

#include "binidsurface.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visosg.h"

#include <osg/Switch>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LightModel>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/BoundingBox>
#include <osg/UserDataContainer>

#include <osgGeo/LayeredTexture>

const char cTowardDown  = 0;
const char cTowardRight = 1;

namespace visBase
{

static osg::Geometry* getOSGGeometry( osg::Geode* geode )
{
    return sCast(osg::Geometry*,geode->getDrawable(0));
}


static const osg::Geometry* getOSGGeometry( const osg::Geode* geode )
{
    return sCast(const osg::Geometry*,geode->getDrawable(0));
}


static osg::Geode* getOSGGeode( osg::UserDataContainer* cont, int idx )
{
    return sCast(osg::Geode*,cont->getUserObject(idx));
}


TileResolutionData::TileResolutionData( const HorizonSectionTile* sectile,
					char resolution )
    : sectile_(sectile)
    , osgswitch_(new osg::Switch)
    , geodes_(new osg::DefaultUserDataContainer)
    , linecolor_(new osg::Vec4Array)
    , resolution_(resolution)
{
    geodes_->ref();
    osgswitch_->ref();
    const HorizonSection& section = sectile_->hrsection_;
    const int spacing = section.spacing_[resolution_];
    if ( spacing>=0 )
    {
	nrverticesperside_ = spacing == 1 ?
	    (int)section.nrcoordspertileside_/spacing :
	(int)section.nrcoordspertileside_/spacing +1 ;
    }

    HorizonSectionTile* tile = const_cast<HorizonSectionTile*>( sectile_ );
    normals_ = tile->getNormals();
    osgvertices_ = tile->getOsgVertexArray();
    buildOsgGeometres();
}


TileResolutionData::~TileResolutionData()
{
    unRefOsgPrimitiveSets();
    geodes_->unref();
    osgswitch_->unref();
}


void TileResolutionData::setTexture( const unsigned int unit,
			osg::Array* tcarr, osg::StateSet* stateset )
{
    setGeometryTexture( unit, tcarr, stateset, Triangle );
    setGeometryTexture( unit, tcarr, stateset, Line );
    setGeometryTexture( unit, tcarr, stateset, Point );
}


void TileResolutionData::setGeometryTexture( const unsigned int unit,
					     const osg::Array* arr,
					     osg::StateSet* stateset,
					     int geometrytype )
{
    if ( geometrytype > geodes_->getNumUserObjects() )
	return;

    osg::Geode* geode = getOSGGeode( geodes_, geometrytype );
    if ( !geode ) return;

    osg::Geometry* geom = getOSGGeometry( geode );
    if ( geom )
    {
	auto* carr = sCast(const osg::Vec2Array*,arr);
	geom->setTexCoordArray( unit, cCast(osg::Vec2Array*,carr) );
	geode->setStateSet( stateset );
    }
}


void TileResolutionData::enableGeometryTypeDisplay( GeometryType type, bool yn )
{
    osgswitch_->setAllChildrenOff();
    osgswitch_->setValue( Triangle, true );
    osgswitch_->setValue( Line, true );
    if ( type>=Triangle && type<=WireFrame )
    {
        osgswitch_->setValue( type, yn );
	if ( yn )
	    dispgeometrytype_ = type;
	else
	   dispgeometrytype_ = Triangle;
    }
    osgswitch_->setValue( Line, true );
    osgswitch_->setValue( Point, true );
}


void TileResolutionData::dirtyGeometry()
{
    for ( int type=0; type<=WireFrame; type++ )
    {
	if ( osgswitch_->getValue((unsigned int)type) )
	    dirtyGeometry( type );
    }
}


void TileResolutionData::dirtyGeometry( int type )
{
    osg::Geode* geode = getOSGGeode( geodes_, type );
    if ( geode )
    {
	getOSGGeometry( geode )->dirtyBound();
	getOSGGeometry( geode )->dirtyGLObjects();
    }
}


#define mClearPrimitiveSet\
    trianglesps_->clear();\
    wireframesps_->clear();\
    linesps_->clear();\
    pointsps_->clear();\


#define mClearOsgPrimitiveSet\
    if ( trianglesosgps_ ) trianglesosgps_->clear();\
    if ( wireframesosgps_ ) wireframesosgps_->clear();\
    if ( linesosgps_ ) linesosgps_->clear();\
    if ( pointsosgps_ ) pointsosgps_->clear();\


void TileResolutionData::hideFromDisplay()
{
    mClearOsgPrimitiveSet;
}


int getCoordinateIndex( int row, int col, int nrcoords )
{
    return row*nrcoords + col;
}


bool TileResolutionData::tesselateResolution( bool onlyifabsness )
{
    const HorizonSection& hrsection = sectile_->hrsection_;
    const int spacing = hrsection.spacing_[resolution_];

    if ( resolution_<0 || needsretesselation_==cNoTesselationNeeded ||
	(needsretesselation_==cShouldRetesselate && onlyifabsness) )
	return false;

    mClearPrimitiveSet;

    updateprimitiveset_ = false;

    tesselatemutex_.lock();

    const osg::Vec3Array* osgvertices = mGetOsgVec3Arr( osgvertices_ );

    for ( int row=0; row<hrsection.nrcoordspertileside_; row+=spacing )
    {
	for ( int col=0; col<hrsection.nrcoordspertileside_; col+=spacing )
	{
	    if ( row==hrsection.nrcoordspertileside_-1 &&
		 col==hrsection.nrcoordspertileside_-1 )
		 continue;

	    const int coordidx = getCoordinateIndex(
		row, col, hrsection.nrcoordspertileside_ );
	    if ( !mIsOsgVec3Def( (*osgvertices)[coordidx] ) )
		continue;

	    tesselateCell( row, col );
	}
    }

    tesselatemutex_.unLock();

    updateprimitiveset_ = true;
    needsretesselation_ = cNoTesselationNeeded;
    return true;
}


void TileResolutionData::setPrimitiveSet( unsigned int geometrytype,
					  osg::DrawElementsUShort* geomps )
{
    if( !geomps || geometrytype>geodes_->getNumUserObjects() )
	return;

    osg::Geode* geode = getOSGGeode( geodes_, geometrytype );
    if( !geode ) return;

    osg::Geometry* geom = getOSGGeometry( geode );
    if( !geom ) return;

    geom->removePrimitiveSet( 0, geom->getNumPrimitiveSets() );
    if ( geomps->size() )
    {
	geom->addPrimitiveSet( geomps );
	geom->computeBound();
    }
}


#define mSetOsgPrimitiveSet( geomtype,geom )\
if ( geomtype##osgps_ )\
    unRefOsgPtr(  geomtype##osgps_ );\
geomtype##osgps_ = new osg::DrawElementsUShort( *geomtype##ps_ );\
refOsgPtr( geomtype##osgps_ );\
setPrimitiveSet( geom, geomtype##osgps_ );\


void TileResolutionData::updatePrimitiveSets()
{
    if ( !updateprimitiveset_ )
	return;

#   ifdef __debug__
    if ( !DataObject::isVisualizationThread() )
    {
	pErrMsg( "Not in visualization thread" );
    }
#   endif

    mSetOsgPrimitiveSet( triangles, Triangle );
    mSetOsgPrimitiveSet( lines, Line );
    mSetOsgPrimitiveSet( points, Point );
    mSetOsgPrimitiveSet( wireframes, WireFrame );

    mClearPrimitiveSet;
    updateprimitiveset_ = false;
}


static void addPointIndex( osg::DrawElementsUShort* geomps, int idx )
{
    geomps->push_back( idx );
}


static void addLineIndexes( osg::DrawElementsUShort* geomps, int idx1, int idx2)
{
    addPointIndex( geomps, idx1 );
    addPointIndex( geomps, idx2 );
}


static void addClockwiseTriangleIndexes( osg::DrawElementsUShort* geomps,
					 int idx0, int idxa, int idxb )
{
    const int pssize = geomps->getNumIndices();
    const int idx1 = pssize%2 ? idxa : idxb;
    const int idx2 = pssize%2 ? idxb : idxa;
    bool continuestrip = pssize > 2;
    if ( continuestrip )
    {
	const int lastidx = geomps->index( pssize-1 );
	if ( lastidx==idx0 && geomps->index(pssize-2)==idx1 )
    	    geomps->push_back( idx2 );
	else if ( lastidx==idx1 && geomps->index(pssize-2)==idx2 )
    	    geomps->push_back( idx0 );
	else if ( lastidx==idx2 && geomps->index(pssize-2)==idx0 )
    	    geomps->push_back( idx1 );
	else
	{
	    continuestrip = false;
	    geomps->push_back( lastidx );
	    geomps->push_back( idx0 );
	}
    }

    if ( !continuestrip )
    {
	geomps->push_back( idx0 );
	geomps->push_back( idx1 );
	geomps->push_back( idx2 );
    }
}


void TileResolutionData::tesselateCell( int row, int col )
{
    const HorizonSection& section = sectile_->hrsection_;
    const int spacing = section.spacing_[resolution_];
    const int nrcoords = section.nrcoordspertileside_;
    const int idxthis = getCoordinateIndex( row, col, nrcoords );

    const int idxright = getCoordinateIndex( row, col+spacing, nrcoords );
    const int idxbottom = getCoordinateIndex( row+spacing, col, nrcoords );
    const int idxrightbottom = getCoordinateIndex(
	row+spacing, col+spacing, nrcoords );

    const osg::Vec3Array* osgvertices = mGetOsgVec3Arr( osgvertices_ );
    const int size = nrcoords*nrcoords;

    if ( !mIsOsgVec3Def( (*osgvertices)[idxthis] ) )
	return;

    bool rightisdef =
	idxright<size ? mIsOsgVec3Def( (*osgvertices)[idxright] ) : false;
    const bool bottomisdef =
	idxbottom<size ? mIsOsgVec3Def( (*osgvertices)[idxbottom] ) : false;
    bool rightbottomisdef =
	idxrightbottom<size ?
	mIsOsgVec3Def( (*osgvertices)[idxrightbottom] ) : false;

    const bool atright = ( idxthis+1 ) % nrcoords == 0;
    if ( atright )
	rightisdef = rightbottomisdef = false;

    if ( !rightisdef )
    {
	if ( !bottomisdef )
	{
	    const int idxleft = (col-spacing)>0 ?
		getCoordinateIndex(row,col-spacing,nrcoords) : -1;
	    const int idxtop = (row-spacing)>0 ?
		getCoordinateIndex(row-spacing,col,nrcoords) : -1;

	    const bool leftdef = (idxleft>=0 && idxleft<size) ?
		mIsOsgVec3Def( (*osgvertices)[idxleft] ) : false;
	    const bool topdef = (idxtop>=0 && idxtop<size) ?
		mIsOsgVec3Def(( *osgvertices )[idxtop]) : false;

	    if ( !leftdef && !topdef )
		addPointIndex( pointsps_, idxthis );
	}
	else if ( bottomisdef )
	{
	    if ( !rightbottomisdef )
	    {
		addLineIndexes( wireframesps_,idxthis, idxbottom  );
		if ( detectIsolatedLine( idxthis, cTowardDown ) )
		    addLineIndexes( linesps_,idxthis, idxbottom );
	    }
	    else
	    {
		addClockwiseTriangleIndexes( trianglesps_, idxthis,
					     idxrightbottom, idxbottom );
		addLineIndexes( wireframesps_, idxthis, idxbottom );
	    }

	}
    }
    else if ( rightisdef )
    {
	if ( bottomisdef )
	{
	    if ( rightbottomisdef )
	    {
		addClockwiseTriangleIndexes( trianglesps_, idxthis,
					     idxright, idxbottom );
		addClockwiseTriangleIndexes( trianglesps_, idxbottom,
					     idxright, idxrightbottom );
		addLineIndexes( wireframesps_, idxthis, idxright );
		addLineIndexes( wireframesps_, idxthis, idxbottom );
	    }
	    else if ( !rightbottomisdef )
	    {
		addClockwiseTriangleIndexes( trianglesps_, idxthis,
					     idxright, idxbottom );
		addLineIndexes( wireframesps_, idxthis, idxright );
		addLineIndexes( wireframesps_, idxthis, idxbottom );
	    }
	}
	else if ( !bottomisdef )
	{
	    if ( rightbottomisdef )
	    {
		addClockwiseTriangleIndexes( trianglesps_, idxthis,
					     idxright, idxrightbottom );
		addLineIndexes( wireframesps_, idxthis, idxright );
	    }
	    else
	    {
		addLineIndexes( wireframesps_, idxthis, idxright );
		if ( detectIsolatedLine( idxthis, cTowardRight ) )
		    addLineIndexes( linesps_,idxthis, idxright );
	    }
	}
    }
}


bool TileResolutionData::detectIsolatedLine( int curidx, char direction )
{
    HorizonSectionTile* curtile = const_cast<HorizonSectionTile*>( sectile_ );
    const HorizonSection& section = curtile->hrsection_;
    const int size = section.nrcoordspertileside_;
    const int currow = (int)Math::Floor( (double)curidx/size );
    const int curcol = curidx - currow*size;
    const bool isfirstrow = currow == 0 ? true : false;
    const bool isfirstcol = curcol == 0 ? true : false;
    const bool islastrow =  currow == size - 1 ? true : false;
    const bool islastcol =  curcol == size - 1 ? true : false;
    const int nrroworcol = section.nrcoordspertileside_;

    int highestresidx = currow*nrroworcol + curcol;
    if ( islastrow ) highestresidx -= section.nrcoordspertileside_;

    //00 01 02
    //10 11 12 -- 11 is this
    //20 21 22

    bool		  nbdef01 = false, nbdef02 = false;
    bool nbdef10 = false,		   nbdef12 = false;
    bool nbdef20 = false, nbdef21 = false, nbdef22 = false;

    const bool useneigbors =  section.usingNeighborsInIsolatedLine();
    unsigned int sum = 0;
    if ( direction == cTowardDown )
    {
	if ( isfirstcol )
	{
	    const HorizonSectionTile* lefttile =
	    curtile->getNeighborTile(LEFTTILE);
	    if ( !lefttile || !useneigbors )
	    {
		nbdef10 = false;
		nbdef20 = false;
	    }
	    else
	    {
		nbdef10 = lefttile->hasDefinedCoordinates(
		highestresidx + nrroworcol - 2 );
		nbdef20 = lefttile->hasDefinedCoordinates(
		highestresidx + 2*nrroworcol - 2 );
	    }
	    nbdef12=curtile->hasDefinedCoordinates( highestresidx + 1 );
	    nbdef22=curtile->hasDefinedCoordinates(highestresidx+nrroworcol+1);
	}
	else if ( islastcol )
	{
	    const HorizonSectionTile* righttile =
		curtile->getNeighborTile( RIGHTTILE );
	    if ( !righttile || !useneigbors )
	    {
		nbdef12 = false;
		nbdef22 = false;
	    }
	    else
	    {
		nbdef12 = righttile->hasDefinedCoordinates(
		    highestresidx - nrroworcol + 2 );
		nbdef22 = righttile->hasDefinedCoordinates(
		    highestresidx + 2 );
	    }
	    nbdef10=curtile->hasDefinedCoordinates(highestresidx - 1);
	    nbdef20=curtile->hasDefinedCoordinates(highestresidx+nrroworcol-1);
	}
	else
	{
	    nbdef10=curtile->hasDefinedCoordinates( highestresidx - 1 );
	    nbdef20=curtile->hasDefinedCoordinates(highestresidx+nrroworcol-1);
	    nbdef12=curtile->hasDefinedCoordinates( highestresidx + 1 );
	    nbdef22=curtile->hasDefinedCoordinates(highestresidx+nrroworcol+1);
	}
	sum = nbdef10 + nbdef20 + nbdef12 + nbdef22;
    }
    else if ( direction == cTowardRight )
    {
	if ( isfirstrow )
	{
	    const HorizonSectionTile* uptile=curtile->getNeighborTile(UPTILE);
	    if ( !uptile || !useneigbors )
	    {
		nbdef01 = false;
		nbdef02 = false;
	    }
	    else
	    {
		const int rcsize = nrroworcol*( nrroworcol - 2 );
		nbdef01=uptile->hasDefinedCoordinates( rcsize + highestresidx );
		nbdef02=uptile->hasDefinedCoordinates( rcsize+highestresidx+1 );
	    }
	    nbdef21=curtile->hasDefinedCoordinates(highestresidx+nrroworcol);
	    nbdef22=curtile->hasDefinedCoordinates(highestresidx+nrroworcol+1);
	}
	else if ( islastrow )
	{
	    const HorizonSectionTile* bottomtile =
		curtile->getNeighborTile( BOTTOMTILE );
	    if ( !bottomtile || !useneigbors )
	    {
		nbdef21 = 0;
		nbdef22 = 0;
	    }
	    else
	    {
		nbdef21=bottomtile->hasDefinedCoordinates( curcol+nrroworcol );
		nbdef22=bottomtile->hasDefinedCoordinates(curcol+nrroworcol+1);
	    }
	    nbdef01=curtile->hasDefinedCoordinates(highestresidx-nrroworcol);
	    nbdef02=curtile->hasDefinedCoordinates(highestresidx-nrroworcol+1);
	}
	else
	{
	    nbdef01=curtile->hasDefinedCoordinates(highestresidx-nrroworcol);
	    nbdef02=curtile->hasDefinedCoordinates(highestresidx-nrroworcol+1);
	    nbdef21=curtile->hasDefinedCoordinates(highestresidx+nrroworcol);
	    nbdef22=curtile->hasDefinedCoordinates(highestresidx+nrroworcol+1);
	}
	sum = nbdef01 + nbdef02 + nbdef21 + nbdef22;
    }
    else
    {
	pErrMsg( "No implementation for other directions." );
	return false;
    }

    return sum == 0 ? true : false;
}


void TileResolutionData::buildOsgGeometres()
{
    for ( int idx = Triangle; idx<= WireFrame; idx++) // 4 type geometries
    {
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
	osgswitch_->addChild( geode );
	geom->setVertexArray( osgvertices_ );
	geom->setDataVariance( osg::Object::DYNAMIC );
	geode->addDrawable( geom );
	osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
	lightmodel->setTwoSided( true );
	geom->getOrCreateStateSet()->setAttributeAndModes( lightmodel.get() );
	geodes_->addUserObject( geode );
    }

    osgswitch_->setAllChildrenOff();
    osgswitch_->setValue( Triangle, true );
    osgswitch_->setValue( Line, true );

    buildTraingleGeometry( Triangle );
    buildLineGeometry( Line, 2 );
    buildLineGeometry( WireFrame, 1 );
    buildPointGeometry( Point );

    createPrimitiveSets();
    refOsgPrimitiveSets();
}


void TileResolutionData::setWireframeColor( OD::Color& color)
{
    sCast(osg::Vec4Array*,linecolor_)->clear();
    sCast(osg::Vec4Array*,linecolor_)->push_back( Conv::to<osg::Vec4>(color) );
    dirtyGeometry();
}


void TileResolutionData::setLineWidth( int width )
{
    osg::Geode* linegeode = getOSGGeode( geodes_, Line );
    if ( !linegeode ) return;

    osg::Geometry* linegeom = getOSGGeometry( linegeode );
    if ( !linegeom ) return;

    osg::ref_ptr<osg::LineWidth> linewidth = new osg::LineWidth;
    linewidth->setWidth( width );
    linegeom->getStateSet()->setAttributeAndModes( linewidth );

    dirtyGeometry( Line );
}


void TileResolutionData::refOsgPrimitiveSets()
{
    refOsgPtr( trianglesps_ );
    refOsgPtr( linesps_ );
    refOsgPtr( pointsps_ );
    refOsgPtr( wireframesps_ );
}


void TileResolutionData::unRefOsgPrimitiveSets()
{
    unRefAndZeroOsgPtr( trianglesosgps_ );
    unRefAndZeroOsgPtr( linesosgps_ );
    unRefAndZeroOsgPtr( pointsosgps_ );
    unRefAndZeroOsgPtr( wireframesosgps_ );
    unRefAndZeroOsgPtr( trianglesps_ );
    unRefAndZeroOsgPtr( linesps_ );
    unRefAndZeroOsgPtr( pointsps_ );
    unRefAndZeroOsgPtr( wireframesps_ );
}


void TileResolutionData::createPrimitiveSets()
{
   trianglesps_=  new osg::DrawElementsUShort( GL_TRIANGLE_STRIP,0 );
   linesps_ =  new osg::DrawElementsUShort( GL_LINES, 0 );
   pointsps_ =  new osg::DrawElementsUShort( GL_POINTS ,0 );
   wireframesps_ =  new osg::DrawElementsUShort( GL_LINES ,0 );
}


void TileResolutionData::buildLineGeometry( int idx, int width )
{
    osg::Geode* linegeode = getOSGGeode( geodes_, idx );
    if ( !linegeode ) return;

    osg::ref_ptr<osg::LineWidth> linewidth = new osg::LineWidth;
    mGetOsgVec4Arr( linecolor_ )->push_back( osg::Vec4d( 1, 1, 1, 0 ) );
    linewidth->setWidth( width );
    osg::Geometry* linegeom = getOSGGeometry( linegeode );
    linegeom->setColorArray( mGetOsgVec4Arr( linecolor_ ) );
    linegeom->setColorBinding( osg::Geometry::BIND_OVERALL );
    osg::ref_ptr<osg::Vec3Array> linenormal = new osg::Vec3Array;
    linenormal->push_back( osg::Vec3 ( 0.0f,-1.0f,0.0f ) );
    linegeom->setNormalArray( linenormal.get() );
    linegeom->setNormalBinding( osg::Geometry::BIND_OVERALL );
    linegeom->getStateSet()->setAttributeAndModes( linewidth );
    linegeom->getStateSet()->setMode( GL_LIGHTING,osg::StateAttribute::OFF );
}


void TileResolutionData::buildTraingleGeometry( int idx )
{
    osg::Geode* trainglegeode = getOSGGeode( geodes_, idx );
    if ( !trainglegeode ) return;

    osg::Geometry* geom = getOSGGeometry( trainglegeode );
    auto* cnormals = sCast(const osg::Vec3Array*,normals_);
    geom->setNormalArray( cCast(osg::Vec3Array*,cnormals) );
    geom->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
}


void TileResolutionData::buildPointGeometry( int idx )
{
    osg::Geode* pointgeode = getOSGGeode( geodes_, idx );
    if ( !pointgeode ) return;

    osg::Geometry* geom = getOSGGeometry( pointgeode );
    osg::ref_ptr<osg::Point> point=new osg::Point;
    point->setSize( 4 );
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    normals->push_back( osg::Vec3(0.0f,-1.0f,0.0f) );
    geom->setNormalArray(normals, osg::Array::BIND_OVERALL );
    geom->getStateSet()->setAttributeAndModes( point );
    pointgeode->setCullingActive( false );
}


const osg::PrimitiveSet*
	TileResolutionData::getPrimitiveSet( GeometryType type ) const
{
    const osg::Geode* geode = getOSGGeode( geodes_, Triangle );
    if ( !geode ) return 0;

    const osg::Geometry* geom = getOSGGeometry( geode );
    if ( !geom || geom->getNumPrimitiveSets()==0 )
    {
	if ( trianglesps_ && trianglesps_->size()>0 )
	    return trianglesps_;
	return 0;
    }
    return geom->getPrimitiveSet( 0 );
}

} // namespace visBase
