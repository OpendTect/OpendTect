    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "vishortileresolutiondata.h"
#include "vishorizonsection.h"
#include "vishorizonsectiontile.h"
#include "vishorizonsectiondef.h"

#include "binidsurface.h"
#include "survinfo.h"
#include "viscoord.h"

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


using namespace visBase;


#define mGetOsgGeode(dtcntr,idx) ( (osg::Geode*) dtcntr->getUserObject( idx ) )

#define mGetOsgGeometry(ptr)( ( osg::Geometry* )ptr->getDrawable( 0 ) )


TileResolutionData::TileResolutionData( const HorizonSectionTile* sectile, 
    char resolution )
    : vertices_( Coordinates::create() )
    , osgswitch_( new osg::Switch )
    , normals_( new osg::Vec3Array )
    , needsretesselation_( cMustRetesselate )
    , allnormalsinvalid_( true ) 
    , sectile_( sectile )
    , resolution_( resolution )
    , nrdefinedvertices_( 0 )
    , updateprimitiveset_( true )
    , txcoords_( new osg::Vec2Array )
    , linecolor_( new osg::Vec4Array )
    , geodes_( new osg::DefaultUserDataContainer )
    , trianglesosgps_( 0 )
    , linesosgps_( 0 )
    , pointsosgps_( 0 )
    , wireframesosgps_( 0 )
    , cosanglexinl_( cos(SI().computeAngleXInl()) )
    , sinanglexinl_( sin(SI().computeAngleXInl()) )
    , needsetposition_( true )

{
    geodes_->ref();
    vertices_->ref();
    const HorizonSection& section = sectile_->hrsection_;
    const int spacing = section.spacing_[resolution_];
    if ( spacing>=0 )
    {
	nrverticesperside_ = spacing == 1 ? 
	    (int)section.nrcoordspertileside_/spacing :
	(int)section.nrcoordspertileside_/spacing +1 ;
    }
    buildOsgGeometres();
    initVertices();
    bbox_.init(); 
};


TileResolutionData::~TileResolutionData()
{
    unRefOsgPrimitiveSets();
    geodes_->unref();
    vertices_->unRef();
}


void TileResolutionData::setTexture( const unsigned int unit, 
    osg::Array* tcarr, osg::StateSet* stateset )
{
    const unsigned int dispgeometrytype = sectile_->dispgeometrytype_; 
    if (dispgeometrytype <0 || dispgeometrytype > geodes_->getNumUserObjects()) 
	return;

    osg::Geode* geode = mGetOsgGeode( geodes_, dispgeometrytype );
    if ( !geode ) return;

    osg::Geometry* geom = mGetOsgGeometry( geode );
    if ( geom )
    {
	geom->setTexCoordArray( unit, mGetOsgVec2Arr(tcarr) );
	geode->setStateSet( stateset );
    }
}


void TileResolutionData::setDisplayGeometryType( int dispgeometrytype )
{
    osgswitch_->setAllChildrenOff();
    osgswitch_->setValue( Triangle, true );
    if ( dispgeometrytype != Triangle ) 
    {
	osgswitch_->setValue( dispgeometrytype, true );
    }
}


void TileResolutionData::setAllVertices( const TypeSet<Coord3>& positions )
{
    const HorizonSection& hrsection = sectile_->hrsection_;
    const int spacing = hrsection.spacing_[resolution_];
    const int nrcoords = hrsection.nrcoordspertileside_;
    int crdidx = 0;
    bbox_.init();

    for ( int row=0; row<nrcoords; row+=spacing )
    {
	for ( int col=0; col<nrcoords; col+=spacing )
	{
	    int coordIdx = col + row*nrcoords;
	    Coord3 vertex = positions[coordIdx];
	    if ( coordIdx >= positions.size() || !vertex.isDefined() )
	    {
		vertex[2] = mUdf(float);
	    }
	    else
		nrdefinedvertices_ ++;
	    vertices_->setPos( crdidx, vertex );
	    const osg::Vec3Array* arr = 
		dynamic_cast<osg::Vec3Array*>(vertices_->osgArray());
	    const osg::Vec3f coord = arr->at( crdidx );
	    if( vertex[2] != mUdf(float) )
		bbox_.expandBy( coord );
	    crdidx++;
	}
    }
}


