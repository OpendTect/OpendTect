#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
________________________________________________________________________

-*/

#include "uiobjbody.h"

#include "mousecursor.h"
#include "refcount.h"
#include "uieventfilter.h"
#include "uiosgutil.h"
#include "vissurvscene.h"

namespace visBase
{
    class Axes;
    class Camera;
    class PolygonSelection;
    class Scene;
    class SceneColTab;
    class ThumbWheel;
    class DataObjectGroup;
}

namespace osg
{
    class Camera;
    class GraphicsContext;
    class Switch;
    class Vec3f;
    class Viewport;
}

namespace osgViewer
{
    class GraphicsWindow;
    class View;
    class Viewer;
}

namespace osgGeo
{
    class TrackballManipulator;
}

class QGestureEvent;

class ui3DViewer;
class ODOSGViewer;
class ODOpenGLWidget;
class TrackBallManipulatorMessenger;
class KeyBindMan;
class uiMouseEventBlockerByGestures;
class SwapCallback;

mClass(uiOSG) OD3DViewer : public uiObjectBody
{
    friend class TrackBallManipulatorMessenger;

public:
				OD3DViewer(ui3DViewer&,uiParent*);
    virtual			~OD3DViewer();

    void			viewAll(bool animate);

    void			setScene(visBase::Scene*);
    visBase::Scene*		getScene();
    const visBase::Scene*	getScene() const;
    SceneID			getSceneID() const;

    bool			serializeScene(const char*) const;

    void			setBackgroundColor(const OD::Color&);
    OD::Color			getBackgroundColor() const;
    Geom::Size2D<int>		getViewportSizePixels() const;

    float			getMouseWheelZoomFactor() const;
				/*!<Always positive. Direction is set by
				    setReversedMouseWheelDirection() */
    void			setMouseWheelZoomFactor(float);

    void			setReversedMouseWheelDirection(bool);
    bool			getReversedMouseWheelDirection() const;

    void			toggleCameraType();
    bool			isCameraPerspective() const;
    bool			isCameraOrthographic() const;
    void			setCameraPerspective(bool yn);

    void			align();
    void			viewPlaneX();
    void			viewPlaneY();
    void			viewPlaneZ();
    void			viewPlaneInl(bool animate=true);
    void			viewPlaneCrl();
    void			viewPlaneN(bool animate=true);
    void			viewPlaneYZ();

    void			uiRotate(float angle,bool horizontal);
    void			uiZoom(float rel,const osg::Vec3f* dir=0);
    void			setCameraZoom(float val);
    float			getCameraZoom() const;

				//Not sure were to put these
    bool			isViewMode() const;
    virtual void		setViewMode(bool viewmode,bool trigger);

    Coord3			getCameraPosition() const;
    visBase::Camera*		getVisCamera()		{ return camera_; }

    virtual void		reSizeEvent(CallBacker*);
    void			toggleViewMode(CallBacker*);

    void			setWheelDisplayMode(OD::WheelMode);
    OD::WheelMode		getWheelDisplayMode() const;

    void			setAnimationEnabled(bool);
    bool			isAnimationEnabled() const;
    void			showRotAxis(bool);
    bool			isAxisShown() const;
    void			setAnnotColor(const OD::Color&);
    void			setAnnotationFont(const FontData&);
    visBase::PolygonSelection*	getPolygonSelector();
    visBase::SceneColTab*	getSceneColTab();

    void			setStartupView();
    void			setHomePos(const IOPar&);
    void			toHomePos();
    void			saveHomePos();
    void			resetHomePos();
    bool			isHomePosEmpty() const;

    void			fillCameraPos(IOPar&) const;
    bool			useCameraPos(const IOPar&);

    const osgViewer::View*	getOsgViewerMainView() const;
    const osgViewer::View*	getOsgViewerHudView() const;
    void			setScenesPixelDensity(float dpi);

    bool			setStereoType(OD::StereoType);
    OD::StereoType		getStereoType() const;
    void			setStereoOffset(float);
    float			getStereoOffset() const;

    void			setMapView(bool yn);
    bool			isMapView() const	{ return mapview_; }

    KeyBindMan&			keyBindMan()		{ return keybindman_; }

    void			removeSwapCallback(CallBacker*);

protected:
    void				enableDragging( bool yn );

    enum ViewModeCursor			{ RotateCursor, PanCursor, ZoomCursor,
					  HoverCursor };
    virtual void			setViewModeCursor( ViewModeCursor );

    virtual void			updateActModeCursor();
    void				mouseCursorChg(CallBacker*);
    MouseCursor				actmodecursor_;

    void				notifyManipulatorMovement(
						    float dh,float dv,float df);
    void				setupTouch();
    void				setupHUD();
    void				setupView();
    void				qtEventCB(CallBacker*);
    void				setFocusCB(CallBacker*);
    void				handleGestureEvent(QGestureEvent*);
    osgGeo::TrackballManipulator*	getCameraManipulator() const;

    osgViewer::GraphicsWindow&		getGraphicsWindow();
    osg::GraphicsContext*		getGraphicsContext();
    const QWidget*			qwidget_() const override;

    uiObject&				uiObjHandle() override;

    void				requestRedraw();

    osg::Camera*			getOsgCamera();
    const osg::Camera*			getOsgCamera() const;
    void				setCameraPos(const osg::Vec3f&,
						     const osg::Vec3f&,
						     bool usetruedir,
						     bool animate);

    Timer*				viewalltimer_;
    void				viewAllCB(CallBacker*);

    void				thumbWheelRotationCB(CallBacker*);
    void				enableThumbWheelHandling(bool yn,
					   const visBase::ThumbWheel* =nullptr);

    ui3DViewer&				handle_;
    IOPar&				printpar_;

    ODOpenGLWidget*			glwidget_;
    WeakPtr<visSurvey::Scene>		scene_;
    RefMan<visBase::Camera>		vishudcamera_;
    RefMan<visBase::Camera>		camera_;
    RefMan<visBase::ThumbWheel>		horthumbwheel_;
    RefMan<visBase::ThumbWheel>		verthumbwheel_;
    RefMan<visBase::ThumbWheel>		distancethumbwheel_;
    OD::WheelMode			wheeldisplaymode_
						= OD::WheelMode::OnHover;

    osg::Switch*			offscreenrenderswitch_;
    ODOSGViewer*			view_			= nullptr;
    osg::Viewport*			viewport_;
    OD::StereoType			stereotype_	= OD::StereoType::None;
    float				stereooffset_		= 0.f;

    ODOSGViewer*			hudview_		= nullptr;
    osg::Switch*			offscreenrenderhudswitch_;
    RefMan<visBase::DataObjectGroup>	hudscene_;

    uiEventFilter			eventfilter_;
    uiMouseEventBlockerByGestures&	mouseeventblocker_;
    RefMan<visBase::Axes>		axes_;
    RefMan<visBase::PolygonSelection>	polygonselection_;
    TrackBallManipulatorMessenger*	manipmessenger_;

    SwapCallback*			swapcallback_		= nullptr;

    IOPar				homepos_;
    RefMan<visBase::SceneColTab>	visscenecoltab_;

    KeyBindMan&				keybindman_;

    bool				mapview_		= false;
};
