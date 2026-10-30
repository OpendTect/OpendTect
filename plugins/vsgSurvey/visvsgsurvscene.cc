/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgsurvscene.h"

#include "visvsgannotation.h"
#include "visvsgdataman.h"
#include "visvsgevent.h"
#include "visvsgmarkerset.h"
#include "visvsgobject.h"
#include "visvsgplanedragger.h"
#include "visvsgsurvtransmgr.h"
#include "visvsgtexturechannels.h"
#include "visvsgtransform.h"

#include "iopar.h"
#include "keyenum.h"
#include "notify.h"
#include "settings.h"
#include "statrand.h"
#include "string2.h"
#include "survgeom3d.h"
#include "survinfo.h"
#include "trckey.h"
#include "typeset.h"
#include "uistrings.h"
#include "zdomain.h"

#include <vsg/io/Options.h>
#include <vsg/maths/transform.h>
#include <vsg/nodes/Group.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexDraw.h>
#include <vsg/state/material.h>
#include <vsg/utils/GraphicsPipelineConfigurator.h>
#include <vsg/utils/ShaderSet.h>

mCreateDGBVisFactoryEntry( dGBVisSurvey::Scene );

namespace dGBVisSurvey
{

const char* Scene::sKeyShowAnnot()	{ return "Show text"; }
const char* Scene::sKeyShowScale()	{ return "Show scale"; }
const char* Scene::sKeyShowGrid()	{ return "Show grid"; }
const char* Scene::sKeyAnnotColor()	{ return "Annotation color"; }
const char* Scene::sKeyZStretch()	{ return "Z Stretch"; }

static const char* sKeydTectScene()	{ return "dTect.Scene"; }
static const char* sKeyAnnotFont()	{ return "Annotation font"; }
static const char* sKeyZScale()		{ return "Z Scale"; }


void initStdClasses()
{
    mIfNotFirstTime( return );

    dGBVis::Annotation::initClass();
    Scene::initClass();
}


Scene::Scene()
    : dGBVis::DataObjectGroup(vsg::Group::create().get())
    , testchannelschanged(this)
    , sceneboundingboxupdated(this)
    , mouseposchange(this)
    , tkzs_(false)
    , inittkzs_(false)
    , annotscale_(false)
    , zdomaininfo_(&SI().zDomainInfo())
    , zscale_(SI().zScale(false))
{
    ref();
    setup();
    unRefNoDelete();
}


Scene::~Scene()
{
    if ( annot_ )
    {
	mDetachCB( annot_->selection(), Scene::annotSelCB );
	mDetachCB( annot_->deSelection(), Scene::annotDeSelCB );
    }

    detachAllNotifiers();
}


void Scene::setup()
{
    events_ = dGBVis::EventCatcher::create();
    mAttachCB( events_->eventhappened, Scene::mouseCB );
    mAttachCB( events_->nothandled, Scene::notHandledCB );

    annot_ = dGBVis::Annotation::create();
    mAttachCB( annot_->selection(), Scene::annotSelCB );
    mAttachCB( annot_->deSelection(), Scene::annotDeSelCB );

    const Settings& fontsetts = Settings::common();
    BufferString font;
    const char* fontkey = FontData::key( FontData::Graphics3D );
    fontsetts.get( IOPar::compKey("Font.def",fontkey), font );
    if ( font.isEmpty() )
	fontsetts.get( IOPar::compKey(sKeydTectScene(),sKeyAnnotFont()), font );

    FontData fd;
    if ( !font.isEmpty() )
	fd.getFrom( font.buf() );

    annot_->setFont( fd );

    const IOPar& sipars = SI().getPars();
    const BufferString zscalekey =
	IOPar::compKey( sKeyZScale(), zDomainInfo().key() );
    const bool res = sipars.get(zscalekey,curzstretch_) ||
		     sipars.get(sKeyZStretch(),curzstretch_) ||
		     sipars.get(sKeyZScale(),curzstretch_);
    if ( !res && SI().zIsTime() )
    {
	const float dist = Math::Sqrt( SI().getArea(false) );
	curzstretch_ = 2.f * dist / SI().zRange(false).width();
	curzstretch_ /= sCast(float,zDomainInfo().userFactor());
    }

    const TrcKeyZSampling& tkzs = SI().sampling( true );
    updateTransforms( tkzs );
    setTrcKeyZSampling( tkzs );

    addInlCrlZObject( annot_.ptr() );
    updateAnnotationText();

    const Settings& setts = Settings::common();
    bool yn = true;
    setts.getYN( IOPar::compKey(sKeydTectScene(),sKeyShowAnnot()), yn );
    showAnnotText( yn );
    yn = true;
    setts.getYN( IOPar::compKey(sKeydTectScene(),sKeyShowScale()), yn );
    showAnnotScale( yn );
    yn = true;
    setts.getYN( IOPar::compKey(sKeydTectScene(),sKeyShowGrid()), yn );
    showAnnotGrid( yn );

    OD::Color col = getAnnotColor();
    setts.get( IOPar::compKey(sKeydTectScene(),sKeyAnnotColor()), col );
    setAnnotColor( col );
}


void Scene::updateTransforms( const TrcKeyZSampling& tkzs )
{
    if ( !tempzstretchtrans_ )
    {
	tempzstretchtrans_ = mDGBVisTrans::create();
	dGBVis::DataObjectGroup::addObject( tempzstretchtrans_.ptr() );
    }

    RefMan<mDGBVisTrans> newinlcrlrotation = mDGBVisTrans::create();
    RefMan<mDGBVisTrans> newinlcrlscale = mDGBVisTrans::create();

    float zfactor = -1 * zscale_ * curzstretch_;
    if ( zDomainInfo().isTime() )
	zfactor /= 2;
    // -1 to compensate for that we want z to increase with depth

    ConstRefMan<Survey::Geometry3D> s3dgeom = SI().get3DGeometry( true );
    SceneTransformManager::computeICRotationTransform( *s3dgeom.ptr(),
		zfactor, tkzs.zsamp_.center(), newinlcrlrotation.ptr(),
		newinlcrlscale.ptr() );

    tempzstretchtrans_->addObject( newinlcrlrotation.ptr() );

    RefMan<mDGBVisTrans> oldinlcrlrotation = inlcrlrotation_;
    inlcrlrotation_ = newinlcrlrotation;
    inlcrlscale_ = newinlcrlscale;

    if ( oldinlcrlrotation )
    {
	tempzstretchtrans_->removeObject(
		tempzstretchtrans_->getFirstIdx(oldinlcrlrotation.ptr()) );

	for ( int idx=0; idx<oldinlcrlrotation->size(); idx++ )
	{
	    RefMan<dGBVis::DataObject> dobj =
				oldinlcrlrotation->getObject( idx );
	    inlcrlrotation_->addObject( dobj.ptr() );
	    dobj->setDisplayTransformation( inlcrlscale_.ptr() );
	}

	oldinlcrlrotation->removeAll();
    }

    RefMan<mDGBVisTrans> newutm2disptransform = mDGBVisTrans::create();
    SceneTransformManager::computeUTM2DisplayTransform(
		*s3dgeom.ptr(), zfactor, tkzs.zsamp_.center(),
		newutm2disptransform.ptr() );

    if ( utm2disptransform_ )
    {
	for ( int idx=0; idx<tempzstretchtrans_->size(); idx++ )
	{
	    dGBVis::DataObject* dobj = tempzstretchtrans_->getObject( idx );
	    if ( dobj->getDisplayTransformation()==utm2disptransform_.ptr() )
		dobj->setDisplayTransformation( newutm2disptransform.ptr() );
	}
    }

    utm2disptransform_ = newutm2disptransform;
    if ( events_ )
	events_->setUtm2Display( utm2disptransform_.ptr() );

    bindDraggers();
    sceneboundingboxupdated.trigger();
}


void Scene::updateAnnotationText()
{
    if ( !annot_ )
	return;

    setAnnotText( 0, uiStrings::sInline() );
    setAnnotText( 1, uiStrings::sCrossline() );
    setAnnotText( 2, zDomainUserName() );

    const ZDomain::Info& zdom = zDomainInfo();
    TrcKeyZSampling scalecs = annot_->getTrcKeyZSampling();
    TypeSet<double> scalefac( 3, 1. );
    if ( zdom.isDepth() &&
	 ((zdom.isDepthMeter() && SI().depthsInFeet()) ||
	  (zdom.isDepthFeet() && !SI().depthsInFeet())) )
    {
	BufferString unitstr;
	if ( zdom.isDepthMeter() )
	{
	    scalecs.zsamp_.scale( mToFeetFactorD );
	    scalefac[2] = mToFeetFactorD;
	    unitstr = getDistUnitString( true, false );
	}
	else if ( zdom.isDepthFeet() )
	{
	    scalecs.zsamp_.scale( mFromFeetFactorD );
	    scalefac[2] = mFromFeetFactorD;
	    unitstr = getDistUnitString( false, false );
	}

	setAnnotText( 2,
		toUiString("%1 (%2)").arg(zDomainUserName()).arg(unitstr) );
	annot_->setScale( scalecs, scalefac.arr(), scalefac.size(), true );
    }
    else
    {
	scalecs.zsamp_.scale( sCast(float,zdom.userFactor()) );
	scalefac[2] = zdom.userFactor();
	annot_->setScale( scalecs, scalefac.arr(), scalefac.size(), true );
    }
}


bool Scene::isRightHandSystem() const
{
    return SI().isRightHandSystem();
}


const ZDomain::Info& Scene::zDomainInfo() const
{
    return *zdomaininfo_;
}


void Scene::setZDomainInfo( const ZDomain::Info& zdinf )
{
    zdomaininfo_ = &zdinf;
    updateAnnotationText();
}


uiString Scene::zDomainUserName() const
{
    return zDomainInfo().userName();
}


void Scene::setTrcKeyZSampling( const TrcKeyZSampling& tkzs )
{
    setTrcKeyZSampling( tkzs, false );
}


void Scene::setTrcKeyZSampling( const TrcKeyZSampling& tkzs, bool workarea )
{
    tkzs_ = tkzs;
    if ( !workarea )
	inittkzs_ = tkzs;
    else
	tkzs_.zsamp_.step_ = inittkzs_.zsamp_.step_;

    if ( annot_ )
	annot_->setTrcKeyZSampling( tkzs );

    sceneboundingboxupdated.trigger();
}


const TrcKeyZSampling& Scene::getTrcKeyZSampling( bool workarea ) const
{
    return workarea ? tkzs_ : inittkzs_;
}


void Scene::setAnnotScale( const TrcKeyZSampling& tkzs,
			   const double* scalefacs, int nrvals )
{
    annotscale_ = tkzs;
    if ( annot_ )
	annot_->setScale( tkzs, scalefacs, nrvals, false );
}


const TrcKeyZSampling& Scene::getAnnotScale( bool getdefault ) const
{
    return annot_ ? annot_->getScale( getdefault ) : annotscale_;
}


int Scene::size() const
{
    if ( !tempzstretchtrans_ || !inlcrlrotation_ )
	return 0;

    return tempzstretchtrans_->size() + inlcrlrotation_->size();
}


int Scene::getFirstIdx( const dGBVis::DataObject* dobj ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( getObject(idx)==dobj )
	    return idx;
    }

