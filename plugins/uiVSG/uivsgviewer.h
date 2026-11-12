#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 uiObject showing a VSG-rendered Vulkan surface. Give it a
 uiMdiAreaWindow as parent to put it in the MDI workspace.

-*/

#include "uivsgmod.h"
#include "uiobj.h"
#include "visvsgscenecoltab.h"
#include "color.h"
#include "fontdata.h"
#include "integerid.h"
#include "notify.h"
#include "ptrman.h"
#include "refcount.h"
#include "uistringset.h"

#include <vsg/all.h>

class dGBVSGViewer;
class dGBVSGWindow;
class uiVSGWindowHandler;
struct VSGMouseStatus;
namespace ODVsg
{ class Axes; class Camera; class DataObjectGroup; class Scene;
  class TextureChannels; class ThumbWheel; }
namespace vsgGeo
{ class AxesHandler; class GradientBackground; class NearFar;
  class ThumbWheelHandler; class Trackball; }

mExpClass(uiVSG) uiVSGViewer : public uiObject
{
public:
			uiVSGViewer(uiParent*,const char* nm="VSG Viewer");
			~uiVSGViewer();

    void		setAnnotationFont(const FontData&);
    FontData		annotationFont() const;

    void		showRotAxis(bool);
    bool		isAxisShown() const;
    void		setModernAxes(bool);
    bool		isModernAxes() const;

    void		showColorBar(bool);
    bool		isColorBarShown() const;
    void		setColorBarPos(ODVsg::SceneColTab::Pos);
    ODVsg::SceneColTab::Pos getColorBarPos() const;
    void		setColorBarSize(int width,int height);
    Geom::Size2D<int>	getColorBarSize() const;

    void		setBackgroundColor(const OD::Color&);
    void		setBackgroundGradient(bool);
    bool		isBackgroundGradient() const;
    OD::Color		getBackgroundColor() const;
    void		setAnnotationColor(const OD::Color&);
    OD::Color		getAnnotationColor() const;

    enum class WheelMode	{ Never, Always, OnHover };
    void		setWheelDisplayMode(WheelMode);
    WheelMode		getWheelDisplayMode() const;
    static WheelMode	startWheelMode();
    void		setWheelsGrowOnHover(bool);
    bool		isWheelsGrowOnHover() const;
    void		setGlassWheels(bool);
    bool		isGlassWheels() const;

    uiRetVal		takeScreenshot(int width,int height,
				       const char* destpath,float dpi=0.f);
			/*!<The labels keep their shape in a picture that is
			    squeezed or stretched. They are sized while a frame
			    is drawn and used from the next one, so the picture
			    is drawn twice. The window shares the labels, so
			    it is frozen for those frames: its view is left
			    out and the last picture stays. */
    bool		maxCaptureSize(uint32_t& maxwidth,
				       uint32_t& maxheight) const;

    ODVsg::Scene* scene();
    void		setScene(ODVsg::Scene*);
			//!<Only before the viewer is first shown. Without it
			//!<the viewer makes a scene of its own
    ODVsg::TextureChannels* textureChannels();
			/*!<The data of the test plane, null without it. */

    void		setViewMode(bool yn);
			/*!<True: left button orbits. False: left button
			    picks and drags. */
    bool		isViewMode() const;
    void		syncCursor()		{ updateCursor(); }

    CNotifier<uiVSGViewer,SceneID> closing;
			//!<Triggered when the viewer is deleted
			//! 
private:

    class		SceneBoundsWatch;
    class		WheelLink;
    class		ColTabLink;

    uiObjectBody&	mkBody(uiParent*,const char* nm);

    void		showWheels(bool);
    void		applyAnnotationColor();
    void		setWheelColor(const OD::Color&);
    void		applyBackground();
    void		thumbWheelRotated(CallBacker*,float delta);
    void		cameraMoved(float hor,float vert,float dist,bool step);
    void		initializeVSG();
    void		setupVSG();
    void		setupHUD();
    void		setupOffscreenCapture();
    void		syncCameraViewport();
    void		windowShownCB(CallBacker*);
    void		windowResizedCB(CallBacker*);
    void		updateCursor();
			/*!<Forwards to uiVSGWindowHandler. */

    dGBVSGWindow*			window_;
    VSGMouseStatus*			statuscb_ = nullptr;
    vsg::ref_ptr<uiVSGWindowHandler>	windowhandler_;
    vsg::ref_ptr<dGBVSGViewer>		viewer_;
    vsg::ref_ptr<vsg::Window>		vsgwindow_;
    vsg::observer_ptr<vsg::RenderGraph>	windowgraph_;
    vsg::ref_ptr<vsg::Camera>		camera_;
    vsg::ref_ptr<vsgGeo::NearFar>	nearfar_;
    PtrMan<SceneBoundsWatch>		boundswatch_;
    vsg::ref_ptr<vsg::Group>		root_;
    RefMan<ODVsg::Scene>		scene_;
    RefMan<ODVsg::Camera>		viscamera_;
    RefMan<ODVsg::Camera>		vishudcamera_;
    RefMan<ODVsg::DataObjectGroup>	hudscene_;
    RefMan<ODVsg::Axes>		axes_;
    RefMan<ODVsg::SceneColTab>		scenecoltab_;
    RefMan<ODVsg::ThumbWheel>		horthumbwheel_;
    RefMan<ODVsg::ThumbWheel>		verthumbwheel_;
    RefMan<ODVsg::ThumbWheel>		distancethumbwheel_;
    vsg::ref_ptr<vsgGeo::ThumbWheelHandler> wheelhandler_;
    vsg::ref_ptr<vsgGeo::GradientBackground> background_;
    vsg::ref_ptr<vsg::ClearAttachments>	bgclear_;
    vsg::ref_ptr<vsg::View>		bgview_;
    vsg::ref_ptr<vsgGeo::AxesHandler>	axeshandler_;
    vsg::ref_ptr<vsgGeo::Trackball>	trackball_;
    PtrMan<WheelLink>			wheellink_;
    PtrMan<ColTabLink>			coltablink_;
    WheelMode				wheelmode_ = WheelMode::OnHover;
    bool				wheelsgrow_ = true;
    bool				wheelsglass_ = true;
    vsg::ref_ptr<vsg::Orthographic>	hudprojection_;
    vsg::ref_ptr<vsg::View>		hudview_;
    vsg::ref_ptr<vsg::ClearAttachments>	hudclear_;
    vsg::ref_ptr<vsg::Camera>		offscreenhudcamera_;
    vsg::ref_ptr<vsg::View>		offscreenhudview_;
    vsg::ref_ptr<vsg::ClearAttachments>	offscreenbgclear_;
    vsg::ref_ptr<vsg::View>		offscreenbgview_;
    vsg::ref_ptr<vsg::ClearAttachments>	offscreenhudclear_;
    FontData				annotfont_;
    bool				hasannotfont_ = false;
    bool				initfailed_ = false;

    vsg::ref_ptr<vsg::Camera>		offscreencamera_;
    vsg::ref_ptr<vsg::View>		offscreenview_;
    vsg::ref_ptr<vsg::Perspective>	offscreenperspective_;
    vsg::ref_ptr<vsg::Switch>		offscreenswitch_;
    vsg::ref_ptr<vsg::Switch>		windowswitch_;
    vsg::ref_ptr<vsg::RenderGraph>	offscreenrendergraph_;
    vsg::ref_ptr<vsg::ImageView>	transferimageview_;
    vsg::ref_ptr<vsg::Image>		captureImage_;
    vsg::ref_ptr<vsg::Node>		capturecommands_;
    VkExtent2D				bufferextent_{ 0, 0 };
    VkExtent2D				captureextent_{ 0, 0 };
};
