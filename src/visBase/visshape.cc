/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visshape.h"

#include "iopar.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexturechannels.h"
#include "vistexturecoords.h"

#include <osg/CullFace>
#include <osg/PrimitiveSet>
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

const char* Shape::sKeyOnOff() 			{ return  "Is on";	}
const char* Shape::sKeyTexture() 		{ return  "Texture";	}
const char* Shape::sKeyMaterial() 		{ return  "Material";	}


    
Shape::Shape()
    : material_( 0 )
{
}


Shape::~Shape()
{
    if ( material_ ) material_->unRef();
}



#define mDefSetGetItem(ownclass, clssname, variable, osgremove, osgset ) \
void ownclass::set##clssname( clssname* newitem ) \
{ \
    if ( variable ) \
    { \
	osgremove; \
	variable->unRef(); \
	variable = 0; \
    } \
 \
    if ( newitem ) \
    { \
	variable = newitem; \
	variable->ref(); \
	osgset; \
    } \
} \
 \
 \
clssname* ownclass::gt##clssname() const \
{ \
    return const_cast<clssname*>( variable ); \
}


mDefSetGetItem( Shape, Material, material_,
    removeNodeState( material_ ),
    addNodeState( material_ ) )


void Shape::setMaterialBinding( int nv )
{
    pErrMsg("Not Implemented" );
}


int Shape::getMaterialBinding() const
{
    pErrMsg("Not implemented");

    return cOverallMaterialBinding();
}