    return -1;
}


int Scene::getFirstIdx( const VisID& did ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const dGBVis::DataObject* obj = getObject( idx );
	if ( obj && obj->id()==did )
	    return idx;
    }

    return -1;
}


dGBVis::DataObject* Scene::getObject( int idx )
{
    if ( !tempzstretchtrans_ || !inlcrlrotation_ || idx<0 )
	return nullptr;

    if ( idx<tempzstretchtrans_->size() )
	return tempzstretchtrans_->getObject( idx );

    return inlcrlrotation_->getObject( idx-tempzstretchtrans_->size() );
}


void Scene::addUTMObject( dGBVis::VisualObject* obj )
{
    if ( !obj )
	return;

    obj->setDisplayTransformation( utm2disptransform_.ptr() );
    tempzstretchtrans_->addObject( obj );
    sceneboundingboxupdated.trigger();
}


void Scene::addInlCrlZObject( dGBVis::DataObject* obj )
{
    if ( !obj )
	return;

    obj->setDisplayTransformation( inlcrlscale_.ptr() );
    inlcrlrotation_->addObject( obj );

    mDynamicCastGet(dGBVis::DepthTabPlaneDragger*,dragger,obj)
    if ( dragger )
    {
	dragger->setNodeTransforms( tempzstretchtrans_.ptr(),
				    inlcrlrotation_.ptr() );
	dragger->setEventCatcher( events_.ptr() );
    }

    sceneboundingboxupdated.trigger();
}