void TileResolutionData::calcNormals( bool allownormalinvalid )
{
    const HorizonSection& hrsection = sectile_->hrsection_;

    int valididx = 0;

    if ( allownormalinvalid )
    {
	const int normalstop = resolution_ < hrsection.lowestresidx_ ? 
	    hrsection.normalstartidx_[resolution_+1]-1 :
	hrsection.totalnormalsize_-1;
	for ( int idx=hrsection.normalstartidx_[resolution_]; 
	    idx<=normalstop; idx++ )
	{
	    valididx = idx - hrsection.normalstartidx_[resolution_];
	    computeNormal( idx, (*mGetOsgVec3Arr(normals_))[valididx] );
	}
    }
    else
    {
	const int sz = invalidnormals_.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    computeNormal( invalidnormals_[idx],
		(*mGetOsgVec3Arr(normals_))[invalidnormals_[idx]]);
	}
    }

}


void TileResolutionData::setDisplayTransformation( const mVisTrans* t )
{ vertices_->setDisplayTransformation( t ); }


osg::BoundingBox& TileResolutionData::updateBBox()
{
    bbox_.init();
    bbox_.expandBy( osgswitch_->computeBound() );
    return bbox_;
}

#define mIsOsgDef( pos ) (pos[2]<9.9e29)

void TileResolutionData::setSingleVertex( int row, int col, 
    const Coord3& pos, bool& dohide )
{
    if ( row<0 || row>nrverticesperside_-1 ||
	 col<0 || col>nrverticesperside_-1  )
	return;

    const HorizonSection& section = sectile_->hrsection_;

    const int coordidx = row*nrverticesperside_ + col;

    Coord3 orgcrd = vertices_->getPos( coordidx );
    if ( pos == vertices_->getPos( coordidx) )
	return;

    osg::Vec3Array* arr = mGetOsgVec3Arr( vertices_->osgArray() );

    const bool olddefined = mIsOsgDef((*arr)[coordidx]);
    const bool newdefined = pos.isDefined();

    if ( !olddefined && !newdefined )
	return;

    if ( !newdefined )
    {
	(*arr)[coordidx][2] =  1e30;
    }
    else
    {
	Coord3 crd;
	mVisTrans::transform( section.transformation_, pos, crd );
	(*arr)[coordidx] = osg::Vec3d( crd[0], crd[1], crd[2] );
	bbox_.expandBy((*arr)[coordidx]);
    }

    char newstatus = cShouldRetesselate;
    if ( olddefined && !newdefined )
    {
	newstatus = cMustRetesselate;
	dohide = true;
    }

    if ( newdefined && !olddefined )
	nrdefinedvertices_ ++;
    else if ( !newdefined && olddefined )
	nrdefinedvertices_--;

    if ( newstatus>needsretesselation_ ) 
	needsretesselation_ = newstatus;

    setInvalidNormal( row, col );
}


#define mClearPrimitiveSet\
    trianglesps_->clear();\
    wireframesps_->clear();\
    linesps_->clear();\
    pointsps_->clear();\


bool TileResolutionData::tesselateResolution( bool onlyifabsness )
{
   if ( needsetposition_ )
   {
     if ( !setVerticesFromHighestResolution() )
	 return false;
   }
    
    if ( resolution_<0 || needsretesselation_==cNoTesselationNeeded ||
	( needsretesselation_==cShouldRetesselate && onlyifabsness ) )
	return false;

    mClearPrimitiveSet;

    updateprimitiveset_ = false;

    tesselatemutex_.lock();

    for ( int idxthis=0; idxthis<vertices_->size(); idxthis++ )
    {
	if ( vertices_->isDefined( idxthis ) )
	    tesselateCell( idxthis );
    }

    tesselatemutex_.unLock();

    updateprimitiveset_ = true;
    needsretesselation_ = cNoTesselationNeeded;

    return true;
}


