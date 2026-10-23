/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgviewer.h"

#include <QVBoxLayout>

#include "vsgviewer.h"
#include "uivsgwindow.h"
#include "uivsgwindowhandler.h"

#include "bufstring.h"
#include "callback.h"
#include "math2.h"
#include "uimainwin.h"
#include "uiodmain.h"
#include "uistatusbar.h"
#include "visvsgaxes.h"
#include "visvsgcamera.h"
#include "visvsgdata.h"
#include "visvsgdatagroup.h"
#include "visvsgevent.h"
#include "visvsgscenecoltab.h"
#include "visvsgsurvscene.h"
#include "visvsgthumbwheel.h"
#include "errmsg.h"
#include "keyenum.h"
#include "notify.h"
#include "settings.h"
#include "stringview.h"
#include "uifont.h"
#include "uimsg.h"
#include "vsggeoaxes.h"
#include "vsggeonearfar.h"
#include "vsggeotext.h"
#include "vsggeothumbwheel.h"
#include "vsggeotrackball.h"

#include <vsg/ui/KeyEvent.h>
#include <vsg/ui/PointerEvent.h>
#include <vsg/ui/ScrollWheelEvent.h>

#include <cmath>
#include <functional>


namespace
{

struct ViewAll
{
    vsg::dvec3		eye_ = vsg::dvec3( 0., -5., 0. );
    vsg::dvec3		center_ = vsg::dvec3( 0., 0., 0. );
    double		near_ = 1.;
    double		far_ = 10000.;
};


ViewAll computeViewAll( const vsg::Node& node, double fovydeg )
{
    ViewAll ret;
    vsg::ref_ptr<vsg::ComputeBounds> cb = vsg::ComputeBounds::create();
    node.accept( *cb );
    if ( !cb->bounds.valid() )
	return ret;

    const vsg::dbox& bb = cb->bounds;
    ret.center_ = (bb.min + bb.max) * 0.5;
    const double radius = mMAX( vsg::length(bb.max-bb.min) * 0.5, 1e-6 );
    const double dist =
		1.1 * radius / std::sin( vsg::radians(fovydeg) * 0.5 );
    const vsg::dvec3 viewdir = vsg::normalize( vsg::dvec3(-0.5,-1.,0.7) );
    ret.eye_ = ret.center_ + viewdir * dist;
    ret.near_ = mMAX( dist - 2.*radius, 1e-3*radius );
    ret.far_ = dist + 2.*radius;
    return ret;
}


// Gives the screen-sized labels the viewport the camera really has: VSG
// changes it by itself when the window is resized or restored.
class ViewportHeightSync : public vsg::Inherit<vsg::Operation,
					       ViewportHeightSync>
{
public:
    ViewportHeightSync( vsg::ref_ptr<vsg::Camera> camera )
	: camera_(camera)
    {}

    void run() override
    {
	if ( camera_ && camera_->viewportState )
	    vsgGeo::Text::setViewportHeight(
			Math::Abs(float(camera_->getViewport().height)) );
    }

private:

    vsg::ref_ptr<vsg::Camera>	camera_;
};


// Keeps the projection in pixels when the window is resized
class FixedOrthographic
    : public vsg::Inherit<vsg::Orthographic,FixedOrthographic>
{
public:
    FixedOrthographic( double l, double r, double b, double t, double nd,
		       double fd )
	: Inherit(l,r,b,t,nd,fd)
    {}

    void changeExtent( const VkExtent2D&, const VkExtent2D& ) override
    {}
};


// Keeps the HUD at the size of the window and the axes in its bottom right
// corner
class HudSync : public vsg::Inherit<vsg::Operation,HudSync>
{
public:
    HudSync( vsg::ref_ptr<vsg::Window> window,
	     vsg::ref_ptr<vsg::RenderGraph> windowgraph,
	     vsg::ref_ptr<vsg::Orthographic> projection,
	     vsg::ref_ptr<vsg::Camera> hudcamera,
	     vsg::ref_ptr<vsg::ClearAttachments> clear,
	     RefMan<dGBVis::Axes> axes,
	     const std::function<void(uint32_t,uint32_t)>& placeitems )
	: window_(window)
	, windowgraph_(windowgraph)
	, projection_(projection)
	, hudcamera_(hudcamera)
	, clear_(clear)
	, axes_(axes)
	, placeitems_(placeitems)
    {}

    void run() override
    {
	const VkExtent2D extent = window_->extent2D();
	const VkExtent2D prevextent = windowgraph_->previous_extent;
	const bool hasprev =
		prevextent.width != vsg::RenderGraph::invalid_dimension &&
		prevextent.height != vsg::RenderGraph::invalid_dimension;
	const VkExtent2D base = hasprev ? prevextent : extent;
	hudcamera_->viewportState->set( 0, 0, mMAX(1u,base.width),
					mMAX(1u,base.height) );
	clear_->rects.front().rect.extent = VkExtent2D{ mMAX(1u,base.width),
							mMAX(1u,base.height) };
	const float offset = axes_->getLength() + axes_->getMargin();
	if ( extent.width==width_ && extent.height==height_ &&
	     offset==offset_ )
	    return;

	width_ = extent.width;
	height_ = extent.height;
	offset_ = offset;
	projection_->right = double( mMAX(1u,width_) );
	projection_->top = double( mMAX(1u,height_) );
	axes_->setPosition( float(width_)-offset, offset );
	if ( placeitems_ )
	    placeitems_( width_, height_ );
    }

private:

