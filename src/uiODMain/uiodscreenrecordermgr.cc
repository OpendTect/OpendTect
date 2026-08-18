/** @file src/uiODMain/uiodscreenrecordermgr.cc */

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodscreenrecordermgr.h"

#include "filepath.h"
#include "oddirs.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodscreenrecorderdlg.h"
#include "uirubberband.h"
#include "uistringset.h"

#include <QByteArray>
#include <QCapturableWindow>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMediaCaptureSession>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QObject>
#include <QPointer>
#include <QScreen>
#include <QScreenCapture>
#include <QTimer>
#include <QUrl>
#include <QWindowCapture>


namespace
{

bool isWaylandCaptureSession()
{
    const QByteArray sessiontype = qgetenv("XDG_SESSION_TYPE").toLower();
    if ( sessiontype == QByteArrayLiteral("wayland") )
	    return true;
    if ( sessiontype == QByteArrayLiteral("x11") )
	    return false;

    return QGuiApplication::platformName().startsWith(
	       QStringLiteral("wayland"), Qt::CaseInsensitive )
	|| !qgetenv("WAYLAND_DISPLAY").isEmpty();
}


void normalizeCaptureSessionType()
{
    /*
     * Qt 6.9 through 6.12 use XDG_SESSION_TYPE to select the Linux
     * screen-capture implementation. Some OD launch environments report
     * "tty" even though the desktop session is X11 or Wayland. Preserve a
     * valid login-session value: the QPA plugin describes how OD renders,
     * but an XCB application may be running through XWayland in a Wayland
     * session. Infer a value only when the inherited one is inconclusive.
     */
    const QByteArray sessiontype = qgetenv("XDG_SESSION_TYPE").toLower();
    if ( ( sessiontype == QByteArrayLiteral("wayland")
      || sessiontype == QByteArrayLiteral("x11") ) )
    {
	    return;
    }

    const QString platformname = QGuiApplication::platformName();
    if ( isWaylandCaptureSession() )
	qputenv( "XDG_SESSION_TYPE", QByteArrayLiteral("wayland") );
    else if ( platformname.compare(QStringLiteral("xcb"),
				    Qt::CaseInsensitive) == 0 )
	qputenv( "XDG_SESSION_TYPE", QByteArrayLiteral("x11") );
}


void selectScreenCaptureBackend()
{
    if ( qEnvironmentVariableIsSet("QT_SCREEN_CAPTURE_BACKEND") )
	return;

    const QByteArray sessiontype = qgetenv("XDG_SESSION_TYPE").toLower();
    const QString platformname = QGuiApplication::platformName();
    if ( sessiontype != QByteArrayLiteral("x11")
      || platformname.compare(QStringLiteral("xcb"),
			      Qt::CaseInsensitive) != 0 )
	return;

    const QList<QScreen*> screens = QGuiApplication::screens();
    if ( screens.size() < 2 )
	return;

    for ( const QScreen* screen : screens )
    {
	if ( screen && screen->virtualSiblings().size() > 1
	  && screen->virtualGeometry() != screen->geometry() )
	{
	    /*
	     * Qt's X11 capture backend treats an XRandR monitor index as an
	     * Xlib screen number. The grabwindow backend uses QScreen geometry
	     * and correctly crops a monitor in an extended desktop.
	     * This must be selected before QScreenCapture is constructed.
	     */
	    qputenv( "QT_SCREEN_CAPTURE_BACKEND",
		     QByteArrayLiteral("grabwindow") );
	    return;
	}
    }
}


QString videoCodecDisplayName( QMediaFormat::VideoCodec codec )
{
    QString name = QMediaFormat::videoCodecDescription( codec );
    if ( name.isEmpty() )
	name = QMediaFormat::videoCodecName( codec );

    return name;
}

} // End namespace


class uiODScreenRecorderMgr::RecorderEngine : public QObject
{
public:

    explicit RecorderEngine(uiODScreenRecorderMgr&);
    ~RecorderEngine() override;

