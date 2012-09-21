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
#include "uiosgviewer.h"
#include "refcount.h"

namespace visBase { class Camera; class Scene; }
namespace osgGA { class GUIActionAdapter; }
class ui3DViewer;
namespace osg
{
    class GraphicsContext;
    class Camera;
    class Vec3f;
    class Viewport;
}


//!Baseclass for different body implementation (direct & indirect) of OSG

class ui3DViewerBody : public uiObjectBody
{
public:
    			ui3DViewerBody( ui3DViewer& h, uiParent* parnt );
    virtual		~ui3DViewerBody();

    void			viewAll();

    void			setSceneID(int);
    visBase::Scene*		getScene()		{ return scene_; }
    const visBase::Scene*	getScene() const 	{ return scene_; }

    bool			serializeScene(const char*) const;

    void			setBackgroundColor(const Color&);
    Color			getBackgroundColor() const;
    Geom::Size2D<int>		getViewportSizePixels() const;

    void			setHomePos();
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

    				//Not sure were to put these
    bool			isViewing() const;
    virtual void		setViewing(bool);
    void			uisetViewing(bool);

    Coord3			getCameraPosition() const;
    visBase::Camera*		getVisCamera() { return camera_; }

    virtual void		reSizeEvent(CallBacker*);

protected:
    virtual osgGA::GUIActionAdapter&	getActionAdapter()	= 0;
    virtual osg::GraphicsContext*	getGraphicsContext()	= 0;

    uiObject&				uiObjHandle();

    osg::Camera*			getOsgCamera();
    const osg::Camera*			getOsgCamera() const;
    void				setCameraPos(const osg::Vec3f&,
						     const osg::Vec3f&, bool);

    uiOsgViewHandle			view_;
    ui3DViewer&				handle_;
    IOPar&				printpar_;

    RefMan<visBase::Camera>		camera_;
    RefMan<visBase::Scene>		scene_;
    osg::Viewport*			viewport_;
};

#endif
