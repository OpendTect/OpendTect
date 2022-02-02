#pragma once
/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "uiosgmod.h"

#include "commondefs.h"

#include <osgViewer/GraphicsWindow>
#include <OpenThreads/ReadWriteMutex>
#include <QOpenGLWidget>

namespace osgViewer { class ViewerBase; }
namespace osgGA { class EventQueue; }
class QInputEvent;


mClass(uiOSG) ODOpenGLWidget : public QOpenGLWidget
{
friend class ui3DViewerBody;
public:
			ODOpenGLWidget(QWidget* parent=nullptr,
				       Qt::WindowFlags f=Qt::WindowFlags());
			~ODOpenGLWidget();

protected:

    void		initializeGL() override;
    void		paintGL() override;
    void		resizeGL(int w,int h) override;

    void		setKeyboardModifiers(QInputEvent*);

    void		keyPressEvent(QKeyEvent*) override;
    void		keyReleaseEvent(QKeyEvent*) override;
    void		mousePressEvent(QMouseEvent*) override;
    void		mouseReleaseEvent(QMouseEvent*) override;
    void		mouseDoubleClickEvent(QMouseEvent*) override;
    void		mouseMoveEvent(QMouseEvent*) override;
    void		wheelEvent(QWheelEvent*) override;

    osgGA::EventQueue*	getEventQueue() const;
    osgViewer::GraphicsWindowEmbedded*
			getGraphicsWindow()	{ return graphicswindow_; }
    void		setViewer(osgViewer::ViewerBase*);

private:
    osgViewer::GraphicsWindowEmbedded*
				graphicswindow_;
    osgViewer::ViewerBase*	viewer_		= nullptr;
    OpenThreads::ReadWriteMutex mutex_;

    bool			isfirstframe_	= true;
    double			scalex_		= 1;
    double			scaley_		= 1;
};

