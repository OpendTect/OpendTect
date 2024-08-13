/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ui3dviewer.h"
#ifdef OD_USE_QOPENGL
# include "od3dviewer.h"
#else
# include "ui3dviewerbody.h"
#endif

#include "iopar.h"
#include "keybindings.h"
#include "keystrs.h"
#include "mouseevent.h"
#include "ptrman.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "visosg.h"

#include "uimain.h"
#include "uiobjbody.h"
#include "visaxes.h"
#include "visdataman.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "vissurvscene.h"


static const char* sKeydTectScene()	{ return "dTect.Scene."; }
static const char* sKeyHomePos()	{ return "Home position"; }
static const char* sKeyWheelDisplayMode() { return "Wheel Display Mode"; }

StringView ui3DViewer::sKeyBindingSettingsKey()
{
    return KeyBindings::sSettingsKey();
}

mDefineNameSpaceEnumUtils(OD,StereoType,"StereoType")
{ sKey::None().str(), "RedCyan", "QuadBuffer", nullptr };

mDefineNameSpaceEnumUtils(OD,WheelMode,"WheelMode")
{ "Never", "Always", "On Hover", nullptr };


// ui3DViewer

ui3DViewer::ui3DViewer( uiParent* parnt, bool direct, const char* nm )
    : uiObject(parnt,nm,mkBody(parnt,direct,nm))
    , destroyed(this)
    , viewmodechanged(this)
    , pageupdown(this)
    , vmcb(0)
{
    PtrMan<IOPar> homepospar = SI().pars().subselect( sKeyHomePos() );
    if ( homepospar )
	osgbody_->setHomePos( *homepospar) ;

    setViewMode( false );  // switches between view & interact mode

    bool enabanimate = false;
    bool res = Settings::common().getYN(
	BufferString(sKeydTectScene(),sKeyAnimate()), enabanimate );
    if ( res )
	enableAnimation( enabanimate );

    BufferString modestr;
    res = Settings::common().get(
	BufferString(sKeydTectScene(),sKeyWheelDisplayMode()), modestr );
    if ( res )
    {
	OD::WheelMode mode;
	OD::parseEnum( modestr, mode );
	setWheelDisplayMode( mode );
    }

    float zoomfactor = MouseEvent::getDefaultMouseWheelZoomFactor();
#ifdef __mac__
    zoomfactor = MouseEvent::getDefaultTrackpadZoomFactor();
#endif

    Settings::common().get( SettingsAccess::sKeyMouseWheelZoomFactor(),
			    zoomfactor );
    setMouseWheelZoomFactor( zoomfactor );
}


ui3DViewer::~ui3DViewer()
{
    detachAllNotifiers();
    delete osgbody_;
}


uiObjectBody& ui3DViewer::mkBody( uiParent* parnt, bool direct, const char* nm )
{
#if OSG_VERSION_LESS_THAN( 3, 5, 0 )
    initQtWindowingSystem();
#endif

#ifdef OD_USE_QOPENGL
    osgbody_ = new OD3DViewer( *this, parnt );
#else
    osgbody_ = new ui3DViewerBody( *this, parnt );
#endif

    osgbody_->setName( nm );
    return *osgbody_;
}


void ui3DViewer::setMapView( bool yn )
{
    osgbody_->setMapView( yn );
}


bool ui3DViewer::isMapView() const
{
    return osgbody_->isMapView();
}


void ui3DViewer::viewAll( bool animate )
{
    mDynamicCastGet(visSurvey::Scene*,survscene,getScene());
    if ( !survscene )
    {
	osgbody_->viewAll( animate );
    }
    else
    {
	bool showtext = survscene->isAnnotTextShown();
	bool showscale = survscene->isAnnotScaleShown();
	if ( !showtext && !showscale )
	{
	    osgbody_->viewAll(animate);
	}
	else
	{
	    survscene->showAnnotText( false );
	    survscene->showAnnotScale( false );
	    osgbody_->viewAll(animate);
	    survscene->showAnnotText( showtext );
	    survscene->showAnnotScale( showscale );
	}
    }
}


void ui3DViewer::enableAnimation( bool yn )
{ if ( osgbody_ ) osgbody_->setAnimationEnabled( yn ); }

bool ui3DViewer::isAnimationEnabled() const
{ return osgbody_ ? osgbody_->isAnimationEnabled() : false; }


