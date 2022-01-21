#pragma once
/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "uiosgmod.h"
#include "uiobjbody.h"
#include "ui3dviewer.h"
#include <osg/ref_ptr>

namespace osg { class Switch; }

namespace osgGeo
{
    class TrackballManipulator;
}

namespace visBase
{
    class Axes;
    class Camera;
    class Scene;
    class ThumbWheel;
}


mClass(uiOSG) OD3DViewer : public uiObjectBody
{
public:
				OD3DViewer(ui3DViewer&,uiParent*);
				~OD3DViewer();

    void			setSceneID(int);
    visBase::Scene*		getScene();
    const visBase::Scene*	getScene() const;

    void			viewAll(bool animate);
    void			align();
    void			viewPlaneX();
    void			viewPlaneY();
    void			viewPlaneZ();
    void			viewPlaneInl();
    void			viewPlaneCrl();
    void			viewPlaneN();
    void			viewPlaneYZ();

    void			setBackgroundColor(const OD::Color&);
    void			setAnnotationColor(const OD::Color&);

    void			setStereoType(ui3DViewer::StereoType);
    ui3DViewer::StereoType	getStereoType() const;
    void			setStereoOffset(float);
    float			getStereoOffset() const;

    void			setMapView(bool);
    bool			isMapView() const	{ return ismapview_; }

    osgGeo::TrackballManipulator* getCameraManipulator() const;

protected:
    uiObject&			uiObjHandle() override;
    const QWidget*		qwidget_() const override;

    void			requestRedraw();

    osg::ref_ptr<osg::Switch>	offscreenrenderswitch_;
    osg::ref_ptr<osg::Switch>	offscreenrenderhudswitch_;

    RefMan<visBase::Axes>		axes_			= nullptr;
    RefMan<visBase::Camera>		camera_			= nullptr;
    RefMan<visBase::Scene>		scene_			= nullptr;
    RefMan<visBase::ThumbWheel>		horthumbwheel_		= nullptr;
    RefMan<visBase::ThumbWheel>		verthumbwheel_		= nullptr;
    RefMan<visBase::ThumbWheel>		zoomthumbwheel_		= nullptr;

    ui3DViewer&				ui3dviewer_;
    bool				ismapview_		= false;
    float				stereooffset_		= 100;
    ui3DViewer::StereoType		stereotype_	= ui3DViewer::None;
};