void Scene::addObject( dGBVis::DataObject* obj )
{
    mDynamicCastGet(dGBVis::VisualObject*,vo,obj)
    if ( vo )
	addUTMObject( vo );

    sceneboundingboxupdated.trigger();
}


void Scene::insertObject( int, dGBVis::DataObject* obj )
{
    addObject( obj );
}


void Scene::removeObject( int idx )
{
    if ( idx<0 || idx>=size() )
	return;

    if ( idx<tempzstretchtrans_->size() )
	tempzstretchtrans_->removeObject( idx );
    else
	inlcrlrotation_->removeObject( idx-tempzstretchtrans_->size() );

    sceneboundingboxupdated.trigger();
}


void Scene::setFixedZStretch( float zstretch )
{
    if ( mIsEqual(zstretch,curzstretch_,mDefEps) )
	return;

    curzstretch_ = zstretch;
    updateTransforms( tkzs_ );
}


float Scene::getFixedZStretch() const
{
    return curzstretch_;
}


float Scene::getTempZStretch() const
{
    return tempzstretchtrans_ ? mCast(float,tempzstretchtrans_->getScale().z_)
			      : 1.f;
}


void Scene::setTempZStretch( float zstretch )
{
    if ( !tempzstretchtrans_ )
    {
	pErrMsg("No z-transform");
	return;
    }

    tempzstretchtrans_->setScale( Coord3(1,1,zstretch) );
    sceneboundingboxupdated.trigger();
}