    vsg::ref_ptr<vsg::Window>		window_;
    vsg::ref_ptr<vsg::RenderGraph>	windowgraph_;
    vsg::ref_ptr<vsg::Orthographic>	projection_;
    vsg::ref_ptr<vsg::Camera>		hudcamera_;
    vsg::ref_ptr<vsg::ClearAttachments>	clear_;
    RefMan<dGBVis::Axes>		axes_;
    std::function<void(uint32_t,uint32_t)> placeitems_;
    uint32_t				width_ = 0;
    uint32_t				height_ = 0;
    float				offset_ = 0.f;
};


constexpr float cWheelLength = 100.f;
constexpr float cWheelWidth = 15.f;
constexpr float cWheelLongSideDistance = 15.f;
constexpr float cWheelShortSideDistance = 40.f;
constexpr float cWheelDepth = -1.f;
constexpr float cPi = 3.14159265358979f;
constexpr float cDiscreteZoomTime = 0.2f;


void placeWheels( dGBVis::ThumbWheel& hor, dGBVis::ThumbWheel& ver,
		  dGBVis::ThumbWheel& dist, uint32_t height )
{
    const float longside = cWheelLongSideDistance + cWheelWidth/2.f;
    const float shortside = cWheelShortSideDistance + cWheelLength/2.f;
    hor.setPosition( true, shortside, longside, cWheelLength, cWheelWidth,
		     cWheelDepth );
    ver.setPosition( false, longside, shortside, cWheelLength, cWheelWidth,
		     cWheelDepth );
    dist.setPosition( false, longside, mMAX(float(height)-shortside,shortside),
		      cWheelLength, cWheelWidth, cWheelDepth );
}


const char* sKeyWheelDisplayMode()
{
    return "dTect.Scene.Wheel Display Mode";
}


bool parseWheelMode( const char* str, uiVSGViewer::WheelMode& mode )
{
    if ( StringView(str) == "Never" )
	mode = uiVSGViewer::WheelMode::Never;
    else if ( StringView(str) == "Always" )
	mode = uiVSGViewer::WheelMode::Always;
    else if ( StringView(str) == "On Hover" )
	mode = uiVSGViewer::WheelMode::OnHover;
    else
	return false;

    return true;
}

} // namespace


struct VSGMouseStatus : public CallBacker
{
    dGBVisSurvey::Scene*	scene_ = nullptr;

    void update( CallBacker* )
    {
	if ( !scene_ )
	    return;

	uiODMain* mainwin = ODMainWin();
	if ( !mainwin || !mainwin->statusBar() )
	    return;

	// Field 0 is the position field uiODSceneMgr writes.
	mainwin->statusBar()->message( scene_->getMousePosString(), 0 );
    }
};


