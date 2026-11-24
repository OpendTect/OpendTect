#if defined(_WIN32)
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
#endif

#if defined(WIN32)
# define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
# define VK_USE_PLATFORM_MACOS_MVK
#else
# define VK_USE_PLATFORM_XCB_KHR
#endif

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QFileDialog>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QScreen>
#include <QShowEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>

#include <cstring>

#include <vsg/all.h>

#include <QCoreApplication>
#include <QExposeEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPlatformSurfaceEvent>
#include <QTimer>
#include <QWheelEvent>
#include <QWindow>

#ifdef __unix__
# include <vsgXchange/freetype.h>
#endif
#include "math2.h"
#include "fontdata.h"
#include "uifont.h"
#include "debug.h"
#include "ptrman.h"
#include <QDebug>
#include <QFontDialog>
#include <QMetaObject>
#include <QString>

#ifdef __win__
# include <windows.h>
# include <dwrite.h>
# pragma comment(lib, "dwrite.lib")
#else
# include <fontconfig/fontconfig.h>
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <map>
#include <utility>
#include <string>
#include <thread>
#include <vector>



// main app

namespace VSGWidget_internal
{
struct SetPipelineStates : public vsg::Visitor
{
    void apply( vsg::Object& object ) override
    { object.traverse( *this ); }
    void apply( vsg::InputAssemblyState& ias ) override
    { ias.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; }
    void apply( vsg::RasterizationState& rs ) override
    { rs.lineWidth = 3.0f; }
};


struct MarkerPoint
{
    vsg::vec3	position{0.f, 0.f, 0.f};
    float	magnitude	= 0.f;
    float	size		= 0.35f;
    vsg::vec4	color{1.f, 1.f, 1.f, 1.f};
    bool	on		= true;
};


#ifdef __win__
template <typename T>
struct DirectWriteCOMPtr
{
    T*		ptr_			= nullptr;

		~DirectWriteCOMPtr()	{ if ( ptr_ ) ptr_->Release(); }

    T**		operator&()		{ return &ptr_; }
    T*		get() const		{ return ptr_; }
    T*		operator->() const	{ return ptr_; }
    explicit	operator bool() const	{ return ptr_ != nullptr; }
};


struct GlyphBitmap
{
    uint32_t			charcode	= 0;
    uint16_t			glyphindex	= 0;
    int				width		= 0;
    int				height		= 0;
    int				originx		= 0;
    int				originy		= 0;
    DWRITE_GLYPH_METRICS	metrics{};
    std::vector<int16_t>	sdf;
};
#endif


enum class CoordFrame
{
    Scene = 0,	// rendering frame: X/Y horizontal, Z up
    NED,	// North, East, Down (Aki & Richards)
    ENU		// East, North, Up (same axes as Scene here)
};


struct MomentTensor
{
    float	xx	= 0.f;
    float	yy	= 0.f;
    float	zz	= 0.f;
    float	xy	= 0.f;
    float	xz	= 0.f;
    float	yz	= 0.f;

    float	pWaveAmplitude( const vsg::vec3& direction ) const;

    MomentTensor	toSceneFrame( CoordFrame source ) const;

    static MomentTensor fromComponents( float xx, float yy, float zz,
					 float xy, float xz, float yz );
    static MomentTensor fromStrikeDipRake( float strikeDeg, float dipDeg,
					   float rakeDeg );
    static MomentTensor doubleCouple( const vsg::vec3& slip,
				      const vsg::vec3& normal );
    static MomentTensor strikeSlipXY();
    static MomentTensor normalFaultXZ();
    static MomentTensor reverseFaultXZ();
    static MomentTensor isotropic( float value=1.f );
};


struct SeismicEvent
{
    std::string		id;
    vsg::vec3		position{0.f, 0.f, 0.f};
    float		depth		= 0.f;
    float		magnitude	= 0.f;
    MomentTensor	tensor;
    CoordFrame		frame		= CoordFrame::Scene;
    bool		hasSDR		= false;
    float		strikeDeg	= 0.f;
    float		dipDeg		= 0.f;
    float		rakeDeg		= 0.f;

    MomentTensor	tensorInScene() const;
};


struct BeachBallDisplaySettings
{
    bool		visible			= true;
    float		magMin			= 3.f;
    float		magMax			= 6.f;
    float		diamMin			= 0.15f;
    float		diamMax			= 0.55f;
    float		filterMagMin		= -1.e9f;
    float		filterMagMax		= 1.e9f;
    int			nrLatitudeSegments	= 48;
    int			nrLongitudeSegments	= 96;
    vsg::vec4		compressionColor{0.03f, 0.03f, 0.03f, 1.f};
    vsg::vec4		dilatationColor{0.95f, 0.95f, 0.95f, 1.f};
};


static float diameterFromMagnitude( float mag, float magMin, float magMax,
	float diamMin, float diamMax );


class VsgBeachBall
{
public:
			VsgBeachBall();

    void		setMomentTensor( const MomentTensor& );
    void		setPosition( const vsg::vec3& );
    void		setRadius( float );
    void		setResolution( int nrLatitudeSegments,
				       int nrLongitudeSegments );
    void		setColors( const vsg::vec4& compression,
				   const vsg::vec4& dilatation );

    vsg::ref_ptr<vsg::Node> build(
				vsg::ref_ptr<vsg::Options> options ) const;

private:
    vsg::vec3		direction( int latitude, int longitude ) const;
    void		addTriangle(
				const vsg::vec3&, const vsg::vec3&,
				const vsg::vec3&,
				std::vector<vsg::vec3>& vertices,
				std::vector<vsg::vec3>& normals,
				std::vector<vsg::vec4>& colors ) const;

    MomentTensor	tensor_;
    vsg::vec3		position_{0.f, 0.f, 0.f};
    float		radius_			= 0.5f;
    int			nrLatitudeSegments_	= 48;
    int			nrLongitudeSegments_	= 96;
    vsg::vec4		compressionColor_{0.03f, 0.03f, 0.03f, 1.f};
    vsg::vec4		dilatationColor_{0.95f, 0.95f, 0.95f, 1.f};
};


class VsgBeachBallSet
{
public:
			VsgBeachBallSet();

    void		setDisplaySettings( const BeachBallDisplaySettings& );
    const BeachBallDisplaySettings& displaySettings() const;

    int			addEvent( const SeismicEvent& );
    void		clear();
    int			nrEvents() const;
    SeismicEvent&	event( int idx );
    const SeismicEvent& event( int idx ) const;

    vsg::ref_ptr<vsg::Node> build(
				vsg::ref_ptr<vsg::Options> options ) const;

private:
    BeachBallDisplaySettings	settings_;
    std::vector<SeismicEvent>	events_;
};


class VsgMarkerSet
{
public:
    enum Shape	{ Cube=0, Cone, Cylinder, Sphere, Cross };

		VsgMarkerSet();

    void	setShape( Shape );
    Shape	shape() const;

    void	setDefaultSize( float );
    float	defaultSize() const;

    void	setHeightRatio( float );
    float	heightRatio() const;

    void	setSingleColor( const vsg::vec4& );
    void	useSingleColor( bool );

    int		addPos( const vsg::vec3& pos, float magnitude,
			const vsg::vec4& color );
    void	clear();
    int		nrPoints() const;
    MarkerPoint&	point( int idx );
    const MarkerPoint&	point( int idx ) const;

    void	turnMarkerOn( int idx, bool yn );
    void	applySizesFromMagnitude( float magMin, float magMax,
					 float diamMin, float diamMax );

    vsg::ref_ptr<vsg::Node> build( vsg::ref_ptr<vsg::Options> options,
				   float groundZ,
				   vsg::ref_ptr<vsg::Data> image ) const;

private:
    vsg::ref_ptr<vsg::Node> createShapeNode(
				vsg::Builder& builder,
				const vsg::GeometryInfo& gi,
				const vsg::StateInfo& si ) const;

    Shape			shape_;
    float			defaultSize_;
    float			heightRatio_;
    bool			useSingleColor_;
    vsg::vec4			singleColor_;
    std::vector<MarkerPoint>	points_;
};

} // namespace VSGWidget_internal


class VSGQtViewer : public vsg::Inherit<vsg::Viewer, VSGQtViewer>
{
public:
    explicit VSGQtViewer(int msectimerinterval=0);
    ~VSGQtViewer();

    void request();
    void stop();
    bool pollEvents(bool discardpreviousevents=true) override;

private:
    void render(double simulationtime=vsg::Viewer::UseTimeSinceStartPoint);
    void setInterval(int msectimerinterval);

    QTimer timer_;
    std::atomic_uint requests_ = 0;
    bool continuousupdate_ = true;
};

EVSG_type_name(VSGQtViewer);


VSGQtViewer::VSGQtViewer( int msectimerinterval )
{
    if ( msectimerinterval > 0 )
	setInterval( msectimerinterval );
}


VSGQtViewer::~VSGQtViewer()
{
    stop();
}


bool VSGQtViewer::pollEvents( bool discardpreviousevents )
{
    if ( discardpreviousevents )
	_events.clear();

    for ( auto& window : _windows )
    {
	_events.splice( _events.end(), window->bufferedEvents );
	window->bufferedEvents.clear();
    }

    return !_events.empty();
}


void VSGQtViewer::request()
{
    ++requests_;
}


void VSGQtViewer::stop()
{
    timer_.stop();
}


void VSGQtViewer::render( double simulationtime )
{
    if ( !continuousupdate_ && requests_.load()==0 )
	return;

    if ( advanceToNextFrame(simulationtime) )
    {
	handleEvents();
	update();
	recordAndSubmit();
	present();
    }
    else if ( status->cancel() )
	QCoreApplication::quit();

    requests_ = 0;
}


void VSGQtViewer::setInterval( int msectimerinterval )
{
    timer_.setInterval( msectimerinterval );
    QObject::connect( &timer_, &QTimer::timeout, [this]() { render(); } );
    timer_.start();
}


class VSGQtWindow : public QWindow
{
public:
    VSGQtWindow(vsg::ref_ptr<VSGQtViewer>,
		vsg::ref_ptr<vsg::WindowTraits>,
		QScreen* targetscreen=nullptr);
    ~VSGQtWindow();

    void initializeWindow();
    vsg::ref_ptr<vsg::Window> vsgWindow() const { return vsgwindow_; }

protected:
    bool event(QEvent*) override;
    void exposeEvent(QExposeEvent*) override;
    void hideEvent(QHideEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    void cleanup();
    int convertCoord(double) const;
    std::pair<int,int> mousePosition(const QMouseEvent&) const;
    void postKey(const QKeyEvent&, bool press);

    vsg::ref_ptr<vsg::WindowTraits> traits_;
    vsg::ref_ptr<VSGQtViewer> viewer_;
    vsg::ref_ptr<vsg::Window> vsgwindow_;
    bool initialized_ = false;
};


namespace
{

class QtKeyboardMap
{
public:
	    QtKeyboardMap();

    bool	getKeySymbol(const QKeyEvent&,vsg::KeySymbol& keysymbol,
		    vsg::KeySymbol& modifiedkeysymbol,
		    vsg::KeyModifier& keymodifier) const;

private:

