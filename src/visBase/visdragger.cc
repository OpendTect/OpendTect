/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/

#include "commondefs.h"
#include "visdragger.h"

#include "visevent.h"
#include "vistransform.h"

#include <osg/Switch>
#include <osgGeo/Draggers>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/AutoTransform>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osgManipulator/Projector>

mCreateFactoryEntry( visBase::Dragger );

namespace visBase
{

class DraggerCallbackHandler : public osgManipulator::DraggerCallback
{
public:
    DraggerCallbackHandler( DraggerBase& dragger )
	: dragger_( dragger )
    {}

    using		osgManipulator::DraggerCallback::receive;
    bool		receive(const osgManipulator::MotionCommand&) override;

protected:

    void		constrain();

    DraggerBase&	dragger_;
    osg::Matrix		startmatrix_;
};



bool DraggerCallbackHandler::receive( const osgManipulator::MotionCommand& cmd )
{
    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	startmatrix_ = dragger_.osgdragger_->getMatrix();
	dragger_.notifyStart();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::MOVE )
    {
	constrain();
	dragger_.notifyMove();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::FINISH )
    {
	dragger_.notifyStop();
	if ( startmatrix_ != dragger_.osgdragger_->getMatrix() )
	    dragger_.changed.trigger();
    }

    return true;
}


void DraggerCallbackHandler::constrain()
{
    Coord3 pos;
    osg::Vec3d osgpos = dragger_.osgdragger_->getMatrix().getTrans();
    mVisTrans::transformBack( dragger_.displaytrans_, osgpos, pos );

    for ( int dim=0; dim<3; dim++ )
    {
	if ( !mIsUdf(dragger_.spaceranges_[dim].start) &&
	     dragger_.spaceranges_[dim].start>pos[dim] )
	{
	    pos[dim] = dragger_.spaceranges_[dim].start;
	}
	if ( !mIsUdf(dragger_.spaceranges_[dim].stop) &&
	     dragger_.spaceranges_[dim].stop<pos[dim] )
	{
	    pos[dim] = dragger_.spaceranges_[dim].stop;
	}
    }

    mVisTrans::transform( dragger_.displaytrans_, pos, osgpos );
    osg::Matrix mat = dragger_.osgdragger_->getMatrix();
    mat.setTrans( osgpos );
    dragger_.osgdragger_->setMatrix( mat );
}


DraggerBase::DraggerBase()
    : started(this)
    , motion(this)
    , finished(this)
    , changed(this)
    , displaytrans_(nullptr)
    , cbhandler_(nullptr)
    , osgdragger_(nullptr)
    , osgroot_(new osg::Group)
{
    setOsgNode( osgroot_ );
    setPickable( true );

    for ( int dim=0; dim<3; dim++ )
	spaceranges_[dim].setUdf();
}


DraggerBase::~DraggerBase()
{
    if ( displaytrans_ ) displaytrans_->unRef();

    if( cbhandler_ )
	cbhandler_->unref();

    osgdragger_->removeDraggerCallback( cbhandler_ );

    if ( osgdragger_ )
	osgdragger_->unref();

}


void DraggerBase::initDragger( osgManipulator::Dragger* d )
{
    if ( osgdragger_ )
    {
	if ( cbhandler_ )
	    osgdragger_->removeDraggerCallback( cbhandler_ );
	osgdragger_->unref();
    }

    if ( cbhandler_ )
	cbhandler_->unref();

    osgdragger_ = d;
    osgdragger_->ref();

    if ( osgdragger_ )
    {
	osgroot_->removeChildren( 0, osgroot_->getNumChildren() );
	osgroot_->addChild( osgdragger_ );

	cbhandler_ = new DraggerCallbackHandler( *this );
	cbhandler_->ref();
	osgdragger_->setHandleEvents(true);
	osgdragger_->addDraggerCallback( cbhandler_ );
	osgdragger_->setIntersectionMask( cDraggerIntersecTraversalMask() );
    }
}


void DraggerBase::setDisplayTransformation( const mVisTrans* nt )
{
    if ( displaytrans_ )
    {
	displaytrans_->unRef();
	displaytrans_ = 0;
    }

    displaytrans_ = nt;
    if ( displaytrans_ )
    {
	displaytrans_->ref();
    }
}


const mVisTrans* DraggerBase::getDisplayTransformation() const
{
    return displaytrans_;
}


void DraggerBase::setSpaceLimits( const Interval<float>& x,
				  const Interval<float>& y,
				  const Interval<float>& z )
{
    spaceranges_[0] = x; spaceranges_[1] = y; spaceranges_[2] = z;
}


void DraggerBase::handleEvents( bool yn )
{
    if ( osgdragger_ ) osgdragger_->setHandleEvents( yn );
}


bool DraggerBase::isHandlingEvents() const
{
    return osgdragger_ ? osgdragger_->getHandleEvents() : true;
}


