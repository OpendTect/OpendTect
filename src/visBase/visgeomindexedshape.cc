/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visgeomindexedshape.h"

#include "datacoldef.h"
#include "datapointset.h"
#include "indexedshape.h"
#include "posvecdataset.h"
#include "survinfo.h"

#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vispolygonoffset.h"
#include "vispolyline.h"
#include "vistexturechannels.h"
#include "vistexturecoords.h"

#include <osg/Geometry>
#include <osg/Geode>
#include <osg/UserDataContainer>
#include <osg/LightModel>

#include "vistransform.h"

#define mNrMaterials		256
#define mNrMaterialSteps	255
#define mUndefMaterial		255

mCreateFactoryEntry( visBase::GeomIndexedShape );

namespace visBase
{

GeomIndexedShape::GeomIndexedShape()
    : VisualObjectImpl( true )
    , shape_( 0 )
    , vtexshape_( VertexShape::create() )
    , colorhandler_( new ColorHandler )
    , colortableenabled_( false )
    , singlematerial_( new Material )
    , coltabmaterial_( new Material )
    , geomshapetype_( Triangle )
    , linestyle_( OD::LineStyle::Solid,2,OD::Color(0,255,0) )
    , useosgnormal_( false )
{
    singlematerial_->ref();
    coltabmaterial_->ref();
    vtexshape_->ref();
    addChild( vtexshape_->osgNode() );

    vtexshape_->setMaterial( singlematerial_ );
    singlematerial_->setColorMode( visBase::Material::Off );
    coltabmaterial_->setColorMode( visBase::Material::Diffuse );
    vtexshape_->setPrimitiveType( Geometry::PrimitiveSet::Triangles );
    vtexshape_->enableCoordinatesChangedCB( false );

    setRenderMode( RenderBothSides );

    setMaterial( new Material );
    setDataSequence( ColTab::Sequence(ColTab::defSeqName()) );
}


GeomIndexedShape::~GeomIndexedShape()
{
    detachAllNotifiers();
    unRefAndNullPtr( singlematerial_ );
    unRefAndNullPtr( coltabmaterial_ );

    delete colorhandler_;
    unRefAndNullPtr( vtexshape_ );
}


GeomIndexedShape::ColorHandler::ColorHandler()
    : material_( new visBase::Material )
    , attributecache_( 0 )
{
    material_->ref();
}


GeomIndexedShape::ColorHandler::~ColorHandler()
{
    material_->unRef();
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
	vtexshape_->setMaterial( coltabmaterial_ );
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


void GeomIndexedShape::addNodeState( visBase::NodeState* ns )
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
	    setMaterial( coltabmaterial_ );
	    vtexshape_->setMaterial( coltabmaterial_ );
    }
    else
    {
	setColorBindType( VertexShape::BIND_OVERALL );
	setMaterial( singlematerial_ );
	vtexshape_->setMaterial( singlematerial_ );
    }

    VisualObjectImpl::materialChangeCB( 0 );
    colortableenabled_  = yn;
}


bool GeomIndexedShape::isColTabEnabled() const
{
    return colortableenabled_;
}


void GeomIndexedShape::setDataMapper( const ColTab::MapperSetup& setup,
				      TaskRunner* tr )
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

    Coordinates* coords = 0;
    Normals* normals = 0;
    TextureCoords* texturecoords = 0;

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
	CoordListAdapter* coordlist =
	    dynamic_cast<CoordListAdapter*>( shape_->coordList() );
	coords = coordlist->getCoordinates();
	NormalListAdapter* normallist =
	    dynamic_cast<NormalListAdapter*>( shape_->normalCoordList() );
	normals = normallist->getNormals();
	TextureCoordListAdapter* texturelist =
	    dynamic_cast<TextureCoordListAdapter*>( shape_->textureCoordList());
	texturecoords = texturelist->getTextureCoords();
    }

    if ( shape_->needsUpdate() && !shape_->update(forall,tr) )
	return false;

    vtexshape_->removeAllPrimitiveSets();

    coords->setDisplayTransformation( getDisplayTransformation() );

    if ( !coords->size() )
	return false;

    vtexshape_->setCoordinates( coords );
    vtexshape_->useOsgAutoNormalComputation( true );

    if ( !useosgnormal_ && normals->nrNormals() )
    {
	normals->setDisplayTransformation( getDisplayTransformation() );
	vtexshape_->setNormals( normals );
	vtexshape_->useOsgAutoNormalComputation( false );
    }

    if ( texturecoords->size() )
	vtexshape_->setTextureCoords( texturecoords );

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


void GeomIndexedShape::getAttribPositions( DataPointSet& set,
					   mVisTrans* toinlcrltrans,
					   TaskRunner*) const
{
    const DataColDef coordindex( sKeyCoordIndex() );
    if ( set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact)==-1 )
	set.dataSet().add( new DataColDef(coordindex) );

    const int col =
	set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact);

    Coordinates* vtxcoords = vtexshape_->getCoordinates();
    if ( !vtxcoords || !vtxcoords->size() )
	return;

    for ( int coordid = 0; coordid<vtxcoords->size(); coordid++ )
    {
	Coord3 pos = vtxcoords->getPos( coordid );
	if ( !pos.isDefined() )
	    continue;

	DataPointSet::Pos dpsetpos;
	if ( toinlcrltrans )
	{
	    mVisTrans::transform( toinlcrltrans, pos );
	    dpsetpos.set( BinID(mNINT32(pos.x),mNINT32(pos.y)) );
	    dpsetpos.z_ = pos.z;
	}
	else
	{
	    const BinID bid = SI().transform( pos );
	    dpsetpos.set( bid );
	    dpsetpos.z_ = pos.z;
	}

	DataPointSet::DataRow datarow( dpsetpos, 1 );
	datarow.data_.setSize( set.nrCols(), mUdf(float) );
	datarow.data_[col-set.nrFixedCols()] =  coordid;
	set.addRow( datarow );
    }

    set.dataChanged();
}


void GeomIndexedShape::setAttribData( const DataPointSet& set,TaskRunner* tr)
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
    unRefAndNullPtr( vtexshape_ );

    if ( shapetype == PolyLine )
	vtexshape_ = visBase::PolyLine::create();
    else if ( shapetype == PolyLine3D )
	vtexshape_ = visBase::PolyLine3D::create();
    else
	vtexshape_ = visBase::VertexShape::create();

    vtexshape_->ref();
    vtexshape_->setMaterial( singlematerial_ );
    vtexshape_->setPrimitiveType( pstype );
    if( shapetype==PolyLine || shapetype==PolyLine3D )
    {
	visBase::PolygonOffset* offset = new visBase::PolygonOffset;
	offset->setFactor( -1.0f );
	offset->setUnits( 1.0f );

	offset->setMode(
	    visBase::PolygonOffset::Protected | visBase::PolygonOffset::On );
	vtexshape_->addNodeState( offset );
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

} // namespace visBase