    std::map<uint32_t,vsg::KeySymbol>	keymap_;

};


QtKeyboardMap::QtKeyboardMap()
    : keymap_{
    { Qt::Key_Left, vsg::KEY_Left },
    { Qt::Key_Up, vsg::KEY_Up },
    { Qt::Key_Right, vsg::KEY_Right },
    { Qt::Key_Down, vsg::KEY_Down }
    }
{
}


bool QtKeyboardMap::getKeySymbol( const QKeyEvent& event,
    vsg::KeySymbol& keysymbol,
    vsg::KeySymbol& modifiedkeysymbol,
    vsg::KeyModifier& keymodifier ) const
{
    uint16_t modifiermask = 0;
    if ( event.modifiers() & Qt::ShiftModifier )
	modifiermask |= vsg::KeyModifier::MODKEY_Shift;
    if ( event.modifiers() & Qt::ControlModifier )
	modifiermask |= vsg::KeyModifier::MODKEY_Control;
    if ( event.modifiers() & Qt::AltModifier )
	modifiermask |= vsg::KeyModifier::MODKEY_Alt;
    if ( event.modifiers() & Qt::MetaModifier )
	modifiermask |= vsg::KeyModifier::MODKEY_Meta;

    keymodifier = static_cast<vsg::KeyModifier>( modifiermask );

    const auto it = keymap_.find( static_cast<uint32_t>(event.key()) );
    if ( it != keymap_.end() )
    {
	keysymbol = it->second;
	modifiedkeysymbol = keysymbol;
	return true;
    }

    keysymbol = static_cast<vsg::KeySymbol>( event.key() );
    if ( keysymbol>='A' && keysymbol<='Z' )
	keysymbol = static_cast<vsg::KeySymbol>(
	int(keysymbol) + int('a'-'A') );

    modifiedkeysymbol = static_cast<vsg::KeySymbol>(
	*(event.text().toLatin1().data()) );
    return true;
}


QtKeyboardMap& keyboardMap()
{
    static QtKeyboardMap map;
    return map;
}


std::pair<vsg::ButtonMask,uint32_t> mouseButtons( const QMouseEvent& event )
{
    uint16_t mask = 0;
    uint32_t button = 0;
    if ( event.buttons() & Qt::LeftButton )
	mask = mask | vsg::BUTTON_MASK_1;
    if ( event.buttons() & Qt::MiddleButton )
	mask = mask | vsg::BUTTON_MASK_2;
    if ( event.buttons() & Qt::RightButton )
	mask = mask | vsg::BUTTON_MASK_3;

    switch ( event.button() )
    {
	case Qt::LeftButton:
	    button = 1;
	    break;
	case Qt::MiddleButton:
	    button = 2;
	    break;
	case Qt::RightButton:
	    button = 3;
	    break;
	default:
	    break;
    }

    return { static_cast<vsg::ButtonMask>(mask), button };
}

} // namespace


VSGQtWindow::VSGQtWindow( vsg::ref_ptr<VSGQtViewer> viewer,
    vsg::ref_ptr<vsg::WindowTraits> traits,
    QScreen* targetscreen )
    : QWindow(targetscreen)
    , viewer_(viewer)
{
    if ( traits )
    {
	traits_ = vsg::WindowTraits::create( *traits );
	setGeometry( traits_->x, traits_->y, traits_->width, traits_->height );
    }
    else
    {
	traits_ = vsg::WindowTraits::create();
	traits_->x = x();
	traits_->y = y();
	traits_->width = width();
	traits_->height = height();
    }
}


VSGQtWindow::~VSGQtWindow()
{
    cleanup();
}


int VSGQtWindow::convertCoord( double coord ) const
{
    return static_cast<int>( std::round(static_cast<qreal>(coord) *
	devicePixelRatio()) );
}


void VSGQtWindow::initializeWindow()
{
    if ( vsgwindow_ )
	return;

    if ( !traits_ )
	traits_ = vsg::WindowTraits::create();

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    traits_->nativeWindow = reinterpret_cast<HWND>( winId() );
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    traits_->nativeWindow = static_cast<::Window>( winId() );
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    traits_->nativeWindow = static_cast<xcb_window_t>( winId() );
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    traits_->nativeWindow = winId();
#endif

    traits_->x = convertCoord( x() );
    traits_->y = convertCoord( y() );
    traits_->width = convertCoord( width() );
    traits_->height = convertCoord( height() );
    vsgwindow_ = vsg::Window::create( traits_ );
    initialized_ = true;
}


void VSGQtWindow::cleanup()
{
    if ( vsgwindow_ )
    {
	if ( viewer_ )
	{
	    viewer_->deviceWaitIdle();
	    viewer_->removeWindow( vsgwindow_ );
	}

	vsgwindow_->releaseWindow();
    }

    vsgwindow_ = {};
    viewer_ = {};
}


bool VSGQtWindow::event( QEvent* event )
{
    if ( event->type() == QEvent::PlatformSurface )
    {
	auto* surfaceevent = dynamic_cast<QPlatformSurfaceEvent*>( event );
	if ( surfaceevent && vsgwindow_ &&
	    surfaceevent->surfaceEventType() ==
	    QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed )
	{
	    const vsg::clock::time_point eventtime = vsg::clock::now();
	    vsgwindow_->bufferedEvents.push_back(
		vsg::CloseWindowEvent::create(vsgwindow_, eventtime) );
	    cleanup();
	}
    }

    return QWindow::event( event );
}


void VSGQtWindow::exposeEvent( QExposeEvent* )
{
    if ( !initialized_ && isExposed() )
	initializeWindow();

    if ( viewer_ )
	viewer_->request();
}


void VSGQtWindow::hideEvent( QHideEvent* )
{
}


void VSGQtWindow::resizeEvent( QResizeEvent* )
{
    if ( !vsgwindow_ )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    vsgwindow_->bufferedEvents.push_back( vsg::ConfigureWindowEvent::create(
	vsgwindow_, eventtime, convertCoord(x()), convertCoord(y()),
	convertCoord(width()), convertCoord(height())) );
    vsgwindow_->resize();
    if ( viewer_ )
	viewer_->request();
}


void VSGQtWindow::postKey( const QKeyEvent& event, bool press )
{
    if ( !vsgwindow_ )
	return;

    vsg::KeySymbol keysymbol;
    vsg::KeySymbol modifiedkeysymbol;
    vsg::KeyModifier keymodifier;
    if ( !keyboardMap().getKeySymbol(event, keysymbol, modifiedkeysymbol,
	keymodifier) )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    if ( press )
	vsgwindow_->bufferedEvents.push_back( vsg::KeyPressEvent::create(
	vsgwindow_, eventtime, keysymbol, modifiedkeysymbol,
	keymodifier) );
    else
	vsgwindow_->bufferedEvents.push_back( vsg::KeyReleaseEvent::create(
	vsgwindow_, eventtime, keysymbol, modifiedkeysymbol,
	keymodifier) );

    if ( viewer_ )
	viewer_->request();
}


void VSGQtWindow::keyPressEvent( QKeyEvent* event )
{
    if ( event )
	postKey( *event, true );
}


void VSGQtWindow::keyReleaseEvent( QKeyEvent* event )
{
    if ( event )
	postKey( *event, false );
}


std::pair<int,int> VSGQtWindow::mousePosition( const QMouseEvent& event ) const
{
    return { convertCoord(event.position().x()),
	convertCoord(event.position().y()) };
}


void VSGQtWindow::mouseMoveEvent( QMouseEvent* event )
{
    if ( !vsgwindow_ || !event )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    const auto mask = mouseButtons( *event ).first;
    const auto [xpos, ypos] = mousePosition( *event );
    vsgwindow_->bufferedEvents.push_back( vsg::MoveEvent::create(
	vsgwindow_, eventtime, xpos, ypos, mask) );
    if ( viewer_ )
	viewer_->request();
}


void VSGQtWindow::mousePressEvent( QMouseEvent* event )
{
    if ( !vsgwindow_ || !event )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    const auto [mask, button] = mouseButtons( *event );
    const auto [xpos, ypos] = mousePosition( *event );
    vsgwindow_->bufferedEvents.push_back( vsg::ButtonPressEvent::create(
	vsgwindow_, eventtime, xpos, ypos, mask, button) );
    if ( viewer_ )
	viewer_->request();
}


void VSGQtWindow::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !vsgwindow_ || !event )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    const auto [mask, button] = mouseButtons( *event );
    const auto [xpos, ypos] = mousePosition( *event );
    vsgwindow_->bufferedEvents.push_back( vsg::ButtonReleaseEvent::create(
	vsgwindow_, eventtime, xpos, ypos, mask, button) );
    if ( viewer_ )
	viewer_->request();
}


void VSGQtWindow::wheelEvent( QWheelEvent* event )
{
    if ( !vsgwindow_ || !event )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    const vsg::vec3 delta = event->angleDelta().y() < 0
	? vsg::vec3( 0.f, -1.f, 0.f ) : vsg::vec3( 0.f, 1.f, 0.f );
    vsgwindow_->bufferedEvents.push_back( vsg::ScrollWheelEvent::create(
	vsgwindow_, eventtime, delta) );
    if ( viewer_ )
	viewer_->request();
}

class VSGWidget;

class VSGWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit	VSGWindow(QWidget* parent=nullptr);
		~VSGWindow();

    void	setVSGWidget(VSGWidget*);

private:
    QSize	currentScreenDPI() const;

    VSGWidget*	vsgwidget_	= nullptr;

private slots:
    void	onOpen();
    void	onSave();
    void	onTakeScreenshot();
    void	onFont();
    void	onMarkerShapeChanged(int);
    void	onExit();
};


class VSGWidget : public QWidget {
    Q_OBJECT

public:
    enum MarkerShape { Cube=0, Cone, Cylinder, Sphere, Cross };

    explicit	VSGWidget(QWidget* parent=nullptr);
		~VSGWidget();

    bool	loadScene( const char* srcpath, std::string& msg );
    bool	saveScene( const char* destpath, std::string& msg ) const;
    bool	doScreenShot( const QSize& dpi, double scalefactor,
			   const char* destpath, std::string& msg ) const;
    void	setFontData(const FontData& fontdata);
    const FontData&	fontData() const { return fontdata_; }
    void	setMarkerShape(MarkerShape);
    MarkerShape markerShape() const { return markershape_; }
    VSGWidget_internal::BeachBallDisplaySettings beachBallSettings() const;
    void	setBeachBallSettings(
		    const VSGWidget_internal::BeachBallDisplaySettings& );

protected:
    void	showEvent(QShowEvent*) override;
    void	resizeEvent(QResizeEvent*) override;

private:
    void	initializeVSG();
    void	requestFont(const FontData&);
    void	applyFont(vsg::ref_ptr<vsg::Font>,const FontData&);

    vsg::ref_ptr<vsg::Data>	createCheckerImage() const;
    vsg::ref_ptr<vsg::Node>	addCube();
    vsg::ref_ptr<vsg::Node>	addReferenceBeachBalls();
    vsg::ref_ptr<vsg::Node>	addMarkers();
    vsg::ref_ptr<vsg::Node>	addGround();
    vsg::ref_ptr<vsg::Node>	addSceneLighting();
    vsg::ref_ptr<vsg::Node>	addLine();
    vsg::ref_ptr<vsg::Node>	addLabel(float cubesize);

    void			setupVsgOptions();

    VSGQtWindow*			qtwindow_	= nullptr;
    QWidget*				container_	= nullptr;
    vsg::ref_ptr<VSGQtViewer>		viewer_;
    vsg::ref_ptr<vsg::Window>		vsgwindow_;
    vsg::ref_ptr<vsg::Camera>		camera_;
    vsg::ref_ptr<vsg::Trackball>	trackball_;
    vsg::ref_ptr<vsg::Group>		root_;
    vsg::ref_ptr<vsg::Group>		markersroot_;
    vsg::ref_ptr<vsg::Group>		beachballsroot_;
    VSGWidget_internal::VsgBeachBallSet beachballset_;
    vsg::ref_ptr<vsg::Options>		vsgoptions_;
    vsg::ref_ptr<vsg::Text>		labeltext_;
    vsg::dvec3				cubecenter_{0.0, 0.0, 0.0};
    vsg::vec3				labelanchor_{0.f, 0.f, 0.f};
    const int				rendertimems_	= 16;

    FontData				fontdata_;
    MarkerShape				markershape_ = Sphere;
    bool				fontinit_ = false;
    unsigned long long			fontrequestid_ = 0;
    std::thread				fontthread_;
};


#include "od_vsgtestapp.moc"