// Off-screen capture helpers, ported from VSG's own vsgoffscreenshot example.
namespace
{

bool isSrgbFormat( VkFormat format )
{
    switch ( format )
    {
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
	return true;
    default:
	return false;
    }
}


VkFormat standardCaptureFormat( VkFormat sourceFormat )
{
    return isSrgbFormat(sourceFormat)
	? VK_FORMAT_R8G8B8A8_SRGB
	: VK_FORMAT_R8G8B8A8_UNORM;
}


bool supportsBlit( vsg::ref_ptr<vsg::Device> device, VkFormat format )
{
    vsg::ref_ptr<vsg::PhysicalDevice> physicalDevice(
				    device->getPhysicalDevice() );
    VkFormatProperties srcFormatProperties;
    vkGetPhysicalDeviceFormatProperties( *physicalDevice, format,
					 &srcFormatProperties );
    VkFormatProperties destFormatProperties;
    vkGetPhysicalDeviceFormatProperties( *physicalDevice,
					 standardCaptureFormat(format),
					 &destFormatProperties );
    return ( (srcFormatProperties.optimalTilingFeatures
	      & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0 )
	&& ( (destFormatProperties.linearTilingFeatures
	      & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0 );
}


vsg::ref_ptr<vsg::Image> createCaptureImage( vsg::ref_ptr<vsg::Device> device,
					     VkFormat sourceFormat,
					     const VkExtent2D& extent )
{
    const VkFormat targetFormat = supportsBlit(device,sourceFormat)
					 ? standardCaptureFormat(sourceFormat)
					 : sourceFormat;

    vsg::ref_ptr<vsg::Image> image = vsg::Image::create();
    image->format = targetFormat;
    image->extent = VkExtent3D{ extent.width, extent.height, 1 };
    image->arrayLayers = 1;
    image->mipLevels = 1;
    image->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image->samples = VK_SAMPLE_COUNT_1_BIT;
    image->tiling = VK_IMAGE_TILING_LINEAR;
    image->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image->compile( device );

    vsg::ref_ptr<vsg::DeviceMemory> deviceMemory = vsg::DeviceMemory::create(
	    device, image->getMemoryRequirements(device->deviceID),
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
    image->bind( deviceMemory, 0 );

    return image;
}


vsg::ref_ptr<vsg::ImageView> createTransferImageView(
	vsg::ref_ptr<vsg::Device> device, VkFormat format,
	const VkExtent2D& extent, bool isdepth )
{
    vsg::ref_ptr<vsg::Image> image = vsg::Image::create();
    image->format = format;
    image->extent = VkExtent3D{ extent.width, extent.height, 1 };
    image->mipLevels = 1;
    image->arrayLayers = 1;
    image->samples = VK_SAMPLE_COUNT_1_BIT;
    image->usage = ( isdepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
				: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
		  | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    return vsg::createImageView( device, image, isdepth
				 ? VK_IMAGE_ASPECT_DEPTH_BIT
				 : VK_IMAGE_ASPECT_COLOR_BIT );
}


vsg::ref_ptr<vsg::Node> createTransferCommands(
	vsg::ref_ptr<vsg::Device> device,
	vsg::ref_ptr<vsg::Image> sourceImage,
	vsg::ref_ptr<vsg::Image> destinationImage )
{
    vsg::ref_ptr<vsg::Commands> commands = vsg::Commands::create();

    vsg::ref_ptr<vsg::ImageMemoryBarrier> transitionDst =
				vsg::ImageMemoryBarrier::create(
	0, VK_ACCESS_TRANSFER_WRITE_BIT,
	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	destinationImage,
	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } );

    // Transitions sourceImage and waits for the render's colour write.
    vsg::ref_ptr<vsg::ImageMemoryBarrier> waitForRenderWrite =
				vsg::ImageMemoryBarrier::create(
	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	sourceImage,
	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } );

    commands->addChild( vsg::PipelineBarrier::create(
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	  | VK_PIPELINE_STAGE_TRANSFER_BIT,
	    VK_PIPELINE_STAGE_TRANSFER_BIT,
	    0, transitionDst, waitForRenderWrite ) );

    const bool sameformat = sourceImage->format == destinationImage->format
	&& sourceImage->extent.width == destinationImage->extent.width
	&& sourceImage->extent.height == destinationImage->extent.height;
    if ( sameformat )
    {
	VkImageCopy region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.extent = destinationImage->extent;

	vsg::ref_ptr<vsg::CopyImage> copyImage = vsg::CopyImage::create();
	copyImage->srcImage = sourceImage;
	copyImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	copyImage->dstImage = destinationImage;
	copyImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	copyImage->regions.push_back( region );
	commands->addChild( copyImage );
    }
    else if ( supportsBlit(device,destinationImage->format) )
    {
	VkImageBlit region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.srcOffsets[1] = VkOffset3D{
		static_cast<int32_t>(sourceImage->extent.width),
		static_cast<int32_t>(sourceImage->extent.height), 1 };
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.dstOffsets[1] = VkOffset3D{
		static_cast<int32_t>(destinationImage->extent.width),
		static_cast<int32_t>(destinationImage->extent.height), 1 };

	vsg::ref_ptr<vsg::BlitImage> blitImage = vsg::BlitImage::create();
	blitImage->srcImage = sourceImage;
	blitImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitImage->dstImage = destinationImage;
	blitImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitImage->regions.push_back( region );
	blitImage->filter = VK_FILTER_NEAREST;
	commands->addChild( blitImage );
    }

    vsg::ref_ptr<vsg::ImageMemoryBarrier> transitionToGeneral =
				vsg::ImageMemoryBarrier::create(
	VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
	VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	destinationImage,
	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } );
    commands->addChild( vsg::PipelineBarrier::create(
	    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
	    0, transitionToGeneral ) );

    return commands;
}


vsg::ref_ptr<vsg::RenderPass> createTransferRenderPass(
				    vsg::ref_ptr<vsg::Device> device,
				    VkFormat imageFormat, VkFormat depthFormat )
{
    vsg::AttachmentDescription colorAttachment =
				    vsg::defaultColorAttachment( imageFormat );
    vsg::AttachmentDescription depthAttachment =
				    vsg::defaultDepthAttachment( depthFormat );

    vsg::RenderPass::Attachments attachments{ colorAttachment,
					      depthAttachment };

    vsg::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    vsg::AttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout
			= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    vsg::SubpassDescription subpass{};
    subpass.colorAttachments.emplace_back( colorAttachmentRef );
    subpass.depthStencilAttachments.emplace_back( depthAttachmentRef );
    vsg::RenderPass::Subpasses subpasses{ subpass };

    vsg::SubpassDependency colorDependency{};
    colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    colorDependency.dstSubpass = 0;
    colorDependency.srcStageMask
			    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorDependency.dstStageMask
			    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				   | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // Must be 0, matching VSG's on-screen render pass exactly.
    colorDependency.dependencyFlags = 0;

    vsg::SubpassDependency depthDependency{};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
				  | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
				  | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask
			    = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
			       | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthDependency.dependencyFlags = 0;

    vsg::RenderPass::Dependencies dependencies{ colorDependency,
						depthDependency };
    return vsg::RenderPass::create( device, attachments,
				    subpasses, dependencies );
}


vsg::ref_ptr<vsg::Framebuffer> createOffscreenFramebuffer(
	vsg::ref_ptr<vsg::Device> device,
	vsg::ref_ptr<vsg::ImageView> transferImageView )
{
    const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    const VkExtent2D extent{ transferImageView->image->extent.width,
			     transferImageView->image->extent.height };

    vsg::ImageViews imageViews;
    imageViews.emplace_back( transferImageView );
    imageViews.emplace_back( createTransferImageView(
			device, depthFormat, extent, true ) );

    vsg::ref_ptr<vsg::RenderPass> renderPass = createTransferRenderPass(
	    device, transferImageView->image->format, depthFormat );
    return vsg::Framebuffer::create( renderPass, imageViews,
				     extent.width, extent.height, 1 );
}

} // namespace


class uiVSGViewer::SceneBoundsWatch : public CallBacker
{
public:
    explicit		SceneBoundsWatch(vsg::ref_ptr<vsgGeo::NearFar> nearfar)
			    : nearfar_(nearfar)
			{}
			~SceneBoundsWatch()	{ detachAllNotifiers(); }

    void		watch( dGBVisSurvey::Scene& scene )
			{
			    mAttachCB( scene.sceneboundingboxupdated,
				       uiVSGViewer::SceneBoundsWatch::mark );
			}

private:

    void		mark( CallBacker* )
			{
			    if ( vsg::ref_ptr<vsgGeo::NearFar> nearfar =
							nearfar_.ref_ptr() )
				nearfar->boundsChanged();
			}

    vsg::observer_ptr<vsgGeo::NearFar>	nearfar_;
};


class uiVSGViewer::WheelLink : public CallBacker
{
public:
			WheelLink( uiVSGViewer& viewer )
			    : viewer_(viewer)
			{}
			~WheelLink()	{ detachAllNotifiers(); }

    void		watch( dGBVis::ThumbWheel& wheel )
			{
			    mAttachCB( wheel.rotation,
				       uiVSGViewer::WheelLink::rotationCB );
			}

private:

    void		rotationCB( CallBacker* cb )
			{
			    mCBCapsuleUnpackWithCaller( float, delta, caller,
							cb );
			    viewer_.thumbWheelRotated( caller, delta );
			}