    bool refreshCapabilities(uiStringSet&,uiString&);
    void refreshSources(uiODScreenRecorderDlg::SourceType,uiStringSet&);
    bool start(uiODScreenRecorderDlg::SourceType,int,int,int,
	       const BufferString&,uiString&);
    void stop();
    void finishBeforeExit();

private:

    enum class SourceKind
    {
	None,
	Screen,
	Window
    };

    bool configureRecorder(int,int,const BufferString&,uiString&);
    void sourceActiveChanged(SourceKind,bool);
    void sourceError(SourceKind,const QString&);
    void recorderStateChanged(QMediaRecorder::RecorderState);
    void recorderError(QMediaRecorder::Error,const QString&);
    void recorderDurationChanged(qint64);
    uiString recorderErrorMessage(QMediaRecorder::Error,const QString&) const;
    void fail(const uiString&);
    void finishRun();
    void stopCaptureSource();

    uiODScreenRecorderMgr& owner_;
    QMediaCaptureSession session_;
    QScreenCapture screencapture_;
    QWindowCapture windowcapture_;
    QMediaRecorder recorder_;
    QTimer starttimer_;
    QList<QPointer<QScreen>> screens_;
    QList<QCapturableWindow> windows_;
    QList<QMediaFormat::VideoCodec> availablecodecs_;
    SourceKind sourcekind_ {SourceKind::None};
    QString requestedoutput_;
    QString actualoutput_;
    QString requestedcodecname_;
    uiString pendingerror_;
    qint64 durationms_ {0};
    int targetframerate_ {0};
    bool activejob_ {false};
    bool recorderrequested_ {false};
    bool userstop_ {false};
    bool outputexisted_ {false};
};


uiODScreenRecorderMgr::RecorderEngine::RecorderEngine(
	uiODScreenRecorderMgr& owner )
    : owner_(owner)
{
    session_.setRecorder( &recorder_ );
    recorder_.setAutoStop( true );

    connect( &screencapture_, &QScreenCapture::activeChanged, this,
	[this]( bool active )
	{
	    sourceActiveChanged( SourceKind::Screen, active );
	} );
    connect( &windowcapture_, &QWindowCapture::activeChanged, this,
	[this]( bool active )
	{
	    sourceActiveChanged( SourceKind::Window, active );
	} );
    connect( &screencapture_, &QScreenCapture::errorOccurred, this,
	[this]( QScreenCapture::Error, const QString& errmsg )
	{
	    sourceError( SourceKind::Screen, errmsg );
	} );
    connect( &windowcapture_, &QWindowCapture::errorOccurred, this,
	[this]( QWindowCapture::Error, const QString& errmsg )
	{
	    sourceError( SourceKind::Window, errmsg );
	} );
    connect( &recorder_, &QMediaRecorder::recorderStateChanged, this,
	[this]( QMediaRecorder::RecorderState state )
	{
	    recorderStateChanged( state );
	} );
    connect( &recorder_, &QMediaRecorder::errorOccurred, this,
	[this]( QMediaRecorder::Error error, const QString& errmsg )
	{
	    recorderError( error, errmsg );
	} );
    connect( &recorder_, &QMediaRecorder::durationChanged, this,
	[this]( qint64 duration )
	{
	    recorderDurationChanged( duration );
	} );
    connect( &recorder_, &QMediaRecorder::actualLocationChanged, this,
	[this]( const QUrl& location )
	{
	    if ( activejob_ )
		actualoutput_ = location.toLocalFile();
	} );

    starttimer_.setSingleShot( true );
    starttimer_.setInterval( 120000 );
    connect( &starttimer_, &QTimer::timeout, this,
	[this]()
	{
	    fail( owner_.tr("Screen capture did not start within two minutes.") );
	} );
}


uiODScreenRecorderMgr::RecorderEngine::~RecorderEngine()
{
    activejob_ = false;
    starttimer_.stop();
    if ( recorder_.recorderState() != QMediaRecorder::StoppedState )
	recorder_.stop();

    screencapture_.stop();
    windowcapture_.stop();
    session_.setScreenCapture( nullptr );
    session_.setWindowCapture( nullptr );
    session_.setRecorder( nullptr );
}