namespace VSGWidget_internal {

float MomentTensor::pWaveAmplitude( const vsg::vec3& n ) const
{
    return xx * n.x * n.x
	 + yy * n.y * n.y
	 + zz * n.z * n.z
	 + 2.f * ( xy * n.x * n.y
		 + xz * n.x * n.z
		 + yz * n.y * n.z );
}


MomentTensor MomentTensor::fromComponents( float mxx, float myy, float mzz,
					  float mxy, float mxz, float myz )
{
    MomentTensor tensor;
    tensor.xx = mxx;
    tensor.yy = myy;
    tensor.zz = mzz;
    tensor.xy = mxy;
    tensor.xz = mxz;
    tensor.yz = myz;
    return tensor;
}


MomentTensor MomentTensor::fromStrikeDipRake( float strikeDeg, float dipDeg,
					     float rakeDeg )
{
    // Aki & Richards convention in NED (North, East, Down).
    const float pi = std::acos( -1.f );
    const float deg2rad = pi / 180.f;
    const float phi = strikeDeg * deg2rad;
    const float delta = dipDeg * deg2rad;
    const float lambda = rakeDeg * deg2rad;

    const float sind = std::sin( delta );
    const float cosd = std::cos( delta );
    const float sinp = std::sin( phi );
    const float cosp = std::cos( phi );
    const float sinl = std::sin( lambda );
    const float cosl = std::cos( lambda );

    const vsg::vec3 normal( -sind * sinp,
			     sind * cosp,
			    -cosd );
    const vsg::vec3 slip( cosl * cosp + sinl * cosd * sinp,
			  cosl * sinp - sinl * cosd * cosp,
			 -sinl * sind );

    return doubleCouple( slip, normal );
}


MomentTensor MomentTensor::toSceneFrame( CoordFrame source ) const
{
    if ( source == CoordFrame::Scene || source == CoordFrame::ENU )
	return *this;

    // NED (N,E,D) -> Scene (X=E, Y=N, Z=Up=-D)
    // v_s = R v_n with R = [[0,1,0],[1,0,0],[0,0,-1]]
    // M_s = R M_n R^T
    const float m[3][3] = {
	{ xx, xy, xz },
	{ xy, yy, yz },
	{ xz, yz, zz }
    };
    const float R[3][3] = {
	{ 0.f, 1.f,  0.f },
	{ 1.f, 0.f,  0.f },
	{ 0.f, 0.f, -1.f }
    };

    float rm[3][3] = {};
    for ( int i=0; i<3; i++ )
	for ( int j=0; j<3; j++ )
	    for ( int k=0; k<3; k++ )
		rm[i][j] += R[i][k] * m[k][j];

    float out[3][3] = {};
    for ( int i=0; i<3; i++ )
	for ( int j=0; j<3; j++ )
	    for ( int k=0; k<3; k++ )
		out[i][j] += rm[i][k] * R[j][k]; // R^T[k][j] = R[j][k]

    return fromComponents( out[0][0], out[1][1], out[2][2],
			   out[0][1], out[0][2], out[1][2] );
}


MomentTensor SeismicEvent::tensorInScene() const
{
    if ( hasSDR )
	return MomentTensor::fromStrikeDipRake(
		strikeDeg, dipDeg, rakeDeg ).toSceneFrame( CoordFrame::NED );

    return tensor.toSceneFrame( frame );
}


MomentTensor MomentTensor::doubleCouple( const vsg::vec3& slip,
					const vsg::vec3& normal )
{
    MomentTensor tensor;
    tensor.xx = 2.f * slip.x * normal.x;
    tensor.yy = 2.f * slip.y * normal.y;
    tensor.zz = 2.f * slip.z * normal.z;
    tensor.xy = slip.x * normal.y + normal.x * slip.y;
    tensor.xz = slip.x * normal.z + normal.x * slip.z;
    tensor.yz = slip.y * normal.z + normal.y * slip.z;
    return tensor;
}


MomentTensor MomentTensor::strikeSlipXY()
{
    return doubleCouple( vsg::vec3(0.f, 1.f, 0.f),
			 vsg::vec3(1.f, 0.f, 0.f) );
}


MomentTensor MomentTensor::normalFaultXZ()
{
    const float sqrtHalf = Math::Sqrt( 0.5f );
    const vsg::vec3 normal( sqrtHalf, 0.f, sqrtHalf );
    const vsg::vec3 downDipSlip( sqrtHalf, 0.f, -sqrtHalf );
    return doubleCouple( downDipSlip, normal );
}


MomentTensor MomentTensor::reverseFaultXZ()
{
    const float sqrtHalf = Math::Sqrt( 0.5f );
    const vsg::vec3 normal( sqrtHalf, 0.f, sqrtHalf );
    const vsg::vec3 upDipSlip( -sqrtHalf, 0.f, sqrtHalf );
    return doubleCouple( upDipSlip, normal );
}


MomentTensor MomentTensor::isotropic( float value )
{
    return fromComponents( value, value, value, 0.f, 0.f, 0.f );
}


VsgBeachBall::VsgBeachBall()
{}


void VsgBeachBall::setMomentTensor( const MomentTensor& tensor )
{
    tensor_ = tensor;
}


void VsgBeachBall::setPosition( const vsg::vec3& position )
{
    position_ = position;
}


void VsgBeachBall::setRadius( float radius )
{
    radius_ = radius > 0.f ? radius : 0.5f;
}


void VsgBeachBall::setResolution( int nrLatitudeSegments,
				  int nrLongitudeSegments )
{
    nrLatitudeSegments_ = std::max( 2, nrLatitudeSegments );
    nrLongitudeSegments_ = std::max( 3, nrLongitudeSegments );
}


void VsgBeachBall::setColors( const vsg::vec4& compression,
			      const vsg::vec4& dilatation )
{
    compressionColor_ = compression;
    dilatationColor_ = dilatation;
}


vsg::vec3 VsgBeachBall::direction( int latitude, int longitude ) const
{
    const float pi = std::acos( -1.f );
    const float theta = pi * latitude / nrLatitudeSegments_;
    const float phi = 2.f * pi * longitude / nrLongitudeSegments_;
    const float sintheta = std::sin( theta );
    return vsg::vec3( sintheta * std::cos(phi),
		      sintheta * std::sin(phi),
		      std::cos(theta) );
}


void VsgBeachBall::addTriangle(
	const vsg::vec3& n0, const vsg::vec3& n1, const vsg::vec3& n2,
	std::vector<vsg::vec3>& vertices, std::vector<vsg::vec3>& normals,
	std::vector<vsg::vec4>& colors ) const
{
    const vsg::vec3 sampleDirection = vsg::normalize( n0 + n1 + n2 );
    const bool compression =
			tensor_.pWaveAmplitude( sampleDirection ) >= 0.f;
    const vsg::vec4 color =
			compression ? compressionColor_ : dilatationColor_;

    const vsg::vec3 triangleNormals[] = { n0, n1, n2 };
    for ( const vsg::vec3& normal : triangleNormals )
    {
	vertices.push_back( position_ + radius_ * normal );
	normals.push_back( normal );
	colors.push_back( color );
    }
}


vsg::ref_ptr<vsg::Node> VsgBeachBall::build(
	vsg::ref_ptr<vsg::Options> options ) const
{
    if ( !options )
	return {};

    std::vector<vsg::vec3> vertexData;
    std::vector<vsg::vec3> normalData;
    std::vector<vsg::vec4> colorData;

    const size_t nrTriangles =
	2u * (nrLatitudeSegments_ - 1u) * nrLongitudeSegments_;
    vertexData.reserve( 3u * nrTriangles );
    normalData.reserve( 3u * nrTriangles );
    colorData.reserve( 3u * nrTriangles );

    for ( int latitude=0; latitude<nrLatitudeSegments_; latitude++ )
    {
	for ( int longitude=0; longitude<nrLongitudeSegments_; longitude++ )
	{
	    const int nextLongitude =
			(longitude + 1) % nrLongitudeSegments_;
	    const vsg::vec3 n00 = direction( latitude, longitude );
	    const vsg::vec3 n10 = direction( latitude + 1, longitude );
	    const vsg::vec3 n11 =
			direction( latitude + 1, nextLongitude );
	    const vsg::vec3 n01 = direction( latitude, nextLongitude );

	    if ( latitude + 1 < nrLatitudeSegments_ )
		addTriangle( n00, n10, n11,
			     vertexData, normalData, colorData );
	    if ( latitude > 0 )
		addTriangle( n00, n11, n01,
			     vertexData, normalData, colorData );
	}
    }

    // The default vsg allocator places new[] in a fixed block. A beach-ball
    // mesh is larger than that block, so _data stays null and data(i) faults.
    vsg::Data::Properties props;
    props.allocatorType = vsg::ALLOCATOR_TYPE_NEW_DELETE;

    vsg::ref_ptr<vsg::vec3Array> vertices = vsg::vec3Array::create(
	    static_cast<uint32_t>(vertexData.size()), props );
    vsg::ref_ptr<vsg::vec3Array> normals = vsg::vec3Array::create(
	    static_cast<uint32_t>(normalData.size()), props );
    vsg::ref_ptr<vsg::vec4Array> colors = vsg::vec4Array::create(
	    static_cast<uint32_t>(colorData.size()), props );
    if ( !vertices || !normals || !colors
      || !vertices->data() || !normals->data() || !colors->data() )
	return {};

    std::copy( vertexData.begin(), vertexData.end(), vertices->begin() );
    std::copy( normalData.begin(), normalData.end(), normals->begin() );
    std::copy( colorData.begin(), colorData.end(), colors->begin() );

    vsg::ref_ptr<vsg::ShaderSet> shaderSet = options->shaderSets["phong"];
    if ( !shaderSet )
	shaderSet = vsg::createPhongShaderSet( options );

    vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> pipelineConfig =
			vsg::GraphicsPipelineConfigurator::create( shaderSet );
    vsg::DataList vertexArrays;
    pipelineConfig->assignArray( vertexArrays, "vsg_Vertex",
				 VK_VERTEX_INPUT_RATE_VERTEX, vertices );
    pipelineConfig->assignArray( vertexArrays, "vsg_Normal",
				 VK_VERTEX_INPUT_RATE_VERTEX, normals );
    pipelineConfig->assignArray( vertexArrays, "vsg_Color",
				 VK_VERTEX_INPUT_RATE_VERTEX, colors );

    vsg::ref_ptr<vsg::PhongMaterialValue> material =
				vsg::PhongMaterialValue::create();
    material->value().ambient.set( 1.f, 1.f, 1.f, 1.f );
    material->value().diffuse.set( 1.f, 1.f, 1.f, 1.f );
    material->value().specular.set( 0.1f, 0.1f, 0.1f, 1.f );
    material->value().shininess = 8.f;
    pipelineConfig->assignDescriptor( "material", material );
    pipelineConfig->init();

    vsg::ref_ptr<vsg::VertexDraw> draw = vsg::VertexDraw::create();
    draw->assignArrays( vertexArrays );
    draw->vertexCount = static_cast<uint32_t>( vertices->size() );
    draw->instanceCount = 1;

    vsg::ref_ptr<vsg::StateGroup> stateGroup = vsg::StateGroup::create();
    pipelineConfig->copyTo( stateGroup );
    stateGroup->addChild( draw );
    return stateGroup;
}


VsgBeachBallSet::VsgBeachBallSet()
{}


void VsgBeachBallSet::setDisplaySettings( const BeachBallDisplaySettings& s )
{
    settings_ = s;
}


const BeachBallDisplaySettings& VsgBeachBallSet::displaySettings() const
{
    return settings_;
}


int VsgBeachBallSet::addEvent( const SeismicEvent& event )
{
    events_.push_back( event );
    return static_cast<int>( events_.size() ) - 1;
}


void VsgBeachBallSet::clear()
{
    events_.clear();
}


int VsgBeachBallSet::nrEvents() const
{
    return static_cast<int>( events_.size() );
}


SeismicEvent& VsgBeachBallSet::event( int idx )
{
    return events_[idx];
}


const SeismicEvent& VsgBeachBallSet::event( int idx ) const
{
    return events_[idx];
}


vsg::ref_ptr<vsg::Node> VsgBeachBallSet::build(
	vsg::ref_ptr<vsg::Options> options ) const
{
    vsg::ref_ptr<vsg::Group> group = vsg::Group::create();
    if ( !settings_.visible || !options )
	return group;

    for ( const SeismicEvent& evt : events_ )
    {
	if ( evt.magnitude < settings_.filterMagMin
	  || evt.magnitude > settings_.filterMagMax )
	    continue;

	const float diameter = diameterFromMagnitude(
		evt.magnitude, settings_.magMin, settings_.magMax,
		settings_.diamMin, settings_.diamMax );

	VsgBeachBall beachBall;
	beachBall.setMomentTensor( evt.tensorInScene() );
	beachBall.setPosition( evt.position );
	beachBall.setRadius( diameter * 0.5f );
	beachBall.setResolution( settings_.nrLatitudeSegments,
				 settings_.nrLongitudeSegments );
	beachBall.setColors( settings_.compressionColor,
			     settings_.dilatationColor );

	if ( vsg::ref_ptr<vsg::Node> node = beachBall.build( options ) )
	    group->addChild( node );
    }

    return group;
}


VsgMarkerSet::VsgMarkerSet()
    : shape_( Sphere )
    , defaultSize_( 0.35f )
    , heightRatio_( 1.f )
    , useSingleColor_( false )
    , singleColor_( 1.f, 1.f, 1.f, 1.f )
{
}


static float diameterFromMagnitude( float mag, float magMin, float magMax,
				    float diamMin, float diamMax )
{
    if ( magMax <= magMin )
	return diamMin;

    float t = ( mag - magMin ) / ( magMax - magMin );
    if ( t < 0.f ) t = 0.f;
    if ( t > 1.f ) t = 1.f;
    return diamMin + t * ( diamMax - diamMin );
}


void VsgMarkerSet::setShape( Shape s )
{
    shape_ = s;
}


VsgMarkerSet::Shape VsgMarkerSet::shape() const
{
    return shape_;
}


void VsgMarkerSet::setDefaultSize( float sz )
{
    defaultSize_ = sz;
}


float VsgMarkerSet::defaultSize() const
{
    return defaultSize_;
}


void VsgMarkerSet::setHeightRatio( float ratio )
{
    heightRatio_ = ratio > 0.f ? ratio : 1.f;
}


float VsgMarkerSet::heightRatio() const
{
    return heightRatio_;
}


void VsgMarkerSet::setSingleColor( const vsg::vec4& c )
{
    singleColor_ = c;
    useSingleColor_ = true;
}


void VsgMarkerSet::useSingleColor( bool yn )
{
    useSingleColor_ = yn;
}


int VsgMarkerSet::addPos( const vsg::vec3& pos, float magnitude,
			  const vsg::vec4& color )
{
    MarkerPoint pt;
    pt.position = pos;
    pt.magnitude = magnitude;
    pt.size = defaultSize_;
    pt.color = color;
    pt.on = true;
    points_.push_back( pt );
    return static_cast<int>( points_.size() ) - 1;
}


void VsgMarkerSet::clear()
{
    points_.clear();
}


int VsgMarkerSet::nrPoints() const
{
    return static_cast<int>( points_.size() );
}


MarkerPoint& VsgMarkerSet::point( int idx )
{
    return points_[idx];
}


const MarkerPoint& VsgMarkerSet::point( int idx ) const
{
    return points_[idx];
}


void VsgMarkerSet::turnMarkerOn( int idx, bool yn )
{
    if ( idx >= 0 && idx < nrPoints() )
	points_[idx].on = yn;
}


void VsgMarkerSet::applySizesFromMagnitude( float magMin, float magMax,
					    float diamMin, float diamMax )
{
    for ( MarkerPoint& pt : points_ )
    {
	pt.size = diameterFromMagnitude( pt.magnitude, magMin, magMax,
					 diamMin, diamMax );
    }
}


vsg::ref_ptr<vsg::Node> VsgMarkerSet::createShapeNode(
	vsg::Builder& builder, const vsg::GeometryInfo& gi,
	const vsg::StateInfo& si ) const
{
    switch ( shape_ )
    {
	case Cube:
	    return builder.createBox( gi, si );
	case Cone:
	    return builder.createCone( gi, si );
	case Cylinder:
	    return builder.createCylinder( gi, si );
	case Sphere:
	    return builder.createSphere( gi, si );
	case Cross:
	{
	    vsg::ref_ptr<vsg::Group> cross = vsg::Group::create();
	    const float len = vsg::length( gi.dx );
	    const float thick = len * 0.15f;
	    vsg::GeometryInfo arm = gi;

	    arm.dx.set( len, 0.f, 0.f );
	    arm.dy.set( 0.f, thick, 0.f );
	    arm.dz.set( 0.f, 0.f, thick );
	    cross->addChild( builder.createBox( arm, si ) );

	    arm.dx.set( thick, 0.f, 0.f );
	    arm.dy.set( 0.f, len, 0.f );
	    arm.dz.set( 0.f, 0.f, thick );
	    cross->addChild( builder.createBox( arm, si ) );

	    arm.dx.set( thick, 0.f, 0.f );
	    arm.dy.set( 0.f, thick, 0.f );
	    arm.dz.set( 0.f, 0.f, len );
	    cross->addChild( builder.createBox( arm, si ) );
	    return cross;
	}
    }

    return builder.createSphere( gi, si );
}


vsg::ref_ptr<vsg::Node> VsgMarkerSet::build(
	vsg::ref_ptr<vsg::Options> options, float groundZ,
	vsg::ref_ptr<vsg::Data> image ) const
{
    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    builder->options = options;
    vsg::ref_ptr<vsg::Group> group = vsg::Group::create();

    vsg::StateInfo si;
    si.lighting = true;
    if ( image )
	si.image = image;

    for ( const MarkerPoint& pt : points_ )
    {
	if ( !pt.on )
	    continue;

	const float size = pt.size;
	const float height = size * heightRatio_;
	vsg::GeometryInfo gi;
	gi.color = useSingleColor_ ? singleColor_ : pt.color;

	if ( shape_ == Cone || shape_ == Cylinder )
	{
	    gi.position.set( pt.position.x, pt.position.y,
			     groundZ + 0.5f * height );
	    gi.dx.set( size, 0.f, 0.f );
	    gi.dy.set( 0.f, size, 0.f );
	    gi.dz.set( 0.f, 0.f, height );
	}
	else
	{
	    gi.position.set( pt.position.x, pt.position.y,
			     groundZ + 0.5f * size );
	    gi.dx.set( size, 0.f, 0.f );
	    gi.dy.set( 0.f, size, 0.f );
	    gi.dz.set( 0.f, 0.f, size );
	}

	if ( auto node = createShapeNode( *builder, gi, si ) )
	    group->addChild( node );
    }

    return group;
}

} // namespace VSGWidget_internal


