#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 QWidget embedding a VSG-rendered Vulkan surface into the main
 window's MDI workspace.

-*/

#include "uivsgmod.h"

#include "visvsgscenecoltab.h"
#include "fontdata.h"
#include "ptrman.h"
#include "refcount.h"
#include "uistringset.h"

#include <QWidget>
#include <vsg/all.h>

class CallBacker;
class dGBVSGViewer;
class dGBVSGWindow;
class uiVSGWindowHandler;
struct VSGMouseStatus;
namespace dGBVis
{ class Axes; class Camera; class DataObjectGroup; class TextureChannels;
  class ThumbWheel; }
namespace dGBVisSurvey { class Scene; }
namespace vsgGeo
{ class AxesHandler; class NearFar; class ThumbWheelHandler; class Trackball; }

mExpClass(uiVSG) uiVSGViewer : public QWidget
{
public:
			uiVSGViewer(QWidget* parent=nullptr);
			~uiVSGViewer();

    void		setAnnotationFont(const FontData&);
    FontData		annotationFont() const;

    void		showRotAxis(bool);
    bool		isAxisShown() const;
    void		setModernAxes(bool);
    bool		isModernAxes() const;

    void		showColorBar(bool);
    bool		isColorBarShown() const;
    void		setColorBarPos(dGBVis::SceneColTab::Pos);
    dGBVis::SceneColTab::Pos getColorBarPos() const;
    void		setColorBarSize(int width,int height);
    Geom::Size2D<int>	getColorBarSize() const;

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

    dGBVisSurvey::Scene* scene();
    dGBVis::TextureChannels* textureChannels();
			/*!<The data of the test plane, null without it. */

    void		setViewMode(bool yn);
			/*!<True: left button orbits. False: left button
			    picks and drags. */
    bool		isViewMode() const;
    void		syncCursor()		{ updateCursor(); }

protected:

    void		showEvent(QShowEvent*) override;
    void		resizeEvent(QResizeEvent*) override;

private:

    class		SceneBoundsWatch;
    class		WheelLink;
    class		ColTabLink;

    void		showWheels(bool);
    void		thumbWheelRotated(CallBacker*,float delta);
    void		cameraMoved(float hor,float vert,float dist,bool step);
    void		initializeVSG();
    void		setupVSG();
    void		setupHUD();
    void		setupOffscreenCapture();
    void		syncCameraViewport();
    void		updateCursor();
			/*!<Forwards to uiVSGWindowHandler. */

    dGBVSGWindow*			qtwindow_ = nullptr;
    QWidget*				container_ = nullptr;
    VSGMouseStatus*			statuscb_ = nullptr;
    vsg::ref_ptr<uiVSGWindowHandler>	windowhandler_;
    vsg::ref_ptr<dGBVSGViewer>		viewer_;
    vsg::ref_ptr<vsg::Window>		vsgwindow_;
    vsg::ref_ptr<vsg::Camera>		camera_;
    vsg::ref_ptr<vsgGeo::NearFar>	nearfar_;
    PtrMan<SceneBoundsWatch>		boundswatch_;
    vsg::ref_ptr<vsg::Group>		root_;
    RefMan<dGBVisSurvey::Scene>		scene_;
    RefMan<dGBVis::Camera>		viscamera_;
    RefMan<dGBVis::Camera>		vishudcamera_;
    RefMan<dGBVis::DataObjectGroup>	hudscene_;
    RefMan<dGBVis::Axes>		axes_;
    RefMan<dGBVis::SceneColTab>		scenecoltab_;
    RefMan<dGBVis::ThumbWheel>		horthumbwheel_;
    RefMan<dGBVis::ThumbWheel>		verthumbwheel_;
    RefMan<dGBVis::ThumbWheel>		distancethumbwheel_;
    vsg::ref_ptr<vsgGeo::ThumbWheelHandler> wheelhandler_;
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
    vsg::ref_ptr<vsg::ClearAttachments>	offscreenhudclear_;
    FontData				annotfont_;
    bool				hasannotfont_ = false;
    bool				initfailed_ = false;
    const int				rendertimems_ = 16;

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
