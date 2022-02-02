/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "uimain.h"
#include "uimainwin.h"
#include "uiosgutil.h"
#include "callback.h"

#include <osgViewer/View>

#include <QOpenGLWidget>
#include <QPointer>
#include <QTimerEvent>

/// The object responsible for the scene re-rendering.
class HeartBeat : public QObject, public CallBacker
{
public:
    virtual			~HeartBeat();

    void			init(osgViewer::ViewerBase*);
    void			initCallbacks(const NotifierAccess&,
					      const NotifierAccess& );
    void			stopTimer();
    void			timerEvent(QTimerEvent*);

    static HeartBeat*		instance();

    int				timerid_	= 0;
    osg::Timer			lastframestarttime_;
    osg::observer_ptr<osgViewer::ViewerBase> viewer_;
    QOpenGLWidget*		glwidget_	= nullptr;

private:
				HeartBeat();

    void			stopTimerCB(CallBacker*);
    void			startTimerCB(CallBacker*);
    bool			needsUpdate();

    static QPointer<HeartBeat>	heartbeat_;
};

QPointer<HeartBeat> HeartBeat::heartbeat_;


void setOSGViewer( osgViewer::ViewerBase* viewer )
{
    HeartBeat::instance()->init( viewer );
}


void setOSGTimerCallbacks( const NotifierAccess& start,
			   const NotifierAccess& stop )
{
//    HeartBeat::instance()->initCallbacks( start, stop );
}


void setOpenGLWidget( QOpenGLWidget* glw )
{
    HeartBeat::instance()->glwidget_ = glw;
}


// HeartBeat
HeartBeat::HeartBeat()
    : timerid_( 0 )
{
    uiMainWin* uimw = uiMain::theMain().topLevel();
    if ( uimw )
	mAttachCB( uimw->windowClosed, HeartBeat::stopTimerCB );
}


HeartBeat::~HeartBeat()
{
    detachAllNotifiers();
    stopTimer();
}


HeartBeat* HeartBeat::instance()
{
    if ( !heartbeat_ )
	heartbeat_ = new HeartBeat();

    return heartbeat_;
}


void HeartBeat::stopTimerCB( CallBacker* )
{
    stopTimer();
}


void HeartBeat::startTimerCB( CallBacker* )
{
    if ( timerid_ == 0	)
    {
	timerid_ = startTimer( 10, Qt::PreciseTimer );
	lastframestarttime_.setStartTick( 0 );
    }
}


void HeartBeat::stopTimer()
{
    if ( timerid_ != 0 )
    {
	killTimer( timerid_ );
	timerid_ = 0;
    }
}


/// Initializes the loop for viewer. Must be called from main thread.
void HeartBeat::init( osgViewer::ViewerBase *viewer )
{
    if ( viewer_ == viewer )
	return;

    stopTimer();

    viewer_ = viewer;

    if ( viewer )
    {
	timerid_ = startTimer( 10, Qt::PreciseTimer );
	lastframestarttime_.setStartTick( 0 );
    }
}


void HeartBeat::initCallbacks( const NotifierAccess& start,
			       const NotifierAccess& stop )
{
    mAttachCB( start, HeartBeat::startTimerCB );
    mAttachCB( stop, HeartBeat::stopTimerCB );
}


bool HeartBeat::needsUpdate()
{
    // check if any event handler has prompted a redraw
    if ( viewer_->getRequestRedraw() )
	return true;

    if ( viewer_->getRequestContinousUpdate() )
	return true;

    std::vector<osgViewer::View*> views;
    viewer_->getViews( views );
    for ( auto* view : views )
    {
	// check if the view needs to update the scene graph
	// this check if camera has update callback and if scene requires to update scene graph
	if ( view->requiresUpdateSceneGraph() )
	    return true;

	// check if the database pager needs to update the scene
	if ( view->getDatabasePager()->requiresUpdateSceneGraph() )
	    return true;

	// check if the image pager needs to update the scene
	if ( view->getImagePager()->requiresUpdateSceneGraph())
	    return true;


	// check if the scene needs to be redrawn
	if ( view->requiresRedraw())
	    return true;
    }

    // check if events are available and need processing
    if ( viewer_->checkEvents())
	return true;

    // and check again if any event handler has prompted a redraw
    if ( viewer_->getRequestRedraw() )
	return true;

    if ( viewer_->getRequestContinousUpdate() )
	return true;

    return false;
}


void HeartBeat::timerEvent( QTimerEvent* )
{
    if ( glwidget_ && needsUpdate() )
	glwidget_->update();
}