VSGWindow::VSGWindow( QWidget* parent )
    : QMainWindow(parent)
{
    auto* menubar = new QMenuBar;
    QMenu* filemenu = menubar->addMenu("&File");

    QAction* openact = filemenu->addAction("Open...");
    QAction* saveact = filemenu->addAction("Save...");
    QAction* screenshotact = filemenu->addAction("Screenshot...");
    QAction* fontact = filemenu->addAction("Font...");
    filemenu->addSeparator();
    QAction* exitact = filemenu->addAction("Exit");

    connect( openact, &QAction::triggered, this, &VSGWindow::onOpen );
    connect( saveact, &QAction::triggered, this, &VSGWindow::onSave );
    connect( screenshotact, &QAction::triggered, this,
	     &VSGWindow::onTakeScreenshot );
    connect( fontact, &QAction::triggered, this, &VSGWindow::onFont );
    connect( exitact, &QAction::triggered, this, &VSGWindow::onExit );

    QMenu* markermenu = menubar->addMenu( "&Markers" );
    QMenu* shapemenu = markermenu->addMenu( "Shape" );
    auto* shapegroup = new QActionGroup( this );
    shapegroup->setExclusive( true );

    auto addShapeAction = [shapemenu,shapegroup](
			    const char* text, VSGWidget::MarkerShape shape,
			    bool checked=false )
    {
	QAction* action = shapemenu->addAction( text );
	action->setCheckable( true );
	action->setChecked( checked );
	action->setData( static_cast<int>(shape) );
	shapegroup->addAction( action );
    };

    addShapeAction( "Cube", VSGWidget::Cube );
    addShapeAction( "Cone", VSGWidget::Cone );
    addShapeAction( "Cylinder", VSGWidget::Cylinder );
    addShapeAction( "Sphere", VSGWidget::Sphere, true );
    addShapeAction( "Cross", VSGWidget::Cross );

    connect( shapegroup, &QActionGroup::triggered, this,
	     [this]( QAction* action )
	     {
		 onMarkerShapeChanged( action->data().toInt() );
	     } );

    QMenu* beachballmenu = menubar->addMenu( "&Beach balls" );
    QAction* showact = beachballmenu->addAction( "Show" );
    showact->setCheckable( true );
    showact->setChecked( true );
    connect( showact, &QAction::toggled, this, [this]( bool on )
    {
	if ( !vsgwidget_ )
	    return;
	auto settings = vsgwidget_->beachBallSettings();
	settings.visible = on;
	vsgwidget_->setBeachBallSettings( settings );
    } );

    auto askRange = [this]( const char* title, const char* minLabel,
			    const char* maxLabel, double valueMin,
			    double valueMax, double lo, double hi,
			    int decimals, float& outMin, float& outMax )
    {
	bool ok = false;
	const double a = QInputDialog::getDouble(
		this, title, minLabel, valueMin, lo, hi, decimals, &ok );
	if ( !ok )
	    return false;
	const double b = QInputDialog::getDouble(
		this, title, maxLabel, valueMax, lo, hi, decimals, &ok );
	if ( !ok )
	    return false;
	outMin = static_cast<float>( a );
	outMax = static_cast<float>( b );
	if ( outMin > outMax )
	    std::swap( outMin, outMax );
	return true;
    };

    connect( beachballmenu->addAction("Diameter..."), &QAction::triggered,
	     this, [this, askRange]()
    {
	if ( !vsgwidget_ )
	    return;
	auto settings = vsgwidget_->beachBallSettings();
	if ( !askRange("Beach balls", "Diameter min", "Diameter max",
		       settings.diamMin, settings.diamMax,
		       0.01, 5.0, 2, settings.diamMin, settings.diamMax) )
	    return;
	vsgwidget_->setBeachBallSettings( settings );
    } );

    connect( beachballmenu->addAction("Resolution..."), &QAction::triggered,
	     this, [this]()
    {
	if ( !vsgwidget_ )
	    return;
	auto settings = vsgwidget_->beachBallSettings();
	bool ok = false;
	const int nlat = QInputDialog::getInt(
		this, "Beach balls", "Latitude segments",
		settings.nrLatitudeSegments, 2, 128, 1, &ok );
	if ( !ok )
	    return;
	const int nlon = QInputDialog::getInt(
		this, "Beach balls", "Longitude segments",
		settings.nrLongitudeSegments, 3, 256, 1, &ok );
	if ( !ok )
	    return;
	settings.nrLatitudeSegments = nlat;
	settings.nrLongitudeSegments = nlon;
	vsgwidget_->setBeachBallSettings( settings );
    } );

    auto askColor = [this]( const char* title, vsg::vec4& color )
    {
	const QColor current = QColor::fromRgbF(
		color.r, color.g, color.b, color.a );
	const QColor chosen = QColorDialog::getColor(
		current, this, title );
	if ( !chosen.isValid() )
	    return false;
	color = vsg::vec4( static_cast<float>(chosen.redF()),
			   static_cast<float>(chosen.greenF()),
			   static_cast<float>(chosen.blueF()),
			   static_cast<float>(chosen.alphaF()) );
	return true;
    };

    connect( beachballmenu->addAction("Compression color..."),
	     &QAction::triggered, this, [this, askColor]()
    {
	if ( !vsgwidget_ )
	    return;
	auto settings = vsgwidget_->beachBallSettings();
	if ( !askColor("Compression color", settings.compressionColor) )
	    return;
	vsgwidget_->setBeachBallSettings( settings );
    } );

    connect( beachballmenu->addAction("Dilatation color..."),
	     &QAction::triggered, this, [this, askColor]()
    {
	if ( !vsgwidget_ )
	    return;
	auto settings = vsgwidget_->beachBallSettings();
	if ( !askColor("Dilatation color", settings.dilatationColor) )
	    return;
	vsgwidget_->setBeachBallSettings( settings );
    } );

    connect( beachballmenu->addAction("Magnitude filter..."),
	     &QAction::triggered, this, [this, askRange]()
    {
	if ( !vsgwidget_ )
	    return;
	auto settings = vsgwidget_->beachBallSettings();
	const double shownMin = std::clamp(
		static_cast<double>(settings.filterMagMin), -2.0, 12.0 );
	const double shownMax = std::clamp(
		static_cast<double>(settings.filterMagMax), -2.0, 12.0 );
	if ( !askRange("Beach balls", "Magnitude min", "Magnitude max",
		       shownMin, shownMax, -2.0, 12.0, 1,
		       settings.filterMagMin, settings.filterMagMax) )
	    return;
	vsgwidget_->setBeachBallSettings( settings );
    } );

    setMenuBar( menubar );
}


VSGWindow::~VSGWindow()
{}


void VSGWindow::setVSGWidget( VSGWidget* vsgwidget )
{
    vsgwidget_ = vsgwidget;
}


QSize VSGWindow::currentScreenDPI() const
{
    QScreen* targetScreen = nullptr;

    if ( windowHandle() )
	targetScreen = windowHandle()->screen();
    if ( !targetScreen )
	targetScreen = screen();
    if ( !targetScreen )
	targetScreen = QGuiApplication::primaryScreen();

    if ( !targetScreen )
	return QSize( 94, 94 );

    return QSize( targetScreen->physicalDotsPerInchX(),
		  targetScreen->physicalDotsPerInchY() );
}


void VSGWindow::onOpen()
{
    if ( !vsgwidget_ )
    {
	QMessageBox::critical( this, "Error", "No viewer available" );
	return;
    }

    const QString path = QFileDialog::getOpenFileName(
	this, "Open Scene", QString(), "VSG Scene (*.vsgb *.vsgt)" );
    if ( path.isEmpty() )
	return;

    std::string msg;
    if ( !vsgwidget_->loadScene(path.toStdString().c_str(), msg) )
    {
	QMessageBox::critical( this, "Error", msg.c_str() );
	return;
    }

    QMessageBox::information( this, "Open Scene", msg.c_str() );
}


void VSGWindow::onSave()
{
    if ( !vsgwidget_ )
    {
	QMessageBox::critical( this, "Error", "No viewer available" );
	return;
    }

    const QString path = QFileDialog::getSaveFileName(
	this, "Save Scene", QString(), "VSG Scene (*.vsgb *.vsgt)" );
    if ( path.isEmpty() )
	return;

    std::string msg;
    if ( !vsgwidget_->saveScene(path.toStdString().c_str(), msg) )
    {
	QMessageBox::critical( this, "Error", msg.c_str() );
	return;
    }

    QMessageBox::information( this, "Save Scene", msg.c_str() );
}


void VSGWindow::onTakeScreenshot()
{
    if ( !vsgwidget_ )
    {
	QMessageBox::critical( this, "Error", "No viewer available" );
	return;
    }

    const double scalefactor = 2.;
    const QString outpath = QDir(QDir::tempPath()).filePath( "screenshot.png" );
    std::string msg;
    if ( !vsgwidget_->doScreenShot(currentScreenDPI(), scalefactor,
				   outpath.toStdString().c_str(), msg) )
    {
	QMessageBox::critical( this, "Error", msg.c_str() );
	return;
    }

    QMessageBox::information( this, "Screenshot", msg.c_str() );
}