    uiVSGViewer&	viewer_;
};


uiVSGViewer::uiVSGViewer( QWidget* parent )
    : QWidget(parent)
{
    setAttribute( Qt::WA_DeleteOnClose );
    setMinimumSize( 600, 400 );
    setWindowTitle( "VSG Viewer" );
    setAttribute( Qt::WA_OpaquePaintEvent );
    setAttribute( Qt::WA_NoSystemBackground );
    viewer_ = dGBVSGViewer::create( rendertimems_ );

    vsg::ref_ptr<vsg::WindowTraits> traits = vsg::WindowTraits::create();
    traits->decoration = true;

    qtwindow_ = new dGBVSGWindow( viewer_, traits );
    container_ = QWidget::createWindowContainer( qtwindow_, this );
    container_->setFocusPolicy( Qt::StrongFocus );
    container_->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    statuscb_ = new VSGMouseStatus;
    windowhandler_ = uiVSGWindowHandler::create( qtwindow_, container_ );
    updateCursor();

    auto* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( container_, 1 );
}


uiVSGViewer::~uiVSGViewer()
{
    boundswatch_ = nullptr;
    wheellink_ = nullptr;
    if ( trackball_ )
	trackball_->setMovementCallback( nullptr );

    if ( viewer_ )
    {
	viewer_->deviceWaitIdle();
	viewer_->getEventHandlers().clear();
	if ( dGBVis::DataObject::getCommonViewer() == viewer_.get() )
	    dGBVis::DataObject::setCommonViewer( nullptr );
    }

    if ( windowhandler_ )
	windowhandler_->setScene( nullptr );

    if ( statuscb_ && scene_ )
	scene_->mouseposchange.remove( mCB(statuscb_,VSGMouseStatus,update) );

    if ( uiODMain* mainwin = ODMainWin() )
    {
	if ( mainwin->statusBar() )
	    mainwin->statusBar()->message( uiString::emptyString(), 0 );
    }

    if ( root_ )
	root_->children.clear();

    axes_ = nullptr;
    scenecoltab_ = nullptr;
    horthumbwheel_ = nullptr;
    verthumbwheel_ = nullptr;
    distancethumbwheel_ = nullptr;
    wheelhandler_ = nullptr;
    axeshandler_ = nullptr;
    trackball_ = nullptr;
    hudscene_ = nullptr;
    vishudcamera_ = nullptr;
    viscamera_ = nullptr;
    scene_ = nullptr;
    delete statuscb_;
    statuscb_ = nullptr;
}


void uiVSGViewer::setViewMode( bool yn )
{
    if ( windowhandler_ )
	windowhandler_->setViewMode( yn );
}


bool uiVSGViewer::isViewMode() const
{
    return windowhandler_ ? windowhandler_->isViewMode() : true;
}


void uiVSGViewer::updateCursor()
{
    if ( windowhandler_ )
	windowhandler_->syncCursor();
}


bool uiVSGViewer::maxCaptureSize( uint32_t& maxwidth,
				  uint32_t& maxheight ) const
{
    if ( bufferextent_.width==0 || bufferextent_.height==0 )
	return false;

    maxwidth = bufferextent_.width;
    maxheight = bufferextent_.height;
    return true;
}


namespace {

bool queryHardwareMaxCaptureSize( vsg::ref_ptr<vsg::Window> vsgwindow,
				  uint32_t& maxwidth, uint32_t& maxheight )
{
    if ( !vsgwindow )
	return false;

    vsg::ref_ptr<vsg::Device> device = vsgwindow->getDevice();
    vsg::PhysicalDevice* physdev = device ? device->getPhysicalDevice()
					  : nullptr;
    if ( !physdev )
	return false;

    const VkFormat sourceformat = vsgwindow->surfaceFormat().format;
    const VkFormat targetformat = supportsBlit(device,sourceformat)
					 ? standardCaptureFormat(sourceformat)
					 : sourceformat;

    VkImageFormatProperties formatprops{};
    const VkResult result = vkGetPhysicalDeviceImageFormatProperties(
	    physdev->vk(), targetformat, VK_IMAGE_TYPE_2D,
	    VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0,
	    &formatprops );
    if ( result != VK_SUCCESS )
	return false;

    maxwidth = formatprops.maxExtent.width;
    maxheight = formatprops.maxExtent.height;
    return true;
}

} // namespace


void uiVSGViewer::showEvent( QShowEvent* event )
{
    QWidget::showEvent( event );
    initializeVSG();

    // Restoring from minimized doesn't reliably fire resizeEvent().
    syncCameraViewport();
}


void uiVSGViewer::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
    syncCameraViewport();
}


void uiVSGViewer::syncCameraViewport()
{
    if ( !vsgwindow_ || !camera_ )
	return;

    const VkExtent2D extent = vsgwindow_->extent2D();
    const uint32_t w = mMAX( 1u, extent.width );
    const uint32_t h = mMAX( 1u, extent.height );

    if ( camera_->viewportState )
	camera_->viewportState->set( 0, 0, w, h );

    vsgGeo::Text::setViewportHeight( float(h) );

    if ( camera_->projectionMatrix )
    {
	if ( auto* persp = dynamic_cast<vsg::Perspective*>(
				camera_->projectionMatrix.get()) )
	    persp->aspectRatio = static_cast<double>(w) / h;
    }
}


namespace
{

constexpr double cHudDepth = 1000.;
constexpr dGBVis::Axes::Style cAxesStyle = dGBVis::Axes::Modern;


// The depth of VSG is reversed: 0 is the far end
vsg::ref_ptr<vsg::ClearAttachments> createDepthClear()
{
    VkClearAttachment attachment{};
    attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    attachment.clearValue.depthStencil = VkClearDepthStencilValue{ 0.f, 0 };
    VkClearRect rect{};
    rect.rect = VkRect2D{ VkOffset2D{0,0}, VkExtent2D{1,1} };
    rect.baseArrayLayer = 0;
    rect.layerCount = 1;
    return vsg::ClearAttachments::create(
		vsg::ClearAttachments::Attachments{ attachment },
		vsg::ClearAttachments::Rects{ rect } );
}

} // namespace