void ui3DViewer::setBackgroundColor( const OD::Color& col )
{
    osgbody_->setBackgroundColor( col );
}

OD::Color ui3DViewer::getBackgroundColor() const
{
    return osgbody_->getBackgroundColor();
}


void ui3DViewer::setAnnotationColor( const OD::Color& col )
{
    osgbody_->setAnnotColor( col );
}


OD::Color ui3DViewer::getAnnotationColor() const
{
    mDynamicCastGet(const visSurvey::Scene*,survscene,getScene());
    return survscene ? survscene->getAnnotColor() : OD::Color::White();
}


void ui3DViewer::setAnnotationFont( const FontData& fd )
{
    osgbody_->setAnnotationFont( fd );
}


void ui3DViewer::getAllKeyBindings( BufferStringSet& keys ) const
{
    osgbody_->keyBindMan().getAllKeyBindings( keys );
}


void ui3DViewer::setKeyBindings( const char* keybindname )
{
    osgbody_->keyBindMan().setKeyBindings( keybindname );
}


const char* ui3DViewer::getCurrentKeyBindings() const
{
    return osgbody_->keyBindMan().getCurrentKeyBindings();
}


void ui3DViewer::viewPlane( PlaneType type )
{
    if ( isMapView() )
	return;

    switch ( type )
    {
	case X:		osgbody_->viewPlaneX(); break;
	case Y:		osgbody_->viewPlaneN(); break;
	case Z:		osgbody_->viewPlaneZ(); break;
	case Inl:	osgbody_->viewPlaneInl(); break;
	case Crl:	osgbody_->viewPlaneCrl(); break;
	case YZ:	osgbody_->viewPlaneYZ(); break;
    }
}


void ui3DViewer::setStartupView()
{
    osgbody_->setStartupView();
}


bool ui3DViewer::isCameraPerspective() const
{
    return osgbody_->isCameraPerspective();
}


void ui3DViewer::setCameraPerspective( bool yn )
{
    osgbody_->setCameraPerspective( yn );
}


bool ui3DViewer::setStereoType( OD::StereoType type )
{
    return osgbody_->setStereoType( type );
}


OD::StereoType ui3DViewer::getStereoType() const
{
    return osgbody_->getStereoType();
}


void ui3DViewer::setStereoOffset( float offset )
{
    osgbody_->setStereoOffset( offset );
}


float ui3DViewer::getStereoOffset() const
{
    return osgbody_->getStereoOffset();
}

void ui3DViewer::setMouseWheelZoomFactor( float factor )
{
    osgbody_->setMouseWheelZoomFactor( factor );
}


float ui3DViewer::getMouseWheelZoomFactor() const
{
    return osgbody_->getMouseWheelZoomFactor();
}


void ui3DViewer::setReversedMouseWheelDirection( bool reversed )
{
    osgbody_->setReversedMouseWheelDirection( reversed );
}


bool ui3DViewer::getReversedMouseWheelDirection() const
{ return osgbody_->getReversedMouseWheelDirection(); }


void ui3DViewer::setScene( visBase::Scene* scene )
{
    osgbody_->setScene( scene );
}


SceneID ui3DViewer::sceneID() const
{
    return osgbody_->getScene() ? osgbody_->getSceneID() : SceneID::udf();
}


void ui3DViewer::setViewMode( bool yn )
{
    if ( yn==isViewMode() )
	return;

    osgbody_->setViewMode( yn, false );
}


bool ui3DViewer::isViewMode() const
{ return osgbody_->isViewMode(); }

void ui3DViewer::rotateH( float angle )
{ osgbody_->uiRotate( angle, true ); }

void ui3DViewer::rotateV( float angle )
{ osgbody_->uiRotate( angle, false ); }

void ui3DViewer::dolly( float rel )
{ osgbody_->uiZoom( rel ); }

void ui3DViewer::setCameraZoom( float val )
{ osgbody_->setCameraZoom( val ); }

float ui3DViewer::getCameraZoom()
{ return osgbody_->getCameraZoom(); }

const Coord3 ui3DViewer::getCameraPosition() const
{ return osgbody_->getCameraPosition(); }

void ui3DViewer::align()
{ osgbody_->align(); }

void ui3DViewer::toHomePos()
{ setStartupView(); }

void ui3DViewer::saveHomePos()
{ osgbody_->saveHomePos(); }

