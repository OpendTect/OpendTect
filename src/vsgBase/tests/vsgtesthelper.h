#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 Headless VSG viewer for the uiVSG test programs: renders into an
 off-screen framebuffer, reads the colour image back, and counts
 validation-layer warnings and errors.

-*/

#include "typeset.h"

#include <vsg/all.h>

#include <atomic>
#include <climits>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace vsgTest
{

class OffscreenViewer
{
public:

    static constexpr VkFormat	sColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    static constexpr VkFormat	sDepthFormat = VK_FORMAT_D32_SFLOAT;

    OffscreenViewer( uint32_t width=256, uint32_t height=256,
		     bool validation=true )
	: width_(width), height_(height)
    {
	try { init( validation ); }
	catch ( const vsg::Exception& exc )
	{
	    errmsg_ = "Vulkan setup failed: " + exc.message;
	    viewer_ = nullptr;
	}
    }

    ~OffscreenViewer()
    {
	if ( viewer_ )
	    viewer_->deviceWaitIdle();

	viewer_ = nullptr;
	if ( messenger_ != VK_NULL_HANDLE && instance_ )
	{
	    PFN_vkDestroyDebugUtilsMessengerEXT destroyfn = nullptr;
	    if ( instance_->getProcAddr(destroyfn,
				"vkDestroyDebugUtilsMessengerEXT") )
		destroyfn( *instance_, messenger_, nullptr );
	}
    }

    bool		isOK() const		{ return viewer_.valid(); }
    const std::string&	errMsg() const		{ return errmsg_; }
    bool		hasValidation() const	{ return hasvalidation_; }
    int			nrValidationMessages() const { return nrmsgs_; }

    vsg::Viewer&	viewer()		{ return *viewer_; }
    vsg::Group&		scene()			{ return *scene_; }
    vsg::Camera&	camera()		{ return *camera_; }
    vsg::Device&	device()		{ return *device_; }

    /*!A second view of the scene, with its own framebuffer, as in the
	screenshot of the viewer: the switch is off. Call before compile().
	beforemain makes it the first child of the command graph, as in the
	viewer. */
    vsg::ref_ptr<vsg::Switch> addSwitchedView( uint32_t width, uint32_t height,
					       vsg::ref_ptr<vsg::Camera>& camera,
					       vsg::ref_ptr<vsg::Image>*
							    colorimage=nullptr,
						       bool beforemain=false )
    {
	vsg::ref_ptr<vsg::Image> color = createImage( sColorFormat,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT, width, height );
	if ( colorimage )
	    *colorimage = color;
	vsg::ref_ptr<vsg::Image> depth = createImage( sDepthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			width, height );
	vsg::ref_ptr<vsg::ImageView> colorview = vsg::createImageView(
			device_, color, VK_IMAGE_ASPECT_COLOR_BIT );
	vsg::ref_ptr<vsg::ImageView> depthview = vsg::createImageView(
			device_, depth, VK_IMAGE_ASPECT_DEPTH_BIT );
	vsg::ref_ptr<vsg::Framebuffer> framebuffer = vsg::Framebuffer::create(
			createRenderPass(), vsg::ImageViews{ colorview, depthview },
			width, height, 1 );

	camera = vsg::Camera::create(
		camera_->projectionMatrix, camera_->viewMatrix,
		vsg::ViewportState::create( VkExtent2D{width,height} ) );

	vsg::ref_ptr<vsg::RenderGraph> rendergraph = vsg::RenderGraph::create();
	rendergraph->framebuffer = framebuffer;
	rendergraph->renderArea.offset = VkOffset2D{ 0, 0 };
	rendergraph->renderArea.extent = VkExtent2D{ width, height };
	rendergraph->setClearValues( VkClearColorValue{{ 0.f, 0.f, 0.f, 1.f }} );
	rendergraph->addChild( vsg::View::create(camera,scene_) );

	vsg::ref_ptr<vsg::Switch> sw = vsg::Switch::create();
	sw->addChild( false, rendergraph );
	if ( beforemain )
	    commandgraph_->children.insert( commandgraph_->children.begin(), sw );
	else
	    commandgraph_->addChild( sw );

	return sw;
    }

    /*!An overlay view like the HUD of the viewer: one unit is one pixel, y is
	up, and the depth is cleared first. \returns the group to add the HUD
	nodes to. */
    vsg::ref_ptr<vsg::Group> addHud()
    {
	vsg::ref_ptr<vsg::Orthographic> projection = vsg::Orthographic::create(
		0., double(width_), 0., double(height_), -1000., 1000. );
	vsg::ref_ptr<vsg::LookAt> lookat = vsg::LookAt::create(
		vsg::dvec3(0.,0.,0.), vsg::dvec3(0.,0.,-1.),
		vsg::dvec3(0.,1.,0.) );
	vsg::ref_ptr<vsg::Camera> hudcamera = vsg::Camera::create(
		projection, lookat,
		vsg::ViewportState::create(VkExtent2D{width_,height_}) );

	VkClearAttachment attachment{};
	attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	attachment.clearValue.depthStencil = VkClearDepthStencilValue{ 0.f, 0 };
	VkClearRect rect{};
	rect.rect = VkRect2D{ VkOffset2D{0,0}, VkExtent2D{width_,height_} };
	rect.baseArrayLayer = 0;
	rect.layerCount = 1;

	vsg::ref_ptr<vsg::Group> scene = vsg::Group::create();
	vsg::ref_ptr<vsg::View> view = vsg::View::create( hudcamera );
	view->addChild( vsg::ClearAttachments::create(
		vsg::ClearAttachments::Attachments{ attachment },
		vsg::ClearAttachments::Rects{ rect } ) );
	view->addChild( scene );
	rendergraph_->addChild( view );
	return scene;
    }

    /*!Around the view of the main framebuffer, which is the window of the
	viewer: switched off, that view is not drawn and keeps its last
	picture. */
    vsg::Switch&	mainSwitch()		{ return *mainswitch_; }

    bool compile()
    {
	if ( !isOK() )
	    return false;

	const vsg::CompileResult result = viewer_->compile();
	if ( !result )
	    errmsg_ = "Viewer compile failed: " + result.message;
	return bool( result );
    }

    /*!waitidle=false leaves frames in flight, as in an interactive session */
    bool frame( int nrframes=1, bool waitidle=true )
    {
	if ( !isOK() )
	    return false;

	for ( int idx=0; idx<nrframes; idx++ )
	{
	    if ( !viewer_->advanceToNextFrame() )
		return false;

	    viewer_->handleEvents();
	    viewer_->update();
	    viewer_->recordAndSubmit();
	    viewer_->present();
	}

	if ( waitidle )
	    viewer_->deviceWaitIdle();
	return true;
    }

    uint64_t frameCount() const
    {
	const vsg::FrameStamp* fs = viewer_->getFrameStamp();
	return fs ? fs->frameCount : 0;
    }

    //!After frame(); row 0 is the top row
    bool readColor( TypeSet<vsg::ubvec4>& pixels )
    { return readImage( colorimage_, width_, height_, pixels, true ); }

    bool readImage( vsg::ref_ptr<vsg::Image> colorimage, uint32_t width,
		    uint32_t height, TypeSet<vsg::ubvec4>& pixels,
		    bool keepbuffer=false )
    {
	if ( !isOK() )
	    return false;

	viewer_->deviceWaitIdle();
	const VkDeviceSize nrbytes = VkDeviceSize(width) * height * 4;
	vsg::ref_ptr<vsg::Buffer> localbuffer;
	vsg::ref_ptr<vsg::Buffer>& readbuffer =
				keepbuffer ? readbuffer_ : localbuffer;
	if ( !readbuffer )
	    readbuffer = vsg::createBufferAndMemory( device_.get(), nrbytes,
			    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			    VK_SHARING_MODE_EXCLUSIVE,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = VkExtent3D{ width, height, 1 };

	vsg::ref_ptr<vsg::CopyImageToBuffer> copy =
					vsg::CopyImageToBuffer::create();
	copy->srcImage = colorimage;
	copy->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	copy->dstBuffer = readbuffer;
	copy->regions.push_back( region );

	vsg::ref_ptr<vsg::CommandPool> pool =
			vsg::CommandPool::create( device_, queuefamily_ );
	vsg::ref_ptr<vsg::Fence> fence = vsg::Fence::create( device_ );
	vsg::ref_ptr<vsg::Queue> queue = device_->getQueue( queuefamily_ );
	vsg::submitCommandsToQueue( pool, fence, 10000000000, queue,
		[&]( vsg::CommandBuffer& cb ) { copy->record( cb ); } );

	const uint32_t devid = device_->deviceID;
	vsg::DeviceMemory* mem = readbuffer->getDeviceMemory( devid );
	void* data = nullptr;
	if ( mem->map(readbuffer->getMemoryOffset(devid),nrbytes,0,&data)
		!= VK_SUCCESS )
	    return false;

	pixels.setSize( int(width*height) );
	std::memcpy( pixels.arr(), data, nrbytes );
	mem->unmap();
	return true;
    }

    vsg::ubvec4 pixel( const TypeSet<vsg::ubvec4>& pixels,
		       uint32_t x, uint32_t y ) const
    { return pixels[int(y*width_ + x)]; }

    uint32_t		width() const		{ return width_; }
    uint32_t		height() const		{ return height_; }

    const vsg::vec4	clearcolor_ = vsg::vec4( 0.f, 0.f, 0.f, 1.f );

private:

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCB(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* userdata )
    {
	if ( severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
	{
	    auto* self = static_cast<OffscreenViewer*>( userdata );
	    self->nrmsgs_++;
	    std::cerr << "[Vulkan] " << (data && data->pMessage
				? data->pMessage : "") << std::endl;
	}

	return VK_FALSE;
    }

    void init( bool validation )
    {
	vsg::Names instext, layers;
	if ( validation )
	{
	    layers = vsg::validateInstanceLayerNames(
				{ "VK_LAYER_KHRONOS_validation" } );
	    hasvalidation_ = !layers.empty() &&
		vsg::isExtensionSupported( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	    if ( hasvalidation_ )
		instext.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	}

	instance_ = vsg::Instance::create( instext, layers,
					   VK_API_VERSION_1_1 );
	if ( hasvalidation_ )
	    addMessenger();

	auto [physdev, queuefamily] =
	    instance_->getPhysicalDeviceAndQueueFamily( VK_QUEUE_GRAPHICS_BIT );
	if ( !physdev || queuefamily < 0 )
	{
	    errmsg_ = "No Vulkan device with graphics support";
	    return;
	}

	queuefamily_ = queuefamily;
	vsg::QueueSettings queuesettings{ vsg::QueueSetting{queuefamily,
							    {1.f}} };
	vsg::ref_ptr<vsg::DeviceFeatures> features =
					vsg::DeviceFeatures::create();
	features->get().samplerAnisotropy = VK_TRUE;
	device_ = vsg::Device::create( physdev, queuesettings, vsg::Names(),
				       vsg::Names(), features );

	colorimage_ = createImage( sColorFormat,
				   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				   VK_IMAGE_USAGE_TRANSFER_SRC_BIT );
	vsg::ref_ptr<vsg::Image> depthimage = createImage( sDepthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT );
	vsg::ref_ptr<vsg::ImageView> colorview = vsg::createImageView(
		device_, colorimage_, VK_IMAGE_ASPECT_COLOR_BIT );
	vsg::ref_ptr<vsg::ImageView> depthview = vsg::createImageView(
		device_, depthimage, VK_IMAGE_ASPECT_DEPTH_BIT );

	vsg::ref_ptr<vsg::RenderPass> renderpass = createRenderPass();
	vsg::ref_ptr<vsg::Framebuffer> framebuffer = vsg::Framebuffer::create(
		renderpass, vsg::ImageViews{ colorview, depthview },
		width_, height_, 1 );

	vsg::ref_ptr<vsg::LookAt> lookat = vsg::LookAt::create(
		vsg::dvec3( 0.0, -5.0, 0.0 ), vsg::dvec3( 0.0, 0.0, 0.0 ),
		vsg::dvec3( 0.0, 0.0, 1.0 ) );
	vsg::ref_ptr<vsg::Orthographic> ortho = vsg::Orthographic::create(
		-1.0, 1.0, -1.0, 1.0, 0.1, 100.0 );
	camera_ = vsg::Camera::create( ortho, lookat,
		vsg::ViewportState::create( VkExtent2D{width_,height_} ) );

	scene_ = vsg::Group::create();

	vsg::ref_ptr<vsg::RenderGraph> rendergraph = vsg::RenderGraph::create();
	rendergraph->framebuffer = framebuffer;
	rendergraph->renderArea.offset = VkOffset2D{ 0, 0 };
	rendergraph->renderArea.extent = VkExtent2D{ width_, height_ };
	rendergraph->setClearValues( VkClearColorValue{{ clearcolor_.r,
		clearcolor_.g, clearcolor_.b, clearcolor_.a }} );
	rendergraph->addChild( vsg::View::create(camera_,scene_) );

	rendergraph_ = rendergraph;
	mainswitch_ = vsg::Switch::create();
	mainswitch_->addChild( true, rendergraph );
	commandgraph_ = vsg::CommandGraph::create( device_, queuefamily_ );
	commandgraph_->addChild( mainswitch_ );

	viewer_ = vsg::Viewer::create();
	viewer_->assignRecordAndSubmitTaskAndPresentation( { commandgraph_ } );
    }

    vsg::ref_ptr<vsg::Image> createImage( VkFormat format,
					  VkImageUsageFlags usage,
					  uint32_t width=0, uint32_t height=0 )
    {
	vsg::ref_ptr<vsg::Image> image = vsg::Image::create();
	image->imageType = VK_IMAGE_TYPE_2D;
	image->format = format;
	image->extent = VkExtent3D{ width ? width : width_,
				    height ? height : height_, 1 };
	image->mipLevels = 1;
	image->arrayLayers = 1;
	image->samples = VK_SAMPLE_COUNT_1_BIT;
	image->tiling = VK_IMAGE_TILING_OPTIMAL;
	image->usage = usage;
	return image;
    }

    vsg::ref_ptr<vsg::RenderPass> createRenderPass()
    {
	vsg::AttachmentDescription coloratt =
				vsg::defaultColorAttachment( sColorFormat );
	coloratt.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	vsg::AttachmentDescription depthatt =
				vsg::defaultDepthAttachment( sDepthFormat );

	vsg::SubpassDescription subpass;
	subpass.colorAttachments.push_back( vsg::AttachmentReference{ 0,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } );
	subpass.depthStencilAttachments.push_back( vsg::AttachmentReference{
		1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL } );

	vsg::SubpassDependency colordep{ VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0 };
	vsg::SubpassDependency depthdep{ VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 0 };
	vsg::SubpassDependency readdep{ 0, VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT, 0 };

	return vsg::RenderPass::create( device_,
		vsg::RenderPass::Attachments{ coloratt, depthatt },
		vsg::RenderPass::Subpasses{ subpass },
		vsg::RenderPass::Dependencies{ colordep, depthdep, readdep } );
    }

    void addMessenger()
    {
	PFN_vkCreateDebugUtilsMessengerEXT createfn = nullptr;
	if ( !instance_->getProcAddr(createfn,
				     "vkCreateDebugUtilsMessengerEXT") )
	    return;

	VkDebugUtilsMessengerCreateInfoEXT info{};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	info.pfnUserCallback = &OffscreenViewer::debugCB;
	info.pUserData = this;
	createfn( *instance_, &info, nullptr, &messenger_ );
    }

    uint32_t				width_;
    uint32_t				height_;
    int					queuefamily_ = -1;
    bool				hasvalidation_ = false;
    std::atomic<int>			nrmsgs_{ 0 };
    std::string				errmsg_;
    VkDebugUtilsMessengerEXT		messenger_ = VK_NULL_HANDLE;

    vsg::ref_ptr<vsg::Instance>		instance_;
    vsg::ref_ptr<vsg::Device>		device_;
    vsg::ref_ptr<vsg::Image>		colorimage_;
    vsg::ref_ptr<vsg::Buffer>		readbuffer_;
    vsg::ref_ptr<vsg::Camera>		camera_;
    vsg::ref_ptr<vsg::Group>		scene_;
    vsg::ref_ptr<vsg::RenderGraph>	rendergraph_;
    vsg::ref_ptr<vsg::Switch>		mainswitch_;
    vsg::ref_ptr<vsg::CommandGraph>	commandgraph_;
    vsg::ref_ptr<vsg::Viewer>		viewer_;
};


inline bool isColor( const vsg::ubvec4& pix, int r, int g, int b,
		     int tol=40 )
{
    return std::abs( int(pix.r)-r ) <= tol &&
	   std::abs( int(pix.g)-g ) <= tol &&
	   std::abs( int(pix.b)-b ) <= tol;
}


struct Bounds
{
    int		nrpix_ = 0;
    int		nrred_ = 0;
    int		minx_ = INT_MAX;
    int		maxx_ = -1;
    int		miny_ = INT_MAX;
    int		maxy_ = -1;

    int		cx() const	{ return (minx_+maxx_)/2; }
    int		cy() const	{ return (miny_+maxy_)/2; }
};


/*!Count and extent of the pixels with a channel above mid-grey; nrred_ counts
    the clearly red ones. */
inline Bounds brightBounds( OffscreenViewer& ov )
{
    Bounds ret;
    TypeSet<vsg::ubvec4> pixels;
    if ( !ov.readColor(pixels) )
	return ret;

    for ( uint32_t y=0; y<ov.height(); y++ )
    {
	for ( uint32_t x=0; x<ov.width(); x++ )
	{
	    const vsg::ubvec4 pix = ov.pixel( pixels, x, y );
	    if ( pix.r<=128 && pix.g<=128 && pix.b<=128 )
		continue;

	    ret.nrpix_++;
	    if ( pix.r>150 && pix.g<80 && pix.b<80 )
		ret.nrred_++;

	    if ( int(x) < ret.minx_ )
		ret.minx_ = int(x);
	    if ( int(x) > ret.maxx_ )
		ret.maxx_ = int(x);
	    if ( int(y) < ret.miny_ )
		ret.miny_ = int(y);
	    if ( int(y) > ret.maxy_ )
		ret.maxy_ = int(y);
	}
    }

    return ret;
}

} // namespace vsgTest