void uiVSGViewer::setupHUD()
{
    vishudcamera_ = dGBVis::Camera::create();
    vsg::ref_ptr<vsg::Camera> hudcamera( vishudcamera_->vsgCamera() );
    hudprojection_ = FixedOrthographic::create( 0., 1., 0., 1., -cHudDepth,
						 cHudDepth );
    hudcamera->projectionMatrix = hudprojection_;
    hudcamera->viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,0.,0.), vsg::dvec3(0.,0.,-1.),
		vsg::dvec3(0.,1.,0.) );

    hudscene_ = dGBVis::DataObjectGroup::create();
    hudclear_ = createDepthClear();
    hudview_ = vsg::View::create( hudcamera );
    hudview_->addChild( hudclear_ );
    hudview_->addChild( vsg::ref_ptr<vsg::Node>(hudscene_->vsgNode()) );

    axes_ = dGBVis::Axes::create();
    axes_->setStyle( cAxesStyle );
    axes_->setSize( 5.f, 55.f );
    axes_->setAnnotationTextSize( FontData::defaultPointSize() + 6 );
    axes_->setAnnotationFont( annotationFont() );
    axes_->setAnnotationColor( scene_->getAnnotColor() );
    hudscene_->addObject( axes_.ptr() );
    axes_->setPrimaryCamera( viscamera_.ptr() );

    scenecoltab_ = dGBVis::SceneColTab::create();
    scenecoltab_->setAnnotFont( annotationFont() );
    scenecoltab_->setLegendColor( scene_->getAnnotColor() );
    hudscene_->addObject( scenecoltab_.ptr() );

    horthumbwheel_ = dGBVis::ThumbWheel::create();
    verthumbwheel_ = dGBVis::ThumbWheel::create();
    distancethumbwheel_ = dGBVis::ThumbWheel::create();
    wheellink_ = new WheelLink( *this );
    horthumbwheel_->setGrowDirection( 1 );
    verthumbwheel_->setGrowDirection( 1 );
    distancethumbwheel_->setGrowDirection( -1 );
    dGBVis::ThumbWheel* wheels[] = { horthumbwheel_.ptr(), verthumbwheel_.ptr(),
				     distancethumbwheel_.ptr() };
    for ( dGBVis::ThumbWheel* wheel : wheels )
    {
	wheel->setGrowOnHover( wheelsgrow_ );
	wheel->setGlassLook( wheelsglass_ );
	hudscene_->addObject( wheel );
	wheellink_->watch( *wheel );
    }
}


void uiVSGViewer::setWheelDisplayMode( WheelMode mode )
{
    wheelmode_ = mode;
    dGBVis::ThumbWheel* wheels[] = { horthumbwheel_.ptr(), verthumbwheel_.ptr(),
				     distancethumbwheel_.ptr() };
    for ( dGBVis::ThumbWheel* wheel : wheels )
    {
	if ( !wheel )
	    continue;

	wheel->turnOn( mode != WheelMode::Never );
	wheel->enableFadeInOut( mode == WheelMode::OnHover );
	if ( !wheelhandler_ )
	    continue;

	wheelhandler_->removeThumbWheel( wheel->vsgWheel() );
	if ( mode != WheelMode::Never )
	    wheelhandler_->addThumbWheel( wheel->vsgWheel() );
    }
}


uiVSGViewer::WheelMode uiVSGViewer::getWheelDisplayMode() const
{
    return wheelmode_;
}


uiVSGViewer::WheelMode uiVSGViewer::startWheelMode()
{
    BufferString modestr;
    WheelMode mode = WheelMode::OnHover;
    if ( Settings::common().get(sKeyWheelDisplayMode(),modestr) )
	parseWheelMode( modestr, mode );

    return mode;
}


void uiVSGViewer::setWheelsGrowOnHover( bool yn )
{
    wheelsgrow_ = yn;
    dGBVis::ThumbWheel* wheels[] = { horthumbwheel_.ptr(), verthumbwheel_.ptr(),
				     distancethumbwheel_.ptr() };
    for ( dGBVis::ThumbWheel* wheel : wheels )
    {
	if ( wheel )
	    wheel->setGrowOnHover( yn );
    }
}


bool uiVSGViewer::isWheelsGrowOnHover() const
{
    return wheelsgrow_;
}


void uiVSGViewer::setGlassWheels( bool yn )
{
    wheelsglass_ = yn;
    dGBVis::ThumbWheel* wheels[] = { horthumbwheel_.ptr(), verthumbwheel_.ptr(),
				     distancethumbwheel_.ptr() };
    for ( dGBVis::ThumbWheel* wheel : wheels )
    {
	if ( wheel )
	    wheel->setGlassLook( yn );
    }
}


bool uiVSGViewer::isGlassWheels() const
{
    return wheelsglass_;
}


void uiVSGViewer::showWheels( bool yn )
{
    dGBVis::ThumbWheel* wheels[] = { horthumbwheel_.ptr(), verthumbwheel_.ptr(),
				     distancethumbwheel_.ptr() };
    for ( dGBVis::ThumbWheel* wheel : wheels )
    {
	if ( wheel )
	    wheel->turnOn( yn );
    }
}


void uiVSGViewer::thumbWheelRotated( CallBacker* caller, float delta )
{
    if ( !trackball_ )
	return;

    if ( caller == horthumbwheel_.ptr() )
	trackball_->turn( delta, true );
    else if ( caller == verthumbwheel_.ptr() )
	trackball_->turn( delta, false );
    else if ( caller == distancethumbwheel_.ptr() )
	trackball_->changeDistance( delta/cPi );
}


void uiVSGViewer::cameraMoved( float hor, float vert, float dist, bool step )
{
    if ( !horthumbwheel_ || !verthumbwheel_ || !distancethumbwheel_ )
	return;

    const float rotationtime = step ? cDiscreteZoomTime : 0.f;
    distancethumbwheel_->setAngle( distancethumbwheel_->getAngle()-dist*cPi,
				   rotationtime );
    horthumbwheel_->setAngle( horthumbwheel_->getAngle() + hor );
    verthumbwheel_->setAngle( verthumbwheel_->getAngle() + vert );
}


void uiVSGViewer::showRotAxis( bool yn )
{
    if ( axes_ )
	axes_->turnOn( yn );
}


bool uiVSGViewer::isAxisShown() const
{
    return axes_ && axes_->isOn();
}


void uiVSGViewer::setModernAxes( bool yn )
{
    if ( axes_ )
	axes_->setStyle( yn ? dGBVis::Axes::Modern : dGBVis::Axes::Classic );
}


bool uiVSGViewer::isModernAxes() const
{
    return axes_ && axes_->getStyle()==dGBVis::Axes::Modern;
}