bool TileResolutionData::setVerticesFromHighestResolution()
{
    HorizonSectionTile* tile = const_cast<HorizonSectionTile*>( sectile_ );
    const visBase::Coordinates* highestrescoords = 
	tile->getHighestResolutionCoordinates();

    if( !tile || !highestrescoords) return false;

    const RefMan<const Transformation> trans = 
	sectile_->hrsection_.transformation_;

    vertices_->setDisplayTransformation( trans );
    const HorizonSection& hrsection = sectile_->hrsection_;
    const int spacing = hrsection.spacing_[resolution_];
    const int nrcoords = hrsection.nrcoordspertileside_;
    int crdidx = 0;
    bbox_.init();

    for ( int row=0; row<nrcoords; row+=spacing )
    {
	for ( int col=0; col<nrcoords; col+=spacing )
	{
	    int coordIdx = col + row*nrcoords;
	    Coord3 vertex = highestrescoords->getPos( coordIdx );
	    if ( coordIdx >= highestrescoords->size() || !vertex.isDefined() )
	    {
		vertex[2] = mUdf(float);
	    }
	    else
		nrdefinedvertices_ ++;
	    vertices_->setPos( crdidx, vertex );
	    const osg::Vec3Array* arr = 
		dynamic_cast<osg::Vec3Array*>(vertices_->osgArray());
	    const osg::Vec3f coord = arr->at( crdidx );
	    if( vertex[2] != mUdf(float) )
		bbox_.expandBy( coord );
	    crdidx++;
	}
    }

    needsretesselation_ = cMustRetesselate;
    allnormalsinvalid_ = true;
    invalidnormals_.erase();
    needsetposition_ = false;

    return true;

}

void TileResolutionData::setPrimitiveSet( unsigned int geometrytype, 
    osg::DrawElementsUShort* geomps )
{
    if(!geomps || geometrytype <0 || geometrytype>geodes_->getNumUserObjects())
	return;

    osg::Geode* geode = mGetOsgGeode( geodes_, geometrytype );
    if( !geode ) return;

    osg::Geometry* geom = mGetOsgGeometry( geode );
    if( !geom ) return;

    if ( geomps->size() )
    {
	geom->removePrimitiveSet( 0, geom->getNumPrimitiveSets() );
	geom->addPrimitiveSet( geomps );
    }
}


