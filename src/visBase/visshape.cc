/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visshape.h"

#include "iopar.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexturechannels.h"
#include "vistexturecoords.h"
#include "vistransform.h"

#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/PrimitiveSet>
#include <osg/StateAttribute>
#include <osg/Switch>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Material>
#include <osg/LightModel>
#include <osgGeo/LayeredTexture>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Optimizer>
#include <osgUtil/CullVisitor>

mCreateFactoryEntry( visBase::VertexShape );

namespace visBase
{

const char* Shape::sKeyOnOff()			{ return  "Is on";	}
const char* Shape::sKeyTexture()		{ return  "Texture";	}
const char* Shape::sKeyMaterial()		{ return  "Material";	}



Shape::Shape()
{
}


Shape::~Shape()
{
}


void Shape::setMaterial( Material* newitem )
{
    if ( material_ )
	removeNodeState( material_.ptr() );

    material_ = newitem;
    if ( newitem )
	addNodeState( newitem );
}


Material* Shape::gtMaterial()
{
    return material_.ptr();
}


const Material* Shape::gtMaterial() const
{
    return material_.ptr();
}


void Shape::setMaterialBinding( int nv )
{
    pErrMsg("Not Implemented" );
}


int Shape::getMaterialBinding() const
{
    pErrMsg("Not implemented");

    return cOverallMaterialBinding();
}


void Shape::enableRenderLighting(bool yn )
{
    osg::StateSet* stateset = getStateSet();
    if ( !stateset )
	return;
    if ( yn )
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    else
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
}


void Shape::setRenderMode( RenderMode mode )
{
    osg::StateSet* stateset = getStateSet();
    if ( !stateset )
	return;

    osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
    lightmodel->setTwoSided( true );
    stateset->setAttributeAndModes( lightmodel, osg::StateAttribute::ON );
    stateset->removeAttribute( osg::StateAttribute::CULLFACE );
    if ( mode == RenderBothSides )
	return;

    osg::ref_ptr<osg::CullFace> cullface = new osg::CullFace;
    cullface->setMode( mode>=RenderFrontSide ? osg::CullFace::FRONT
					     : osg::CullFace::BACK );
    stateset->setAttributeAndModes( cullface, osg::StateAttribute::ON );
}


void Shape::fillPar( IOPar& iopar ) const
{
    if ( material_ )
    {
	IOPar matpar;
	material_->fillPar( matpar );
	iopar.mergeComp( matpar, sKeyMaterial() );
    }

    iopar.setYN( sKeyOnOff(), isOn() );
}


int Shape::usePar( const IOPar& par )
{
    bool ison;
    if ( par.getYN( sKeyOnOff(), ison) )
	turnOn( ison );

    if ( material_ )
    {
	PtrMan<IOPar> matpar = par.subselect( sKeyMaterial() );
	material_->usePar( *matpar );
    }

    return 1;
}

//=============================================================================


class VertexShape::TextureCallbackHandler :
				public osgGeo::LayeredTexture::Callback
{
public:
    TextureCallbackHandler( VertexShape& vtxshape )
	: vtxshape_( vtxshape )
    {}

    void requestRedraw() const override		{ vtxshape_.forceRedraw(); }

protected:
    VertexShape&		vtxshape_;
};


class VertexShape::NodeCallbackHandler: public osg::NodeCallback
{
public:
    NodeCallbackHandler( VertexShape& vtxshape )
	: vtxshape_( vtxshape )
    {}