void Scene::setZScale( float zscale )
{
    zscale_ = zscale;
    updateTransforms( tkzs_ );
}


float Scene::getZScale() const
{
    return zscale_;
}


float Scene::getApparentVelocity( float zstretch ) const
{
    if ( !zDomainInfo().isTime() )
	return zstretch;

    //The depth when t=1 s
    //in xy units
    float depthat1sec = zscale_ * zstretch / 2;

    //In meters
    if ( SI().xyInFeet() )
	depthat1sec *= mFromFeetFactorF;

    //in depth units
    if ( SI().zInFeet() && SI().depthsInFeet() )
	depthat1sec *= mToFeetFactorF;

    //compensate for twt
    return 2*depthat1sec;
}


const mDGBVisTrans* Scene::getTempZStretchTransform() const
{ return tempzstretchtrans_.ptr(); }


const mDGBVisTrans* Scene::getInlCrl2DisplayTransform() const
{ return inlcrlrotation_.ptr(); }


const mDGBVisTrans* Scene::getUTM2DisplayTransform() const
{ return utm2disptransform_.ptr(); }


void Scene::showAnnotText( bool yn )
{ annot_->showText( yn ); }


bool Scene::isAnnotTextShown() const
{ return annot_->isTextShown(); }


void Scene::showAnnotScale( bool yn )
{ annot_->showScale( yn ); }


bool Scene::isAnnotScaleShown() const
{ return annot_->isScaleShown(); }


void Scene::showAnnotGrid( bool yn )
{ annot_->showGridLines( yn ); }


bool Scene::isAnnotGridShown() const
{ return annot_->isGridLinesShown(); }


void Scene::showAnnot( bool yn )
{ annot_->turnOn( yn ); }


bool Scene::isAnnotShown() const
{ return annot_->isOn(); }


const FontData& Scene::getAnnotFont() const
{
    return annot_->getFont();
}


void Scene::setAnnotFont( const FontData& fd )
{
    annot_->setFont( fd );
}


void Scene::setAnnotText( int dim, const uiString& txt )
{
    if ( (dim==0 && SI().inlRange(true).width()>0) ||
	 (dim==1 && SI().crlRange(true).width()>0) ||
	 (dim==2 && SI().zRange(true).width()>0) )
	annot_->setText( dim, txt );
}


void Scene::setAnnotColor( const OD::Color& col )
{
    annot_->setColor( col );
}


const OD::Color Scene::getAnnotColor() const
{
    return annot_->getColor();
}


namespace
{

void fillSolidQuad( const vsg::ref_ptr<vsg::StateGroup>& group,
		    vsg::ref_ptr<vsg::vec3Array>& vertices,
		    vsg::ref_ptr<vsg::vec4Array>& colors,
		    const vsg::vec4& color )
{
    vertices = vsg::vec3Array::create( 6 );
    vertices->properties.dataVariance = vsg::DYNAMIC_DATA;
    colors = vsg::vec4Array::create( 1 );
    colors->properties.dataVariance = vsg::DYNAMIC_DATA;
    colors->at( 0 ) = color;

    vsg::ref_ptr<vsg::ShaderSet> shaderset = vsg::createFlatShadedShaderSet();
    vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> config =
			vsg::GraphicsPipelineConfigurator::create( shaderset );
    vsg::DataList arrays;
    config->assignArray( arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX,
			 vertices );
    config->assignArray( arrays, "vsg_Color", VK_VERTEX_INPUT_RATE_INSTANCE,
			 colors );
    vsg::ref_ptr<vsg::PhongMaterialValue> mat =
			vsg::PhongMaterialValue::create();
    mat->value().diffuse.set( 1.f, 1.f, 1.f, 1.f );
    config->assignDescriptor( "material", mat );
    config->init();

    vsg::ref_ptr<vsg::VertexDraw> draw = vsg::VertexDraw::create();
    draw->assignArrays( arrays );
    draw->vertexCount = 6;
    draw->instanceCount = 1;
    config->copyTo( group );
    group->addChild( draw );
}


class PickQuad : public dGBVis::VisualObjectImpl
{
public:
    static RefMan<PickQuad> create( const Coord3& center, const Coord3& half,
				    const OD::Color& col )
    {
	RefMan<PickQuad> ret = new PickQuad( center, half, col );
	return ret;
    }