#define mSetOsgPrimitiveSet( geomtype,geom )\
if( geomtype##ps_->size() )\
{\
    geomtype##osgps_ = new osg::DrawElementsUShort( *geomtype##ps_ );\
    setPrimitiveSet( geom, geomtype##osgps_ );\
}\


void TileResolutionData::updatePrimitiveSets()
{
    if ( !updateprimitiveset_ )
	return;

#   ifdef __debug__
    if ( !DataObject::isVisualizationThread() )
    {
	pErrMsg("Not in visualization thread");
    }
#   endif

    mSetOsgPrimitiveSet( triangles, Triangle );
    mSetOsgPrimitiveSet( lines, Line );
    mSetOsgPrimitiveSet( points, Point );
    mSetOsgPrimitiveSet( wireframes, WireFrame );

    mClearPrimitiveSet;
    updateprimitiveset_ = false;
}


#define mAddPointIndex( geomps, idx ) { geomps->push_back( idx ); }

#define mAddLineIndexes( geomps, idx1, idx2 )\
{\
    mAddPointIndex( geomps, idx1 );\
    mAddPointIndex( geomps, idx2 );\
}

#define mAddClockwiseTriangleIndexes( geomps, idx0, idxa, idxb ) \
{ \
    const int pssize = geomps->getNumIndices(); \
    const int idx1 = pssize%2 ? idxa : idxb; \
    const int idx2 = pssize%2 ? idxb : idxa; \
    bool continuestrip = pssize > 2; \
    if ( continuestrip ) \
    { \
	const int lastidx = geomps->index( pssize-1 ); \
	if ( lastidx==idx0 && geomps->index(pssize-2)==idx1 ) \
    	    geomps->push_back( idx2 ); \
	else if ( lastidx==idx1 && geomps->index(pssize-2)==idx2 ) \
    	    geomps->push_back( idx0 ); \
	else if ( lastidx==idx2 && geomps->index(pssize-2)==idx0 ) \
    	    geomps->push_back( idx1 ); \
	else \
	{ \
	    continuestrip = false; \
	    geomps->push_back( lastidx ); \
	    geomps->push_back( idx0 ); \
	} \
    } \
    if ( !continuestrip ) \
    { \
	geomps->push_back( idx0 ); \
	geomps->push_back( idx1 ); \
	geomps->push_back( idx2 ); \
    } \
}


void TileResolutionData::tesselateCell( int idxthis )
{
    const HorizonSection& section = sectile_->hrsection_;
    const int spacing = section.spacing_[resolution_];
    const int nrroworcol = section.nrcoordspertileside_/spacing;
    const int resnrroworcol = resolution_ == 0 ? nrroworcol : nrroworcol +1 ;
    const bool usewireframe = sectile_->usewireframe_;

    const int idxright = idxthis + 1;
    const int idxbottom = idxthis + resnrroworcol;
    const int idxrightbottom = idxthis + resnrroworcol + 1;

    bool rightisdef = vertices_->isDefined( idxright );  
    const bool bottomisdef = vertices_->isDefined( idxbottom ); 
    bool rightbottomisdef = vertices_->isDefined( idxrightbottom );

    const bool atright = ( idxthis+1 ) % resnrroworcol == 0;
    if ( atright )
	rightisdef = rightbottomisdef = false; 

    if ( !rightisdef )
    {
	if ( !bottomisdef )
	{
	    mAddPointIndex( pointsps_, idxthis ); 
	}
	else if ( bottomisdef )
	{
	    if ( !rightbottomisdef )
	    {
		mAddLineIndexes( linesps_,idxthis, idxbottom );
		if ( usewireframe )
		    mAddLineIndexes( wireframesps_,idxthis, idxbottom  );
	    }
	    else 
	    {
		mAddClockwiseTriangleIndexes( trianglesps_, idxthis,
					      idxrightbottom, idxbottom );
		mAddLineIndexes( linesps_, idxthis, idxbottom );
		if ( usewireframe )
		    mAddLineIndexes( wireframesps_, idxthis, idxbottom );
	    }

	}
    }
    else if ( rightisdef )
    {
	if ( bottomisdef )
	{
	    if ( rightbottomisdef ) 
	    {
		mAddClockwiseTriangleIndexes( trianglesps_, idxthis,
					      idxright, idxbottom );
		mAddClockwiseTriangleIndexes( trianglesps_, idxbottom, 
					      idxright, idxrightbottom );
		mAddLineIndexes( linesps_, idxthis, idxright );
		mAddLineIndexes( linesps_, idxthis, idxbottom );
		if ( usewireframe )
		{
		    mAddLineIndexes( wireframesps_, idxthis, idxright );
		    mAddLineIndexes( wireframesps_, idxthis, idxbottom );
		}
	    }
	    else if ( !rightbottomisdef ) 
	    {
		mAddClockwiseTriangleIndexes( trianglesps_, idxthis, 
					      idxright, idxbottom );
		if ( usewireframe )
		{
		    mAddLineIndexes( wireframesps_, idxthis, idxright );
		    mAddLineIndexes( wireframesps_, idxthis, idxbottom );
		}
		mAddLineIndexes( linesps_, idxthis, idxright );
		mAddLineIndexes( linesps_, idxthis, idxbottom );
	    }
	}
	else if ( !bottomisdef )
	{
	    if ( rightbottomisdef ) 
	    {
		mAddClockwiseTriangleIndexes( trianglesps_, idxthis,
					      idxright, idxrightbottom );
		mAddLineIndexes( linesps_,idxthis, idxright );
		if ( usewireframe )
		    mAddLineIndexes( wireframesps_, idxthis, idxright );
	    }
	    else 
	    {
		mAddLineIndexes( linesps_,idxthis, idxright );
		if ( usewireframe )
		    mAddLineIndexes( wireframesps_, idxthis, idxright );
	    }
	}
    }
}


#define mExtractPosition(flag,location) \
{ \
    const Coord3 thispos = section.geometry_->getKnot(arrpos,false); \
    if ( thispos.isDefined() ) \
    { \
	Coord3 crd; \
	mVisTrans::transform( section.transformation_, thispos, crd ); \
	location##pos.x = ( flag ? -1 : 1 ) * idx * rcdist; \
	location##pos.y = crd.z; \
	location##found = true; \
    } \
}


double TileResolutionData::calcGradient( int row, int col,
    const StepInterval<int>& rcrange, bool isrow )
{
    const HorizonSection& section = sectile_->hrsection_;
    const float rcdist = isrow ? section.rowdistance_ : section.coldistance_;
    const int rc = isrow ? row : col;
    bool beforefound = false; bool afterfound = false;
    Coord beforepos, afterpos;

    for ( int idx=section.spacing_[resolution_]; idx>=0; idx-- ) 
    { 
	if ( !beforefound ) 
	{ 
	    const int currc = rc - idx*rcrange.step;
	    if ( currc >= rcrange.start )
	    {
		const RowCol arrpos = isrow ? 
		    RowCol( currc, col ) : RowCol( row, currc );
		mExtractPosition( true, before );
	    }
	} 

	if ( idx>0 && !afterfound ) 
	{ 
	    const int currc = rc + idx*rcrange.step; 
	    if ( currc <= rcrange.stop )
	    {
		const RowCol arrpos = isrow ? 
		    RowCol( currc, col ) : RowCol( row, currc );
		mExtractPosition( false, after );
	    }
	} 

	if ( afterfound && beforefound )
	    break;
    }

    return !afterfound || !beforefound ? 0
	: (afterpos.y-beforepos.y)/(afterpos.x-beforepos.x);
}


void TileResolutionData::computeNormal( int nmidx,osg::Vec3& normal ) 
{
    const HorizonSection& section = sectile_->hrsection_;
    const RowCol origin_ = sectile_->origin_;

    if ( !section.geometry_ )
	return;

    RowCol step;
    if ( section.userchangedisplayrg_ )
	step = RowCol( section.displayrrg_.step, section.displaycrg_.step );
    else
	step = RowCol( section.geometry_->rowRange().step,
	section.geometry_->colRange().step );

    const int normalrow = ( nmidx-section.normalstartidx_[resolution_] )/
	section.normalsidesize_[resolution_];
    const int normalcol = ( nmidx-section.normalstartidx_[resolution_] )%
	section.normalsidesize_[resolution_];

    const int row = origin_.row() + step.row() * normalrow*
	section.spacing_[resolution_];
    const int col = origin_.col() + step.col() * normalcol*
	section.spacing_[resolution_];

    const double drow=calcGradient(row,col,section.geometry_->rowRange(),true );
    const double dcol=calcGradient(row,col,section.geometry_->colRange(),false);

    normal[0] = drow*cosanglexinl_+dcol*sinanglexinl_;
    normal[1] = dcol*cosanglexinl_-drow*sinanglexinl_;
    normal[2] = -1; 
}


void TileResolutionData::buildOsgGeometres()
{
    for ( int idx = Triangle; idx< WireFrame; idx++) // 4 type geometries
    {
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
	osgswitch_->addChild( geode );
	geom->setVertexArray( vertices_->osgArray() );
	geom->setDataVariance( osg::Object::DYNAMIC );
	geode->addDrawable( geom );
	osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
	lightmodel->setTwoSided( true );
	geom->getOrCreateStateSet()->setAttributeAndModes( lightmodel.get() );
	geodes_->addUserObject( geode );
    }

    buildTraingleGeometry( Triangle );
    buildLineGeometry( Line );
    buildLineGeometry( WireFrame );
    buildPointGeometry( Point );

    createPrimitiveSets();
    refOsgPrimitiveSets();

}


void TileResolutionData::initVertices()
{
    const int coordsize = nrverticesperside_*nrverticesperside_;
    osg::Vec3Array* normals = mGetOsgVec3Arr( normals_ );
    normals->resize( coordsize );
    std::fill( normals->begin(), normals->end(), osg::Vec3f( 0, 0, -1 ) );
    mGetOsgVec2Arr(txcoords_)->resize( coordsize );
}

void TileResolutionData::setLineColor( Color& color)
{
    mGetOsgVec4Arr( linecolor_ )->clear();
    mGetOsgVec4Arr( linecolor_ )->push_back( Conv::to<osg::Vec4>( color ) );

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
    unRefOsgPtr( trianglesps_ ); 
    unRefOsgPtr( linesps_ ); 
    unRefOsgPtr( pointsps_ ); 
    unRefOsgPtr( wireframesps_ );
}


void TileResolutionData::createPrimitiveSets()
{
   trianglesps_=  new osg::DrawElementsUShort( GL_TRIANGLE_STRIP,0 );
   linesps_ =  new osg::DrawElementsUShort( GL_LINES, 0 );
   pointsps_ =  new osg::DrawElementsUShort( GL_POINTS ,0 );
   wireframesps_ =  new osg::DrawElementsUShort( GL_LINES ,0 );
}


void TileResolutionData::buildLineGeometry( int idx ) 
{
    osg::Geode* linegeode = mGetOsgGeode( geodes_, idx );
    if ( ! linegeode ) return;

    osg::ref_ptr<osg::LineWidth> linewidth = new osg::LineWidth;
    mGetOsgVec4Arr( linecolor_ )->push_back( osg::Vec4d( 1, 1, 1, 0 ) );
    linewidth->setWidth(1.0);
    osg::Geometry* linegeom = mGetOsgGeometry( linegeode );
    linegeom->setColorArray( mGetOsgVec4Arr( linecolor_ ) );
    linegeom->setColorBinding( osg::Geometry::BIND_OVERALL );
    osg::ref_ptr<osg::Vec3Array> linenormal = new osg::Vec3Array;
    linenormal->push_back( osg::Vec3 ( 0.0f,-1.0f,0.0f ) );
    linegeom->setNormalArray( linenormal.get() );
    linegeom->setNormalBinding( osg::Geometry::BIND_OVERALL );
    linegeom->getStateSet()->setAttributeAndModes( linewidth );
    linegeom->getStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
}


void TileResolutionData::buildTraingleGeometry( int idx )
{
    osg::Geode* trainglegeode = mGetOsgGeode( geodes_, idx );
    if ( !trainglegeode ) return;

    osg::Geometry* geom = mGetOsgGeometry( trainglegeode );
    geom->setNormalArray( mGetOsgVec3Arr( normals_ ) );
    geom->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
}


void TileResolutionData::buildPointGeometry( int idx )
{
    osg::Geode* pointgeode = mGetOsgGeode( geodes_, idx );
    if ( !pointgeode ) return;

    osg::Geometry* geom = mGetOsgGeometry( pointgeode );
    osg::ref_ptr<osg::Point> point=new osg::Point;
    point->setSize( 4 );
    geom->getStateSet()->setAttributeAndModes( point );
}


void TileResolutionData::setInvalidNormal( int row, int col )
{
    if ( row<0 || row>nrverticesperside_-1 ||
	col<0 || col>nrverticesperside_-1  )
	return;

    if ( allnormalsinvalid_ )
	return;

    const HorizonSection& hrsection = sectile_->hrsection_;
    const int spacing = hrsection.spacing_[resolution_];

    int rowstart = row-spacing;
    if ( rowstart>( nrverticesperside_ - 1 ) ) 
	return;
    if ( rowstart<0 )
	rowstart = 0;

    int rowstop = row+spacing;
    if ( rowstop<0 ) 
	return;
    if ( rowstop>( nrverticesperside_ - 1 ) )
	rowstop = nrverticesperside_ - 1;

    for ( int rowidx=rowstart; rowidx<=rowstop; rowidx++ )
    {
	if ( ( rowidx%spacing ) && ( rowidx!=nrverticesperside_-1 ) ) 
	    continue;

	const int nmrow = rowidx==nrverticesperside_-1 
	    ? nrverticesperside_-1 
	    : rowidx/spacing;

	int colstart = col-spacing;
	if ( colstart>nrverticesperside_ -1 ) continue;
	if ( colstart<0 ) colstart = 0;

	int colstop = col+spacing;
	if ( colstop<0 ) continue;
	if ( colstop>nrverticesperside_-1 ) 
	    colstop = nrverticesperside_-1;

	int colstartni = hrsection.normalstartidx_[resolution_] + 
	    nmrow * hrsection.normalsidesize_[resolution_];
	for ( int colidx=colstart; colidx<=colstop; colidx++ )
	{
	    if ( (colidx%spacing) && 
		(colidx!=nrverticesperside_-1) )
		continue;
	    const int nmcol = colidx==nrverticesperside_-1 
		? hrsection.normalsidesize_[resolution_]-1
		: colidx/spacing;
	    invalidnormals_.addIfNew( colstartni + nmcol );
	}
    }

}