void Shape::renderOneSide( int side )
{
    osg::StateSet* stateset = osgNode()->getOrCreateStateSet();
    if ( !stateset )
	return;

    osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
    lightmodel->setTwoSided( true );
    stateset->setAttributeAndModes( lightmodel, osg::StateAttribute::ON );

    stateset->removeAttribute( osg::StateAttribute::CULLFACE );
    if ( side == 0 )
	return;

    osg::ref_ptr<osg::CullFace> cullface = new osg::CullFace;
    cullface->setMode( side<0 ? osg::CullFace::FRONT : osg::CullFace::BACK );
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


class ShapeNodeCallbackHandler: public osg::NodeCallback
{
public:
    ShapeNodeCallbackHandler( VertexShape& vtxshape )
	: vtxshape_( vtxshape )
    {}

    virtual void	operator()(osg::Node*,osg::NodeVisitor*);
    void		updateTexture();

protected:
    VertexShape&		vtxshape_;
};


#define mGetLayeredTexture( laytex ) \
    osgGeo::LayeredTexture* laytex = \
		vtxshape_.channels_ ? vtxshape_.channels_->getOsgTexture() : 0;

void ShapeNodeCallbackHandler::operator()( osg::Node* node,
					   osg::NodeVisitor* nv )
{
    mGetLayeredTexture( laytex );

    if ( laytex && nv->getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR )
    {
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


void ShapeNodeCallbackHandler::updateTexture()
{
    mGetLayeredTexture( laytex );

    if ( !laytex || !vtxshape_.osggeom_ || !vtxshape_.texturecoords_ )
	return;

    laytex->setTextureSizePolicy( osgGeo::LayeredTexture::AnySize );
    laytex->reInitTiling();
    const Coord size( Conv::to<Coord>(laytex->imageEnvelopeSize()) );
    if ( size.x>laytex->maxTextureSize() || size.y>laytex->maxTextureSize() )
    {
	pErrMsg( "Texture size overflow, because tiling not yet supported" );
    }

    if ( laytex->isOn() &&
		vtxshape_.coords_->size()!=vtxshape_.texturecoords_->size() )
    {
	pErrMsg( "One texture coordinate per vertex expected" );
    }

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

    vtxshape_.needstextureupdate_ = false;
}


//=============================================================================

    
#define mVertexShapeConstructor( geode ) \
     normals_( 0 ) \
    , coords_( 0 ) \
    , texturecoords_( 0 ) \
    , geode_( geode ) \
    , node_( 0 ) \
    , osggeom_( 0 ) \
    , primitivetype_( Geometry::PrimitiveSet::Other ) \
    , useosgsmoothnormal_( false ) \
    , channels_( 0 ) \
    , osgcallbackhandler_( 0 ) \
    , needstextureupdate_( false )


VertexShape::VertexShape()
    : mVertexShapeConstructor( new osg::Geode )
    , colorbindtype_( BIND_OFF )
    , normalbindtype_( BIND_PER_VERTEX )
{
    setupOsgNode();
}
    

VertexShape::VertexShape( Geometry::IndexedPrimitiveSet::PrimitiveType tp,
			  bool creategeode )
    : mVertexShapeConstructor( creategeode ? new osg::Geode : 0 )
{
    setupOsgNode();
    setPrimitiveType( tp );
}


void VertexShape::setMaterial( Material* mt )
{
    if ( !mt ) return;
    if ( material_ && osggeom_ )
	osggeom_->setColorArray( 0 );

    Shape::setMaterial( mt );
    materialChangeCB( 0 );
}


void VertexShape::setupOsgNode()
{
    node_ = new osg::Group;	// Needed because pushStateSet() applied in
    node_->ref();		// osgcallbackhandler_ has no effect on geodes
    setOsgNode( node_ );
    osgcallbackhandler_ = new ShapeNodeCallbackHandler( *this );
    osgcallbackhandler_->ref();
    node_->setUpdateCallback( osgcallbackhandler_ );
    node_->setCullCallback( osgcallbackhandler_ );

    if ( geode_ )
    {
	useOsgAutoNormalComputation( false );
	geode_->ref();
	osggeom_ = new osg::Geometry;
	setNormalBindType( BIND_PER_VERTEX );
	setColorBindType( BIND_OVERALL );
	osggeom_->setDataVariance( osg::Object::STATIC );
	osggeom_->setFastPathHint( true );
	geode_->addDrawable( osggeom_ );
	node_->asGroup()->addChild( geode_ );
	useVertexBufferRender( false );
    }
    setCoordinates( Coordinates::create() );
    if ( geode_ && coords_ )
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
    if ( coords == coords_ )
	return;

    if ( coords_ )
    {
	 if ( osggeom_ ) osggeom_->setVertexArray(0);
	 unRefAndZeroPtr( coords_ );
    }
    coords_ = coords;

    if ( coords_ )
    {
	coords_->ref();
	if ( osggeom_ )
	    osggeom_->setVertexArray(mGetOsgVec3Arr( coords_->osgArray()));
    }

}


void VertexShape::setPrimitiveType( Geometry::PrimitiveSet::PrimitiveType tp )
{
    primitivetype_ = tp;

    if ( osggeom_ )
    {
	if ( primitivetype_==Geometry::PrimitiveSet::Lines ||
	    primitivetype_==Geometry::PrimitiveSet::LineStrips )
	{
	    osggeom_->getOrCreateStateSet()->setMode( GL_LIGHTING,
						     osg::StateAttribute::OFF );
	}
    }
}


VertexShape::~VertexShape()
{
    if ( getMaterial() )
	getMaterial()->change.remove( mCB(this,VertexShape,materialChangeCB) );

    if ( osgcallbackhandler_ )
    {
	node_->removeUpdateCallback( osgcallbackhandler_ );
	node_->removeCullCallback( osgcallbackhandler_ );
	osgcallbackhandler_->unref();
    }

    if ( geode_ ) geode_->unref();
    if ( node_ ) node_->unref();
    if ( normals_ ) normals_->unRef();
    if ( coords_ ) coords_->unRef();
    if ( texturecoords_ ) texturecoords_->unRef();

    deepUnRef( primitivesets_ );
}


void VertexShape::dirtyCoordinates()
{
    if ( !osggeom_ ) return;

    osggeom_->dirtyDisplayList();
    osggeom_->dirtyBound();
    if ( useosgsmoothnormal_ )
	osgUtil::SmoothingVisitor::smooth( *osggeom_ );
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
    if ( osggeom_ )
    {
	osggeom_->setNormalBinding(
	    osg::Geometry::AttributeBinding( normalbindtype ) );
	normalbindtype_ = normalbindtype;
    }
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
}


void VertexShape::setDisplayTransformation( const mVisTrans* tr )
{ coords_->setDisplayTransformation( tr ); }


const mVisTrans* VertexShape::getDisplayTransformation() const
{ return  coords_->getDisplayTransformation(); }


mDefSetGetItem( VertexShape, Normals, normals_,
if ( osggeom_ ) osggeom_->setNormalArray( 0 ),
if ( osggeom_ )
{
    osggeom_->setNormalArray(mGetOsgVec3Arr(normals_->osgArray()));
}
);

mDefSetGetItem( VertexShape, TextureCoords, texturecoords_,
		needstextureupdate_=true , needstextureupdate_=true );


void VertexShape::setTextureChannels( TextureChannels* channels )
{
    channels_ = channels;
    needstextureupdate_ = true;
}


#define mCheckCreateShapeHints() \
    return;



IndexedShape::IndexedShape( Geometry::IndexedPrimitiveSet::PrimitiveType tp )
    : VertexShape( tp, true )
{}


#define setGetIndex( resourcename, fieldname )  \
int IndexedShape::nr##resourcename##Index() const \
{ return 0; } \
 \
 \
void IndexedShape::set##resourcename##Index( int pos, int idx ) \
{ } \
 \
 \
void IndexedShape::remove##resourcename##IndexAfter(int pos) \
{ } \
 \
 \
int IndexedShape::get##resourcename##Index( int pos ) const \
{ return -1; } \
 \
 \
void IndexedShape::set##resourcename##Indices( const int* ptr, int sz ) \
{ } \
\
void IndexedShape::set##resourcename##Indices( const int* ptr, int sz, \
					       int start ) \
{ } \


setGetIndex( Coord, coordIndex );
setGetIndex( TextureCoord, textureCoordIndex );
setGetIndex( Normal, normalIndex );
setGetIndex( Material, materialIndex );


int IndexedShape::getClosestCoordIndex( const EventInfo& ei ) const
{
    pErrMsg( "Not implemented in osg. Needed?");
    return -1;
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

    p->ref();
    p->setPrimitiveType( primitivetype_ );
    
    mDynamicCastGet(OSGPrimitiveSet*, osgps, p );
    addPrimitiveSetToScene( osgps->getPrimitiveSet() );

    primitivesets_ += p;
}
    

void VertexShape::removePrimitiveSet( const Geometry::PrimitiveSet* p )
{
    Threads::Locker lckr( lock_, Threads::Locker::WriteLock );
    const int pidx = primitivesets_.indexOf( p );
    if ( pidx == -1 ) return;
    mDynamicCastGet( OSGPrimitiveSet*, osgps,primitivesets_[pidx] );
    removePrimitiveSetFromScene( osgps->getPrimitiveSet() );
    primitivesets_.removeSingle( pidx )->unRef();

}


void VertexShape::removeAllPrimitiveSets()
{
    Threads::Locker lckr( lock_, Threads::Locker::WriteLock );
    for ( int idx = primitivesets_.size()-1; idx >= 0; idx-- )
	removePrimitiveSet( primitivesets_[idx] );
}


void VertexShape::addPrimitiveSetToScene( osg::PrimitiveSet* ps )
{
    osggeom_->addPrimitiveSet( ps );
}


void VertexShape::updatePartialGeometry( Interval<int> psrange )
{
    /* wait for further implementing only update psrange, rests of primitive
       sets will be static */

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

void VertexShape::removePrimitiveSetFromScene( const osg::PrimitiveSet* ps )
{
    const int idx = osggeom_->getPrimitiveSetIndex( ps );
    osggeom_->removePrimitiveSet( idx );
}

#define mImplOsgFuncs \
osg::PrimitiveSet* getPrimitiveSet() { return element_.get(); } \
void setPrimitiveType( Geometry::PrimitiveSet::PrimitiveType tp ) \
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
    virtual void	setEmpty()
    			{ element_->erase(element_->begin(),element_->end()); }
    virtual void	append( int idx ) { element_->push_back( idx ); }
    virtual int		pop() { return 0; }
    virtual int		set(int,int) { return 0; }

    void set(const int* ptr, int num)
    {
	element_->clear();
	element_->reserve( num );
	for ( int idx=0; idx<num; idx++, ptr++ )
	    element_->push_back( *ptr );
    }
    void append(const int* ptr, int num)
    {
	element_->reserve( size() +num );
	for ( int idx=0; idx<num; idx++, ptr++ )
	    element_->push_back( *ptr );
    }

    virtual int get(int idx) const
    {
	if ( idx >= size())
	    return 0;
	else
	    return element_->at( idx );
    }
    
    virtual int	size() const
    {
	return element_->size();
    }

    virtual int	indexOf(const int idx)
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
    
    int			size() const 	   { return element_->getCount();}
    int			get(int idx) const { return element_->getFirst()+idx;}

    void		setRange( const Interval<int>& rg )
    {
	element_->setFirst( rg.start );
	element_->setCount( rg.width()+1 );
    }
    
    Interval<int>	getRange() const
    {
	const int first = element_->getFirst();
	return Interval<int>( first, first+element_->getCount()-1 );
    }

    int			indexOf(const int) { return -1; }
    void		append( int ){};
    void		append(const int* ptr, int num){};
    void		setEmpty(){};

    osg::ref_ptr<osg::DrawArrays> element_;
};
    
    
    

class CoinIndexedPrimitiveSet : public Geometry::IndexedPrimitiveSet
{
public:
    virtual void	setEmpty() { indices_.erase(); }
    virtual void	append( int index ) { indices_ += index; }
    virtual int		pop()
    			{
			    const int idx = size()-1;
			    if ( idx<0 ) return mUdf(int);
			    const int res = indices_[idx];
			    indices_.removeSingle(idx);
			    return res;
			}
    
    virtual int		size() const { return indices_.size(); }
    virtual int		get(int pos) const { return indices_[pos]; }
    virtual int		set(int pos,int index)
			{ return indices_[pos] = index; }
    virtual void	set( const int* ptr,int num)
    {
	indices_ = TypeSet<int>( ptr, num );
    }
    
    virtual void	append( const int* ptr, int num )
    { indices_.append( ptr, num ); }
    
    TypeSet<int>	indices_;
};
    
    
class CoinRangePrimitiveSet : public Geometry::RangePrimitiveSet
{
public:
		   	CoinRangePrimitiveSet() : rg_(mUdf(int), mUdf(int) ) {}
    
    int			size() const 			{ return rg_.width()+1;}
    int			get(int idx) const 		{ return rg_.start+idx;}
    void		setRange(const Interval<int>& rg) { rg_ = rg; }
    Interval<int>	getRange() const 		{ return rg_; }
    
    Interval<int>	rg_;
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
