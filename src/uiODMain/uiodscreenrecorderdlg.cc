/** @file src/uiODMain/uiodscreenrecorderdlg.cc */

/*+
________________________________________________________________________

 Copyright:     (C) 1995-2026 dGB Beheer B.V.
 License:       https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodscreenrecorderdlg.h"

#include "bufstring.h"
#include "envvars.h"
#include "odver.h"
#include "uibutton.h"
#include "uiclipboard.h"
#include "uilabel.h"
#include "uimain.h"
#include "uiodmain.h"
#include "uitextedit.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QScreen>
#include <QSysInfo>
#include <QWindowCapture>

#include <QStringList>
/* For GPU reporting */
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QWindow>
//#include <rhi/qrhi.h>
/* End for GPU reporting */




namespace
{
/*struct GpuDiagnostics
{
    bool valid = false;

    QString backend;
    QString vendor;
    QString renderer;
    QString apiVersion;
    QString shadingLanguageVersion;
    QString contextVersion;
    QString contextProfile;

    int maximumTextureSize = 0;
    bool computeShaders = false;
};*/   

QString openGLString( QOpenGLFunctions* gl, GLenum name )
{
    const GLubyte* value = gl->glGetString( name );

    return value
         ? QString::fromLatin1(
               reinterpret_cast<const char*>(value) )
         : QString();
}

QString yesNo( bool yn )
{
    return yn ? QStringLiteral("yes") : QStringLiteral("no");
}

void addSection( BufferString& report, const QString& title )
{
    if ( !report.isEmpty() )
	report.addNewLine();

    report.add( "== " ).add( title ).add( " ==" ).addNewLine();
}


QString openGLProfileText( QSurfaceFormat::OpenGLContextProfile profile )
{
    switch ( profile )
    {
        case QSurfaceFormat::CoreProfile:
            return QStringLiteral("Core");

        case QSurfaceFormat::CompatibilityProfile:
            return QStringLiteral("Compatibility");

        case QSurfaceFormat::NoProfile:
            return QStringLiteral("No profile");
    }

    return QStringLiteral("Unknown");
}


void addValue( BufferString& report, const char* key, const QString& value )
{
    report.add( key ).add( ": " )
	  .add( value.isEmpty() ? QStringLiteral("<empty>") : value )
	  .addNewLine();
}


void addCurrentOpenGLDiagnostics( BufferString& report,
                                  QOpenGLContext& context,
                                  const QString& contextsource )
{
    QOpenGLFunctions* gl = context.functions();
    if ( !gl )
    {
        addValue( report, "Status",
                  QStringLiteral("No QOpenGLFunctions interface") );
        return;
    }

    const QSurfaceFormat format = context.format();

    addValue( report, "Context source", contextsource );
    addValue( report, "Graphics API",
              context.isOpenGLES()
                  ? QStringLiteral("OpenGL ES")
                  : QStringLiteral("OpenGL") );

    addValue( report, "GPU vendor",
              openGLString(gl,GL_VENDOR) );
    addValue( report, "GPU renderer",
              openGLString(gl,GL_RENDERER) );
    addValue( report, "OpenGL version",
              openGLString(gl,GL_VERSION) );
    addValue( report, "GLSL version",
              openGLString(gl,GL_SHADING_LANGUAGE_VERSION) );

    addValue( report, "Context version",
              QStringLiteral("%1.%2")
                  .arg(format.majorVersion())
                  .arg(format.minorVersion()) );

    addValue( report, "Context profile",
              openGLProfileText(format.profile()) );

    GLint maxtexturesize = 0;
    gl->glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxtexturesize );

    addValue( report, "Maximum 2D texture dimension",
              QString::number(maxtexturesize) );

    const int major = format.majorVersion();
    const int minor = format.minorVersion();

    bool compute = false;

