/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odgraphicswindow.h"

#include "uimain.h"

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "prog.h"

#include <QApplication>
#include <QFileDialog>

#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Version>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgManipulator/TabBoxDragger>
#include <osgViewer/Viewer>

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain::preInitForOpenGL();
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "uiTools" );

    const CommandLineParser clp( argc, argv );
    BufferStringSet files;
    clp.getNormalArguments( files );
    BufferString file;
    if ( !files.isEmpty() )
	file = files.first();

#if OSG_VERSION_LESS_THAN( 3, 5, 0 )
    initQtWindowingSystem();
#endif

    while ( !File::exists(file) )
    {
	file = QFileDialog::getOpenFileName();
	if ( file.isEmpty() )
	    { od_cout() << "Please select an osg file.\n" ; return 1; }
    }

    osg::Node* root = osgDB::readNodeFile( file.buf() );
    if ( !root )
	return 1;

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->setSceneData( root );
    viewer->setCameraManipulator( new osgGA::TrackballManipulator );
    setViewer( viewer.get() );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiOSG" );
    PtrMan<ODGLWidget> topdlg = new ODGLWidget;
    PtrMan<ODGraphicsWindow> graphicswin = new ODGraphicsWindow( topdlg );
    viewer->getCamera()->setViewport(
		    new osg::Viewport(0, 0, topdlg->width(), topdlg->height()));
    viewer->getCamera()->setGraphicsContext( graphicswin );
    PIM().loadAuto( true );
    topdlg->show();

    return app.exec();
}