    void setDisplayTransformation( const mDGBVisTrans* trans ) override
    {
	display_ = trans;
	rebuild();
    }

private:
    PickQuad( const Coord3& center, const Coord3& half, const OD::Color& col )
	: VisualObjectImpl(true)
	, center_(center)
	, half_(half)
	, basecolor_(col)
    {
	setPickable( true, true );
	node_ = vsg::StateGroup::create();
	fillSolidQuad( node_, vertices_, colors_, dGBVis::toVec4(col,1.f) );
	addChild( node_.get() );
	selection()->notify( mCB(this,PickQuad,selCB) );
	deSelection()->notify( mCB(this,PickQuad,deSelCB) );
	rebuild();
    }

    ~PickQuad()
    {
	if ( selection() )
	    selection()->remove( mCB(this,PickQuad,selCB) );
	if ( deSelection() )
	    deSelection()->remove( mCB(this,PickQuad,deSelCB) );
    }

    void selCB( CallBacker* )
    {
	setColor( OD::Color(255,220,0) );
    }

    void deSelCB( CallBacker* )
    {
	setColor( basecolor_ );
    }

    void setColor( const OD::Color& col )
    {
	if ( !colors_ )
	    return;

	colors_->at( 0 ) = dGBVis::toVec4( col, 1.f );
	colors_->dirty();
	requestSingleRedraw();
    }

    void rebuild()
    {
	if ( !vertices_ )
	    return;

	Coord3 corner[4];
	corner[0] = Coord3( center_.x_-half_.x_, center_.y_-half_.y_,
			    center_.z_ );
	corner[1] = Coord3( center_.x_+half_.x_, center_.y_-half_.y_,
			    center_.z_ );
	corner[2] = Coord3( center_.x_+half_.x_, center_.y_+half_.y_,
			    center_.z_ );
	corner[3] = Coord3( center_.x_-half_.x_, center_.y_+half_.y_,
			    center_.z_ );
	const int order[6] = { 0, 1, 2, 0, 2, 3 };
	for ( int idx=0; idx<6; idx++ )
	{
	    Coord3 disp = corner[order[idx]];
	    dGBVis::Transformation::transform( display_.ptr(),
					       corner[order[idx]], disp );
	    vertices_->at( idx ) = vsg::vec3( float(disp.x_), float(disp.y_),
					      float(disp.z_) );
	}

	vertices_->dirty();
	requestSingleRedraw();
    }

    Coord3				center_;
    Coord3				half_;
    OD::Color				basecolor_;
    ConstRefMan<mDGBVisTrans>		display_;
    vsg::ref_ptr<vsg::StateGroup>	node_;
    vsg::ref_ptr<vsg::vec3Array>	vertices_;
    vsg::ref_ptr<vsg::vec4Array>	colors_;
};


} // namespace


void Scene::setInteractionEnabled( bool yn )
{
    interact_ = yn;
}


void Scene::setEventCatcher( dGBVis::EventCatcher* catcher )
{
    if ( events_.ptr() == catcher )
	return;

    if ( events_ )
    {
	mDetachCB( events_->eventhappened, Scene::mouseCB );
	mDetachCB( events_->nothandled, Scene::notHandledCB );
    }

    events_ = catcher;
    if ( !events_ )
	return;

    mAttachCB( events_->eventhappened, Scene::mouseCB );
    mAttachCB( events_->nothandled, Scene::notHandledCB );
    events_->setUtm2Display( utm2disptransform_.ptr() );
    bindDraggers();
}


void Scene::bindDraggers()
{
    if ( !inlcrlrotation_ )
	return;

    for ( int idx=0; idx<inlcrlrotation_->size(); idx++ )
    {
	mDynamicCastGet(dGBVis::DepthTabPlaneDragger*,dragger,
			inlcrlrotation_->getObject(idx))
	if ( !dragger )
	    continue;

	dragger->setNodeTransforms( tempzstretchtrans_.ptr(),
				    inlcrlrotation_.ptr() );
	dragger->setEventCatcher( events_.ptr() );
    }
}