bool uiODScreenRecorderMgr::RecorderEngine::refreshCapabilities(
	uiStringSet& codecnames, uiString& unavailablemsg )
{
    codecnames.setEmpty();
    availablecodecs_.clear();
    unavailablemsg.setEmpty();

    if ( !recorder_.isAvailable() )
    {
	unavailablemsg = owner_.tr(
	    "Qt Multimedia has no available video-recording backend.");
	return false;
    }

    QMediaFormat mp4format( QMediaFormat::MPEG4 );
    const QList<QMediaFormat::VideoCodec> advertisedcodecs =
	mp4format.supportedVideoCodecs( QMediaFormat::Encode );
    for ( const QMediaFormat::VideoCodec codec : advertisedcodecs )
    {
	QMediaFormat candidate( QMediaFormat::MPEG4 );
	candidate.setVideoCodec( codec );
	if ( !candidate.isSupported(QMediaFormat::Encode) )
	    continue;

	availablecodecs_.append( codec );
	codecnames.add( toUiString(videoCodecDisplayName(codec)) );
    }

    if ( availablecodecs_.isEmpty() )
    {
	unavailablemsg = owner_.tr(
	    "The active Qt Multimedia backend has no MP4 video encoder.");
	return false;
    }

    return true;
}


void uiODScreenRecorderMgr::RecorderEngine::refreshSources(
	uiODScreenRecorderDlg::SourceType sourcetype,
	uiStringSet& sourcenames )
{
    sourcenames.setEmpty();
    if ( sourcetype == uiODScreenRecorderDlg::SourceType::Screen )
    {
	screens_.clear();
	if ( isWaylandCaptureSession() )
	{
	    screens_.append( QPointer<QScreen>() );
	    sourcenames.add( owner_.tr(
		"Desktop portal selection (chosen after Start)") );
	    return;
	}

	const QList<QScreen*> applicationscreens = QGuiApplication::screens();
	for ( int idx=0; idx<applicationscreens.size(); idx++ )
	{
	    QScreen* screen = applicationscreens[idx];
	    screens_.append( screen );
	    const QRect geometry = screen->geometry();
	    uiString screenname = owner_.tr("%1 (%2 x %3 at %4,%5)")
		.arg( toUiString(screen->name()) )
		.arg( geometry.width() )
		.arg( geometry.height() )
		.arg( geometry.x() )
		.arg( geometry.y() );
	    if ( screen == QGuiApplication::primaryScreen() )
		screenname.appendPhrase( owner_.tr("Primary screen"),
				 uiString::Space, uiString::OnSameLine );
	    sourcenames.add( screenname );
	}
    }
    else
    {
	windows_ = QWindowCapture::capturableWindows();
	for ( int idx=0; idx<windows_.size(); idx++ )
	{
	    const QString description = windows_[idx].description();
	    sourcenames.add( description.isEmpty()
		? owner_.tr("Window %1").arg(idx+1)
		: toUiString(description) );
	}
    }
}


