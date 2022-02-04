/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
________________________________________________________________________

-*/

#include "ui3dviewer.h"
#include "ui3dviewerbody.h"

#include "iopar.h"
#include "keybindings.h"
#include "keystrs.h"
#include "settingsaccess.h"
#include "survinfo.h"

#include "visaxes.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "vissurvscene.h"
#include "visthumbwheel.h"

static const char* sKeydTectScene()	{ return "dTect.Scene."; }
static const char* sKeyWheelDisplayMode() { return "Wheel Display Mode"; }

FixedString ui3DViewer::sKeyBindingSettingsKey()
{
    return KeyBindings::sSettingsKey();
}

mDefineEnumUtils(ui3DViewer,StereoType,"StereoType")
{ sKey::None().str(), "RedCyan", "QuadBuffer", 0 };

mDefineEnumUtils(ui3DViewer,WheelMode,"WheelMode")
{ "Never", "Always", "On Hover", 0 };


ui3DViewer::ui3DViewer( uiParent* parnt, const char* nm )
    : uiObject(parnt,nm,mkBody(parnt,nm))
    , destroyed(this)
    , viewmodechanged(this)
    , pageupdown(this)
    , vmcb(0)
{
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
	WheelMode mode; parseEnum( modestr, mode );
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
    delete osgbody_;
}


uiObjectBody& ui3DViewer::mkBody( uiParent* parnt, const char* nm )
{
#if OSG_VERSION_LESS_THAN( 3, 5, 0 )
    initQtWindowingSystem();
#endif

    osgbody_ = new ui3DViewerBody( *this, parnt );
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
    if ( isMapView() ) return;

    switch ( type )
    {
	case X: osgbody_->viewPlaneX(); break;
	case Y: osgbody_->viewPlaneN(); break;
	case Z: osgbody_->viewPlaneZ(); break;
	case Inl: osgbody_->viewPlaneInl(); break;
	case Crl: osgbody_->viewPlaneCrl(); break;
	case YZ:osgbody_->viewPlaneYZ(); break;
    }
}


bool ui3DViewer::isCameraPerspective() const
{
    return osgbody_->isCameraPerspective();
}


bool ui3DViewer::setStereoType( StereoType type )
{
    return osgbody_->setStereoType( (ui3DViewerBody::StereoType)type );
}


ui3DViewer::StereoType ui3DViewer::getStereoType() const
{
    return (ui3DViewer::StereoType) osgbody_->getStereoType();
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
{ return osgbody_->getMouseWheelZoomFactor(); }


void ui3DViewer::setReversedMouseWheelDirection( bool reversed )
{
    osgbody_->setReversedMouseWheelDirection( reversed );
}


bool ui3DViewer::getReversedMouseWheelDirection() const
{ return osgbody_->getReversedMouseWheelDirection(); }


void ui3DViewer::setSceneID( int sceneid )
{
    osgbody_->setSceneID( sceneid );

    OD::Color bgcol = OD::Color::Anthracite();
    Settings::common().get(
	BufferString(sKeydTectScene(),ui3DViewer::sKeyBGColor()), bgcol );
    setBackgroundColor( bgcol );
}


int ui3DViewer::sceneID() const
{ return osgbody_->getScene() ? osgbody_->getScene()->id() : -1; }


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
{ osgbody_->toHomePos(); }

void ui3DViewer::saveHomePos()
{ osgbody_->saveHomePos(); }

void ui3DViewer::showRotAxis( bool yn ) // OSG-TODO
{ osgbody_->showRotAxis( yn ); }

void ui3DViewer::setWheelDisplayMode( WheelMode mode )
{ osgbody_->setWheelDisplayMode( (ui3DViewerBody::WheelMode)mode ); }

ui3DViewer::WheelMode ui3DViewer::getWheelDisplayMode() const
{ return (ui3DViewer::WheelMode)osgbody_->getWheelDisplayMode(); }

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
    par.set( sKeySceneID(), getScene()->id() );
    par.set( sKeyBGColor(), (int)getBackgroundColor().rgb() );
    par.set( sKeyStereo(), toString( getStereoType() ) );
    float offset = getStereoOffset();
    par.set( sKeyStereoOff(), offset );
    par.setYN( sKeyPersCamera(), isCameraPerspective() );

    osgbody_->fillCameraPos( par );
}


bool ui3DViewer::usePar( const IOPar& par )
{
    int sceneid;
    if ( !par.get(sKeySceneID(),sceneid) ) return false;

    setSceneID( sceneid );
    if ( !getScene() ) return false;

    int col;
    if ( par.get(sKeyBGColor(),col) )
    {
	OD::Color newcol;
	newcol.setRgb( col );
	setBackgroundColor( newcol );
    }

    StereoType stereotype;
    if ( parseEnum( par, sKeyStereo(), stereotype ) )
	setStereoType( stereotype );

    float offset;
    if ( par.get( sKeyStereoOff(), offset )  )
	setStereoOffset( offset );

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