void uiVSGViewer::showColorBar( bool yn )
{
    if ( scenecoltab_ )
	scenecoltab_->turnOn( yn );
}


bool uiVSGViewer::isColorBarShown() const
{
    return scenecoltab_ && scenecoltab_->isOn();
}


void uiVSGViewer::setColorBarPos( dGBVis::SceneColTab::Pos pos )
{
    if ( scenecoltab_ )
	scenecoltab_->setPos( pos );
}


dGBVis::SceneColTab::Pos uiVSGViewer::getColorBarPos() const
{
    return scenecoltab_ ? scenecoltab_->getPos() : dGBVis::SceneColTab::Bottom;
}


void uiVSGViewer::setColorBarSize( int width, int height )
{
    if ( scenecoltab_ )
	scenecoltab_->setSize( width, height );
}


Geom::Size2D<int> uiVSGViewer::getColorBarSize() const
{
    return scenecoltab_ ? scenecoltab_->getSize() : Geom::Size2D<int>();
}


void uiVSGViewer::initializeVSG()
{
    if ( vsgwindow_ || initfailed_ || !qtwindow_ || !viewer_ )
	return;

    try
    {
	setupVSG();
    }
    catch ( const vsg::Exception& exc )
    {
	// Stop the render timer before it fires into a half-set-up viewer.
	initfailed_ = true;
	viewer_->stop();
	vsgwindow_ = nullptr;
	BufferString errmsg( "VSG viewer setup failed: " );
	errmsg.add( exc.message.c_str() ).add( " (VkResult " )
	      .add( exc.result ).add( ")" );
	pErrMsg( errmsg );
	uiMSG().error( toUiString("VSG viewer setup failed:\n%1 (VkResult %2)")
			.arg(exc.message.c_str()).arg(exc.result) );
    }
}


void uiVSGViewer::setupVSG()
{
    qtwindow_->initializeWindow();
    vsgwindow_ = qtwindow_->vsgWindow();
    if ( !vsgwindow_ )
	return;

    vsgwindow_->clearColor() = vsg::vec4( 0.2f, 0.2f, 0.2f, 1.f );

    const uint32_t w = mMAX( 1u, vsgwindow_->extent2D().width );
    const uint32_t h = mMAX( 1u, vsgwindow_->extent2D().height );

    root_ = vsg::Group::create();
    scene_ = dGBVisSurvey::Scene::create();
    const FontData font = hasannotfont_ ? annotfont_
		: FontList().get(FontData::Graphics3D).fontData();
    scene_->setAnnotFont( font );
    root_->addChild( vsg::ref_ptr<vsg::Node>(scene_->vsgNode()) );

    const double fovy = 30.0;
    const ViewAll viewall = computeViewAll( *root_, fovy );
    vsg::ref_ptr<vsg::LookAt> lookAt = vsg::LookAt::create(
	    viewall.eye_, viewall.center_, vsg::dvec3( 0.0, 0.0, 1.0 ) );
    vsg::ref_ptr<vsg::Perspective> perspective = vsg::Perspective::create(
	    fovy, static_cast<double>(w) / h, viewall.near_, viewall.far_ );
    vsg::ref_ptr<vsg::ViewportState> viewport =
			vsg::ViewportState::create( VkExtent2D{ w, h } );
    viscamera_ = dGBVis::Camera::create();
    camera_ = viscamera_->vsgCamera();
    camera_->projectionMatrix = perspective;
    camera_->viewMatrix = lookAt;
    camera_->viewportState = viewport;
    viewer_->addUpdateOperation( ViewportHeightSync::create(camera_),
				 vsg::UpdateOperations::ALL_FRAMES );

    vsg::ref_ptr<vsgGeo::Trackball> trackball =
					vsgGeo::Trackball::create( camera_ );
    trackball->addWindow( qtwindow_->vsgWindow() );
    trackball_ = trackball;
    trackball_->setMovementCallback(
	[this]( float hor, float vert, float dist, bool step )
	{ cameraMoved( hor, vert, dist, step ); } );
    if ( scene_->eventCatcher() )
    {
	scene_->eventCatcher()->setCamera( camera_.get() );
	scene_->eventCatcher()->setSceneRoot( root_.get() );
    }
    if ( windowhandler_ )
    {
	windowhandler_->setTrackball( trackball.get() );
	windowhandler_->setScene( scene_.ptr() );
	windowhandler_->setViewMode( windowhandler_->isViewMode() );
    }

    scene_->addInteractionTest();

    wheelhandler_ = vsgGeo::ThumbWheelHandler::create();
    viewer_->addEventHandler( wheelhandler_ );
    if ( windowhandler_ )
	viewer_->addEventHandler( windowhandler_ );
    if ( scene_->eventCatcher() )
	viewer_->addEventHandler( vsg::ref_ptr<vsg::Visitor>(
			scene_->eventCatcher()->eventHandler() ) );
    viewer_->addEventHandler( trackball );
    if ( statuscb_ )
    {
	statuscb_->scene_ = scene_.ptr();
	scene_->mouseposchange.notify(
			    mCB(statuscb_,VSGMouseStatus,update) );
    }

    vsg::ref_ptr<vsg::CommandGraph> commandGraph =
			vsg::createCommandGraphForView( qtwindow_->vsgWindow(),
							camera_,
							root_ );

    setupHUD();
    axeshandler_ = vsgGeo::AxesHandler::create();
    axeshandler_->setAxes( axes_->vsgAxes() );
    viewer_->addEventHandler( axeshandler_ );
    vsg::observer_ptr<vsgGeo::AxesNode> axesnode( axes_->vsgAxes() );
    qtwindow_->setLeaveCallback( [axesnode]()
	{
	    if ( vsg::ref_ptr<vsgGeo::AxesNode> axes = axesnode.ref_ptr() )
		axes->clearPointer();
	} );
    vsg::ref_ptr<vsg::RenderGraph> windowgraph =
		commandGraph->children.front().cast<vsg::RenderGraph>();
    windowgraph->addChild( hudview_ );
    RefMan<dGBVis::ThumbWheel> horwheel = horthumbwheel_;
    RefMan<dGBVis::ThumbWheel> verwheel = verthumbwheel_;
    RefMan<dGBVis::ThumbWheel> distwheel = distancethumbwheel_;
    RefMan<dGBVis::SceneColTab> coltab = scenecoltab_;
    const std::function<void(uint32_t,uint32_t)> placeitems =
	[horwheel,verwheel,distwheel,coltab]( uint32_t width,
					      uint32_t height ) mutable
	{
	    placeWheels( *horwheel.ptr(), *verwheel.ptr(), *distwheel.ptr(),
			 height );
	    coltab->setWindowSize( int(width), int(height) );
	};
    viewer_->addUpdateOperation( HudSync::create(vsgwindow_,windowgraph,
		hudprojection_,
		vsg::ref_ptr<vsg::Camera>(vishudcamera_->vsgCamera()),
		hudclear_,axes_,placeitems),
		vsg::UpdateOperations::ALL_FRAMES );

    windowswitch_ = vsg::Switch::create();
    for ( auto& child : commandGraph->children )
	windowswitch_->addChild( true, child );

    setupOffscreenCapture();

    nearfar_ = vsgGeo::NearFar::create();
    nearfar_->setCamera( camera_ );
    nearfar_->setScene( root_ );
    nearfar_->setOffscreen( offscreenperspective_ );
    boundswatch_ = new SceneBoundsWatch( nearfar_ );
    boundswatch_->watch( *scene_ );
    viewer_->addUpdateOperation( nearfar_,
				 vsg::UpdateOperations::ALL_FRAMES );

    commandGraph->children.clear();
    commandGraph->addChild( offscreenswitch_ );
    commandGraph->addChild( windowswitch_ );

    viewer_->assignRecordAndSubmitTaskAndPresentation( { commandGraph } );

    offscreenswitch_->setAllChildren( true );
    viewer_->compile();
    offscreenswitch_->setAllChildren( false );

    // After the first compile: from now on, changes to dGBVis objects are
    // compiled and released through this viewer's update operation.
    dGBVis::DataObject::setCommonViewer( viewer_.get() );

    setWheelDisplayMode( startWheelMode() );
    scenecoltab_->turnOn( false );
}