bool uiODScreenRecorderMgr::RecorderEngine::configureRecorder(
	int codecidx, int targetframerate, const BufferString& outputfnm,
	uiString& errmsg )
{
    if ( !recorder_.isAvailable() )
    {
	errmsg = owner_.tr(
	    "Qt Multimedia has no available video-recording backend.");
	return false;
    }

    if ( targetframerate < 0 || targetframerate > 240 )
    {
	errmsg = owner_.tr("The selected target frame rate is invalid.");
	return false;
    }

    if ( codecidx < -1 || codecidx >= availablecodecs_.size() )
    {
	errmsg = owner_.tr("The selected video codec is no longer available.");
	return false;
    }

    QMediaFormat format( QMediaFormat::MPEG4 );
    QMediaFormat::VideoCodec videocodec =
	QMediaFormat::VideoCodec::Unspecified;
    if ( codecidx >= 0 )
    {
	videocodec = availablecodecs_[codecidx];
	format.setVideoCodec( videocodec );
	if ( !format.isSupported(QMediaFormat::Encode) )
	{
	    errmsg = owner_.tr(
		"The selected codec '%1' is no longer available for MP4 "
		"encoding.")
		.arg( toUiString(videoCodecDisplayName(videocodec)) );
	    return false;
	}
    }

    /*
     * In Automatic mode the codec remains unspecified so the active Qt
     * Multimedia backend can select an MP4-compatible encoder. A codec
     * explicitly chosen by the user is never silently replaced.
     */
    if ( codecidx < 0 )
	owner_.tr("Automatic (Qt chooses)").fillQString(
		requestedcodecname_ );
    else
	requestedcodecname_ = videoCodecDisplayName( videocodec );
    targetframerate_ = targetframerate;
    requestedoutput_ = QString::fromUtf8( outputfnm.buf() );
    outputexisted_ = QFileInfo::exists( requestedoutput_ );
    recorder_.setMediaFormat( format );
    // Set quality explicitly.
    //recorder_.setQuality( QMediaRecorder::HighQuality );
    recorder_.setEncodingMode( QMediaRecorder::ConstantQualityEncoding );
    recorder_.setQuality( videocodec == QMediaFormat::VideoCodec::MPEG4
	? QMediaRecorder::VeryHighQuality
	: QMediaRecorder::HighQuality );
    recorder_.setVideoFrameRate(
	targetframerate > 0 ? static_cast<qreal>(targetframerate) : 0.0 );

    // Qt 6.12 can also request the target rate from the capture source.
#if QT_VERSION >= QT_VERSION_CHECK(6,12,0)
    if ( targetframerate > 0 )
    {
	screencapture_.setFrameRate( targetframerate );
	windowcapture_.setFrameRate( targetframerate );
    }
    else
    {
	screencapture_.resetFrameRate();
	windowcapture_.resetFrameRate();
    }
#endif

    recorder_.setOutputLocation( QUrl::fromLocalFile(requestedoutput_) );
    return true;
}


bool uiODScreenRecorderMgr::RecorderEngine::start(
	uiODScreenRecorderDlg::SourceType sourcetype, int sourceidx,
	int codecidx, int targetframerate, const BufferString& outputfnm,
	uiString& errmsg )
{
    if ( activejob_ )
    {
	errmsg = owner_.tr("A screen recording is already in progress.");
	return false;
    }

    if ( !configureRecorder(codecidx,targetframerate,outputfnm,errmsg) )
	return false;

    session_.setScreenCapture( nullptr );
    session_.setWindowCapture( nullptr );
    sourcekind_ = SourceKind::None;

    if ( sourcetype == uiODScreenRecorderDlg::SourceType::Screen )
    {
	if ( sourceidx < 0 || sourceidx >= screens_.size() )
	{
	    errmsg = owner_.tr("The selected screen is no longer available.");
	    return false;
	}

	QScreen* screen = screens_[sourceidx].data();
	if ( !isWaylandCaptureSession() && !screen )
	{
	    errmsg = owner_.tr("The selected screen is no longer available.");
	    return false;
	}

	screencapture_.setScreen(
		isWaylandCaptureSession() ? nullptr : screen );
	session_.setScreenCapture( &screencapture_ );
	sourcekind_ = SourceKind::Screen;
    }
    else
    {
	if ( sourceidx < 0 || sourceidx >= windows_.size()
	  || !windows_[sourceidx].isValid() )
	{
	    errmsg = owner_.tr("The selected window is no longer available.");
	    return false;
	}

	windowcapture_.setWindow( windows_[sourceidx] );
	session_.setWindowCapture( &windowcapture_ );
	sourcekind_ = SourceKind::Window;
    }

    pendingerror_.setEmpty();
    actualoutput_.clear();
    durationms_ = 0;
    recorderrequested_ = false;
    userstop_ = false;
    activejob_ = true;
    starttimer_.start();

    if ( sourcekind_ == SourceKind::Screen )
	screencapture_.start();
    else
	windowcapture_.start();

    return activejob_;
}