    if ( context.isOpenGLES() )
    {
        // Compute shaders are core in OpenGL ES 3.1.
        compute = major > 3 || (major == 3 && minor >= 1);
    }
    else
    {
        // Compute shaders are core in desktop OpenGL 4.3.
        compute =
            major > 4
            || (major == 4 && minor >= 3)
            || context.hasExtension(
                QByteArrayLiteral("GL_ARB_compute_shader") );
    }

    addValue( report, "Compute shaders", yesNo(compute) );

#ifdef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
    if ( compute )
    {
        GLint maxinvocations = 0;
        gl->glGetIntegerv(
            GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
            &maxinvocations );

        addValue( report,
                  "Maximum compute work-group invocations",
                  QString::number(maxinvocations) );
    }
#endif
}


void addGpuDiagnostics( BufferString& report )
{
    addSection( report, QStringLiteral("Application graphics and GPU") );

#if QT_CONFIG(opengl)

    /*
     * If an OpenGL context happens to be current on this thread,
     * use it directly.
     */
    if ( QOpenGLContext* current = QOpenGLContext::currentContext() )
    {
        addCurrentOpenGLDiagnostics(
            report, *current,
            QStringLiteral("Existing current context") );
        return;
    }

    /*
     * Otherwise create a temporary context for capability queries.
     */
    QScreen* probescreen = nullptr;

    if ( QWindow* focuswindow = QGuiApplication::focusWindow() )
        probescreen = focuswindow->screen();

    if ( !probescreen )
        probescreen = QGuiApplication::primaryScreen();

    QOpenGLContext probecontext;

    if ( probescreen )
        probecontext.setScreen( probescreen );

    /*
     * If Qt has an application-wide share context, sharing with it
     * ties this context to the same OpenGL implementation/share group.
     */
    QOpenGLContext* sharecontext =
        QOpenGLContext::globalShareContext();

    if ( sharecontext )
    {
        probecontext.setShareContext( sharecontext );
        probecontext.setFormat( sharecontext->format() );
    }
    else
    {
        probecontext.setFormat( QSurfaceFormat::defaultFormat() );
    }

    if ( !probecontext.create() )
    {
        addValue( report, "Status",
                  QStringLiteral(
                      "Failed to create temporary OpenGL context") );
        return;
    }

    QOffscreenSurface surface( probescreen );
    surface.setFormat( probecontext.format() );
    surface.create();

    if ( !surface.isValid() )
    {
        addValue( report, "Status",
                  QStringLiteral(
                      "Failed to create OpenGL offscreen surface") );
        return;
    }

    if ( !probecontext.makeCurrent(&surface) )
    {
        addValue( report, "Status",
                  QStringLiteral(
                      "Failed to make temporary OpenGL context current") );
        return;
    }

    addCurrentOpenGLDiagnostics(
        report, probecontext,
        sharecontext
            ? QStringLiteral(
                  "Temporary context sharing with Qt application")
            : QStringLiteral(
                  "Temporary context on active/default screen") );

    probecontext.doneCurrent();

#else

    addValue( report, "Status",
              QStringLiteral(
                  "This Qt build has no OpenGL support") );

#endif
}


/*void addValue( BufferString& report, const char* key, const QString& value )
{
    report.add( key ).add( ": " )
	  .add( value.isEmpty() ? QStringLiteral("<empty>") : value )
	  .addNewLine();
}*/


QString environmentValue( const char* name )
{
    const char* value = GetOSEnvVar( name );
    return value ? QString::fromUtf8(value) : QStringLiteral("<unset>");
}



QString rectangleText( const QRect& rect )
{
    return QStringLiteral("%1,%2  %3 x %4")
	.arg( rect.x() )
	.arg( rect.y() )
	.arg( rect.width() )
	.arg( rect.height() );
}


QString orientationText( Qt::ScreenOrientation orientation )
{
    switch ( orientation )
    {
        case Qt::PrimaryOrientation:
	        return QStringLiteral("Primary");
        case Qt::PortraitOrientation:
	        return QStringLiteral("Portrait");
        case Qt::LandscapeOrientation:
	        return QStringLiteral("Landscape");
        case Qt::InvertedPortraitOrientation:
	        return QStringLiteral("Inverted portrait");
        case Qt::InvertedLandscapeOrientation:
	        return QStringLiteral("Inverted landscape");
    }

    return QStringLiteral("Unknown");
}


