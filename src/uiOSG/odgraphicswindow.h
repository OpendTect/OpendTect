#pragma once
/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009 Wang Rui
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.	The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/


#include "uiosgmod.h"
#include "commondefs.h"
#include <osgViewer/GraphicsWindow>

#include <QMutex>
#include <QEvent>
#include <QQueue>
#include <QSet>
#include <QGLWidget>
#include <osg/Version>

class QInputEvent;
class QGestureEvent;

namespace osgViewer
{
    class ViewerBase;
}

class ODGraphicsWindow;

#if OSG_VERSION_LESS_THAN(3, 5, 6)
/// The function sets the WindowingSystem to Qt.
void mGlobal(uiOSG) initQtWindowingSystem();
#endif

/** The function sets the viewer that will be used after entering
 *  the Qt main loop (QCoreApplication::exec()).
 *
 *  The function also initializes internal structures required for proper
 *  scene rendering.
 *
 *  The method must be called from main thread. */
void mGlobal(uiOSG) setViewer( osgViewer::ViewerBase *viewer );


mExpClass(uiOSG) ODGLWidget : public QGLWidget
{
    typedef QGLWidget inherited;

public:

				ODGLWidget(QWidget* parent=nullptr,
					   const QGLWidget* shareWidget=nullptr,
					   Qt::WindowFlags f=Qt::WindowFlags(),
					   bool forwardKeyEvents=false);
				ODGLWidget(QGLContext* context,
					   QWidget* parent=nullptr,
					   const QGLWidget* shareWidget=nullptr,
					   Qt::WindowFlags f=Qt::WindowFlags(),
					   bool forwardKeyEvents=false);
				ODGLWidget(const QGLFormat& format,
					   QWidget* parent=nullptr,
					   const QGLWidget* shareWidget=nullptr,
					   Qt::WindowFlags f=Qt::WindowFlags(),
					   bool forwardKeyEvents=false);
    virtual			~ODGLWidget();

    inline void			setGraphicsWindow( ODGraphicsWindow* gw )
				{ _gw = gw; }
    inline ODGraphicsWindow*	getGraphicsWindow()		{ return _gw; }
    inline const ODGraphicsWindow* getGraphicsWindow() const	{ return _gw; }

    inline bool			getForwardKeyEvents() const
				{ return _forwardKeyEvents; }
    virtual void		setForwardKeyEvents( bool yn )
				{ _forwardKeyEvents = yn; }

    inline bool			getTouchEventsEnabled() const
				{ return _touchEventsEnabled; }
    void			setTouchEventsEnabled(bool yn);

    void			setKeyboardModifiers(QInputEvent*);

    void			keyPressEvent(QKeyEvent*) override;
    void			keyReleaseEvent(QKeyEvent*) override;
    void			mousePressEvent(QMouseEvent*) override;
    void			mouseReleaseEvent(QMouseEvent*) override;
    void			mouseDoubleClickEvent(QMouseEvent*) override;
    void			mouseMoveEvent(QMouseEvent*) override;
    void			wheelEvent(QWheelEvent*) override;
    virtual bool		gestureEvent(QGestureEvent*);

protected:

    int getNumDeferredEvents()
    {
	QMutexLocker lock(&_deferredEventQueueMutex);
	return _deferredEventQueue.count();
    }
    void enqueueDeferredEvent( QEvent::Type eventType,
			       QEvent::Type removeEventType = QEvent::None )
    {
	QMutexLocker lock(&_deferredEventQueueMutex);

	if (removeEventType != QEvent::None)
	{
	    if (_deferredEventQueue.removeOne(removeEventType))
		_eventCompressor.remove(eventType);
	}

	if (_eventCompressor.find(eventType) == _eventCompressor.end())
	{
	    _deferredEventQueue.enqueue(eventType);
	    _eventCompressor.insert(eventType);
	}
    }
    void processDeferredEvents();

    friend class ODGraphicsWindow;
    ODGraphicsWindow*		_gw;

    QMutex			_deferredEventQueueMutex;
    QQueue<QEvent::Type>	_deferredEventQueue;
    QSet<QEvent::Type>		_eventCompressor;

    bool			_touchEventsEnabled;

    bool			_forwardKeyEvents;
    qreal			_devicePixelRatio;

    void			resizeEvent(QResizeEvent*) override;
    void			moveEvent(QMoveEvent*) override;
    void			glDraw() override;
    bool			event(QEvent*) override;
};


mExpClass(uiOSG) ODGraphicsWindow : public osgViewer::GraphicsWindow
{
public:
				ODGraphicsWindow(osg::GraphicsContext::Traits*,
						 QWidget* parent=nullptr,
						 const QGLWidget* shareWidget=0,
					 Qt::WindowFlags f=Qt::WindowFlags());
				ODGraphicsWindow(ODGLWidget* widget);
    virtual			~ODGraphicsWindow();

    inline ODGLWidget*		getGLWidget()		{ return _widget; }
    inline const ODGLWidget*	getGLWidget() const	{ return _widget; }

    struct WindowData : public osg::Referenced
    {
	WindowData( ODGLWidget* widget = nullptr, QWidget* parent = nullptr )
	    : _widget(widget), _parent(parent) {}

	ODGLWidget* _widget;
	QWidget* _parent;
    };

    bool			init(QWidget* parent,
				     const QGLWidget* shareWidget,
				     Qt::WindowFlags f );

    static QGLFormat		traits2qglFormat(
					const osg::GraphicsContext::Traits*);
    static void			qglFormat2traits(const QGLFormat& format,
						 osg::GraphicsContext::Traits*);
    static osg::GraphicsContext::Traits* createTraits(const QGLWidget* widget);

    bool			setWindowRectangleImplementation(int x,int y,
						int width,int height) override;
    void			getWindowRectangle(int& x,int& y,int& width,
						   int& height) override;
    bool			setWindowDecorationImplementation(
							bool yn) override;
    bool			getWindowDecoration() const override;
    void			grabFocus() override;
    void			grabFocusIfPointerInWindow() override;
    void			raiseWindow() override;
    void			setWindowName(const std::string&) override;
    std::string			getWindowName() override;
    void			useCursor(bool cursoron) override;
    void			setCursor(MouseCursor) override;
    inline bool			getTouchEventsEnabled() const
				{ return _widget->getTouchEventsEnabled(); }
    virtual void		setTouchEventsEnabled( bool yn )
				{ _widget->setTouchEventsEnabled(yn); }

    bool			valid() const override;
    bool			realizeImplementation() override;
    bool			isRealizedImplementation() const override;
    void			closeImplementation() override;
    bool			makeCurrentImplementation() override;
    bool			releaseContextImplementation() override;
    void			swapBuffersImplementation() override;
    void			runOperations() override;

    void			requestWarpPointer(float x,float y) override;

protected:

    friend class	ODGLWidget;
    ODGLWidget*		_widget;
    bool		_ownsWidget;
    QCursor		_currentCursor;
    bool		_realized;
};
