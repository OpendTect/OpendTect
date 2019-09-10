/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/

#include "prog.h"
#include "file.h"

# ifdef __msvc__
#  include "winmain.h"
# endif

#include <QApplication>
#include <QFileDialog>

#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>

#include <osg/Version>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgManipulator/TabBoxDragger>

#include <osgDB/ReadFile>
#include "odgraphicswindow.h"

int main( int argc, char** argv )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc, argv );

    BufferString file;
    if ( argc>1 )
	file = argv[1];

    QApplication app(argc, argv);
#if OSG_VERSION_LESS_THAN( 3, 5, 0 )
    osgQt::initQtWindowingSystem();
#endif

    while ( !File::exists(file) )
    {
	file = QFileDialog::getOpenFileName();
	if ( file.isEmpty() )
	    { od_cout() << "Please select an osg file.\n" ; return 1; }
    }

    osg::Node* root = osgDB::readNodeFile( file.buf() );
    if ( !root )
	return ExitProgram( 1 );

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->setSceneData( root );
    viewer->setCameraManipulator( new osgGA::TrackballManipulator );
    setViewer( viewer.get() );

    ODGLWidget* glw = new ODGLWidget;
    ODGraphicsWindow* graphicswin = new ODGraphicsWindow( glw );

    viewer->getCamera()->setViewport(
		    new osg::Viewport(0, 0, glw->width(), glw->height() ) );
    viewer->getCamera()->setGraphicsContext( graphicswin );

    glw->show();

    return ExitProgram( app.exec() );
}