bool isVideoContainer( QMediaFormat::FileFormat format )
{
    switch ( format )
    {
        case QMediaFormat::WMV:
        case QMediaFormat::AVI:
        case QMediaFormat::Matroska:
        case QMediaFormat::MPEG4:
        case QMediaFormat::Ogg:
        case QMediaFormat::QuickTime:
        case QMediaFormat::WebM:
	        return true;
        default:
	        return false;
    }
}



BufferString collectDiagnostics()
{
    BufferString report;

    addSection( report, QStringLiteral("Application and Qt") );

    addValue( report, "OpendTect version",
	      QString::fromUtf8(GetFullODVersion()) );
    addValue( report, "Application", QCoreApplication::applicationName() );
    addValue( report, "Executable", QCoreApplication::applicationFilePath() );
    addValue( report, "Qt compile-time version",
	      QStringLiteral(QT_VERSION_STR) );
    addValue( report, "Qt runtime version", QString::fromLatin1(qVersion()) );
    addValue( report, "Active Qt platform plugin",
	      QGuiApplication::platformName() );

   
    //addSection( report, QStringLiteral("Qt RHI and GPU") );
    addGpuDiagnostics( report );    

    /*QRhiDriverInfo info = rhi->driverInfo();

    qInfo() << "Backend:"  << rhi->backendName();
    qInfo() << "Device:"   << info.deviceName;
    qInfo() << "Vendor ID:" << Qt::hex << info.vendorId;
    qInfo() << "Device ID:" << Qt::hex << info.deviceId;
    qInfo() << "Type:"      << info.deviceType;
    qInfo() << "Compute:"<< rhi->isFeatureSupported(QRhi::Compute);

    qInfo() << "Max texture size:"
        << rhi->resourceLimit(QRhi::TextureSizeMax);

    qInfo() << "Max threads/workgroup:"
        << rhi->resourceLimit(QRhi::MaxThreadsPerThreadGroup);*/
          

    addSection( report, QStringLiteral("Qt installation") );

    addValue( report, "Prefix", QLibraryInfo::path(QLibraryInfo::PrefixPath) );
    addValue( report, "Libraries",
	      QLibraryInfo::path(QLibraryInfo::LibrariesPath) );
    addValue( report, "Plugins",
	      QLibraryInfo::path(QLibraryInfo::PluginsPath) );

    const QStringList librarypaths = QCoreApplication::libraryPaths();
    addValue( report, "Runtime plugin search paths",
	      librarypaths.join(QStringLiteral("\n    ")) );

    addSection( report, QStringLiteral("Operating system") );

    addValue( report, "Product", QSysInfo::prettyProductName() );
    addValue( report, "Kernel", QSysInfo::kernelType()
			       + QStringLiteral(" ")
			       + QSysInfo::kernelVersion() );
    addValue( report, "Current CPU architecture",
	      QSysInfo::currentCpuArchitecture() );
    addValue( report, "Build CPU architecture",
	      QSysInfo::buildCpuArchitecture() );
    addValue( report, "Build ABI", QSysInfo::buildAbi() );

    addSection( report, QStringLiteral("Capture-related environment") );

    static const char* environmentnames[] =
    {
	"XDG_SESSION_TYPE",
	"XDG_CURRENT_DESKTOP",
	"XDG_SESSION_DESKTOP",
	"XDG_SESSION_ID",
	"XDG_SEAT",
	"XDG_RUNTIME_DIR",
	"DISPLAY",
	"WAYLAND_DISPLAY",
	"XAUTHORITY",
	"DBUS_SESSION_BUS_ADDRESS",
	"PIPEWIRE_REMOTE",
	"QT_QPA_PLATFORM",
	"QT_QPA_PLATFORMTHEME",
	"QT_PLUGIN_PATH",
	"QT_MEDIA_BACKEND",
	"QT_SCREEN_CAPTURE_BACKEND",
	"QT_WINDOW_CAPTURE_BACKEND",
	"QT_SCALE_FACTOR",
	"QT_SCREEN_SCALE_FACTORS",
	"QT_DEBUG_PLUGINS",
	"QT_LOGGING_RULES"
    };

    for ( const char* name : environmentnames )
	    addValue( report, name, environmentValue(name) );

    const QString qpaplatform = QGuiApplication::platformName();
    const QString xdgsession = environmentValue( "XDG_SESSION_TYPE" );

    if ( qpaplatform == QStringLiteral("xcb")
	 && xdgsession.compare(QStringLiteral("x11"),Qt::CaseInsensitive) != 0 )
    {
	report.addNewLine()
	      .add( "WARNING: Qt is using the XCB/X11 platform plugin, but "
		    "XDG_SESSION_TYPE does not report x11." )
	      .addNewLine()
	      .add( "Qt Multimedia may use XDG_SESSION_TYPE when selecting "
		    "its screen-capture implementation." )
	      .addNewLine();
    }
    else if ( qpaplatform.startsWith(QStringLiteral("wayland"))
	      && xdgsession.compare(QStringLiteral("wayland"),
				    Qt::CaseInsensitive) != 0 )
    {
	report.addNewLine()
	      .add( "WARNING: Qt is using a Wayland platform plugin, but "
		    "XDG_SESSION_TYPE does not report wayland." )
	      .addNewLine();
    }

    addSection( report, QStringLiteral("Screens") );

    uiMain& uimain = uiMain::instance();
    const QList<QScreen*> screens = QGuiApplication::screens();

    addValue( report, "Qt screen count", QString::number(screens.size()) );
    addValue( report, "OD screen count", QString::number(uimain.nrScreens()) );

    for ( int idx=0; idx<screens.size(); idx++ )
    {
	const QScreen* screen = screens[idx];
	const uiSize fullsz = uimain.getScreenSize( idx, false );
	const uiSize availablesz = uimain.getScreenSize( idx, true );

	addSection( report, QStringLiteral("Screen %1: %2")
			    .arg(idx).arg(screen->name()) );

	addValue( report, "Primary",
		  yesNo(screen == QGuiApplication::primaryScreen()) );
	addValue( report, "Manufacturer", screen->manufacturer() );
	addValue( report, "Model", screen->model() );
	addValue( report, "Serial number", screen->serialNumber() );
	addValue( report, "Geometry", rectangleText(screen->geometry()) );
	addValue( report, "Available geometry",
		  rectangleText(screen->availableGeometry()) );
	addValue( report, "Virtual geometry",
		  rectangleText(screen->virtualGeometry()) );
	addValue( report, "OD full size", QStringLiteral("%1 x %2")
					.arg(fullsz.width()).arg(fullsz.height()) );
	addValue( report, "OD available size", QStringLiteral("%1 x %2")
				.arg(availablesz.width())
				.arg(availablesz.height()) );
	addValue( report, "Device pixel ratio",
		  QString::number(screen->devicePixelRatio(),'f',3) );
	addValue( report, "Logical DPI", QStringLiteral("%1 x %2")
				 .arg(screen->logicalDotsPerInchX(),0,'f',2)
				 .arg(screen->logicalDotsPerInchY(),0,'f',2) );
	addValue( report, "Physical DPI", QStringLiteral("%1 x %2")
				  .arg(screen->physicalDotsPerInchX(),0,'f',2)
				  .arg(screen->physicalDotsPerInchY(),0,'f',2) );
	addValue( report, "Refresh rate", QStringLiteral("%1 Hz")
				   .arg(screen->refreshRate(),0,'f',2) );
	addValue( report, "Orientation",
		  orientationText(screen->orientation()) );
	addValue( report, "Color depth",
		  QStringLiteral("%1 bits").arg(screen->depth()) );
    }

    addSection( report, QStringLiteral("Qt Multimedia video capabilities") );

    // This intentionally does not create or query any audio objects.
    QMediaRecorder recorder;
    addValue( report, "QMediaRecorder available",
	      yesNo(recorder.isAvailable()) );

    QMediaFormat capabilities;
    QStringList containernames;
    const auto containers =
	capabilities.supportedFileFormats( QMediaFormat::Encode );
    for ( const QMediaFormat::FileFormat container : containers )
    {
	if ( isVideoContainer(container) )
	    containernames.append( QMediaFormat::fileFormatName(container) );
    }

    addValue( report, "Encodable video containers",
	      containernames.join(QStringLiteral(", ")) );

    QStringList videocodecnames;
    const auto videocodecs =
        capabilities.supportedVideoCodecs( QMediaFormat::Encode );
    for ( const QMediaFormat::VideoCodec codec : videocodecs )
    {
	    videocodecnames.append( QMediaFormat::videoCodecName(codec) );
    }

    addValue( report, "Encodable video codecs",
	      videocodecnames.join(QStringLiteral(", ")) );

    QMediaFormat mp4h264( QMediaFormat::MPEG4 );
    mp4h264.setVideoCodec( QMediaFormat::VideoCodec::H264 );
    mp4h264.setAudioCodec( QMediaFormat::AudioCodec::Unspecified );
    addValue( report, "MPEG-4/H.264 video-only supported",
	      yesNo(mp4h264.isSupported(QMediaFormat::Encode)) );

    const auto windows = QWindowCapture::capturableWindows();
    addValue( report, "Enumerated capturable windows",
	      QString::number(windows.size()) );
    for ( int idx=0; idx<windows.size(); idx++ )
    {
	    report.add( "  Window " ).add( idx ).add( ": " )
	      .add( windows[idx].description() ).addNewLine();
    }

    report.addNewLine()
	  .add( "Note: an empty capturable-window list is diagnostic "
		"information, not by itself proof that screen capture is "
		"unsupported. Wayland may require portal-based selection." )
	  .addNewLine();

    return report;
}


} // End namespace