void uiODScreenRecorderMgr::RecorderEngine::sourceActiveChanged(
	SourceKind sourcekind, bool active )
{
    if ( !activejob_ || sourcekind != sourcekind_ )
	return;

    if ( active )
    {
	if ( !recorderrequested_ && !userstop_ )
	{
	    recorderrequested_ = true;
	    recorder_.record();
	    if ( recorder_.error() != QMediaRecorder::NoError )
		recorderError( recorder_.error(), recorder_.errorString() );
	}
	return;
    }

    if ( !userstop_ && pendingerror_.isEmpty() )
	fail( owner_.tr("The screen-capture source stopped unexpectedly.") );
}


void uiODScreenRecorderMgr::RecorderEngine::sourceError(
	SourceKind sourcekind, const QString& errmsg )
{
    if ( !activejob_ || sourcekind != sourcekind_ )
	return;

    fail( errmsg.isEmpty()
	? owner_.tr("The screen-capture source reported an error.")
	: owner_.tr("Screen capture failed: %1").arg(toUiString(errmsg)) );
}


void uiODScreenRecorderMgr::RecorderEngine::recorderStateChanged(
	QMediaRecorder::RecorderState state )
{
    if ( !activejob_ || state != QMediaRecorder::StoppedState )
	return;

    if ( !userstop_ && pendingerror_.isEmpty() )
    {
	const QString recordererrmsg = recorder_.errorString();
	pendingerror_ = recorder_.error() != QMediaRecorder::NoError
	    ? recorderErrorMessage( recorder_.error(), recordererrmsg )
	    : owner_.tr("The media recorder stopped unexpectedly.");
    }

    finishRun();
}


void uiODScreenRecorderMgr::RecorderEngine::recorderError(
	QMediaRecorder::Error error, const QString& errmsg )
{
    if ( !activejob_ )
	return;

    fail( recorderErrorMessage(error,errmsg) );
}


uiString uiODScreenRecorderMgr::RecorderEngine::recorderErrorMessage(
	QMediaRecorder::Error error, const QString& details ) const
{
    uiString reason;
    switch ( error )
    {
    case QMediaRecorder::FormatError:
	reason = owner_.tr("The selected format or codec could not be used.");
	break;
    case QMediaRecorder::ResourceError:
	reason = owner_.tr("The video encoder or another recording resource "
			   "could not be initialized.");
	break;
    case QMediaRecorder::OutOfSpaceError:
	reason = owner_.tr("There is not enough space to continue recording.");
	break;
    case QMediaRecorder::LocationNotWritable:
	reason = owner_.tr("The output location is not writable.");
	break;
    case QMediaRecorder::NoError:
	reason = owner_.tr("The media recorder reported an error.");
	break;
    }

    const uiString codecname = requestedcodecname_.isEmpty()
	? owner_.tr("the selected codec")
	: toUiString(requestedcodecname_);
    const uiString frameratetext = targetframerate_ > 0
	? owner_.tr("%1 fps").arg(targetframerate_)
	: owner_.tr("automatic frame rate");
    uiString message = owner_.tr(
	"Video recording with %1 at %2 failed. %3")
	.arg( codecname ).arg( frameratetext ).arg( reason );
    if ( !details.isEmpty() )
	message.appendPhrase( toUiString(details), uiString::Space,
			      uiString::OnNewLine );

    message.appendPhrase(
	owner_.tr("Try Automatic, another codec, or a lower frame rate."),
	uiString::Space, uiString::OnNewLine );
    return message;
}


void uiODScreenRecorderMgr::RecorderEngine::recorderDurationChanged(
	qint64 duration )
{
    if ( !activejob_ )
	return;

    durationms_ = duration;
    if ( duration > 0 )
	starttimer_.stop();

    owner_.durationChanged( duration );
}


void uiODScreenRecorderMgr::RecorderEngine::stop()
{
    if ( !activejob_ || userstop_ )
	return;

    userstop_ = true;
    starttimer_.stop();
    if ( recorder_.recorderState() != QMediaRecorder::StoppedState )
	recorder_.stop();
    else
	finishRun();
}