void VSGWindow::onFont()
{
    if ( !vsgwidget_ )
	return;

    PtrMan<QFont> cur = uiFont::createQFont( vsgwidget_->fontData() );
    if ( !cur )
	return;

    bool ok = false;
    const QFont chosen = QFontDialog::getFont( &ok, *cur, this,
						"3D Label Font" );
    if ( !ok )
	return;

    FontData fd;
    uiFont::getFontData( fd, chosen );
    vsgwidget_->setFontData( fd );
}


void VSGWindow::onMarkerShapeChanged( int shape )
{
    if ( !vsgwidget_ || shape < VSGWidget::Cube || shape > VSGWidget::Cross )
	return;

    vsgwidget_->setMarkerShape(
			static_cast<VSGWidget::MarkerShape>(shape) );
}


void VSGWindow::onExit()
{
    close();
}


VSGWidget::VSGWidget( QWidget* parent )
    : QWidget(parent)
{
    setMinimumSize( 600, 400 );
    setAttribute( Qt::WA_OpaquePaintEvent );
    setAttribute( Qt::WA_NoSystemBackground );
    viewer_ = VSGQtViewer::create( rendertimems_ );

    vsg::ref_ptr<vsg::WindowTraits> traits = vsg::WindowTraits::create();
    traits->decoration = false;
    traits->deviceFeatures = vsg::DeviceFeatures::create();
    traits->deviceFeatures->get().wideLines = VK_TRUE;
    traits->swapchainPreferences.imageUsage =
	VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
	VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    qtwindow_ = new VSGQtWindow( viewer_, traits );
    container_ = QWidget::createWindowContainer( qtwindow_, this );
    container_->setFocusPolicy( Qt::StrongFocus );
    auto* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( container_ );
}


VSGWidget::~VSGWidget()
{
    ++fontrequestid_;
    if ( fontthread_.joinable() )
	fontthread_.join();

    QCoreApplication::removePostedEvents( this );

    auto* vsgwin = dynamic_cast<VSGWindow*>( QApplication::activeWindow() );
    if ( vsgwin )
	vsgwin->setVSGWidget( nullptr );
}


void VSGWidget::setMarkerShape( MarkerShape shape )
{
    if ( markershape_ == shape )
	return;

    markershape_ = shape;
    if ( !markersroot_ || !viewer_ )
	return;

    vsg::ref_ptr<vsg::Node> markers = addMarkers();
    if ( !markers )
	return;

    viewer_->deviceWaitIdle();
    markersroot_->children.clear();
    markersroot_->addChild( markers );
    viewer_->compile();
}


VSGWidget_internal::BeachBallDisplaySettings
VSGWidget::beachBallSettings() const
{
    return beachballset_.displaySettings();
}


void VSGWidget::setBeachBallSettings(
	const VSGWidget_internal::BeachBallDisplaySettings& settings )
{
    beachballset_.setDisplaySettings( settings );
    if ( !beachballsroot_ || !viewer_ )
	return;

    vsg::ref_ptr<vsg::Node> beachballs = addReferenceBeachBalls();
    if ( !beachballs )
	return;

    viewer_->deviceWaitIdle();
    beachballsroot_->children.clear();
    beachballsroot_->addChild( beachballs );
    viewer_->compile();
}


bool VSGWidget::saveScene( const char* destpath, std::string& msg ) const
{
    if ( !root_ )
    {
	msg = "Error: Scene is not initialized";
	return false;
    }

    if ( !destpath || !*destpath )
    {
	msg = "Error: Invalid destination path";
	return false;
    }

    if ( !vsg::write( root_, destpath, vsgoptions_ ) )
    {
	msg = std::string( "Failed to save scene to " ) + destpath;
	return false;
    }

    QString infomsg = QString( "Scene saved to %1" ).arg( destpath );
    auto* lookAt = camera_
	    ? dynamic_cast<vsg::LookAt*>( camera_->viewMatrix.get() )
	    : nullptr;
    if ( lookAt )
    {
	const vsg::dmat4 vm = lookAt->transform();
	QJsonArray mtx;
	const double* p = vm.data();
	for ( int i = 0; i < 16; i++ )
	    mtx.append( p[i] );

	QJsonObject obj;
	obj["version"] = 1;
	obj["view_matrix"] = mtx;

	const QString camPath = QString( destpath ) + ".camera.json";
	QFile camFile( camPath );
	if ( camFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
	    camFile.write( QJsonDocument(obj).toJson(
					  QJsonDocument::Indented ) );
	    camFile.close();
	}
	else
	{
	    infomsg += QString( "\nWarning: failed to write camera state: %1" )
		       .arg( camPath );
	}
    }

    msg = infomsg.toStdString();
    return true;
}


bool VSGWidget::loadScene( const char* srcpath, std::string& msg )
{
    if ( !viewer_ )
    {
	msg = "Error: Viewer is not initialized";
	return false;
    }

    if ( !srcpath || !*srcpath )
    {
	msg = "Error: Invalid source path";
	return false;
    }

    setupVsgOptions();
    vsg::ref_ptr<vsg::Node> loaded = vsg::read_cast<vsg::Node>( srcpath,
								vsgoptions_ );
    if ( !loaded )
    {
	msg = std::string( "Failed to load scene from " ) + srcpath;
	return false;
    }

    vsg::ref_ptr<vsg::Group> newRoot = vsg::Group::create();
    newRoot->addChild( loaded );
    root_ = newRoot;

    vsg::ref_ptr<vsg::CommandGraph> commandGraph =
			vsg::createCommandGraphForView(
					vsgwindow_, camera_, root_,
					VK_SUBPASS_CONTENTS_INLINE, false );
    viewer_->assignRecordAndSubmitTaskAndPresentation( { commandGraph } );
    viewer_->compile();

    labeltext_ = {};
    markersroot_ = {};
    beachballsroot_ = {};
    cubecenter_.set( 0.0, 0.0, 0.0 );
    labelanchor_.set( 0.f, 0.f, 0.f );

    QString infomsg = QString( "Scene loaded from %1" ).arg( srcpath );
    const QString camPath = QString( srcpath ) + ".camera.json";
    QFile camFile( camPath );
    if ( camFile.open( QIODevice::ReadOnly ) )
    {
	const QJsonDocument doc = QJsonDocument::fromJson( camFile.readAll() );
	camFile.close();
	if ( doc.isObject() )
	{
	    const QJsonArray mtx = doc.object().value(
						 "view_matrix" ).toArray();
	    if ( mtx.size() == 16 )
	    {
		double vals[16];
		for ( int i = 0; i < 16; i++ )
		    vals[i] = mtx.at(i).toDouble();

		auto* lookAt = camera_
		    ? dynamic_cast<vsg::LookAt*>( camera_->viewMatrix.get() )
		    : nullptr;
		if ( lookAt )
		{
		    lookAt->set( vsg::dmat4( vals ) );
		    if ( trackball_ )
			trackball_->setViewpoint(
				vsg::LookAt::create( *lookAt ), 0.0 );
		}
		else if ( camera_ )
		{
		    vsg::ref_ptr<vsg::LookAt> newLookAt = vsg::LookAt::create();
		    newLookAt->set( vsg::dmat4( vals ) );
		    camera_->viewMatrix = newLookAt;
		}
	    }
	    else
	    {
		infomsg += "\nWarning: invalid camera sidecar format";
	    }
	}
	else
	{
	    infomsg += "\nWarning: invalid camera sidecar JSON";
	}
    }

    msg = infomsg.toStdString();
    return true;
}


bool VSGWidget::doScreenShot( const QSize& /*dpi*/, double scalefactor,
			      const char* destpath, std::string& msg ) const
{
    if ( !vsgwindow_ || !viewer_ || !root_ )
    {
	msg = "Error: Viewer or scene not initialized";
	return false;
    }

    if ( !destpath || !*destpath )
    {
	msg = "Error: Invalid destination path";
	return false;
    }

    viewer_->deviceWaitIdle();

    vsg::ref_ptr<vsg::Device> device = vsgwindow_->getDevice();
    vsg::ref_ptr<vsg::PhysicalDevice> physicalDevice =
				vsgwindow_->getPhysicalDevice();
    vsg::ref_ptr<vsg::Swapchain> swapchain = vsgwindow_->getSwapchain();
    if ( !device || !physicalDevice || !swapchain )
    {
	msg = "Error: Vulkan swapchain not available";
	return false;
    }

    if ( vsgwindow_->numFrames() < 2 )
    {
	msg = "Error: Not enough frames rendered for screenshot";
	return false;
    }

    const uint32_t width = vsgwindow_->extent2D().width;
    const uint32_t height = vsgwindow_->extent2D().height;
    vsg::ref_ptr<vsg::Image> sourceImage =
			vsgwindow_->imageView( vsgwindow_->
				imageIndex(1) )->image;

    const VkFormat sourceImageFormat = swapchain->getImageFormat();
    VkFormat targetImageFormat = sourceImageFormat;

    VkFormatProperties srcFormatProperties;
    vkGetPhysicalDeviceFormatProperties( *physicalDevice, sourceImageFormat,
					 &srcFormatProperties );
    VkFormatProperties destFormatProperties;
    vkGetPhysicalDeviceFormatProperties( *physicalDevice,
					 VK_FORMAT_R8G8B8A8_UNORM,
					 &destFormatProperties );

    const bool supportsBlit =
	    ( (srcFormatProperties.optimalTilingFeatures
	       & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0 )
	 && ( (destFormatProperties.linearTilingFeatures
	       & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0 );
    if ( supportsBlit )
	targetImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    vsg::ref_ptr<vsg::Image> destinationImage = vsg::Image::create();
    destinationImage->imageType = VK_IMAGE_TYPE_2D;
    destinationImage->format = targetImageFormat;
    destinationImage->extent.width = width;
    destinationImage->extent.height = height;
    destinationImage->extent.depth = 1;
    destinationImage->arrayLayers = 1;
    destinationImage->mipLevels = 1;
    destinationImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    destinationImage->samples = VK_SAMPLE_COUNT_1_BIT;
    destinationImage->tiling = VK_IMAGE_TILING_LINEAR;
    destinationImage->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    destinationImage->compile( device );

    vsg::ref_ptr<vsg::DeviceMemory> deviceMemory =
			vsg::DeviceMemory::create(
	    device, destinationImage->getMemoryRequirements(device->deviceID),
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
    destinationImage->bind( deviceMemory, 0 );

    vsg::ref_ptr<vsg::Commands> commands = vsg::Commands::create();

    vsg::ref_ptr<vsg::ImageMemoryBarrier> transitionDst =
			vsg::ImageMemoryBarrier::create(
	    0, VK_ACCESS_TRANSFER_WRITE_BIT,
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	    destinationImage,
	    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } );
    vsg::ref_ptr<vsg::ImageMemoryBarrier> transitionSrc =
			vsg::ImageMemoryBarrier::create(
	    VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
	    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	    sourceImage,
	    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } );
    commands->addChild( vsg::PipelineBarrier::create(
	    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
	    transitionDst, transitionSrc ) );

    if ( supportsBlit )
    {
	VkImageBlit region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.srcOffsets[0] = VkOffset3D{ 0, 0, 0 };
	region.srcOffsets[1] = VkOffset3D{ static_cast<int32_t>(width),
					   static_cast<int32_t>(height), 1 };
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.dstOffsets[0] = VkOffset3D{ 0, 0, 0 };
	region.dstOffsets[1] = VkOffset3D{ static_cast<int32_t>(width),
					   static_cast<int32_t>(height), 1 };

	vsg::ref_ptr<vsg::BlitImage> blitImage = vsg::BlitImage::create();
	blitImage->srcImage = sourceImage;
	blitImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitImage->dstImage = destinationImage;
	blitImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitImage->regions.push_back( region );
	blitImage->filter = VK_FILTER_NEAREST;
	commands->addChild( blitImage );
    }
    else
    {
	VkImageCopy region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vsg::ref_ptr<vsg::CopyImage> copyImage = vsg::CopyImage::create();
	copyImage->srcImage = sourceImage;
	copyImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	copyImage->dstImage = destinationImage;
	copyImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	copyImage->regions.push_back( region );
	commands->addChild( copyImage );
    }

    vsg::ref_ptr<vsg::ImageMemoryBarrier> transitionDstToGeneral =
			vsg::ImageMemoryBarrier::create(
	    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
	    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	    destinationImage,
	    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } );
    vsg::ref_ptr<vsg::ImageMemoryBarrier> transitionSrcToPresent =
			vsg::ImageMemoryBarrier::create(
	    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
	    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	    sourceImage,
	    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } );
    commands->addChild( vsg::PipelineBarrier::create(
	    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
	    transitionDstToGeneral, transitionSrcToPresent ) );

    vsg::ref_ptr<vsg::Fence> fence = vsg::Fence::create( device );
    const uint32_t queueFamilyIndex =
	    physicalDevice->getQueueFamily( VK_QUEUE_GRAPHICS_BIT );
    vsg::ref_ptr<vsg::CommandPool> commandPool =
			vsg::CommandPool::create( device, queueFamilyIndex );
    vsg::ref_ptr<vsg::Queue> queue = device->getQueue( queueFamilyIndex );
    vsg::submitCommandsToQueue( commandPool, fence, 100000000000, queue,
	    [&]( vsg::CommandBuffer& commandBuffer ) {
		commands->record( commandBuffer );
	    } );

    VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout( *device,
				 destinationImage->vk(device->deviceID),
				 &subResource, &subResourceLayout );

    const size_t destRowWidth = width * sizeof( vsg::ubvec4 );
    vsg::ref_ptr<vsg::Data> imageData;
    if ( destRowWidth == subResourceLayout.rowPitch )
    {
	imageData = vsg::MappedData<vsg::ubvec4Array2D>::create(
		deviceMemory, subResourceLayout.offset, 0,
		vsg::Data::Properties{ targetImageFormat }, width, height );
    }
    else
    {
	vsg::ref_ptr<vsg::MappedData<vsg::ubyteArray>> mappedData =
			vsg::MappedData<vsg::ubyteArray>::create(
		deviceMemory, subResourceLayout.offset, 0,
		vsg::Data::Properties{ targetImageFormat },
		subResourceLayout.rowPitch * height );
	imageData = vsg::ubvec4Array2D::create(
		width, height, vsg::Data::Properties{ targetImageFormat } );
	for ( uint32_t row = 0; row < height; ++row )
	{
	    std::memcpy( imageData->dataPointer(row * width),
		mappedData->dataPointer(row * subResourceLayout.rowPitch),
		destRowWidth );
	}
    }

    QImage img( static_cast<const uchar*>( imageData->dataPointer() ),
		static_cast<int>( width ), static_cast<int>( height ),
		QImage::Format_RGBA8888 );
    img = img.copy();
    if ( scalefactor > 0.0 && std::abs(scalefactor - 1.0) > 1e-6 )
    {
	img = img.scaled( static_cast<int>( width * scalefactor ),
			  static_cast<int>( height * scalefactor ),
			  Qt::IgnoreAspectRatio,
			  Qt::SmoothTransformation );
    }

    const QString outpath( destpath );
    if ( !img.save(outpath) )
    {
	msg = "Failed to save screenshot to " + outpath.toStdString();
	return false;
    }

    msg = "Screenshot saved to '" + outpath.toStdString()
	    + "' (" + std::to_string(img.width()) + "x"
	    + std::to_string(img.height()) + ")";
    return true;
}