void uiVSGViewer::setupOffscreenCapture()
{
    vsg::ref_ptr<vsg::Device> device = vsgwindow_->getDevice();

    uint32_t hwmaxw = 0, hwmaxh = 0;
    if ( !queryHardwareMaxCaptureSize(vsgwindow_,hwmaxw,hwmaxh) )
    {
	hwmaxw = 4096;
	hwmaxh = 4096;
    }
    const uint32_t cap = 8192;
	bufferextent_.width = mMIN( hwmaxw, cap );
    bufferextent_.height = mMIN( hwmaxh, cap );

    captureextent_ = vsgwindow_->extent2D();
    if ( captureextent_.width<1 )
	captureextent_.width = 1;
    if ( captureextent_.height<1 )
	captureextent_.height = 1;
    if ( captureextent_.width>bufferextent_.width )
	captureextent_.width = bufferextent_.width;
    if ( captureextent_.height>bufferextent_.height )
	captureextent_.height = bufferextent_.height;

    const VkFormat offscreenformat = vsgwindow_->surfaceFormat().format;

    transferimageview_ = createTransferImageView( device, offscreenformat,
						  bufferextent_, false );
    captureImage_ = createCaptureImage( device,
					offscreenformat, bufferextent_ );
    capturecommands_ = createTransferCommands(
			device, transferimageview_->image, captureImage_ );

    offscreenrendergraph_ = vsg::RenderGraph::create();
    offscreenrendergraph_->framebuffer =
		    createOffscreenFramebuffer( device, transferimageview_ );
    offscreenrendergraph_->renderArea.offset = VkOffset2D{ 0, 0 };
    offscreenrendergraph_->renderArea.extent = captureextent_;
    offscreenrendergraph_->setClearValues(
	    VkClearColorValue{{0.2f,0.2f,0.2f,1.f}} );

    offscreenperspective_ = vsg::Perspective::create( 30.0,
	    static_cast<double>(captureextent_.width) / captureextent_.height,
	    1.0, 10000.0 );
    offscreencamera_ = vsg::Camera::create( offscreenperspective_,
	    camera_->viewMatrix, vsg::ViewportState::create(captureextent_) );

    offscreenview_ = vsg::View::create( offscreencamera_, root_ );
    offscreenview_->addChild( vsg::createHeadlight() );
    offscreenrendergraph_->addChild( offscreenview_ );

    offscreenhudcamera_ = vsg::Camera::create( hudprojection_,
		vishudcamera_->vsgCamera()->viewMatrix,
		vsg::ViewportState::create(captureextent_) );
    offscreenhudclear_ = createDepthClear();
    offscreenhudclear_->rects.front().rect.extent = captureextent_;
    offscreenhudview_ = vsg::View::create( offscreenhudcamera_ );
    offscreenhudview_->addChild( offscreenhudclear_ );
    offscreenhudview_->addChild(
		    vsg::ref_ptr<vsg::Node>(hudscene_->vsgNode()) );
    offscreenrendergraph_->addChild( offscreenhudview_ );

    offscreenswitch_ = vsg::Switch::create();
    offscreenswitch_->addChild( false, offscreenrendergraph_ );
    offscreenswitch_->addChild( false, capturecommands_ );
}


void uiVSGViewer::setAnnotationFont( const FontData& fd )
{
    annotfont_ = fd;
    hasannotfont_ = true;
    if ( scene_ )
	scene_->setAnnotFont( fd );

    if ( axes_ )
	axes_->setAnnotationFont( fd );

    if ( scenecoltab_ )
	scenecoltab_->setAnnotFont( fd );
}


FontData uiVSGViewer::annotationFont() const
{
    if ( scene_ )
	return scene_->getAnnotFont();

    if ( hasannotfont_ )
	return annotfont_;

    return FontList().get( FontData::Graphics3D ).fontData();
}