void uiODScreenRecorderMgr::RecorderEngine::fail( const uiString& errmsg )
{
    if ( !activejob_ )
	return;

    if ( pendingerror_.isEmpty() )
	pendingerror_ = errmsg;

    starttimer_.stop();
    if ( recorder_.recorderState() != QMediaRecorder::StoppedState )
    {
	owner_.setState( uiODScreenRecorderMgr::RecorderState::Finalizing,
			 owner_.tr("Finalizing the recording after an error...") );
	recorder_.stop();
    }
    else
	finishRun();
}


void uiODScreenRecorderMgr::RecorderEngine::stopCaptureSource()
{
    if ( sourcekind_ == SourceKind::Screen )
	screencapture_.stop();
    else if ( sourcekind_ == SourceKind::Window )
	windowcapture_.stop();

    session_.setScreenCapture( nullptr );
    session_.setWindowCapture( nullptr );
    sourcekind_ = SourceKind::None;
}


void uiODScreenRecorderMgr::RecorderEngine::finishRun()
{
    if ( !activejob_ )
	return;

    starttimer_.stop();
    const bool cancelled = userstop_ && durationms_ <= 0
			&& pendingerror_.isEmpty();
    QString outputfnm = actualoutput_;
    if ( outputfnm.isEmpty() )
	outputfnm = requestedoutput_;

    if ( pendingerror_.isEmpty() && !cancelled )
    {
	const QFileInfo outputinfo( outputfnm );
	if ( !outputinfo.exists() || outputinfo.size() <= 0 )
	    pendingerror_ = owner_.tr(
		"The recording stopped, but no video was written to '%1'.")
		.arg( toUiString(outputfnm) );
    }
    else if ( cancelled && !outputexisted_ )
    {
	const QFileInfo outputinfo( outputfnm );
	if ( outputinfo.exists() && outputinfo.size() == 0 )
	    QFile::remove( outputfnm );
    }

    activejob_ = false;
    stopCaptureSource();

    const uiString finalerror = pendingerror_;
    pendingerror_.setEmpty();
    requestedoutput_.clear();
    actualoutput_.clear();
    requestedcodecname_.clear();
    durationms_ = 0;
    targetframerate_ = 0;
    recorderrequested_ = false;
    userstop_ = false;
    outputexisted_ = false;

    if ( !finalerror.isEmpty() )
	owner_.recordingFailed( finalerror );
    else if ( cancelled )
	owner_.recordingCancelled();
    else
	owner_.recordingDone( BufferString(outputfnm.toUtf8().constData()) );
}


void uiODScreenRecorderMgr::RecorderEngine::finishBeforeExit()
{
    if ( !activejob_ )
	return;

    stop();
    if ( !activejob_ )
	return;

    QEventLoop eventloop;
    QTimer timeout;
    timeout.setSingleShot( true );
    connect( &timeout, &QTimer::timeout, &eventloop, &QEventLoop::quit );
    connect( &recorder_, &QMediaRecorder::recorderStateChanged, &eventloop,
	[&eventloop]( QMediaRecorder::RecorderState state )
	{
	    if ( state == QMediaRecorder::StoppedState )
		eventloop.quit();
	} );
    timeout.start( 10000 );
    eventloop.exec( QEventLoop::ExcludeUserInputEvents );

    if ( activejob_ )
    {
	// Do not hold OD shutdown indefinitely if a backend never reports Stopped.
	activejob_ = false;
	starttimer_.stop();
	stopCaptureSource();
	pendingerror_.setEmpty();
	requestedoutput_.clear();
	actualoutput_.clear();
	requestedcodecname_.clear();
	durationms_ = 0;
	targetframerate_ = 0;
	recorderrequested_ = false;
	userstop_ = false;
	outputexisted_ = false;
    }
}


uiODScreenRecorderMgr::uiODScreenRecorderMgr( uiODMain& appl )
    : stateChanged(this)
    , recordingFinished(this)
    , appl_(appl)
{
    normalizeCaptureSessionType();
    selectScreenCaptureBackend();
    engine_ = new RecorderEngine( *this );
    mAttachCB( appl_.beforeExit, uiODScreenRecorderMgr::beforeExitCB );
}