vsg::ref_ptr<vsg::Data> VSGWidget::createCheckerImage() const
{
    const uint32_t texw = 128;
    const uint32_t texh = 128;
    vsg::ref_ptr<vsg::ubvec4Array2D> image =
				vsg::ubvec4Array2D::create( texw,
							    texh );
    image->properties.format = VK_FORMAT_R8G8B8A8_UNORM;

    for ( uint32_t y=0; y<texh; y++ )
    {
	for ( uint32_t x=0; x<texw; x++ )
	{
	    const bool dark = ( (x/16 + y/16) % 2 ) == 0;
	    image->set( x, y, vsg::ubvec4(
			dark ? 30 : 200,
			dark ? 80 : 220,
			dark ? 180 : 40,
			255) );
	}
    }

    return image;
}


vsg::ref_ptr<vsg::Node> VSGWidget::addCube()
{
    setupVsgOptions();

    const float cubesize = 0.8f;
    cubecenter_.set( 0.0, 0.0, 0.0 );

    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    builder->options = vsgoptions_;

    vsg::GeometryInfo gi;
    gi.position.set( static_cast<float>(cubecenter_.x),
		     static_cast<float>(cubecenter_.y),
		     static_cast<float>(cubecenter_.z) );
    gi.dx.set( cubesize, 0.f, 0.f );
    gi.dy.set( 0.f, cubesize, 0.f );
    gi.dz.set( 0.f, 0.f, cubesize );

    vsg::StateInfo si;
    si.lighting = true;
    si.image = createCheckerImage();

    return builder->createBox( gi, si );
}


vsg::ref_ptr<vsg::Node> VSGWidget::addReferenceBeachBalls()
{
    setupVsgOptions();

    using namespace VSGWidget_internal;

    if ( beachballset_.nrEvents() > 0 )
	return beachballset_.build( vsgoptions_ );

    BeachBallDisplaySettings settings;
    settings.magMin = 3.f;
    settings.magMax = 6.f;
    settings.diamMin = 0.15f;
    settings.diamMax = 0.55f;
    settings.nrLatitudeSegments = 96;
    settings.nrLongitudeSegments = 192;
    beachballset_.setDisplaySettings( settings );

    // Reference events: Scene-frame tensors (existing factories) so the
    // look stays unchanged. SDR fields are filled for future menu/import.
    const struct {
	const char*		id;
	float			x;
	float			magnitude;
	MomentTensor		tensor;
	float			strikeDeg;
	float			dipDeg;
	float			rakeDeg;
	bool			hasSDR;
    } refs[] = {
	{ "strike-slip", -1.2f, 3.f, MomentTensor::strikeSlipXY(),
	  0.f, 90.f, 0.f, true },
	{ "normal", -0.4f, 4.f, MomentTensor::normalFaultXZ(),
	  0.f, 45.f, -90.f, true },
	{ "reverse", 0.4f, 5.f, MomentTensor::reverseFaultXZ(),
	  0.f, 45.f, 90.f, true },
	{ "isotropic", 1.2f, 6.f, MomentTensor::isotropic(),
	  0.f, 0.f, 0.f, false }
    };

    for ( const auto& ref : refs )
    {
	SeismicEvent evt;
	evt.id = ref.id;
	evt.position = vsg::vec3( ref.x, 0.7f, 0.8f );
	evt.magnitude = ref.magnitude;
	evt.tensor = ref.tensor;
	evt.frame = CoordFrame::Scene;
	evt.hasSDR = ref.hasSDR;
	evt.strikeDeg = ref.strikeDeg;
	evt.dipDeg = ref.dipDeg;
	evt.rakeDeg = ref.rakeDeg;
	beachballset_.addEvent( evt );
    }

    qInfo() << "Reference beach balls, left to right:"
	    << "strike-slip, normal, reverse, isotropic explosion";
    return beachballset_.build( vsgoptions_ );
}


vsg::ref_ptr<vsg::Node> VSGWidget::addMarkers()
{
    setupVsgOptions();

    const float magMin = 3.f;
    const float magMax = 6.f;
    const float diamMin = 0.15f;
    const float diamMax = 0.55f;
    const float groundZ = -0.15f;
    const int nperCluster = 80;
    const float halfExtent = 4.0f;

    const vsg::vec2 offsets[] = {
	{ -5.f,  5.f },
	{  5.f,  5.f },
	{ -5.f, -5.f },
	{  5.f, -5.f },
	{  0.f,  0.f }
    };
    const int nclusters =
	static_cast<int>( sizeof(offsets) / sizeof(offsets[0]) );

    auto nextU01 = []( unsigned& state ) -> float
    {
	state = state * 1664525u + 1013904223u;
	return ( state & 0x00FFFFFFu ) / float( 0x01000000u );
    };

    VSGWidget_internal::VsgMarkerSet markerset;
    using VsgShape = VSGWidget_internal::VsgMarkerSet::Shape;
    VsgShape vsgshape = VsgShape::Sphere;
    switch ( markershape_ )
    {
	case Cube:
	    vsgshape = VsgShape::Cube;
	    break;
	case Cone:
	    vsgshape = VsgShape::Cone;
	    break;
	case Cylinder:
	    vsgshape = VsgShape::Cylinder;
	    break;
	case Sphere:
	    vsgshape = VsgShape::Sphere;
	    break;
	case Cross:
	    vsgshape = VsgShape::Cross;
	    break;
    }

    markerset.setShape( vsgshape );
    markerset.setDefaultSize( diamMin );
    markerset.setHeightRatio( 1.2f );

    unsigned rng = 42u;

    for ( int clusteridx=0; clusteridx<nclusters; clusteridx++ )
    {
	for ( int idx=0; idx<nperCluster; idx++ )
	{
	    const float u = nextU01( rng );
	    const float v = nextU01( rng );
	    const float w = nextU01( rng );

	    const vsg::vec3 pos(
		offsets[clusteridx].x - halfExtent
		    + u * ( 2.f * halfExtent ),
		offsets[clusteridx].y + 0.7f - halfExtent
		    + v * ( 2.f * halfExtent ),
		0.f );
	    const float magnitude = magMin + w * ( magMax - magMin );
	    const float t = ( magnitude - magMin ) / ( magMax - magMin );
	    const vsg::vec4 color( t, 0.3f + 0.5f * t, 1.f - t, 1.f );

	    markerset.addPos( pos, magnitude, color );
	}
    }

    markerset.applySizesFromMagnitude( magMin, magMax, diamMin, diamMax );
    return markerset.build( vsgoptions_, groundZ, createCheckerImage() );
}


vsg::ref_ptr<vsg::Node> VSGWidget::addGround()
{
    setupVsgOptions();

    const float groundsize = 20.0f;

    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    builder->options = vsgoptions_;

    vsg::GeometryInfo gi;
    gi.position.set( 0.f, 0.7f, -0.15f );
    gi.dx.set( groundsize, 0.f, 0.f );
    gi.dy.set( 0.f, groundsize, 0.f );
    gi.color.set( 0.9f, 0.9f, 0.92f, 1.f );

    vsg::StateInfo si;
    si.lighting = true;
    si.two_sided = true;

    return builder->createQuad( gi, si );
}


vsg::ref_ptr<vsg::Node> VSGWidget::addSceneLighting()
{
    vsg::ref_ptr<vsg::Group> lights = vsg::Group::create();

    vsg::ref_ptr<vsg::AmbientLight> ambient = vsg::AmbientLight::create();
    ambient->name = "ambient";
    ambient->color.set( 1.0f, 1.0f, 1.0f );
    ambient->intensity = 0.05f;
    lights->addChild( ambient );

    vsg::ref_ptr<vsg::DirectionalLight> sun = vsg::DirectionalLight::create();
    sun->name = "sun";
    sun->color.set( 1.0f, 1.0f, 1.0f );
    sun->intensity = 1.0f;
    sun->direction = vsg::normalize( vsg::dvec3( 1, 0.4, -0.35 ) );
    sun->shadowSettings = vsg::HardShadows::create( 3 );
    lights->addChild( sun );

    return lights;
}


vsg::ref_ptr<vsg::Node> VSGWidget::addLine()
{
    vsg::ref_ptr<vsg::vec3Array> vertices = vsg::vec3Array::create(
	    { {-1.f, 0.f, 0.f}, {1.f, 0.f, 0.f} } );
    vsg::ref_ptr<vsg::vec4Array> colors = vsg::vec4Array::create(
	    { {1.f, 0.f, 0.f, 1.f}, {1.f, 0.f, 0.f, 1.f} } );

    vsg::ref_ptr<vsg::ShaderSet> shaderSet =vsg::createFlatShadedShaderSet();
    vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> pipelineConfig =
			vsg::GraphicsPipelineConfigurator::create( shaderSet );

    vsg::DataList vertexArrays;
    pipelineConfig->assignArray( vertexArrays, "vsg_Vertex",
				 VK_VERTEX_INPUT_RATE_VERTEX, vertices );
    pipelineConfig->assignArray( vertexArrays, "vsg_Color",
				 VK_VERTEX_INPUT_RATE_VERTEX, colors );

    vsg::ref_ptr<vsg::PhongMaterialValue> mat =
				vsg::PhongMaterialValue::create();
    mat->value().diffuse.set( 1.0f, 1.0f, 1.0f, 1.0f );
    pipelineConfig->assignDescriptor( "material", mat );

    VSGWidget_internal::SetPipelineStates sps;
    pipelineConfig->accept( sps );
    pipelineConfig->init();

    vsg::ref_ptr<vsg::VertexDraw> draw = vsg::VertexDraw::create();
    draw->assignArrays( vertexArrays );
    draw->vertexCount = static_cast<uint32_t>( vertices->size() );
    draw->instanceCount = 1;

    vsg::ref_ptr<vsg::StateGroup> stateGroup = vsg::StateGroup::create();
    pipelineConfig->copyTo( stateGroup );
    stateGroup->addChild( draw );
    return stateGroup;
}