    void		operator()(osg::Node*,osg::NodeVisitor*) override;
    void		updateTexture();

protected:
    VertexShape&		vtxshape_;
};


#define mGetLayeredTexture( laytex ) \
    osgGeo::LayeredTexture* laytex = \
		vtxshape_.channels_ ? vtxshape_.channels_->getOsgTexture() : 0;

void VertexShape::NodeCallbackHandler::operator()( osg::Node* node,
						   osg::NodeVisitor* nv )
{
    mGetLayeredTexture( laytex );

    if ( laytex && nv->getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR )
    {
	vtxshape_.forceRedraw( false );

	if ( vtxshape_.needstextureupdate_ || laytex->needsRetiling() )
	    updateTexture();

	traverse( node, nv );
    }
    else if ( laytex && nv->getVisitorType()==osg::NodeVisitor::CULL_VISITOR )
    {
	osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
	if ( laytex->getSetupStateSet() )
	    cv->pushStateSet( laytex->getSetupStateSet() );

	traverse( node, nv );

	if ( laytex->getSetupStateSet() )
	    cv->popStateSet();
    }
    else
	traverse( node, nv );
}


void VertexShape::NodeCallbackHandler::updateTexture()
{
    mGetLayeredTexture( laytex );

    if ( !laytex || !vtxshape_.osggeom_ || !vtxshape_.texturecoords_ )
	return;

    laytex->setTextureSizePolicy( osgGeo::LayeredTexture::AnySize );
    laytex->reInitTiling();

#ifdef __debug__
    const Coord size( Conv::to<Coord>(laytex->imageEnvelopeSize()) );
    if ( size.x_>laytex->maxTextureSize() || size.y_>laytex->maxTextureSize() )
    {
	pErrMsg( "Texture size overflow, because tiling not yet supported" );
    }

    const int nrcoords = vtxshape_.coords_->size();
    const int nrtexturecoords = vtxshape_.texturecoords_->size();
    if ( laytex->isOn() && nrcoords!=nrtexturecoords )
    {
	pErrMsg( "One texture coordinate per vertex expected" );
    }
#endif

    const osg::Vec2f origin( 0.0f, 0.0f );
    const osg::Vec2f opposite( laytex->textureEnvelopeSize() );
    std::vector<osgGeo::LayeredTexture::TextureCoordData> tcdata;

    vtxshape_.osggeom_->setStateSet( !laytex->isOn() ? 0 :
		    laytex->createCutoutStateSet(origin, opposite, tcdata) );

    for ( int unit=0; unit<laytex->nrTextureUnits(); unit++ )
	vtxshape_.osggeom_->setTexCoordArray( unit, 0 );

    for ( int idx=0; idx<tcdata.size(); idx++ )
    {
	vtxshape_.osggeom_->setTexCoordArray( tcdata[idx]._textureUnit,
			mGetOsgVec2Arr(vtxshape_.texturecoords_->osgArray()) );
    }

    vtxshape_.setUpdateVar( vtxshape_.needstextureupdate_ , false );
}


//=============================================================================

VertexShape::VertexShape()
    : geode_(new osg::Geode)
    , primitivetype_(Geometry::PrimitiveSet::Other)
    , colorbindtype_(BIND_OFF)
    , normalbindtype_(BIND_PER_VERTEX)
{
    ref();
    setupOsgNode();
    if ( coords_ )
	 mAttachCB( coords_->change, VertexShape::coordinatesChangedCB );

    unRefNoDelete();
}


VertexShape::VertexShape( Geometry::IndexedPrimitiveSet::PrimitiveType tp,
			  bool creategeode, bool createnode )
    : geode_(creategeode ? new osg::Geode : nullptr)
    , primitivetype_(Geometry::PrimitiveSet::Other)
    , ownsnodeptr_(createnode)
{
    ref();
    if ( createnode )
    {
	setupOsgNode();
	setPrimitiveType( tp );
    }

    if ( coords_ )
	mAttachCB( coords_->change, VertexShape::coordinatesChangedCB );

    unRefNoDelete();
}


void VertexShape::setMaterial( Material* mt )
{
    if ( !mt )
	return;

    if ( material_ && osggeom_ )
	osggeom_->setColorArray( nullptr );

    Shape::setMaterial( mt );
    materialChangeCB( nullptr );
}


void VertexShape::setupOsgNode()
{
    if ( ownsnodeptr_ )
    {
	node_ = new osg::Group; // Needed because pushStateSet() applied in
	refOsgPtr( node_ );	// nodecallbackhandler_ has no effect on geodes
	setOsgNode( node_ );
    }

    if ( !node_ )
    {
	pErrMsg( "Please review the construction of objects. "
		 "\"node_\" shouldn't be null" );
	return;
    }

    nodecallbackhandler_ = new NodeCallbackHandler( *this );
    refOsgPtr( nodecallbackhandler_ );
    node_->setCullCallback( nodecallbackhandler_ );
    texturecallbackhandler_ = new TextureCallbackHandler( *this );
    refOsgPtr( texturecallbackhandler_ );

    if ( geode_ )
    {
	useOsgAutoNormalComputation( false );
	refOsgPtr( geode_ );
	osggeom_ = new osg::Geometry;
	setNormalBindType( BIND_PER_VERTEX );
	setColorBindType( BIND_OVERALL );
	osggeom_->setDataVariance( osg::Object::STATIC );
	geode_->addDrawable( osggeom_ );
	node_->asGroup()->addChild( geode_ );
	useVertexBufferRender( false );
    }

    RefMan<Coordinates> coords = Coordinates::create();
    setCoordinates( coords.ptr() );
    if ( geode_ && coords_ && coords_->size() &&
	 osggeom_ && osggeom_->getNumPrimitiveSets() )
    {
	osgUtil::Optimizer optimizer;
	optimizer.optimize( geode_ );
    }
}


void VertexShape::useVertexBufferRender( bool yn )
{
    if ( osggeom_ )
    {
	osggeom_->setUseDisplayList( !yn );
	osggeom_->setUseVertexBufferObjects( yn );
    }
}


void VertexShape::setCoordinates( Coordinates* coords )
{
    if ( coords == coords_.ptr() )
	return;

    if ( coords_ )
    {
	 mDetachCB( coords_->change, VertexShape::coordinatesChangedCB );
	 if ( osggeom_ ) osggeom_->setVertexArray(0);
    }

    coords_ = coords;
    if ( coords_ )
    {
	mAttachCB( coords_->change, VertexShape::coordinatesChangedCB );
	if ( osggeom_ )
	    osggeom_->setVertexArray( mGetOsgVec3Arr( coords_->osgArray() ) );
    }

}


void VertexShape::setAttribAndMode( osg::StateAttribute* drawstyl )
{
    if ( !drawstyl || !osggeom_ )
	return;

    osggeom_->getOrCreateStateSet()->setAttributeAndModes( drawstyl,
		    osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
}


void VertexShape::setPrimitiveType( Geometry::PrimitiveSet::PrimitiveType tp )
{
    primitivetype_ = tp;

    if ( osggeom_ )
    {
	if ( primitivetype_==Geometry::PrimitiveSet::Lines ||
	     primitivetype_==Geometry::PrimitiveSet::LineStrips ||
	     primitivetype_ == Geometry::PrimitiveSet::Points )
	{
	    osggeom_->getOrCreateStateSet()->setMode( GL_LIGHTING,
						    osg::StateAttribute::OFF );
	}
    }
}


VertexShape::~VertexShape()
{
    detachAllNotifiers();
    if ( getMaterial() )
	getMaterial()->change.remove( mCB(this,VertexShape,materialChangeCB) );

    setTextureChannels( nullptr );
    unRefOsgPtr( texturecallbackhandler_ );
    forceRedraw( false );
    if ( nodecallbackhandler_ )
	node_->setCullCallback( nullptr );

    unRefOsgPtr( nodecallbackhandler_ );
    setCoordinates( nullptr );
    unRefOsgPtr( geode_ );
    if ( ownsnodeptr_ )
	unRefOsgPtr( node_ );
}


void VertexShape::dirtyCoordinates()
{
    if ( !osggeom_ ) return;

    osggeom_->dirtyGLObjects();
    osggeom_->dirtyBound();
    if ( useosgsmoothnormal_ )
    {
	osgUtil::SmoothingVisitor::smooth( *osggeom_ );
	osggeom_->setNormalBinding(
	    osg::Geometry::AttributeBinding(normalbindtype_) );
    }
}


Coord3 VertexShape::getOsgNormal( int idx ) const
{
    Coord3 nm = Coord3( 0, 0, 0 );
    if ( osggeom_ )
    {
	const osg::Array* arr = osggeom_->getNormalArray();
	const osg::Vec3Array* osgnormals = sCast(const osg::Vec3Array*,arr);
	if ( osgnormals->size() > idx )
	    nm = Conv::to<Coord3>( (*osgnormals)[idx] );
    }
    return nm;
}


void VertexShape::useOsgAutoNormalComputation( bool yn )
{
    useosgsmoothnormal_ = yn;
}

void VertexShape::setColorBindType( BindType bt )
{
    colorbindtype_ =  bt;
}


int VertexShape::getNormalBindType()
{
    return normalbindtype_;
}


void VertexShape::setNormalBindType( BindType normalbindtype )
{
    normalbindtype_ = normalbindtype;
}


void VertexShape::materialChangeCB( CallBacker* )
{
    if ( !osggeom_  || !material_  || !coords_ ) return;

    if ( colorbindtype_ == BIND_OFF )
    {
	if( coords_->size() && coords_->size() == material_->nrOfMaterial() )
	    colorbindtype_ = BIND_PER_VERTEX;
	else
	    colorbindtype_ = BIND_OVERALL;
    }

    material_->setColorBindType( colorbindtype_ );
    if ( osggeom_->getVertexArray()->getNumElements() > 0 )
        material_->attachGeometry( osggeom_ );

    osg::StateSet* ss = getStateSet();

    const bool transparent = getMaterial()->getTransparency() > 0.0;

    if ( transparent && ss->getRenderingHint()!=osg::StateSet::TRANSPARENT_BIN )
    {
	osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
	blendFunc->setFunction( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	ss->setAttributeAndModes( blendFunc );
	ss->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    }

    if ( !transparent && ss->getRenderingHint()!=osg::StateSet::OPAQUE_BIN )
    {
	ss->removeAttribute( osg::StateAttribute::BLENDFUNC );
	ss->setRenderingHint( osg::StateSet::OPAQUE_BIN );
    }
    requestSingleRedraw();
}


void VertexShape::coordinatesChangedCB( CallBacker* )
{
    if ( !usecoordinateschangedcb_ )
	return;

    if ( osggeom_ && primitivesets_.size() && coords_->size() )
	dirtyCoordinates();
}


void VertexShape::setDisplayTransformation( const mVisTrans* tr )
{
    coords_->setDisplayTransformation( tr );
    if ( osggeom_)
	osggeom_->dirtyGLObjects();

}


const mVisTrans* VertexShape::getDisplayTransformation() const
{ return  coords_->getDisplayTransformation(); }


void VertexShape::setNormals( Normals* newitem )
{
    if ( osggeom_ && normals_.ptr() )
	osggeom_->setNormalArray( nullptr );

    normals_ = newitem;
    if ( osggeom_ && newitem )
    {
	osggeom_->setNormalArray( mGetOsgVec3Arr(normals_->osgArray()) );
	osggeom_->setNormalBinding(
		osg::Geometry::AttributeBinding( normalbindtype_ ) );
    }
}


Normals* VertexShape::gtNormals()
{
    return normals_.ptr();
}


const Normals* VertexShape::gtNormals() const
{
    return normals_.ptr();
}


void VertexShape::setTextureCoords( TextureCoords* newitem )
{
    if ( texturecoords_ )
	setUpdateVar( needstextureupdate_, true );

    texturecoords_ = newitem;
    if ( newitem )
	setUpdateVar( needstextureupdate_, true );
}


TextureCoords* VertexShape::gtTextureCoords()
{
    return texturecoords_.ptr();
}


const TextureCoords* VertexShape::gtTextureCoords() const
{
    return texturecoords_.ptr();
}


void VertexShape::setTextureChannels( TextureChannels* channels )
{
    if ( channels_ && texturecallbackhandler_ )
	channels_->getOsgTexture()->removeCallback( texturecallbackhandler_ );

    channels_ = channels;

    if ( channels_ && texturecallbackhandler_ )
	channels_->getOsgTexture()->addCallback( texturecallbackhandler_ );

    setUpdateVar( needstextureupdate_, true );
}


const unsigned char* VertexShape::getTextureData( int& width, int& height,
    int& pixelsz ) const
{
    const osgGeo::LayeredTexture* laytex =
	channels_ ? channels_->getOsgTexture() : 0;

    osgGeo::LayeredTexture* laytexture =
	const_cast<osgGeo::LayeredTexture*>( laytex );

    if ( !laytexture ) return 0;

    const osg::Image* img = laytexture->getCompositeTextureImage();
    if ( !img ) return 0;
    width = img->s();
    height = img->t();
    pixelsz = img->getPixelSizeInBits();

    return img->data();
}


void VertexShape::forceRedraw( bool yn )
{
    Threads::Locker lckr( redrawlock_, Threads::Locker::WriteLock );
    if ( isredrawing_!=yn && nodecallbackhandler_ )
    {
	isredrawing_ = yn;
	node_->setUpdateCallback( yn ? nodecallbackhandler_ : 0 );
    }
}


void VertexShape::setUpdateVar( bool& variable, bool yn )
{
    if ( yn )
	forceRedraw( true );

    variable = yn;
}


class OSGPrimitiveSet
{
public:
    virtual osg::PrimitiveSet*	getPrimitiveSet()	= 0;

    static GLenum getGLEnum(Geometry::PrimitiveSet::PrimitiveType tp)
    {
	switch ( tp )
	{
	    case Geometry::PrimitiveSet::Triangles:
		return GL_TRIANGLES;
	    case Geometry::PrimitiveSet::TriangleFan:
		return GL_TRIANGLE_FAN;
	    case Geometry::PrimitiveSet::TriangleStrip:
		return GL_TRIANGLE_STRIP;
	    case Geometry::PrimitiveSet::Lines:
		return GL_LINES;
	    case Geometry::PrimitiveSet::Points:
		return GL_POINTS;
	    case Geometry::PrimitiveSet::LineStrips:
		return GL_LINE_STRIP;
	    default:
		break;
	}

	return GL_POINTS;
    }
};


void VertexShape::addPrimitiveSet( Geometry::PrimitiveSet* p )
{
    Threads::Locker lckr( lock_, Threads::Locker::WriteLock );
    if ( !p || primitivesets_.indexOf( p ) != -1 )
	return;

    p->setPrimitiveType( primitivetype_ );

    mDynamicCastGet(OSGPrimitiveSet*, osgps, p );
    addPrimitiveSetToScene( osgps->getPrimitiveSet() );

    primitivesets_.add( p );
}


void VertexShape::removePrimitiveSet( const Geometry::PrimitiveSet* p )
{
    Threads::Locker lckr( lock_, Threads::Locker::WriteLock );
    const int pidx = primitivesets_.indexOf( p );
    if ( pidx == -1 ) return;
    mDynamicCastGet( OSGPrimitiveSet*, osgps,primitivesets_[pidx] );
    removePrimitiveSetFromScene( osgps->getPrimitiveSet() );
    primitivesets_.removeSingle( pidx );
}


void VertexShape::removeAllPrimitiveSets()
{
    Threads::Locker lckr( lock_, Threads::Locker::WriteLock );
    removeAllPrimitiveSetsFromScene();
    primitivesets_.erase();
}


void VertexShape::addPrimitiveSetToScene( osg::PrimitiveSet* ps )
{
    osggeom_->addPrimitiveSet( ps );
}


void VertexShape::updatePartialGeometry( Interval<int> psrange )
{
    /* wait for further implementing only update psrange, rests of primitive
       sets will be Static */

   /* osg::Vec4Array* colorarr = mGetOsgVec4Arr( material_->getColorArray() );
    osg::Vec4Array* osgcolorarr = mGetOsgVec4Arr( osggeom_->getColorArray() );*/

    useVertexBufferRender( true );
    osggeom_->dirtyBound();
   /* Threads::Locker lckr( lock_, Threads::Locker::WriteLock );
    for ( int idx = psrange.start; idx< psrange.stop; idx++ )
	(*osgcolorarr)[idx] = (*colorarr)[idx];
    lckr.unlockNow();*/
    useVertexBufferRender( false );
}


int VertexShape::nrPrimitiveSets() const
{ return primitivesets_.size(); }


Geometry::PrimitiveSet* VertexShape::getPrimitiveSet( int idx )
{
    return primitivesets_[idx];
}


const Geometry::PrimitiveSet* VertexShape::getPrimitiveSet( int idx ) const
{
    return primitivesets_[idx];
}


void VertexShape::removePrimitiveSetFromScene( const osg::PrimitiveSet* ps )
{
    const int idx = osggeom_->getPrimitiveSetIndex( ps );
    osggeom_->removePrimitiveSet( idx );
}


void VertexShape::removeAllPrimitiveSetsFromScene()
{
    for ( int idx=osggeom_->getNumPrimitiveSets()-1; idx>=0; idx-- )
	osggeom_->removePrimitiveSet( idx );
}


#define mImplOsgFuncs \
osg::PrimitiveSet* getPrimitiveSet() override { return element_.get(); } \
void setPrimitiveType( Geometry::PrimitiveSet::PrimitiveType tp ) override \
{ \
    Geometry::PrimitiveSet::setPrimitiveType( tp ); \
    element_->setMode( getGLEnum( getPrimitiveType() )); \
}


template <class T>
class OSGIndexedPrimitiveSet : public Geometry::IndexedPrimitiveSet,
			       public OSGPrimitiveSet
{
public:
			OSGIndexedPrimitiveSet()
			    : element_( new T ) {}

			mImplOsgFuncs
    void		setEmpty() override
			{ element_->erase(element_->begin(),element_->end()); }
    void		append( int idx ) override
			{ element_->push_back( idx ); }
    int			pop() override { return 0; }
    int			set(int,int) override { return 0; }

    void set( const int* ptr, int num ) override
    {
	element_->clear();
	element_->reserve( num );
	for ( int idx=0; idx<num; idx++, ptr++ )
	    element_->push_back( *ptr );
    }
    void append( const int* ptr, int num ) override
    {
	element_->reserve( size() +num );
	for ( int idx=0; idx<num; idx++, ptr++ )
	    element_->push_back( *ptr );
    }

    int get( int idx ) const override
    {
	if ( idx >= size())
	    return 0;
	else
	    return element_->at( idx );
    }

    int	size() const override
    {
	return element_->size();
    }

    int	indexOf( const int idx ) override
    {
	typename T::const_iterator res = std::find(
	    element_->begin(), element_->end(), idx );
	if ( res==element_->end() ) return -1;
	return mCast( int,res-element_->begin() );
    }

    osg::ref_ptr<T>	element_;
};


class OSGRangePrimitiveSet : public Geometry::RangePrimitiveSet,
			     public OSGPrimitiveSet
{
public:
			OSGRangePrimitiveSet()
			    : element_( new osg::DrawArrays )
			{}

			mImplOsgFuncs

    int			size() const override   { return element_->getCount();}
    int			get(int idx) const override
			{ return element_->getFirst()+idx;}

    void		setRange( const Interval<int>& rg ) override
    {
	element_->setFirst( rg.start_ );
	element_->setCount( rg.width(false)+1 );
    }

    Interval<int>	getRange() const override
    {
	const int first = element_->getFirst();
	return Interval<int>( first, first+element_->getCount()-1 );
    }

    int			indexOf( const int ) override	{ return -1; }
    void		append( int ) override				{};
    void		append(const int* ptr, int num) override	{};
    void		setEmpty() override				{};

    osg::ref_ptr<osg::DrawArrays> element_;
};


Geometry::PrimitiveSet*
    PrimitiveSetCreator::doCreate( bool indexed, bool large )
{
    if ( indexed )
	return large
	       ? (Geometry::IndexedPrimitiveSet*)
		new OSGIndexedPrimitiveSet<osg::DrawElementsUInt>
	       : (Geometry::IndexedPrimitiveSet*)
		new OSGIndexedPrimitiveSet<osg::DrawElementsUShort>;

    return new OSGRangePrimitiveSet;
}

} // namespace visBase
