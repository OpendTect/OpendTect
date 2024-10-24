/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visgeomindexedshape.h"

#include "coltab.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "indexedshape.h"
#include "posvecdataset.h"
#include "survinfo.h"

#include "viscoord.h"
#include "visnormals.h"
#include "vispolygonoffset.h"
#include "vispolyline.h"
#include "vistexturecoords.h"

#include <osg/Geometry>
#include <osg/Geode>
#include <osg/UserDataContainer>
#include <osg/LightModel>


#define mNrMaterials		256
#define mNrMaterialSteps	255
#define mUndefMaterial		255

mCreateFactoryEntry( visBase::GeomIndexedShape );

namespace visBase
{

GeomIndexedShape::GeomIndexedShape()
    : VisualObjectImpl(true)
    , colorhandler_(new ColorHandler)
    , linestyle_(OD::LineStyle::Solid,2,OD::Color(0,255,0))
{
    ref();
    singlematerial_ = Material::create();
    coltabmaterial_ = Material::create();
    vtexshape_ = VertexShape::create();
    addChild( vtexshape_->osgNode() );

    vtexshape_->setMaterial( singlematerial_.ptr() );
    singlematerial_->setColorMode( Material::Off );
    coltabmaterial_->setColorMode( Material::Diffuse );
    vtexshape_->setPrimitiveType( Geometry::PrimitiveSet::Triangles );
    vtexshape_->enableCoordinatesChangedCB( false );

    setRenderMode( RenderBothSides );

    RefMan<Material> newmat = Material::create();
    setMaterial( newmat.ptr() );
    setDataSequence( ColTab::Sequence(ColTab::defSeqName()) );
    unRefNoDelete();
}


GeomIndexedShape::~GeomIndexedShape()
{
    detachAllNotifiers();
    delete colorhandler_;
}


GeomIndexedShape::ColorHandler::ColorHandler()
    : attributecache_(0)
{
    material_ = Material::create();
}


GeomIndexedShape::ColorHandler::~ColorHandler()
{
}


void GeomIndexedShape::setRenderMode( RenderMode mode )
{
    vtexshape_->setRenderMode( mode );
}


void GeomIndexedShape::setMaterial( Material* mat )
{
    if ( !vtexshape_ )
	return;

    if ( material_ )
	mDetachCB( material_->change, GeomIndexedShape::matChangeCB );

    if ( !mat )
	return;

    VisualObjectImpl::setMaterial( mat );
    if ( material_ )
	mAttachCB( material_->change, GeomIndexedShape::matChangeCB );

    colorhandler_->material_->setPropertiesFrom( *mat );
}


void GeomIndexedShape::updateMaterialFrom( const Material* mat )
{
    if ( !mat ) return;

    singlematerial_->setFrom( *mat );

    if ( isColTabEnabled() && colorhandler_ )
    {
	colorhandler_->material_->setPropertiesFrom( *mat );
	mapAttributeToColorTableMaterial();
    }
    enableColTab( colortableenabled_ );
}


void GeomIndexedShape::matChangeCB( CallBacker* )
{
    updateMaterialFrom( getMaterial() );
}


void GeomIndexedShape::updateGeometryMaterial()
{
    if ( getMaterial() )
    {
	colorhandler_->material_->setPropertiesFrom( *getMaterial() );
	mapAttributeToColorTableMaterial();
	vtexshape_->setColorBindType( VertexShape::BIND_PER_VERTEX );
	vtexshape_->setMaterial( coltabmaterial_.ptr() );
    }
}

void GeomIndexedShape::setNormalBindType( VertexShape::BindType type )
{
    if ( vtexshape_ )
	vtexshape_->setNormalBindType( type );
}


void GeomIndexedShape::setColorBindType( VertexShape::BindType type )
{
    if ( vtexshape_ )
	vtexshape_->setColorBindType( type );
}


void GeomIndexedShape::addNodeState( NodeState* ns )
{
    if ( vtexshape_ )
	vtexshape_->addNodeState( ns );
}


void GeomIndexedShape::enableColTab( bool yn )
{
    if ( !vtexshape_->getMaterial() ) return;

    if ( yn )
    {
	    setColorBindType( VertexShape::BIND_PER_VERTEX );
	    setMaterial( coltabmaterial_.ptr() );
	    vtexshape_->setMaterial( coltabmaterial_.ptr() );
    }
    else
    {
	setColorBindType( VertexShape::BIND_OVERALL );
	setMaterial( singlematerial_.ptr() );
	vtexshape_->setMaterial( singlematerial_.ptr() );
    }

    VisualObjectImpl::materialChangeCB( nullptr );
    colortableenabled_  = yn;
}


bool GeomIndexedShape::isColTabEnabled() const
{
    return colortableenabled_;
}


void GeomIndexedShape::setDataMapper( const ColTab::MapperSetup& setup,
				      TaskRunner* taskr )
{
    if ( setup!=colorhandler_->mapper_.setup_ )
    {
	colorhandler_->mapper_.setup_ = setup;
	reClip();
	updateGeometryMaterial();
    }
}


const ColTab::MapperSetup* GeomIndexedShape::getDataMapper() const
{
    return colorhandler_ ? &colorhandler_->mapper_.setup_ : 0;
}


void GeomIndexedShape::setDataSequence( const ColTab::Sequence& seq )
{
    if ( seq!=colorhandler_->sequence_ )
    {
	colorhandler_->sequence_ = seq;
	TypeSet<OD::Color> colors;
	for ( int idx=0; idx<mNrMaterialSteps; idx++ )
	{
	    const float val = ( (float) idx )/( mNrMaterialSteps-1 );
	    const OD::Color col = seq.color( val );
	    colors += col;
	}

	colors += seq.undefColor();
	colorhandler_->material_->setColors( colors, false );
    }

   if ( isColTabEnabled() )
	updateGeometryMaterial();
}


const ColTab::Sequence* GeomIndexedShape::getDataSequence() const
{
    return colorhandler_ ? &colorhandler_->sequence_ : 0;
}


void GeomIndexedShape::setDisplayTransformation( const mVisTrans* nt )
{
    if ( !useosgnormal_ && vtexshape_->getNormals() )
    {
        vtexshape_->getNormals()->setDisplayTransformation( nt );
	if ( !renderside_ )
	    vtexshape_->getNormals()->inverse();
    }

    vtexshape_->setDisplayTransformation( nt );
    vtexshape_->dirtyCoordinates();
    vtexshape_->turnOn( true );

}


const mVisTrans* GeomIndexedShape::getDisplayTransformation() const
{ return vtexshape_->getDisplayTransformation(); }


void GeomIndexedShape::setSurface( Geometry::IndexedShape* ns, TaskRunner* tr )
{
    shape_ = ns;
    touch( false, true, tr );
}


bool GeomIndexedShape::touch( bool forall, bool createnew, TaskRunner* tr )
{
    if ( !shape_ )
	return false;

    if ( !shape_->needsUpdate() && createnew )
	return true;

    RefMan<Coordinates> coords;
    RefMan<Normals> normals;
    RefMan<TextureCoords> texturecoords;
    if ( createnew )
    {
	coords = Coordinates::create();
	normals = Normals::create();
	texturecoords = TextureCoords::create();
	shape_->setCoordList( new CoordListAdapter(*coords),
	    new NormalListAdapter( *normals ),
	    new TextureCoordListAdapter( *texturecoords ),createnew );
	shape_->getGeometry().erase();
    }
    else
    {
	auto* coordlist = dCast( CoordListAdapter*, shape_->coordList() );
	coords = coordlist->getCoordinates();
	auto* normallist = dCast( NormalListAdapter*,shape_->normalCoordList());
	normals = normallist->getNormals();
	auto* texturelist = dCast( TextureCoordListAdapter*,
				   shape_->textureCoordList() );
	texturecoords = texturelist->getTextureCoords();
    }

    if ( shape_->needsUpdate() && !shape_->update(forall,tr) )
	return false;

    vtexshape_->removeAllPrimitiveSets();

    coords->setDisplayTransformation( getDisplayTransformation() );

    if ( !coords->size() )
	return false;

    vtexshape_->setCoordinates( coords.ptr() );
    vtexshape_->useOsgAutoNormalComputation( true );

    if ( !useosgnormal_ && normals->nrNormals() )
    {
	normals->setDisplayTransformation( getDisplayTransformation() );
	vtexshape_->setNormals( normals.ptr() );
	vtexshape_->useOsgAutoNormalComputation( false );
    }

    if ( texturecoords->size() )
	vtexshape_->setTextureCoords( texturecoords.ptr() );

    ObjectSet<Geometry::IndexedGeometry>& geoms=shape_->getGeometry();

    if ( !geoms.size() )
	return false;

    for ( int idx=0; idx<geoms.size(); idx++ )
    {
	Geometry::IndexedGeometry* idxgeom = geoms[idx];
	if( !idxgeom || idxgeom->getCoordsPrimitiveSet()->size() == 0 )
	    continue;

	vtexshape_->addPrimitiveSet( idxgeom->getCoordsPrimitiveSet() );

	if ( idxgeom->primitivetype_ == Geometry::IndexedGeometry::Lines &&
	    (geomshapetype_==PolyLine || geomshapetype_==PolyLine3D) )
	{
	    vtexshape_->setLineStyle( linestyle_ );
	}
    }

    vtexshape_->dirtyCoordinates();

    return true;

}


bool GeomIndexedShape::getAttribPositions( DataPointSet& set,
					   mVisTrans* toinlcrltrans,
					   TaskRunner* ) const
{
    const DataColDef coordindex( sKeyCoordIndex() );
    if ( set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact)==-1 )
	set.dataSet().add( new DataColDef(coordindex) );