void VSGWidget::setupVsgOptions()
{
    if ( vsgoptions_ )
	return;

    vsgoptions_ = vsg::Options::create();
    vsgoptions_->sharedObjects = vsg::SharedObjects::create();

    for ( const auto& path : vsg::getEnvPaths("VSG_FILE_PATH") )
	vsgoptions_->paths.push_back( path );

    const QString appdir = QCoreApplication::applicationDirPath();
    vsgoptions_->paths.push_back( (appdir + "/folder_vsg").toStdString() );
    const QFileInfo srcfi( QString::fromLocal8Bit( __FILE__ ) );
    const QString srcfolder =
		srcfi.absoluteDir().absoluteFilePath( "../vsgBase/folder_vsg" );
    if ( QDir( srcfolder ).exists() )
	vsgoptions_->paths.push_back( srcfolder.toStdString() );

    // HardShadows only fills shadow maps; Phong must sample them. Force the
    // VSG_SHADOWS_HARD define so Builder-created lit geometry receives shadows.
    vsg::ref_ptr<vsg::ShaderSet> phong =
			vsg::createPhongShaderSet( vsgoptions_ );
    if ( !phong->defaultShaderHints )
	phong->defaultShaderHints = vsg::ShaderCompileSettings::create();
    phong->defaultShaderHints->defines.insert( "VSG_SHADOWS_HARD" );
    phong->variants.clear();
    vsgoptions_->shaderSets["phong"] = phong;
}


static vsg::ref_ptr<vsg::Font> loadStartupFont(
	vsg::ref_ptr<vsg::Options> options )
{
    if ( auto font = vsg::read_cast<vsg::Font>( "times.vsgb", options ) )
	return font;

    if ( auto font = vsg::read_cast<vsg::Font>(
		"folder_vsg/times.vsgb", options ) )
	return font;

    const QFileInfo srcfi( QString::fromLocal8Bit( __FILE__ ) );
    const QString startup = QDir( srcfi.absoluteDir().absoluteFilePath(
		    "../vsgBase/folder_vsg" ) ).absoluteFilePath( "times.vsgb" );
    if ( QFile::exists( startup ) )
	return vsg::read_cast<vsg::Font>( startup.toStdString(), options );

    return {};
}


static vsg::ref_ptr<vsg::Options> createWorkerFontOptions()
{
    auto options = vsg::Options::create();
#ifndef __win__
    options->setValue( vsgXchange::freetype::texel_margin_ratio, 0.5f );
    options->setValue( vsgXchange::freetype::quad_margin_ratio, 0.25f );
    options->add( vsgXchange::freetype::create() );
#endif
    return options;
}

#ifdef __win__
static bool openDirectWriteFace( const QString& family, bool bold, bool italic,
		 VSGWidget_internal::DirectWriteCOMPtr<IDWriteFactory>& factory,
		 VSGWidget_internal::DirectWriteCOMPtr<IDWriteFontFace>& face )
{
    if ( FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
				    __uuidof(IDWriteFactory),
				    reinterpret_cast<IUnknown**>(&factory))) )
	return false;

    VSGWidget_internal::DirectWriteCOMPtr<IDWriteFontCollection> collection;
    if ( FAILED(factory->GetSystemFontCollection(&collection)) )
	return false;

    UINT32 familyindex = 0;
    BOOL familyexists = FALSE;
    if ( FAILED(collection->FindFamilyName(
	    reinterpret_cast<const wchar_t*>(family.utf16()),
	    &familyindex,&familyexists)) || !familyexists )
	return false;

    VSGWidget_internal::DirectWriteCOMPtr<IDWriteFontFamily> fontfamily;
    if ( FAILED(collection->GetFontFamily(familyindex,&fontfamily)) )
	return false;

    const DWRITE_FONT_WEIGHT weight =
	    bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    const DWRITE_FONT_STYLE style =
	    italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

    VSGWidget_internal::DirectWriteCOMPtr<IDWriteFont> font;
    if ( FAILED(fontfamily->GetFirstMatchingFont(weight,
						 DWRITE_FONT_STRETCH_NORMAL,
						 style,&font)) )
	return false;

    return SUCCEEDED( font->CreateFontFace(&face) );
}


static void makeSdfFromAlpha( const std::vector<uint8_t>& alpha,
	int width, int height, int margin, std::vector<int16_t>& sdf )
{
    const int outw = width + 2 * margin;
    const int outh = height + 2 * margin;
    sdf.assign( size_t(outw) * size_t(outh), 0 );

    auto insideAt = [&]( int x, int y ) -> bool
    {
	const int gx = x - margin;
	const int gy = y - margin;
	if ( gx < 0 || gy < 0 || gx >= width || gy >= height )
	    return false;
	return alpha[size_t(gy) * size_t(width) + size_t(gx)] > 127;
    };

    const float maxdist = float( margin );
    for ( int y = 0; y < outh; ++y )
    {
	for ( int x = 0; x < outw; ++x )
	{
	    const bool inside = insideAt( x, y );
	    float bestdist = maxdist;
	    const int r0 = std::max( 0, y - margin );
	    const int r1 = std::min( outh - 1, y + margin );
	    const int c0 = std::max( 0, x - margin );
	    const int c1 = std::min( outw - 1, x + margin );
	    for ( int ry = r0; ry <= r1; ++ry )
	    {
		for ( int rx = c0; rx <= c1; ++rx )
		{
		    if ( insideAt( rx, ry ) == inside )
			continue;
		    const float dx = float( rx - x );
		    const float dy = float( ry - y );
		    bestdist = std::min( bestdist, std::sqrt (dx*dx + dy*dy) );
		}
	    }

	    float signeddist = inside ? bestdist : -bestdist;
	    signeddist = std::clamp( signeddist / maxdist, -1.f, 1.f );
	    sdf[size_t(y) * size_t(outw) + size_t(x)] =
		    int16_t( signeddist *
			     float( std::numeric_limits<int16_t>::max() ) );
	}
    }
}


static vsg::ref_ptr<vsg::Font> createVsgFontDirectWrite( const FontData& fd )
{
    PtrMan<QFont> qf = uiFont::createQFont( fd );
    const QString family = qf ? qf->family() : QString( fd.family() );
    const bool bold = fd.weight() >= FontData::Bold;

    VSGWidget_internal::DirectWriteCOMPtr<IDWriteFactory> factory;
    VSGWidget_internal::DirectWriteCOMPtr<IDWriteFontFace> face;
    if ( !openDirectWriteFace(family,bold,fd.isItalic(),factory,face) )
	return {};

    DWRITE_FONT_METRICS fontmetrics{};
    face->GetMetrics( &fontmetrics );
    if ( fontmetrics.designUnitsPerEm == 0 )
	return {};

    constexpr float emsize = 48.f;
    constexpr int margin = 6;
    constexpr uint32_t firstchar = 32;
    constexpr uint32_t lastchar = 126;
    const float du_to_px =
	    emsize / float( fontmetrics.designUnitsPerEm );

    std::vector<VSGWidget_internal::GlyphBitmap> glyphs;
    glyphs.reserve( lastchar - firstchar + 1 );

    for ( uint32_t ch = firstchar; ch <= lastchar; ++ch )
    {
	const UINT32 codepoint = ch;
	UINT16 glyphindex = 0;
	if ( FAILED(face->GetGlyphIndices(&codepoint,1,&glyphindex))
	     || glyphindex == 0 )
	    continue;

	DWRITE_GLYPH_METRICS gmetrics{};
	if ( FAILED( face->GetDesignGlyphMetrics(&glyphindex,1,
						 &gmetrics,FALSE)) )
	    continue;

	FLOAT advance = float( gmetrics.advanceWidth ) * du_to_px;
	DWRITE_GLYPH_OFFSET offset{ 0.f, 0.f };
	DWRITE_GLYPH_RUN run{};
	run.fontFace = face.get();
	run.fontEmSize = emsize;
	run.glyphCount = 1;
	run.glyphIndices = &glyphindex;
	run.glyphAdvances = &advance;
	run.glyphOffsets = &offset;
	run.isSideways = FALSE;
	run.bidiLevel = 0;

	VSGWidget_internal::DirectWriteCOMPtr<IDWriteGlyphRunAnalysis> analysis;
	if ( FAILED(factory->CreateGlyphRunAnalysis(&run,1.0f,nullptr,
						DWRITE_RENDERING_MODE_ALIASED,
						DWRITE_MEASURING_MODE_NATURAL,
						0.0f,0.0f,&analysis)) )
	    continue;

	RECT bounds{};
	if ( FAILED(analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1,
						    &bounds)) )
	    continue;

	const int width = bounds.right - bounds.left;
	const int height = bounds.bottom - bounds.top;
	if ( width <= 0 || height <= 0 )
	{
	    VSGWidget_internal::GlyphBitmap empty;
	    empty.charcode = ch;
	    empty.glyphindex = glyphindex;
	    empty.metrics = gmetrics;
	    glyphs.push_back( std::move( empty ) );
	    continue;
	}

	std::vector<uint8_t> alpha( size_t(width) * size_t(height) );
	if ( FAILED( analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1,
						  &bounds,alpha.data(),
						  UINT32(alpha.size()))) )
	    continue;

	VSGWidget_internal::GlyphBitmap gb;
	gb.charcode = ch;
	gb.glyphindex = glyphindex;
	gb.width = width;
	gb.height = height;
	gb.originx = bounds.left;
	gb.originy = bounds.top;
	gb.metrics = gmetrics;
	makeSdfFromAlpha( alpha, width, height, margin, gb.sdf );
	glyphs.push_back( std::move( gb ) );
    }

    if ( glyphs.empty() )
	return {};

    int atlaswidth = 512;
    int xpos = margin;
    int ypos = margin;
    int rowheight = 0;
    int neededheight = margin;
    for ( const auto& gb : glyphs )
    {
	if ( gb.width <= 0 || gb.height <= 0 || gb.sdf.empty() )
	    continue;

	const int cellw = gb.width + 2 * margin;
	const int cellh = gb.height + 2 * margin;
	if ( xpos + cellw + margin > atlaswidth )
	{
	    xpos = margin;
	    ypos += rowheight + margin;
	    rowheight = 0;
	}

	xpos += cellw + margin;
	rowheight = std::max( rowheight, cellh );
	neededheight = std::max( neededheight, ypos + cellh + margin );
    }

    int atlasheight = 64;
    while ( atlasheight < neededheight )
	atlasheight *= 2;

    auto atlas = vsg::shortArray2D::create( uint32_t(atlaswidth),
				uint32_t(atlasheight),
				vsg::Data::Properties{VK_FORMAT_R16_SNORM} );
    for ( auto& c : *atlas )
	c = std::numeric_limits<int16_t>::lowest();

    auto glyphmetrics = vsg::GlyphMetricsArray::create(
					    uint32_t( glyphs.size() + 1) );
    auto charmap = vsg::uintArray::create( lastchar + 1 );
    for ( auto& c : *charmap )
	c = 0;

    vsg::GlyphMetrics nullmetrics{};
    glyphmetrics->set( 0, nullmetrics );

    xpos = margin;
    ypos = margin;
    rowheight = 0;
    uint32_t destindex = 1;

    for ( const auto& gb : glyphs )
    {
	const bool hasbitmap =
		gb.width > 0 && gb.height > 0 && !gb.sdf.empty();
	const int cellw = hasbitmap ? gb.width + 2 * margin : 0;
	const int cellh = hasbitmap ? gb.height + 2 * margin : 0;
	if ( hasbitmap )
	{
	    if ( xpos + cellw + margin > atlaswidth )
	    {
		xpos = margin;
		ypos += rowheight + margin;
		rowheight = 0;
	    }

	    for ( int r = 0; r < cellh; ++r )
	    {
		const size_t srcrow = size_t(r) * size_t(cellw);
		const size_t dstrow =
			atlas->index( uint32_t(xpos), uint32_t(ypos + r) );
		for ( int c = 0; c < cellw; ++c )
		    atlas->at( dstrow + size_t(c) ) =
			    gb.sdf[srcrow + size_t(c)];
	    }
	}

	const float pixel_size = emsize;
	vsg::GlyphMetrics gm{};
	if ( hasbitmap )
	{
	    gm.uvrect.set(
		( float(xpos) - 1.f ) / float( atlaswidth - 1 ),
		float( ypos + cellh ) / float( atlasheight - 1 ),
		float( xpos + cellw ) / float( atlaswidth - 1 ),
		( float(ypos) - 1.f ) / float( atlasheight - 1 ) );
	    gm.width = float( cellw ) / pixel_size;
	    gm.height = float( cellh ) / pixel_size;
	}
	gm.horiBearingX =
		( float(gb.originx) - float(margin) ) / pixel_size;
	gm.horiBearingY =
		( -float(gb.originy) + float(margin) ) / pixel_size;
	gm.horiAdvance =
		float( gb.metrics.advanceWidth ) * du_to_px / pixel_size;
	gm.vertBearingX =
		float( gb.metrics.leftSideBearing ) * du_to_px / pixel_size;
	gm.vertBearingY =
		float( gb.metrics.topSideBearing ) * du_to_px / pixel_size;
	gm.vertAdvance =
		float( gb.metrics.advanceHeight ) * du_to_px / pixel_size;

	glyphmetrics->set( destindex, gm );
	charmap->set( gb.charcode, destindex );
	++destindex;

	if ( hasbitmap )
	{
	    xpos += cellw + margin;
	    rowheight = std::max( rowheight, cellh );
	}
    }

    auto font = vsg::Font::create();
    font->atlas = atlas;
    font->glyphMetrics = glyphmetrics;
    font->charmap = charmap;
    font->ascender =
	    float( fontmetrics.ascent ) * du_to_px / emsize;
    font->descender =
	    -float( fontmetrics.descent ) * du_to_px / emsize;
    font->height =
	    float( fontmetrics.ascent + fontmetrics.descent
		   + fontmetrics.lineGap ) * du_to_px / emsize;
    return font;
}
#endif


