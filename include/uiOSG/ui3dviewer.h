#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgmod.h"

#include "color.h"
#include "uiobj.h"
#include "uiosgutil.h"

class BufferStringSet;
class FontData;
class ui3DViewerBody;
class OD3DViewer;

namespace visBase { class Scene; class PolygonSelection; class SceneColTab; }
namespace osgViewer { class View; }

mExpClass(uiOSG) ui3DViewer : public uiObject
{

public:
			ui3DViewer(uiParent*,bool direct=true,
				   const char* nm="ui3DViewer");
			~ui3DViewer();

    void		setMapView(bool yn);
    bool		isMapView() const;

    void		setScene(visBase::Scene*);
    visBase::Scene*	getScene();
    const visBase::Scene* getScene() const;
    SceneID		sceneID() const;

    void		setViewMode(bool);
    bool		isViewMode() const;

    void		enableAnimation(bool);
    bool		isAnimationEnabled() const;

    void		rotateH(float angle);
    void		rotateV(float angle);
    void		dolly(float rel); // relative size
    void		setCameraZoom(float);
    float		getCameraZoom();
    const Coord3	getCameraPosition() const;

    enum PlaneType	{ X, Y, Z, Inl, Crl, YZ };
    void		viewPlane(PlaneType);
    void		align();

    bool		setStereoType(OD::StereoType);
    OD::StereoType	getStereoType() const;
    void		setStereoOffset(float);
    float		getStereoOffset() const;

    void		setMouseWheelZoomFactor(float);
			/*!<Always positive, direction is set by
			    setReversedMouseWheelDirection() */
    float		getMouseWheelZoomFactor() const;

    void		setReversedMouseWheelDirection(bool);
    bool		getReversedMouseWheelDirection() const;

    void		setStartupView();
    void		viewAll(bool animate=true);
    void		toHomePos();
    void		saveHomePos();
    void		resetHomePos();

    void		showRotAxis(bool);
    void		setAnnotationColor(const OD::Color&);
    OD::Color		getAnnotationColor() const;
    void		setAnnotationFont(const FontData&);
    bool		rotAxisShown() const;

    void		toggleCameraType();
    bool		isCameraPerspective() const;
    void		setCameraPerspective(bool yn);

    void		setWheelDisplayMode(OD::WheelMode);
    OD::WheelMode	getWheelDisplayMode() const;

    void		setBackgroundColor(const OD::Color&) override;
    OD::Color		getBackgroundColor() const;

    Geom::Size2D<int>	getViewportSizePixels() const;

    Notifier<ui3DViewer> destroyed;
    Notifier<ui3DViewer> viewmodechanged;
    CNotifier<ui3DViewer,bool> pageupdown;
    CallBack*		vmcb;

    void		setKeyBindings(const char* keybindname);
    void		getAllKeyBindings(BufferStringSet&) const;
    const char*		getCurrentKeyBindings() const;

    visBase::PolygonSelection*	getPolygonSelector();
    visBase::SceneColTab*	getSceneColTab();
    const osgViewer::View*	getOsgViewerMainView() const;
    const osgViewer::View*	getOsgViewerHudView() const;
    void			setScenesPixelDensity(float dpi);
    float			getScenesPixelDensity() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		savePropertySettings() const;

    static StringView	sKeyBindingSettingsKey();

private:

    static const char* sKeySceneID()	{ return "Scene ID"; }
    static const char* sKeyAnimate()	{ return "Animate"; }
    static const char* sKeyBGColor()	{ return "Background color"; }
    static const char* sKeyStereo()	{ return "Stereo viewing"; }
    static const char* sKeyQuadBuf()	{ return "Quad buffer"; }
    static const char* sKeyStereoOff()	{ return "Stereo offset"; }
    static const char* sKeyPrintDlg()	{ return "Print dlg"; }
    static const char* sKeyPersCamera()	{ return "Perspective camera"; }
    static const char* sKeyMapView()	{ return "MapView"; }

    uiObjectBody&	mkBody(uiParent*,bool direct,const char*);

#ifdef OD_USE_QOPENGL
    friend class	OD3DViewer;
    OD3DViewer*		osgbody_;
#else
    friend class	ui3DViewerBody;
    ui3DViewerBody*	osgbody_;
#endif
};
