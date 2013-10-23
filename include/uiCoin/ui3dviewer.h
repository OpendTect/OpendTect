#ifndef ui3dviewer_h
#define ui3dviewer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uicoinmod.h"
#include "uiobj.h"
#include "uigroup.h"
#include "color.h"
#include "position.h"
#include "enums.h"

class uiSoViewerBody;
class ui3DViewerBody;
class BufferStringSet;

namespace visBase { class Scene; class PolygonSelection; };

mExpClass(uiCoin) ui3DViewer : public uiObject
{
friend class		uiSoViewerBody;
friend class		ui3DViewerBody;

public:

                        ui3DViewer(uiParent*,
				bool direct,
				const char* nm="ui3DViewer");
			~ui3DViewer();

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
    			DeclareEnumUtils(StereoType);
    bool		setStereoType(StereoType);
    StereoType		getStereoType() const;
    void		setStereoOffset(float);
    float		getStereoOffset() const;

    void		viewAll();
    void		toHomePos();
    void		saveHomePos();
    void		showRotAxis(bool);
    void		setAxisAnnotColor(const Color&);
    bool		rotAxisShown() const;
    void		toggleCameraType();
    bool		isCameraPerspective() const;
    void		switchSeekMode();

    void		setBackgroundColor(const Color&);
    Color		getBackgroundColor() const;

    Geom::Size2D<int>	getViewportSizePixels() const;

    Notifier<ui3DViewer> destroyed;
    Notifier<ui3DViewer> viewmodechanged;
    CNotifier<ui3DViewer,bool> pageupdown;
    CallBack*		vmcb;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		savePropertySettings() const;

    void		setKeyBindings(const char* keybindname);
    void		getAllKeyBindings(BufferStringSet&);
    const char*		getCurrentKeyBindings() const;

    float               getHeadOnLightIntensity() const;
    void                setHeadOnLightIntensity(float);
    visBase::PolygonSelection* getPolygonSelector() const;

private:
    static const char* sKeySceneID()    { return "Scene ID"; }
    static const char* sKeyAnimate()	{ return "Animate"; }
    static const char* sKeyBGColor()    { return "Background color"; }
    static const char* sKeyHomePos()    { return "Home position"; }
    static const char* sKeyStereo()     { return "Stereo viewing"; }
    static const char* sKeyQuadBuf()    { return "Quad buffer"; }
    static const char* sKeyStereoOff()  { return "Stereo offset"; }
    static const char* sKeyPrintDlg()   { return "Print dlg"; }
    static const char* sKeyPersCamera() { return "Perspective camera"; }

    ui3DViewerBody*	osgbody_;

    uiObjectBody&	mkBody(uiParent*,bool direct,const char*);

    IOPar		homepos_;
};


#endif