uiRetVal uiVSGViewer::takeScreenshot( int width, int height,
				       const char* destpath, float dpi )
{
    if ( !vsgwindow_ || !viewer_ || !root_ || !camera_ || !offscreenswitch_ )
	return uiRetVal( toUiString("Viewer not initialized") );

    if ( width<1 || height<1 || !destpath || !*destpath )
	return uiRetVal( toUiString("Invalid screenshot parameters") );

    if ( static_cast<uint32_t>(width)>bufferextent_.width
      || static_cast<uint32_t>(height)>bufferextent_.height )
	return uiRetVal( toUiString("Requested size exceeds this GPU's "
	    "maximum capture size (%1x%2 pixels)")
	    .arg(bufferextent_.width).arg(bufferextent_.height) );

    captureextent_.width = static_cast<uint32_t>( width );
    captureextent_.height = static_cast<uint32_t>( height );

    double projaspect = 1.;
    if ( auto* persp = dynamic_cast<vsg::Perspective*>(
			    camera_->projectionMatrix.get()) )
    {
	projaspect = persp->aspectRatio;
	offscreenperspective_->fieldOfViewY = persp->fieldOfViewY;
	offscreenperspective_->aspectRatio = persp->aspectRatio;
	offscreenperspective_->nearDistance = persp->nearDistance;
	offscreenperspective_->farDistance = persp->farDistance;
    }

    offscreenrendergraph_->renderArea.offset = VkOffset2D{ 0, 0 };
    offscreenrendergraph_->renderArea.extent = captureextent_;
    if ( offscreencamera_->viewportState )
	offscreencamera_->viewportState->set( 0, 0,
				captureextent_.width, captureextent_.height );

    offscreenhudcamera_->viewportState->set( 0, 0, captureextent_.width,
					     captureextent_.height );
    offscreenhudclear_->rects.front().rect.extent = captureextent_;

    const double textscale = projaspect / ( double(width) / height );
    const bool stretched = Math::Abs( textscale - 1. ) > 1e-3;
    vsgGeo::Text::setHorizontalScale( float(textscale) );

    // The capture view has its own view id. Pipelines built for the
    // on-screen view are not valid for it, and recording one asserts.
    if ( offscreenrendergraph_->framebuffer && offscreenview_ )
    {
	vsg::ref_ptr<vsg::CompileTraversal> compile =
				vsg::CompileTraversal::create();
	compile->add( *offscreenrendergraph_->framebuffer, offscreenview_ );
	compile->compile( offscreenview_ );
    }

    if ( offscreenrendergraph_->framebuffer && offscreenhudview_ )
    {
	vsg::ref_ptr<vsg::CompileTraversal> compile =
					vsg::CompileTraversal::create();
	compile->add( *offscreenrendergraph_->framebuffer, offscreenhudview_ );
	compile->compile( offscreenhudview_ );
    }

    showWheels( false );
    offscreenswitch_->setAllChildren( true );
    viewer_->request();
    viewer_->render();
    viewer_->deviceWaitIdle();
    if ( stretched )
    {
	windowswitch_->setAllChildren( false );
	viewer_->request();
	viewer_->render();
	viewer_->deviceWaitIdle();

	vsgGeo::Text::setHorizontalScale( 1.f );
	viewer_->request();
	viewer_->render();
	viewer_->deviceWaitIdle();
	windowswitch_->setAllChildren( true );
    }

    offscreenswitch_->setAllChildren( false );
    showWheels( wheelmode_ != WheelMode::Never );
    vsgGeo::Text::setHorizontalScale( 1.f );
    viewer_->request();

    vsg::ref_ptr<vsg::Device> device = vsgwindow_->getDevice();
    if ( !captureImage_ || !device )
	return uiRetVal( toUiString("Viewer not initialized") );

    viewer_->waitForFences( 0, 1000000000 );

    try
    {
	const uint32_t w = static_cast<uint32_t>( width );
	const uint32_t h = static_cast<uint32_t>( height );

	VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout( *device,
				     captureImage_->vk(device->deviceID),
				     &subResource, &subResourceLayout );

	vsg::ref_ptr<vsg::DeviceMemory> deviceMemory(
		    captureImage_->getDeviceMemory( device->deviceID ) );

	const size_t destRowWidth = w * sizeof( vsg::ubvec4 );
	vsg::ref_ptr<vsg::MappedData<vsg::ubyteArray>> mappedData =
			vsg::MappedData<vsg::ubyteArray>::create(
		deviceMemory, subResourceLayout.offset, 0,
		vsg::Data::Properties{ captureImage_->format },
		subResourceLayout.rowPitch * h );
	vsg::ref_ptr<vsg::Data> imageData = vsg::ubvec4Array2D::create(
		w, h, vsg::Data::Properties{ captureImage_->format } );
	for ( uint32_t row = 0; row < h; ++row )
	    OD::memCopy( imageData->dataPointer(row * w),
		mappedData->dataPointer(row * subResourceLayout.rowPitch),
		od_int64(destRowWidth) );

	// Force fully opaque, since stray alpha can render as blank/transparent
	{
	    unsigned char* raw = static_cast<unsigned char*>(
			    imageData->dataPointer() );
	    const size_t npixels = static_cast<size_t>(w) * h;
	    for ( size_t p=0; p<npixels; ++p )
		raw[p*4 + 3] = 255;
	}

	QImage img( static_cast<const uchar*>( imageData->dataPointer() ),
		    static_cast<int>( w ), static_cast<int>( h ),
		    QImage::Format_RGBA8888 );
	img = img.copy();

	if ( dpi>0.f )
	{
	    const double dotsPerMeter = static_cast<double>(dpi) / 0.0254;
	    img.setDotsPerMeterX( static_cast<int>(dotsPerMeter + 0.5) );
	    img.setDotsPerMeterY( static_cast<int>(dotsPerMeter + 0.5) );
	}

	const QString outpath( destpath );
	if ( !img.save(outpath) )
	    return uiRetVal( toUiString("Failed to save screenshot to %1")
				 .arg(destpath) );

	return uiRetVal::OK();
    }
    catch ( const vsg::Exception& exc )
    {
	return uiRetVal( toUiString("Screenshot failed: %1")
			     .arg(exc.message.c_str()) );
    }
}
