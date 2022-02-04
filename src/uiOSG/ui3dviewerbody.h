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
    class CompositeViewer;
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

mClass(uiOSG) ui3DViewerBody : public uiObjectBody
{
    friend class TrackBallManipulatorMessenger;

public:
				ui3DViewerBody(ui3DViewer&,uiParent*);
    virtual			~ui3DViewerBody();

    void			viewAll(bool animate);

    void			setSceneID(int);
    visBase::Scene*		getScene()		{ return scene_; }
    const visBase::Scene*	getScene() const	{ return scene_; }

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

    void			setHomePos(const IOPar&);
    void			resetToHomePosition();

    void			toggleCameraType();
    bool			isCameraPerspective() const;
    bool			isCameraOrthographic() const;

    void			align();
    void			viewPlaneX();
    void			viewPlaneY();
    void			viewPlaneZ();
    void			viewPlaneInl();
    void			viewPlaneCrl();
    void			viewPlaneN();
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

    enum WheelMode		{ Never, Always, OnHover };
    void			setWheelDisplayMode(WheelMode);
    WheelMode			getWheelDisplayMode() const;

    void			setAnimationEnabled(bool);
    bool			isAnimationEnabled() const;
    void			showRotAxis(bool);
    bool			isAxisShown() const;
    void			setAnnotColor(const OD::Color&);
    void			setAnnotationFont(const FontData&);
    visBase::PolygonSelection*	getPolygonSelector();
    visBase::SceneColTab*	getSceneColTab();

    void			toHomePos();
    void			saveHomePos();
    bool			isHomePosEmpty() { return homepos_.isEmpty(); }
    void			fillCameraPos(IOPar&) const;
    bool			useCameraPos(const IOPar&);
    const osgViewer::View*	getOsgViewerMainView() const;
    const osgViewer::View*	getOsgViewerHudView() const;
    void			setScenesPixelDensity(float dpi);

    enum StereoType		{ None, RedCyan, QuadBuffer };

    bool			setStereoType(StereoType);
    StereoType			getStereoType() const;
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
    static osgViewer::CompositeViewer*	getCompositeViewer();
    osgGeo::TrackballManipulator*	getCameraManipulator() const;

    osgViewer::GraphicsWindow&		getGraphicsWindow();
    osg::GraphicsContext*		getGraphicsContext();

    uiObject&				uiObjHandle();
    const QWidget*			qwidget_() const;

    void				requestRedraw();

    osg::Camera*			getOsgCamera();
    const osg::Camera*			getOsgCamera() const;
    void				setCameraPos(const osg::Vec3f&,
						     const osg::Vec3f&,bool);

    void				thumbWheelRotationCB(CallBacker*);
    void				enableThumbWheelHandling(bool yn,
					    visBase::ThumbWheel* tw=nullptr);

    ui3DViewer&				handle_;
    IOPar&				printpar_;

    ODOpenGLWidget*			glwidget_;
    RefMan<visBase::Camera>		camera_;
    RefMan<visBase::Scene>		scene_;
    RefMan<visBase::ThumbWheel>		horthumbwheel_		= nullptr;
    RefMan<visBase::ThumbWheel>		verthumbwheel_		= nullptr;
    RefMan<visBase::ThumbWheel>		distancethumbwheel_	= nullptr;
    WheelMode				wheeldisplaymode_	= OnHover;

    osg::Switch*			offscreenrenderswitch_;
    osgViewer::CompositeViewer*		compositeviewer_;
    ODOSGViewer*			view_			= nullptr;
    osg::Viewport*			viewport_;
    StereoType				stereotype_		= None;
    float				stereooffset_		= 0.f;

    ODOSGViewer*			hudview_		= nullptr;
    osg::Switch*			offscreenrenderhudswitch_;
    RefMan<visBase::DataObjectGroup>	hudscene_		= nullptr;

    uiEventFilter			eventfilter_;
    uiMouseEventBlockerByGestures&	mouseeventblocker_;
    RefMan<visBase::Axes>		axes_			= nullptr;
    RefMan<visBase::PolygonSelection>	polygonselection_	= nullptr;
    TrackBallManipulatorMessenger*	manipmessenger_;

    SwapCallback*			swapcallback_		= nullptr;

    IOPar				homepos_;
    RefMan<visBase::SceneColTab>	visscenecoltab_		= nullptr;

    KeyBindMan&				keybindman_;

    bool				mapview_		= false;
};