uiODScreenRecorderMgr::~uiODScreenRecorderMgr()
{
    detachAllNotifiers();
    finishBeforeExit();
    delete engine_;
    delete dialog_;
}


void uiODScreenRecorderMgr::toggleRecording()
{
    /*if ( !dialog_ )
	dialog_ = new uiODScreenRecorderDlg( appl_ );

    dialog_->show();*/
    if ( state_ == RecorderState::Idle )
    {
        //setState(RecorderState::Starting, tr("Starting screen recording..."));
        showDialog();
    }
    else if ( state_ == RecorderState::Recording
	   || state_ == RecorderState::Starting )
    {
	stopCB( nullptr );
    }
    else
	showDialog();
}


void uiODScreenRecorderMgr::showDialog()
{
    if ( !dialog_ )
    {
	dialog_ = new uiODScreenRecorderDlg( appl_ );
	mAttachCB( dialog_->startRequested, uiODScreenRecorderMgr::startCB );
	mAttachCB( dialog_->stopRequested, uiODScreenRecorderMgr::stopCB );
	mAttachCB( dialog_->refreshSourcesRequested,
		   uiODScreenRecorderMgr::refreshSourcesCB );
	mAttachCB( dialog_->sourceTypeChanged,
		   uiODScreenRecorderMgr::sourceTypeChangedCB );
	refreshSourcesCB( nullptr );
    }

    dialog_->show();
    dialog_->raise();
}


void uiODScreenRecorderMgr::setState( RecorderState state,
	const uiString& status )
{
    state_ = state;
    if ( dialog_ && !exiting_ )
    {
	switch ( state_ )
	{
	case RecorderState::Idle:
	    dialog_->setIdle( status );
	    break;
	case RecorderState::Starting:
	    dialog_->setStarting( status );
	    break;
	case RecorderState::Recording:
	    dialog_->setRecording( status );
	    break;
	case RecorderState::Finalizing:
	    dialog_->setFinalizing( status );
	    break;
	}
    }

    if ( !exiting_ )
	stateChanged.trigger();
}


void uiODScreenRecorderMgr::startCB( CallBacker* )
{
    if ( !dialog_ || state_ != RecorderState::Idle )
	return;

    BufferString outputfnm = dialog_->outputFileName();
    if ( outputfnm.isEmpty() )
    {
	uiMSG().error( tr("Please select an output file.") );
	return;
    }

    QString qoutputfnm = QString::fromUtf8( outputfnm.buf() );
    QFileInfo outputinfo( qoutputfnm );
    if ( outputinfo.suffix().isEmpty() )
    {
	qoutputfnm += QStringLiteral(".mp4");
	outputfnm = qoutputfnm.toUtf8().constData();
	dialog_->setOutputFileName( outputfnm.buf() );
	outputinfo.setFile( qoutputfnm );
    }
    else if ( outputinfo.suffix().compare(QStringLiteral("mp4"),
					   Qt::CaseInsensitive) != 0 )
    {
	uiMSG().error( tr("The output file must have the .mp4 extension.") );
	return;
    }

    const QFileInfo outputdirinfo( outputinfo.absolutePath() );
    if ( !outputdirinfo.exists() || !outputdirinfo.isDir()
	 || !outputdirinfo.isWritable() )
    {
	uiMSG().error( tr("Cannot write to output directory '%1'.")
		       .arg(toUiString(outputinfo.absolutePath())) );
	return;
    }

    if ( outputinfo.exists() )
    {
	if ( !outputinfo.isWritable() )
	{
	    uiMSG().error( tr("Cannot write to file '%1'.")
			   .arg(toUiString(qoutputfnm)) );
	    return;
	}

	if ( !uiMSG().askOverwrite(
		tr("File '%1' already exists, overwrite?")
		.arg(toUiString(qoutputfnm))) )
	    return;
    }

    lastoutputfile_.setEmpty();
    setState( RecorderState::Starting, tr("Starting screen recording...") );
    uiString errmsg;
    if ( !engine_->start(dialog_->sourceType(),dialog_->sourceIndex(),
			 dialog_->codecIndex(),dialog_->targetFrameRate(),
			 outputfnm,errmsg) )
    {
	if ( state_ != RecorderState::Idle )
	    recordingFailed( errmsg );
	return;
    }
    else
    {
	const BufferString outputdir = FilePath(outputfnm).pathOnly();
	SetPicturesDir( outputdir.buf() );
    }
}