TrcKeyValue Scene::getMousePos() const
{
    if ( mousetrckey_.isUdf() || !xytmousepos_.isDefined() )
	return TrcKeyValue::udf();

    return TrcKeyValue( mousetrckey_, float(xytmousepos_.z_) );
}


uiString Scene::getMousePosString() const
{
    return mouseposstr_;
}


void Scene::applyEvent( const dGBVis::EventInfo& eventinfo )
{
    updateStatus( eventinfo );
    if ( interact_ )
	updateSelection( eventinfo );
}


void Scene::mouseCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const dGBVis::EventInfo&,eventinfo,cb);
    updateStatus( eventinfo );
}


void Scene::notHandledCB( CallBacker* cb )
{
    if ( !interact_ )
	return;

    mCBCapsuleUnpack(const dGBVis::EventInfo&,eventinfo,cb);
    updateMarkerInteraction( eventinfo );
    if ( events_ && events_->isHandled() )
	return;

    updateSelection( eventinfo );
}


void Scene::updateStatus( const dGBVis::EventInfo& eventinfo )
{
    if ( eventinfo.type!=dGBVis::MouseMovement &&
	 eventinfo.type!=dGBVis::MouseClick )
	return;

    mouseposstr_.setEmpty();
    xytmousepos_ = Coord3::udf();
    mousetrckey_.setUdf();

    if ( eventinfo.pickedobjids.isEmpty() ||
	 !eventinfo.worldpickedpos.isDefined() )
    {
	mouseposchange.trigger();
	return;
    }

    xytmousepos_ = eventinfo.worldpickedpos;

    if ( xytmousepos_.isDefined() )
	mousetrckey_ = TrcKey( SI().transform(
			    Coord(xytmousepos_.x_,xytmousepos_.y_)) );

    if ( !mousetrckey_.isUdf() )
    {
	const double zuser = xytmousepos_.z_ * zDomainInfo().userFactor();
	BufferString zname;
	zDomainUserName().getFullString( &zname );
	BufferString txt;
	txt.add( "Inl: " ).add( mousetrckey_.inl() )
	   .add( "  Crl: " ).add( mousetrckey_.crl() )
	   .add( "  " ).add( zname.buf() )
	   .add( ": " ).add( zuser );
	mouseposstr_ = toUiString( txt.buf() );
    }

    mouseposchange.trigger();
}


int Scene::markerUnderCursor( const dGBVis::EventInfo& eventinfo ) const
{
    if ( !picks_ || !events_ )
	return -1;

    const int idx = events_->pickedMarkerIndex();
    if ( idx<0 || !eventinfo.pickedobjids.isPresent(picks_->id()) )
	return -1;

    return idx;
}


bool Scene::surfaceUnderCursor( const Coord& mousepos,
				Coord3& displaypos )
{
    if ( !events_ || !picks_ )
	return false;

    const bool active = picks_->isPickable( true );
    const bool passive = picks_->isPickable( false );
    picks_->setPickable( false, false );
    const bool hit = events_->pickDisplayPos( mousepos, displaypos );
    picks_->setPickable( active, passive );
    return hit;
}


void Scene::updateMarkerInteraction( const dGBVis::EventInfo& eventinfo )
{
    if ( !picks_ || !events_ )
	return;

    const int buttons = int( eventinfo.buttonstate_ );
    const bool left = buttons & int(OD::LeftButton);
    if ( eventinfo.type==dGBVis::MouseClick && eventinfo.pressed && left )
    {
	const int under = markerUnderCursor( eventinfo );
	if ( under < 0 )
	    return;

	draggingmarker_ = under;
	events_->setHandled();
	return;
    }

    if ( draggingmarker_ < 0 )
	return;

    if ( eventinfo.type==dGBVis::MouseMovement && left )
    {
	Coord3 displaypos;
	Coord3 objectpos;
	if ( surfaceUnderCursor(eventinfo.mousepos,displaypos) &&
	     displayToInlCrl(displaypos,objectpos) )
	    picks_->setPos( draggingmarker_, objectpos );

	events_->setHandled();
	return;
    }

    if ( eventinfo.type==dGBVis::MouseClick && !eventinfo.pressed && left )
    {
	const int idx = draggingmarker_;
	draggingmarker_ = -1;
	if ( !eventinfo.dragging && (buttons & int(OD::ControlButton)) )
	    picks_->removePos( idx );

	events_->setHandled();
    }
}