uiODScreenRecorderDlg::uiODScreenRecorderDlg( uiODMain& appl )
    : uiDialog( &appl, Setup(tr("Screen Recorder Diagnostics"),mNoHelpKey)
				.modal(false) )
{
    setCtrlStyle( CloseOnly );
    setDeleteOnClose( false );

    auto* infolbl = new uiLabel(
	this, tr("Qt screen-capture runtime diagnostics. "
		 "No recording or audio capture is performed.") );

    diagnosticstxt_ =
	new uiTextEdit( this, "Screen capture diagnostics", true );
    diagnosticstxt_->setWordWrapMode( uiTextEdit::NoWrap );
    diagnosticstxt_->setPrefWidthInChar( 110 );
    diagnosticstxt_->setPrefHeightInChar( 34 );
    diagnosticstxt_->allowTextSelection( true );
    diagnosticstxt_->showScrollBar( OD::Horizontal );
    diagnosticstxt_->showScrollBar( OD::Vertical );
    diagnosticstxt_->attach( alignedBelow, infolbl );

    auto* refreshbut =
	new uiPushButton( this, tr("Refresh"),
			  mCB(this,uiODScreenRecorderDlg,refreshCB), true );
    refreshbut->attach( alignedBelow, diagnosticstxt_ );

    auto* copybut =
	new uiPushButton( this, tr("Copy"),
			  mCB(this,uiODScreenRecorderDlg,copyCB), true );
    copybut->attach( rightOf, refreshbut );

    refreshCB( nullptr );
}


uiODScreenRecorderDlg::~uiODScreenRecorderDlg()
{
}


void uiODScreenRecorderDlg::refreshCB( CallBacker* )
{
    diagnosticstxt_->setText( collectDiagnostics() );
}

/** @brief Copies the diagnostic information to the clipboard. */
void uiODScreenRecorderDlg::copyCB( CallBacker* )
{
    uiClipboard::setText( diagnosticstxt_->text() );
}