void ui3DViewer::resetHomePos()
{ osgbody_->resetHomePos(); }

void ui3DViewer::showRotAxis( bool yn )
{ osgbody_->showRotAxis( yn ); }

void ui3DViewer::setWheelDisplayMode( OD::WheelMode mode )
{ osgbody_->setWheelDisplayMode( mode ); }

OD::WheelMode ui3DViewer::getWheelDisplayMode() const
{ return osgbody_->getWheelDisplayMode(); }

bool ui3DViewer::rotAxisShown() const
{ return osgbody_->isAxisShown(); }

visBase::PolygonSelection* ui3DViewer::getPolygonSelector()
{ return osgbody_->getPolygonSelector(); }

visBase::SceneColTab* ui3DViewer::getSceneColTab()
{ return osgbody_->getSceneColTab(); }

void ui3DViewer::toggleCameraType()
{ osgbody_->toggleCameraType(); }

Geom::Size2D<int> ui3DViewer::getViewportSizePixels() const
{ return osgbody_->getViewportSizePixels(); }


void ui3DViewer::savePropertySettings() const
{
#define mSaveProp(set,str,func) \
    Settings::common().set( BufferString(sKeydTectScene(),str), func );

    mSaveProp( set, sKeyBGColor(), getBackgroundColor() );
    mSaveProp( setYN, sKeyAnimate(), isAnimationEnabled() );
    mSaveProp( set, sKeyWheelDisplayMode(),
	       getWheelModeString(getWheelDisplayMode()) );
    Settings::common().write();
}


void ui3DViewer::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), getScene()->uiName() );
    par.set( sKeySceneID(), getScene()->id() );
    par.set( sKeyBGColor(), (int)getBackgroundColor().rgb() );
    par.set( sKeyStereo(), toString( getStereoType() ) );
    float offset = getStereoOffset();
    par.set( sKeyStereoOff(), offset );
    par.setYN( sKeyPersCamera(), isCameraPerspective() );
    par.setYN( sKeyMapView(), isMapView() );

    osgbody_->fillCameraPos( par );
}


bool ui3DViewer::usePar( const IOPar& par )
{
    SceneID sceneid;
    if ( !par.get(sKeySceneID(),sceneid) )
	return false;

    const VisID scenevisid( sceneid.asInt() );
    visBase::DataObject* obj = visBase::DM().getObject( scenevisid );
    RefMan<visBase::Scene> scene = dCast(visBase::Scene*,obj);
    if ( !scene )
	return false;

    setScene( scene.ptr() );
    scene = getScene();
    if ( !scene )
	return false;

    uiString scenenm;
    if ( par.get(sKey::Name(),scenenm) )
	scene->setUiName( scenenm );

    int col;
    if ( par.get(sKeyBGColor(),col) )
    {
	OD::Color newcol;
	newcol.setRgb( col );
	setBackgroundColor( newcol );
    }

    OD::StereoType stereotype;
    if ( OD::parseEnum( par, sKeyStereo(), stereotype ) )
	setStereoType( stereotype );

    float offset;
    if ( par.get( sKeyStereoOff(), offset )  )
	setStereoOffset( offset );

    bool iscameraperspective;
    if ( par.getYN(sKeyPersCamera(),iscameraperspective) )
	setCameraPerspective( iscameraperspective );

    PtrMan<IOPar> homepos = par.subselect( sKeyHomePos() );
    if ( homepos && osgbody_->isHomePosEmpty() )
	osgbody_->setHomePos( *homepos );

    bool mapview;
    if ( par.getYN(sKeyMapView(),mapview) )
	setMapView( mapview );

    osgbody_->useCameraPos( par );
    return true;
}


visBase::Scene* ui3DViewer::getScene()
{ return osgbody_->getScene(); }

const visBase::Scene* ui3DViewer::getScene() const
{ return const_cast<ui3DViewer*>(this)->getScene(); }

const osgViewer::View*	ui3DViewer::getOsgViewerMainView() const
{ return osgbody_->getOsgViewerMainView(); }

const osgViewer::View*	ui3DViewer::getOsgViewerHudView() const
{ return osgbody_->getOsgViewerHudView(); }

void ui3DViewer::setScenesPixelDensity(float dpi)
{ osgbody_->setScenesPixelDensity( dpi ); }

float ui3DViewer::getScenesPixelDensity() const
{ return getScene()->getPixelDensity(); }
