#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
________________________________________________________________________

-*/

#include "uiosgmod.h"
#include "uiobj.h"
#include "uigroup.h"
#include "color.h"
#include "position.h"
#include "enums.h"

class BufferStringSet;
class FontData;
class ui3DViewerBody;

namespace visBase { class Scene; class PolygonSelection; class SceneColTab; }
namespace osgViewer { class View; }

mExpClass(uiOSG) ui3DViewer : public uiObject
{
friend class		ui3DViewerBody;

public:
			ui3DViewer(uiParent*,
				bool direct,
				const char* nm="ui3DViewer");
			~ui3DViewer();

    void		setMapView(bool yn);
    bool		isMapView() const;

    void		setSceneID(int);
    visBase::Scene*	getScene();
    const visBase::Scene* getScene() const;
    int			sceneID() const;

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

    enum StereoType	{ None, RedCyan, QuadBuffer };
			mDeclareEnumUtils(StereoType)
    bool		setStereoType(StereoType);
    StereoType		getStereoType() const;
    void		setStereoOffset(float);
    float		getStereoOffset() const;

    void		setMouseWheelZoomFactor(float);
			/*!<Always positive, direction is set by
			    setReversedMouseWheelDirection() */
    float		getMouseWheelZoomFactor() const;

    void		setReversedMouseWheelDirection(bool);
    bool		getReversedMouseWheelDirection() const;

    void		viewAll(bool animate=true);
    void		toHomePos();
    void		saveHomePos();
    void		showRotAxis(bool);
    void		setAnnotationColor(const OD::Color&);
    OD::Color		getAnnotationColor() const;
    void		setAnnotationFont(const FontData&);
    bool		rotAxisShown() const;
    void		toggleCameraType();
    bool		isCameraPerspective() const;

    enum WheelMode	{ Never, Always, OnHover };
			mDeclareEnumUtils(WheelMode)
    void		setWheelDisplayMode(WheelMode);
    WheelMode		getWheelDisplayMode() const;

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

    static FixedString	sKeyBindingSettingsKey();

private:

    static const char* sKeySceneID()	{ return "Scene ID"; }
    static const char* sKeyAnimate()	{ return "Animate"; }
    static const char* sKeyBGColor()	{ return "Background color"; }
    static const char* sKeyStereo()	{ return "Stereo viewing"; }
    static const char* sKeyQuadBuf()	{ return "Quad buffer"; }
    static const char* sKeyStereoOff()	{ return "Stereo offset"; }
    static const char* sKeyPrintDlg()	{ return "Print dlg"; }
    static const char* sKeyPersCamera()	{ return "Perspective camera"; }

    ui3DViewerBody*	osgbody_;

    uiObjectBody&	mkBody(uiParent*,bool direct,const char*);

};