void Scene::updateSelection( const dGBVis::EventInfo& eventinfo )
{
    if ( eventinfo.type != dGBVis::MouseClick )
	return;

    const int buttons = int( eventinfo.buttonstate_ );
    dGBVis::DataObject* hit = nullptr;
    for ( const auto& id : eventinfo.pickedobjids )
    {
	dGBVis::DataObject* obj = dGBVis::DM().getObject( id );
	if ( obj && (obj->selectable() || obj->rightClickable()) )
	{
	    hit = obj;
	    break;
	}
    }

    if ( (buttons & int(OD::LeftButton)) && eventinfo.pressed )
    {
	if ( hit && hit->selectable() && events_ )
	    events_->setHandled();
	return;
    }

    if ( (buttons & int(OD::LeftButton)) && !eventinfo.pressed &&
	 !eventinfo.dragging )
    {
	const bool ctrl = buttons & int(OD::ControlButton);
	const bool surface =
		dynamic_cast<dGBVis::DepthTabPlaneDragger*>(hit) ||
		dynamic_cast<PickQuad*>(hit);
	if ( ctrl )
	{
	    if ( surface && eventinfo.displaypickedpos.isDefined() )
		addSurfacePick( eventinfo.displaypickedpos );
	    return;
	}

	if ( hit && hit->selectable() )
	    dGBVis::DM().selMan().select( hit->id() );
	else
	    dGBVis::DM().selMan().deSelectAll();
	return;
    }

    if ( (buttons & int(OD::RightButton)) && !eventinfo.pressed &&
	 !eventinfo.dragging && hit && hit->rightClickable() )
    {
	mDynamicCastGet(dGBVis::VisualObject*,vo,hit)
	if ( vo )
	    vo->rightClick();
    }
}


void Scene::addSurfacePick( const Coord3& displaypos )
{
    Coord3 objectpos;
    if ( !displayToInlCrl(displaypos,objectpos) )
	return;

    if ( !picks_ )
    {
	picks_ = dGBVis::MarkerSet::create();
	picks_->setScreenSize( 5.f );
	picks_->setMarkersSingleColor( OD::Color::Red() );
	if ( events_ )
	    picks_->setCamera( events_->camera() );
	addInlCrlZObject( picks_.ptr() );
    }

    picks_->addPos( objectpos );
}


bool Scene::displayToInlCrl( const Coord3& displaypos, Coord3& objectpos ) const
{
    if ( !inlcrlscale_ )
	return false;

    Coord3 localpt = displaypos;
    if ( tempzstretchtrans_ || inlcrlrotation_ )
    {
	vsg::dmat4 world = tempzstretchtrans_
		? tempzstretchtrans_->getMatrix()
		: inlcrlrotation_->getMatrix();
	if ( tempzstretchtrans_ && inlcrlrotation_ )
	    world = tempzstretchtrans_->getMatrix() *
		    inlcrlrotation_->getMatrix();

	const vsg::dvec4 local = vsg::inverse( world ) *
		vsg::dvec4( displaypos.x_, displaypos.y_, displaypos.z_, 1. );
	localpt = Coord3( local.x, local.y, local.z );
    }

    dGBVis::Transformation::transformBack( inlcrlscale_.ptr(), localpt,
					   objectpos );
    return objectpos.isDefined();
}


void Scene::annotSelCB( CallBacker* )
{
    if ( !annot_ )
	return;

    annotsavedcolor_ = annot_->getColor();
    annot_->setColor( OD::Color(255,220,0) );
}


void Scene::annotDeSelCB( CallBacker* )
{
    if ( annot_ )
	annot_->setColor( annotsavedcolor_ );
}


static RefMan<dGBVis::TextureChannels> createTestChannels( int nrcells,
						    Stats::RandGen& valuegen )
{
    TypeSet<float> values;
    for ( int idx=0; idx<nrcells*nrcells; idx++ )
	values += idx%50==49 ? mUdf(float) : float(valuegen.get());

    RefMan<dGBVis::TextureChannels> channels = dGBVis::TextureChannels::create();
    channels->setSize( 0, 1, nrcells, nrcells );
    ColTab::MapperSetup mappersetup;
    mappersetup.type_ = ColTab::MapperSetup::Auto;
    channels->setColTabMapperSetup( 0, mappersetup );
    channels->setColTabSequence( ColTab::Sequence("") );
    channels->setUnMappedData( 0, 0, values.arr(), OD::UsePtr, nullptr );
    return channels;
}


