#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgmod.h"

#include "color.h"
#include "ui3dviewerbase.h"
#include "uiosgutil.h"

class BufferStringSet;
class FontData;
class ui3DViewerBody;
class OD3DViewer;

namespace visBase { class Scene; class PolygonSelection; class SceneColTab; }
namespace osgViewer { class View; }

mExpClass(uiOSG) ui3DViewer : public ui3DViewerBase
{

public:
			ui3DViewer(uiParent*,bool direct=true,
				   const char* nm="ui3DViewer");
			~ui3DViewer();

    void		setMapView(bool yn);
    bool		isMapView() const;

    void		setScene(visBase::Scene*) override;
    visBase::Scene*	getScene();
    const visBase::Scene* getScene() const;
    SceneID		sceneID() const override;

    void		setViewMode(bool) override;
    bool		isViewMode() const override;

    void		enableAnimation(bool);
    bool		isAnimationEnabled() const;

    void		rotateH(float angle);
    void		rotateV(float angle);
    void		dolly(float rel); // relative size
    void		setCameraZoom(float);
    float		getCameraZoom();
    const Coord3	getCameraPosition() const;

    void		viewPlane(PlaneType) override;
    void		align() override;

    bool		setStereoType(OD::StereoType) override;
    OD::StereoType	getStereoType() const override;
    void		setStereoOffset(float) override;
    float		getStereoOffset() const override;

    void		setMouseWheelZoomFactor(float);
			/*!<Always positive, direction is set by
			    setReversedMouseWheelDirection() */
    float		getMouseWheelZoomFactor() const;

    void		setReversedMouseWheelDirection(bool);
    bool		getReversedMouseWheelDirection() const;

    void		setStartupView() override;
    void		viewAll(bool animate=true) override;
    void		toHomePos() override;
    void		saveHomePos() override;
    void		resetHomePos() override;

    void		showRotAxis(bool) override;
    void		setAnnotationColor(const OD::Color&) override;
    OD::Color		getAnnotationColor() const override;
    void		setAnnotationFont(const FontData&) override;
    bool		rotAxisShown() const override;

    void		toggleCameraType() override;
    bool		isCameraPerspective() const override;
    void		setCameraPerspective(bool yn);

    void		setWheelDisplayMode(OD::WheelMode);
    OD::WheelMode	getWheelDisplayMode() const;

    void		setBackgroundColor(const OD::Color&) override;
    OD::Color		getBackgroundColor() const override;

    Geom::Size2D<int>	getViewportSizePixels() const;

    Notifier<ui3DViewer> destroyed;
    CallBack*		vmcb;

    void		setKeyBindings(const char* keybindname);
    void		getAllKeyBindings(BufferStringSet&) const;
    const char*		getCurrentKeyBindings() const;

    visBase::PolygonSelection*	getPolygonSelector() override;
    visBase::SceneColTab*	getSceneColTab() override;
    const osgViewer::View*	getOsgViewerMainView() const;
    const osgViewer::View*	getOsgViewerHudView() const;
    void			setScenesPixelDensity(float dpi);
    float			getScenesPixelDensity() const;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;
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