    const int col =
	set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact);

    const Coordinates* vtxcoords = vtexshape_->getCoordinates();
    if ( !vtxcoords || !vtxcoords->size() )
	return false;

    for ( int coordid = 0; coordid<vtxcoords->size(); coordid++ )
    {
	Coord3 pos = vtxcoords->getPos( coordid );
	if ( !pos.isDefined() )
	    continue;

	DataPointSet::Pos dpsetpos;
	if ( toinlcrltrans )
	{
	    mVisTrans::transform( toinlcrltrans, pos );
            dpsetpos.set( BinID(mNINT32(pos.x_),mNINT32(pos.y_)) );
            dpsetpos.z_ = pos.z_;
	}
	else
	{
	    const BinID bid = SI().transform( pos );
	    dpsetpos.set( bid );
            dpsetpos.z_ = pos.z_;
	}

	DataPointSet::DataRow datarow( dpsetpos, 1 );
	datarow.data_.setSize( set.nrCols(), mUdf(float) );
	datarow.data_[col-set.nrFixedCols()] =  coordid;
	set.addRow( datarow );
    }

    set.dataChanged();
    return true;
}


void GeomIndexedShape::setAttribData( const DataPointSet& set,
				      TaskRunner* taskr )
{
    const DataColDef coordindex( sKeyCoordIndex() );
    const int col =
	set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact);

    if ( col==-1 )
	return;

    const BinIDValueSet& vals = set.bivSet();
    if ( vals.nrVals()<col+1 )
	return;

    ArrayValueSeries<float,float>& cache = colorhandler_->attributecache_;
    cache.setSize( vals.totalSize() );
    cache.setAll( mUdf(float) );

    BinIDValueSet::SPos pos;
    while ( vals.next( pos ) )
    {
	const float* ptr = vals.getVals( pos );
	const int coordidx = mNINT32( ptr[col] );
	const float val = ptr[col+1];

	if ( coordidx>=cache.size() )
	{
	    int oldsz = cache.size();
	    cache.setSize( coordidx+1 );
	    if ( !cache.arr() )
		return;

	    const float udf = mUdf( float );
	    for ( int idx=oldsz; idx<=coordidx; idx++ )
		cache.setValue( idx, udf );
	}

	cache.setValue( coordidx, val );
    }

    if ( colorhandler_->mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	reClip();

    updateGeometryMaterial();
}


