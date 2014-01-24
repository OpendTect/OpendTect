#ifndef ui3dviewerbody_h
#define ui3dviewerbody_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiobjbody.h"

#include "refcount.h"
#include "visosg.h"
#include "uieventfilter.h"

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

namespace osgGA { class GUIActionAdapter; }
namespace osgViewer { class CompositeViewer; class View; }
namespace osgGeo { class TrackballManipulator; }

class ui3DViewer;
class TrackBallManipulatorMessenger;

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


//!Baseclass for different body implementation (direct & indirect) of OSG

mClass(uiOSG) ui3DViewerBody : public uiObjectBody
{
public:
    			ui3DViewerBody( ui3DViewer& h, uiParent* parnt );
    virtual		~ui3DViewerBody();

    void			viewAll( bool animate );

    void			setSceneID(int);
    visBase::Scene*		getScene()		{ return scene_; }
    const visBase::Scene*	getScene() const 	{ return scene_; }

    bool			serializeScene(const char*) const;

    void			setBackgroundColor(const Color&);
    Color			getBackgroundColor() const;
    Geom::Size2D<int>		getViewportSizePixels() const;

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
    visBase::Camera*		getVisCamera() { return camera_; }

    virtual void		reSizeEvent(CallBacker*);
    void			toggleViewMode(CallBacker*);

    void			setAnimationEnabled(bool) {} // OSG-TODO
    bool			isAnimationEnabled() { return true; }
    void			showRotAxis(bool);
    bool			isAxisShown() const;
    void			setAxisAnnotColor(const Color&);
    visBase::PolygonSelection*	getPolygonSelector() const;
    visBase::SceneColTab* 	getSceneColTab() const;
    void			notifyManipulatorMovement(float dh, float dv,
                                                          float df );
    void			toHomePos();
    void			saveHomePos();
    bool			isHomePosEmpty() { return homepos_.isEmpty(); }
    void			fillCameraPos(IOPar&) const;
    bool			useCameraPos(const IOPar&);

protected:

    void				setupHUD();
    void				setupView();
    void				qtEventCB(CallBacker*);

    static osgViewer::CompositeViewer*	getCompositeViewer();
    virtual osgGA::GUIActionAdapter&	getActionAdapter()	= 0;
    virtual osg::GraphicsContext*	getGraphicsContext()	= 0;

    uiObject&				uiObjHandle();

    void				requestRedraw();

    osg::Camera*			getOsgCamera();
    const osg::Camera*			getOsgCamera() const;
    void				setCameraPos(const osg::Vec3f&,
						     const osg::Vec3f&, bool);
    
    void				thumbWheelRotationCB(CallBacker*);


    ui3DViewer&						handle_;
    IOPar&						printpar_;

    RefMan<visBase::Camera>				camera_;
    RefMan<visBase::Scene>				scene_;
    RefMan<visBase::ThumbWheel>				horthumbwheel_;
    RefMan<visBase::ThumbWheel>				verthumbwheel_;
    RefMan<visBase::ThumbWheel>				distancethumbwheel_;
    osg::Group*						sceneroot_;
    osgViewer::CompositeViewer*				compositeviewer_;
    osgViewer::View*					view_;
    osg::Viewport*					viewport_;

    osgViewer::View*					hudview_;
    RefMan<visBase::DataObjectGroup>			hudscene_;

    uiEventFilter					eventfilter_;
    RefMan<visBase::Axes>				axes_;
    RefMan<visBase::PolygonSelection>			polygonselection_;
    TrackBallManipulatorMessenger*			manipmessenger_;

    bool			    			setinitialcamerapos_;
    IOPar						homepos_;
    RefMan<visBase::SceneColTab>			visscenecoltab_;
};

#endif
