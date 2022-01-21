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
#include <QOpenGLWidget>

namespace osgViewer { class Viewer; }
namespace osgGA { class EventQueue; }


mClass(uiOSG) ODGraphicsWindow2 : public osgViewer::GraphicsWindow
{
public:
			ODGraphicsWindow2();
			~ODGraphicsWindow2();

protected:
};



mClass(uiOSG) ODOpenGLWidget : public QOpenGLWidget
{
public:
			ODOpenGLWidget(QWidget* parent=nullptr,
				       Qt::WindowFlags f=Qt::WindowFlags());
			~ODOpenGLWidget();
protected:

    void		initializeGL() override;
    void		paintGL() override;
    void		resizeGL(int w,int h) override;

    osgGA::EventQueue*	getEventQueue() const;

    ODGraphicsWindow2*	graphicswindow_;
    osgViewer::Viewer*	viewer_;

    double		scalex_		= 1;
    double		scaley_		= 1;
};