void GeomIndexedShape::mapAttributeToColorTableMaterial()
{
    if ( !colorhandler_ || colorhandler_->attributecache_.size()<=0 )
	return;

    TypeSet<OD::Color> colors;

    for ( int idx=0; idx<vtexshape_->getCoordinates()->size(); idx++ )
    {
	const int coloridx = ColTab::Mapper::snappedPosition(
	    &colorhandler_->mapper_,colorhandler_->attributecache_[idx],
	    mNrMaterialSteps, mUndefMaterial );

	colors.add( colorhandler_->material_->getColor(coloridx ) );
    }

    coltabmaterial_->setColors( colors, false );
    coltabmaterial_->setPropertiesFrom( *colorhandler_->material_ );
}


void GeomIndexedShape::reClip()
{
    colorhandler_->mapper_.setData( colorhandler_->attributecache_ );
}


void GeomIndexedShape::setLineStyle( const OD::LineStyle& lnstyle )
{
    if ( lnstyle == linestyle_ )
	return;

    linestyle_ = lnstyle;

    if ( vtexshape_ )
	vtexshape_->setLineStyle( lnstyle );
    else
	touch( true );
}


void GeomIndexedShape::setGeometryShapeType( GeomShapeType shapetype,
			Geometry::PrimitiveSet::PrimitiveType pstype )
{
    if ( shapetype == geomshapetype_ )
	return;

    removeChild( vtexshape_->osgNode() );
    if ( shapetype == PolyLine )
	vtexshape_ = PolyLine::create();
    else if ( shapetype == PolyLine3D )
	vtexshape_ = PolyLine3D::create();
    else
	vtexshape_ = VertexShape::create();

    vtexshape_->setMaterial( singlematerial_.ptr() );
    vtexshape_->setPrimitiveType( pstype );
    if ( shapetype==PolyLine || shapetype==PolyLine3D )
    {
	RefMan<PolygonOffset> offset = PolygonOffset::create();
	offset->setFactor( -1.0f );
	offset->setUnits( 1.0f );

	offset->setMode(
	    PolygonOffset::Protected | PolygonOffset::On );
	vtexshape_->addNodeState( offset.ptr() );
    }

    addChild( vtexshape_->osgNode() );

    geomshapetype_ = shapetype;

}

void GeomIndexedShape::useOsgNormal( bool yn )
{
     useosgnormal_ = yn;
}


void GeomIndexedShape::setTextureChannels( TextureChannels* channels )
{
    vtexshape_->setTextureChannels( channels );
}


void GeomIndexedShape::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( vtexshape_ )
	vtexshape_->setPixelDensity( dpi );

}


VertexShape* GeomIndexedShape::getVertexShape()
{
    return vtexshape_.ptr();
}


const VertexShape* GeomIndexedShape::getVertexShape() const
{
    return vtexshape_.ptr();
}

} // namespace visBase