void Scene::addInteractionTest()
{
    if ( testsceneadded_ || !tkzs_.isDefined() )
	return;

    const StepInterval<int> inlrg = tkzs_.hsamp_.inlRange();
    const StepInterval<int> crlrg = tkzs_.hsamp_.crlRange();
    const StepInterval<float>& zrg = tkzs_.zsamp_;
    const Coord3 center( 0.5*(inlrg.start_+inlrg.stop_),
			 0.5*(crlrg.start_+crlrg.stop_),
			 0.5*(zrg.start_+zrg.stop_) );
    const Coord3 size( mMAX(1,inlrg.width())*0.5,
		       mMAX(1,crlrg.width())*0.5,
		       0. );

    const int nrcells = dGBVis::DepthTabPlaneDragger::nrCells();
    Stats::RandGen valuegen;

    RefMan<dGBVis::DepthTabPlaneDragger> dragger =
			dGBVis::DepthTabPlaneDragger::create();
    dragger->setDim( 2 );
    dragger->setSize( size );
    dragger->setCenter( center );
    dragger->setSpaceLimits(
	Interval<float>(float(inlrg.start_),float(inlrg.stop_)),
	Interval<float>(float(crlrg.start_),float(crlrg.stop_)),
	Interval<float>(zrg.start_,zrg.stop_) );
    dragger->setChannels( createTestChannels(nrcells,valuegen).ptr() );
    mAttachCB( dragger->started, Scene::draggerStartedCB );
    addInlCrlZObject( dragger.ptr() );

    const Coord3 size2( mMAX(1,inlrg.width())*0.25,
			mMAX(1,crlrg.width())*0.25,
			0. );
    Coord3 center2 = center;
    center2.x_ += inlrg.width() * 0.12;
    center2.y_ -= crlrg.width() * 0.12;
    center2.z_ -= mMAX( 1e-3, double(zrg.width())*0.25 );
    RefMan<dGBVis::DepthTabPlaneDragger> inner =
			dGBVis::DepthTabPlaneDragger::create();
    inner->setDim( 2 );
    inner->setColor( OD::Color(230,140,40) );
    inner->setSize( size2 );
    inner->setCenter( center2 );
    inner->setSpaceLimits(
	Interval<float>(float(inlrg.start_),float(inlrg.stop_)),
	Interval<float>(float(crlrg.start_),float(crlrg.stop_)),
	Interval<float>(zrg.start_,zrg.stop_) );
    inner->setChannels( createTestChannels(nrcells,valuegen).ptr() );
    mAttachCB( inner->started, Scene::draggerStartedCB );
    addInlCrlZObject( inner.ptr() );

    const Coord3 half( mMAX(1,inlrg.width())*0.08,
		       mMAX(1,crlrg.width())*0.08, 0. );
    const double zoff = mMAX( 1e-3, double(zrg.width())*0.15 );
    Coord3 q1 = center;
    q1.x_ -= inlrg.width() * 0.2;
    q1.z_ += zoff;
    Coord3 q2 = center;
    q2.x_ += inlrg.width() * 0.2;
    q2.z_ += zoff;
    addInlCrlZObject( PickQuad::create(q1,half,OD::Color(220,60,60)).ptr() );
    addInlCrlZObject( PickQuad::create(q2,half,OD::Color(60,180,80)).ptr() );

    testchannels_ = createTestChannels( nrcells, valuegen );
    Coord3 center3 = center;
    center3.z_ -= mMAX( 1e-3, double(zrg.width())*0.05 );
    RefMan<dGBVis::DepthTabPlaneDragger> colored =
			dGBVis::DepthTabPlaneDragger::create();
    colored->setDim( 2 );
    colored->setSize( size );
    colored->setCenter( center3 );
    colored->setSpaceLimits(
	Interval<float>(float(inlrg.start_),float(inlrg.stop_)),
	Interval<float>(float(crlrg.start_),float(crlrg.stop_)),
	Interval<float>(zrg.start_,zrg.stop_) );
    colored->setChannels( testchannels_.ptr() );
    mAttachCB( colored->started, Scene::draggerStartedCB );
    addInlCrlZObject( colored.ptr() );
    testsceneadded_ = true;
}


void Scene::draggerStartedCB( CallBacker* cb )
{
    mDynamicCastGet(dGBVis::DepthTabPlaneDragger*,dragger,cb)
    dGBVis::TextureChannels* channels = dragger ? dragger->getChannels()
						: nullptr;
    if ( !channels || channels==testchannels_.ptr() )
	return;

    testchannels_ = channels;
    testchannelschanged.trigger();
}

} // namespace dGBVisSurvey