Dragger::Dragger()
    : rightclicknotifier_(this)
    , rightclickeventinfo_(nullptr)
    , inactiveshape_(nullptr)
    , ismarkershape_(true)
    , draggersizescale_(100)
    , defaultdraggergeomsize_(0.025)
    , rotation_(0,0,0)
    , rotangle_(0.0)
    , arrowcolor_(OD::Color(255,255,0))
{
    setDefaultRotation();
    turnOn( true );
}


void Dragger::setDefaultRotation()
{
    setRotation( Coord3(0,1,0), -M_PI_2 );
}


bool Dragger::defaultRotation() const
{
    return rotation_== Coord3(0,1,0) && mIsEqual(rotangle_,-M_PI_2,1e-5);
}


Dragger::~Dragger()
{
    unRefAndZeroPtr( inactiveshape_ );
}


bool Dragger::selectable() const
{ return isHandlingEvents(); }


void Dragger::setDraggerType( Type tp )
{
    if ( tp==Translate1D )
    {
	osgGeo::Translate1DDragger* dragger = new osgGeo::Translate1DDragger;
	dragger->setInactivationModKeyMask( mCast(unsigned int,~0) );
	initDragger( dragger );
	is2dtranslate_ = false;
    }
    else if ( tp==Translate2D )
    {
	osgGeo::Translate2DDragger* dragger = new osgGeo::Translate2DDragger;
	dragger->setInactivationModKeyMask( mCast(unsigned int,~0) );
	initDragger( dragger );
	is2dtranslate_ = true;
    }
    else if ( tp==Translate3D || tp==Scale3D )
    {
	pErrMsg("Not impl");
    }
}


static Dragger* movingdragger_ = 0;

bool Dragger::isMoving() const
{
    return movingdragger_==this;
}


void Dragger::notifyStart()
{
    movingdragger_ = 0;
    updateDragger( false );
    started.trigger();
}


void Dragger::notifyStop()
{
    movingdragger_ = 0;
    finished.trigger();
    updateDragger( true );
}


void Dragger::notifyMove()
{
    movingdragger_ = this;
    setScaleAndTranslation( true );
    motion.trigger();
}


void Dragger::setOwnShape( DataObject* newshape, bool activeshape )
{
    newshape->ref();
    unRefAndZeroPtr( inactiveshape_ );
    inactiveshape_ = newshape;
}


void Dragger::updateDragger( bool ismarkershape )
{
    ismarkershape_ = ismarkershape;
    osgdragger_->removeChildren( 0 , osgdragger_->getNumChildren() );

    if ( ismarkershape && inactiveshape_ )
	osgdragger_->addChild( inactiveshape_->osgNode() );
    else
	osgdragger_->addChild( createDefaultDraggerGeometry() );
    setScaleAndTranslation();
}


void Dragger::triggerRightClick( const EventInfo* eventinfo )
{
    rightclickeventinfo_ = eventinfo;
    rightclicknotifier_.trigger();
}


const TypeSet<VisID>* Dragger::rightClickedPath() const
{ return rightclickeventinfo_ ? &rightclickeventinfo_->pickedobjids : 0; }


const EventInfo* Dragger::rightClickedEventInfo() const
{ return rightclickeventinfo_; }


float Dragger::getSize() const
{
    return draggersizescale_;
}


void Dragger::setSize( const float markersize )
{
    draggersizescale_ = 1.6*markersize/defaultdraggergeomsize_;
}


void Dragger::setRotation( const Coord3& vec, const float rotationangle )
{
    rotation_ = vec;
    rotangle_ = rotationangle;
}


void Dragger::setPos( const Coord3& pos )
{
    if ( !osgdragger_ ) return;
    Coord3 newpos;
    mVisTrans::transform( displaytrans_, pos, newpos );
    markerpos_ = newpos;
    updateDragger( true );
}


void Dragger::setScaleAndTranslation( bool move )
{
    const float scale = ismarkershape_ ? 1.0f : draggersizescale_;

    osg::Vec3 trans;
    if ( !move )
	trans = Conv::to<osg::Vec3>( markerpos_ );
    else
	trans = osgdragger_->getMatrix().getTrans();

    osgdragger_->setMatrix( osg::Matrix::scale(scale, scale, scale ) *
	osg::Matrix::rotate( M_PI_2, osg::Vec3(1,0,0) ) *
	osg::Matrix::rotate( rotangle_, Conv::to<osg::Vec3>(rotation_) ) *
	osg::Matrix::translate( Conv::to<osg::Vec3>( trans ) ) );
}


Coord3 Dragger::getPos() const
{
    if ( !osgdragger_ )
	return Coord3( 0, 0, 0 );

    osg::Vec3d pos = osgdragger_->getMatrix().getTrans();
    Coord3 coord;
    mVisTrans::transformBack( displaytrans_, pos, coord );
    return coord;
}


osg::MatrixTransform* Dragger::createDefaultDraggerGeometry()
{
    return createTranslateDefaultGeometry();
}


