#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiobjbody.h"

#include "mousecursor.h"
#include "refcount.h"
#include "visosg.h"
#include "uieventfilter.h"

class QGestureEvent;

namespace visBase
{
    class Axes;
    class Camera;
    class PolygonSelection;
    class Scene;
    class SceneColTab;
    class Transformation;
    class ThumbWheel;
    class DataObjectGroup;
}


namespace osgViewer { class CompositeViewer; class View; class GraphicsWindow; }
namespace osgGeo { class TrackballManipulator; }

class ui3DViewer;
class TrackBallManipulatorMessenger;
class KeyBindMan;
class uiMouseEventBlockerByGestures;
class SwapCallback;

namespace osg
{
    class Group;
    class GraphicsContext;
    class Camera;
    class MatrixTransform;
    class Projection;
    class Vec3f;
    class Viewport;
}

namespace osgViewer { class View; }

//!Baseclass for different body implementation (direct & indirect) of OSG

mClass(uiOSG) ui3DViewerBody : public uiObjectBody
{
    friend class TrackBallManipulatorMessenger;

public:
    virtual			~ui3DViewerBody();

    void			viewAll(bool animate);

    void			setSceneID(SceneID);
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
    const osgViewer::View*	getOsgViewerMainView() const { return view_; }
    const osgViewer::View*	getOsgViewerHudView() const { return hudview_; }
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
					ui3DViewerBody(ui3DViewer&,uiParent*);

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


    virtual osgViewer::GraphicsWindow&	getGraphicsWindow()	= 0;
    virtual osg::GraphicsContext*	getGraphicsContext()	= 0;

    uiObject&				uiObjHandle() override;

    void				requestRedraw();

    osg::Camera*			getOsgCamera();
    const osg::Camera*			getOsgCamera() const;
    void				setCameraPos(const osg::Vec3f&,
						     const osg::Vec3f&,bool);

    void				thumbWheelRotationCB(CallBacker*);
    void				enableThumbWheelHandling(bool yn,
					    const visBase::ThumbWheel* tw=0);

    ui3DViewer&				handle_;
    IOPar&				printpar_;

    RefMan<visBase::Camera>		camera_;
    RefMan<visBase::Scene>		scene_;
    RefMan<visBase::ThumbWheel>		horthumbwheel_;
    RefMan<visBase::ThumbWheel>		verthumbwheel_;
    RefMan<visBase::ThumbWheel>		distancethumbwheel_;
    int					wheeldisplaymode_;

    osg::Switch*			offscreenrenderswitch_;
    osgViewer::CompositeViewer*		compositeviewer_;
    osgViewer::View*			view_;
    osg::Viewport*			viewport_;
    StereoType				stereotype_;
    float				stereooffset_;

    osgViewer::View*			hudview_;
    osg::Switch*			offscreenrenderhudswitch_;
    RefMan<visBase::DataObjectGroup>	hudscene_;

    uiEventFilter			eventfilter_;
    uiMouseEventBlockerByGestures&	mouseeventblocker_;
    RefMan<visBase::Axes>		axes_;
    RefMan<visBase::PolygonSelection>	polygonselection_;
    TrackBallManipulatorMessenger*	manipmessenger_;

    SwapCallback*			swapcallback_;

    IOPar				homepos_;
    RefMan<visBase::SceneColTab>	visscenecoltab_;

    KeyBindMan&				keybindman_;

    bool				mapview_;
};