#ifndef __win__
static QString ttfPathForFamily( const QString& family, bool bold, bool italic )
{
    FcPattern* pat = FcPatternCreate();
    if ( !pat )
	return {};

    FcPatternAddString( pat, FC_FAMILY,
	reinterpret_cast<const FcChar8*>( family.toUtf8().constData() ) );
    FcPatternAddInteger( pat, FC_WEIGHT,
	bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR );
    FcPatternAddInteger( pat, FC_SLANT,
	italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN );
    FcConfigSubstitute( nullptr, pat, FcMatchPattern );
    FcDefaultSubstitute( pat );
    FcResult result;
    FcPattern* match = FcFontMatch( nullptr, pat, &result );
    FcPatternDestroy( pat );
    if ( !match )
	return {};

    FcChar8* file = nullptr;
    QString path;
    if ( FcPatternGetString( match, FC_FILE, 0, &file ) == FcResultMatch
	 && file )
	path = QString::fromUtf8( reinterpret_cast<const char*>( file ) );

    FcPatternDestroy( match );
    return path;
}
#endif


static vsg::ref_ptr<vsg::Font> createVsgFont(
	const FontData& fd, vsg::ref_ptr<vsg::Options> options )
{
#ifdef __win__
    (void)options;
    vsg::ref_ptr<vsg::Font> font = createVsgFontDirectWrite( fd );
#else
    PtrMan<QFont> qf = uiFont::createQFont( fd );
    const QString family = qf ? qf->family() : QString( fd.family() );
    const bool bold = fd.weight() >= FontData::Bold;
    const QString ttf = ttfPathForFamily( family, bold, fd.isItalic() );
    if ( ttf.isEmpty() )
	return {};

    vsg::ref_ptr<vsg::Font> font =
	    vsg::read_cast<vsg::Font>( ttf.toStdString(), options );
#endif
    if ( !font )
	return {};

    font->createFontImages();
    if ( font->atlasImageInfo && font->atlasImageInfo->sampler )
	font->atlasImageInfo->sampler->maxLod = 0.0f;

    return font;
}


static void enableTextBlending( vsg::ref_ptr<vsg::Text> text,
				vsg::ref_ptr<vsg::Options> options )
{
    if ( !text )
	return;

    auto shaderSet = vsg::createTextShaderSet( options );
    for ( auto& pipelineState : shaderSet->defaultGraphicsPipelineStates )
    {
	if ( auto cbs = pipelineState.cast<vsg::ColorBlendState>() )
	    cbs->configureAttachments( true );
    }
    text->shaderSet = shaderSet;
}


void VSGWidget::applyFont( vsg::ref_ptr<vsg::Font> font,
			   const FontData& fd )
{
    if ( !font || !labeltext_ || !viewer_ )
	return;

    viewer_->deviceWaitIdle();
    if ( auto layout = labeltext_->layout.cast<vsg::StandardLayout>() )
    {
	const float fontscale = fd.pointSize() > 0
		? float(fd.pointSize()) / 24.f : 1.f;
	layout->horizontal = vsg::vec3( 0.18f * fontscale, 0.f, 0.f );
	layout->vertical = vsg::vec3( 0.f, 0.18f * fontscale, 0.f );
    }

    labeltext_->font = font;
    enableTextBlending( labeltext_, vsgoptions_ );
    labeltext_->setup( 0, vsgoptions_ );
    viewer_->compile();
}


void VSGWidget::requestFont( const FontData& fd )
{
    fontdata_ = fd;
    fontinit_ = true;
    const unsigned long long requestid = ++fontrequestid_;
    if ( fontthread_.joinable() )
	fontthread_.join();

    fontthread_ = std::thread( [this,fd,requestid]()
    {
	const vsg::ref_ptr<vsg::Font> font =
		createVsgFont( fd, createWorkerFontOptions() );
	if ( !font || requestid != fontrequestid_ )
	    return;

	QMetaObject::invokeMethod( this,
	    [this,requestid,fd,font]()
	    {
		if ( requestid != fontrequestid_ || !font
			|| !labeltext_ || !viewer_ )
		    return;
		applyFont( font, fd );
	    },
	    Qt::QueuedConnection );
    } );
}


void VSGWidget::setFontData( const FontData& fd )
{
    requestFont( fd );
}


vsg::ref_ptr<vsg::Node> VSGWidget::addLabel( float cubesize )
{
    setupVsgOptions();

    const float cubehalf = cubesize * 0.5f;
    labelanchor_.set( static_cast<float>(cubecenter_.x)+cubehalf,
		      static_cast<float>(cubecenter_.y)+cubehalf,
		      static_cast<float>(cubecenter_.z)+cubehalf + 0.02f );

    if ( !fontinit_ )
    {
	fontdata_.setFamily(  __iswin__ ? "Arial" : "Liberation Sans Narrow" );
	fontdata_.setPointSize( 24 );
	fontdata_.setWeight( FontData::Normal );
	fontdata_.setItalic( false );
	fontdata_.setStyleName( "" );
	fontinit_ = true;
    }

    vsg::ref_ptr<vsg::Font> font = loadStartupFont( vsgoptions_ );
    if ( !font )
    {
	qWarning() << "Could not load times.vsgb (VSG_FILE_PATH or"
		   << "plugins/vsgBase/folder_vsg/)";
	return nullptr;
    }

    const float fontscale = fontdata_.pointSize() > 0
	    ? float(fontdata_.pointSize()) / 24.f : 1.f;

    vsg::ref_ptr<vsg::StandardLayout> layout =
				vsg::StandardLayout::create();
    layout->billboard = true;
    layout->position = labelanchor_;
    layout->horizontal = vsg::vec3( 0.18f * fontscale, 0.f, 0.f );
    layout->vertical = vsg::vec3( 0.f, 0.18f * fontscale, 0.f );
    layout->horizontalAlignment = vsg::StandardLayout::LEFT_ALIGNMENT;
    layout->verticalAlignment = vsg::StandardLayout::BOTTOM_ALIGNMENT;
    layout->color = vsg::vec4( 1.f, 1.f, 1.f, 1.f );
    labeltext_ = vsg::Text::create();
    labeltext_->font = font;
    labeltext_->layout = layout;
    labeltext_->text = vsg::stringValue::create( "Hello, VSG!" );
    enableTextBlending( labeltext_, vsgoptions_ );
    labeltext_->setup( 0, vsgoptions_ );
    return labeltext_;
}


void VSGWidget::showEvent( QShowEvent* event )
{
    QWidget::showEvent( event );
    initializeVSG();
}


void VSGWidget::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
    if ( !vsgwindow_ || !qtwindow_ )
	return;

    const uint32_t w = static_cast<uint32_t>(
			    std::max(1,qtwindow_->width()) );
    const uint32_t h = static_cast<uint32_t>(
			    std::max(1,qtwindow_->height()) );

    if ( camera_ && camera_->viewportState )
	camera_->viewportState->set( 0, 0, w, h );

    if ( camera_ && camera_->projectionMatrix )
    {
	if ( auto* persp = dynamic_cast<vsg::Perspective*>(
				camera_->projectionMatrix.get()) )
	    persp->aspectRatio = static_cast<double>(w) / h;
    }
}


void VSGWidget::initializeVSG()
{
    if ( vsgwindow_ || !qtwindow_ || !viewer_ )
	return;

    qtwindow_->initializeWindow();
    vsgwindow_ = qtwindow_->vsgWindow();
    if ( !vsgwindow_ )
    {
	QMessageBox::critical( this, "Error",
			       "Failed to initialize VSG window" );
	return;
    }

    vsgwindow_->clearColor() = vsg::vec4( 0.05f, 0.05f, 0.05f, 1.f );

    const uint32_t w = std::max( 1u, vsgwindow_->extent2D().width );
    const uint32_t h = std::max( 1u, vsgwindow_->extent2D().height );

    const double distance = 4.5;
    const double azimuthDeg = -50.0;
    const double elevationDeg = 25.0;
    const double degToRad = std::acos( -1.0 ) / 180.0;
    const double azimuthRad = azimuthDeg * degToRad;
    const double elevationRad = elevationDeg * degToRad;
    const vsg::dvec3 eye(
	    cubecenter_.x
		+ distance * std::cos(elevationRad) * std::cos(azimuthRad),
	    cubecenter_.y
		+ distance * std::cos(elevationRad) * std::sin(azimuthRad),
	    cubecenter_.z
		+ distance * std::sin(elevationRad) );
    vsg::ref_ptr<vsg::LookAt> lookAt = vsg::LookAt::create(
	    eye,
	    cubecenter_,
	    vsg::dvec3( 0.0, 0.0, 1.0 ) );
    vsg::ref_ptr<vsg::Perspective> perspective = vsg::Perspective::create(
	    30.0, static_cast<double>(w) / h, 0.1, 50.0 );
    vsg::ref_ptr<vsg::ViewportState> viewport =
			vsg::ViewportState::create( VkExtent2D{ w, h } );
    camera_ = vsg::Camera::create( perspective, lookAt, viewport );
    trackball_ = vsg::Trackball::create( camera_ );
    trackball_->addWindow( qtwindow_->vsgWindow() );
    viewer_->addEventHandler( trackball_ );

    setupVsgOptions();

    root_ = vsg::Group::create();
    root_->addChild( addSceneLighting() );
    root_->addChild( addGround() );
    root_->addChild( addCube() );
    beachballsroot_ = vsg::Group::create();
    beachballsroot_->addChild( addReferenceBeachBalls() );
    root_->addChild( beachballsroot_ );
    markersroot_ = vsg::Group::create();
    markersroot_->addChild( addMarkers() );
    root_->addChild( markersroot_ );
    root_->addChild( addLine() );

    const float cubesize = 0.8f;
    if ( vsg::ref_ptr<vsg::Node> label = addLabel( cubesize ) )
	root_->addChild( label );
    else
	qWarning() << "3D label skipped: times.vsgb not found."
		   << "Set VSG_FILE_PATH to plugins/vsgBase/folder_vsg"
		   << "or copy folder_vsg next to the executable.";

    vsg::ref_ptr<vsg::ShaderSet> phong = vsgoptions_->shaderSets["phong"];
    vsg::ref_ptr<vsg::View> view = vsg::View::create( camera_, root_ );
    if ( view->viewDependentState )
    {
	view->viewDependentState->shaderSet = phong;
	view->viewDependentState->maxShadowDistance = 20.0;
	view->viewDependentState->shadowMapBias = 0.001;
    }

    vsg::ref_ptr<vsg::RenderGraph> renderGraph =
			vsg::RenderGraph::create( vsgwindow_, view );
    renderGraph->contents = VK_SUBPASS_CONTENTS_INLINE;

    vsg::ref_ptr<vsg::CommandGraph> commandGraph =
			vsg::CommandGraph::create( vsgwindow_ );
    commandGraph->addChild( renderGraph );

    viewer_->assignRecordAndSubmitTaskAndPresentation( { commandGraph } );

    vsg::ref_ptr<vsg::ResourceHints> hints = vsg::ResourceHints::create();
    hints->numShadowMapsRange = { 1, 8 };
    hints->shadowMapSize = { 4096, 4096 };
    root_->setObject( "ResourceHints", hints );
    viewer_->compile( hints );
    requestFont( fontdata_ );
}


int main( int argc, char** argv )
{
    od_init_test_program( argc, argv, false );
    QApplication app( argc, argv );

    VSGWindow window;
    window.setWindowTitle( "Minimal Qt6 + VSG Embedded Example" );

    auto* mdiarea = new QMdiArea;
    window.setCentralWidget( mdiarea );

    auto* vsgwidget = new VSGWidget;
    auto* container = new QWidget;

    auto* layout = new QVBoxLayout( container );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( vsgwidget );

    container->setLayout( layout );

    auto* subwindow = new QMdiSubWindow;
    subwindow->setWindowTitle( "3D View" );
    subwindow->setWidget( container );
    subwindow->setAttribute( Qt::WA_DeleteOnClose );
    mdiarea->addSubWindow( subwindow );
    subwindow->showMaximized();

    window.setVSGWidget( vsgwidget );
    window.resize( 800, 600 );
    window.show();

    return app.exec();
}