void uiODScreenRecorderMgr::stopCB( CallBacker* )
{
    if ( state_ != RecorderState::Starting
	 && state_ != RecorderState::Recording )
	return;

    setState( RecorderState::Finalizing, tr("Finalizing recording...") );
    engine_->stop();
}


void uiODScreenRecorderMgr::refreshSourcesCB( CallBacker* )
{
    if ( !dialog_ || state_ != RecorderState::Idle )
	return;

    uiStringSet codecnames;
    uiString capabilityerrmsg;
    const bool recorderavailable =
	engine_->refreshCapabilities( codecnames, capabilityerrmsg );
    dialog_->setCodecOptions( codecnames );
    dialog_->setRecorderAvailability( recorderavailable,
				      capabilityerrmsg );

    uiStringSet sourcenames;
    engine_->refreshSources( dialog_->sourceType(), sourcenames );
    dialog_->setSources( sourcenames );
    if ( !recorderavailable )
	dialog_->setIdle( capabilityerrmsg );
    else if ( sourcenames.isEmpty() )
    {
	const uiString msg = dialog_->sourceType()
	    == uiODScreenRecorderDlg::SourceType::Window
	    ? tr("No capturable windows are available on this platform/session.")
	    : tr("No capturable screens are available.");
	dialog_->setIdle( msg );
    }
    else
	dialog_->setIdle( tr("Ready") );
}


void uiODScreenRecorderMgr::sourceTypeChangedCB( CallBacker* )
{
    refreshSourcesCB( nullptr );
}


void uiODScreenRecorderMgr::beforeExitCB( CallBacker* )
{
    finishBeforeExit();
}


void uiODScreenRecorderMgr::finishBeforeExit()
{
    if ( !engine_ || state_ == RecorderState::Idle )
	return;

    if ( state_ != RecorderState::Finalizing )
	setState( RecorderState::Finalizing, tr("Finalizing recording...") );
    exiting_ = true;
    engine_->finishBeforeExit();
    state_ = RecorderState::Idle;
}


void uiODScreenRecorderMgr::recordingStarted()
{
    if ( state_ == RecorderState::Starting )
	setState( RecorderState::Recording, tr("Recording 00:00:00") );
}


void uiODScreenRecorderMgr::recordingDone( const BufferString& outputfnm )
{
    lastoutputfile_ = outputfnm;
    setState( RecorderState::Idle,
	      tr("Saved recording to '%1'.").arg(outputfnm) );
    if ( !exiting_ )
	recordingFinished.trigger();
}


void uiODScreenRecorderMgr::recordingFailed( const uiString& errmsg )
{
    setState( RecorderState::Idle, tr("Recording failed") );
    if ( !exiting_ )
	uiMSG().error( errmsg );
}


void uiODScreenRecorderMgr::recordingCancelled()
{
    setState( RecorderState::Idle, tr("Recording cancelled") );
}


void uiODScreenRecorderMgr::durationChanged( od_int64 durationms )
{
    if ( durationms <= 0 )
	return;

    recordingStarted();
    if ( !dialog_ || state_ != RecorderState::Recording )
	return;

    const od_int64 totalseconds = durationms / 1000;
    const od_int64 hours = totalseconds / 3600;
    const od_int64 minutes = (totalseconds / 60) % 60;
    const od_int64 seconds = totalseconds % 60;
    const QString durationtxt = QStringLiteral("%1:%2:%3")
	.arg( hours, 2, 10, QLatin1Char('0') )
	.arg( minutes, 2, 10, QLatin1Char('0') )
	.arg( seconds, 2, 10, QLatin1Char('0') );
    dialog_->setRecording(
	tr("Recording %1").arg(toUiString(durationtxt)) );
}