osg::MatrixTransform* Dragger::createTranslateDefaultGeometry()
{
    // Create a line.
    osg::ref_ptr<osg::Geode> linegeode = new osg::Geode;
    {
	osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
	osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array( 2 );
	(*vertices)[0] = osg::Vec3( 0.0f,0.0f,-0.5f );
	(*vertices)[1] = osg::Vec3( 0.0f,0.0f,0.5f );
	geometry->setVertexArray( vertices );

	osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
	colors->push_back( Conv::to<osg::Vec4>(arrowcolor_) );
	geometry->setColorArray( colors );
	geometry->setColorBinding( osg::Geometry::BIND_OVERALL );

	geometry->addPrimitiveSet(
	    new osg::DrawArrays(osg::PrimitiveSet::LINES,0,2) );
	linegeode->addDrawable( geometry );
    }

    // Turn off lighting for line and set line width.
    osg::LineWidth* linewidth = new osg::LineWidth();
    linewidth->setWidth(4.0f);
    linegeode->getOrCreateStateSet()->setAttributeAndModes(
	linewidth, osg::StateAttribute::ON);
    linegeode->getOrCreateStateSet()->setMode(
	GL_LIGHTING,osg::StateAttribute::OFF);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osg::ref_ptr<osg::Material> material = new osg::Material;
    material->setDiffuse( osg::Material::FRONT_AND_BACK,
			  Conv::to<osg::Vec4>(arrowcolor_) );
    geode->getOrCreateStateSet()->setAttribute( material );

    // Create left cone.
    {
	osg::Cone* cone = new osg::Cone ( osg::Vec3(0.0f, 0.0f, -0.5f),
	    4*defaultdraggergeomsize_, 0.2 );
	osg::Quat rotation; rotation.makeRotate(
	    osg::Vec3(0.0f,0.0f,-1.0f), osg::Vec3( 0.0f, 0.0f, 1.0f) );
	cone->setRotation( rotation );
	osg::ShapeDrawable* conedrawble = new osg::ShapeDrawable( cone );
	osg::TessellationHints* hint = new osg::TessellationHints;
	hint->setDetailRatio( 0.8 );
	conedrawble->setTessellationHints( hint );
	geode->addDrawable( conedrawble ) ;
    }

    // Create right cone.
    {
	osg::Cone* cone = new osg::Cone (
	    osg::Vec3( 0.0f, 0.0f, 0.5f ), 4*defaultdraggergeomsize_, 0.2 );
	osg::ShapeDrawable* conedrawble = new osg::ShapeDrawable( cone );
	osg::TessellationHints* hint = new osg::TessellationHints;
	hint->setDetailRatio( 0.8 );
	conedrawble->setTessellationHints( hint );
	geode->addDrawable( conedrawble ) ;
    }

    // Create an invisible cylinder for picking the line.
    {
	osg::Cylinder* cylinder = new osg::Cylinder (
	    osg::Vec3( 0.0f,0.0f,0.0f ), 0.015f, 1.0f );
	osg::Drawable* drawable = new osg::ShapeDrawable( cylinder );
	osgManipulator::setDrawableToAlwaysCull( *drawable );
	geode->addDrawable( drawable );
    }

    geode->getOrCreateStateSet()->setMode(GL_NORMALIZE,osg::StateAttribute::ON);

    // MatrixTransform to rotate the geometry according to the normal of the
    // plane.
    osg::MatrixTransform* xform = new osg::MatrixTransform;
    // Create an arrow in the X axis.
    {
	osg::MatrixTransform* arrow = new osg::MatrixTransform;
	arrow->addChild( linegeode );
	arrow->addChild( geode );

	// Rotate X-axis arrow appropriately.
	osg::Quat rotation; rotation.makeRotate(
	    osg::Vec3( 1.0f, 0.0f, 0.0f ), osg::Vec3( 0.0f, 0.0f, 1.0f ) );
	arrow->setMatrix( osg::Matrix( rotation ) );

	xform->addChild( arrow );
    }

    if ( is2dtranslate_ )
    // Create an arrow in the Z axis.
    {
	osg::Group* arrow = new osg::Group;
	arrow->addChild( linegeode );
	arrow->addChild( geode );
	xform->addChild( arrow );
    }

    // Rotate the xform so that the geometry lies on the plane.
    {
	osg::ref_ptr<osgManipulator::PlaneProjector >projector =
	    new osgManipulator::PlaneProjector( osg::Plane( 0.0,1.0,0.0,0.0 ) );
	osg::Vec3 normal = projector->getPlane().getNormal();
	normal.normalize();
	osg::Quat rotation; rotation.makeRotate(
	    osg::Vec3( 0.0f, 1.0f, 0.0f ), normal );
	xform->setMatrix( osg::Matrix( rotation ) );
    }

    return xform;
}


void Dragger::setDisplayTransformation( const mVisTrans* nt )
{
    if ( displaytrans_ == nt )
	return;

    Coord3 crd = getPos();
    visBase::DraggerBase::setDisplayTransformation( nt );
    setPos( crd );
}


void Dragger::setArrowColor( const OD::Color& color )
{
    arrowcolor_ = color;
}


const OD::Color& Dragger::getArrowColor() const
{ return arrowcolor_; }


}; // namespace visBase
