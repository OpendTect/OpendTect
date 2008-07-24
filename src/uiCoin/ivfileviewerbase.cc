/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/

#include <VolumeViz/nodes/SoVolumeRendering.h>

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/SoInput.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/nodes/SoSeparator.h>

#include "filegen.h"
#include "uifiledlg.h"

#ifdef USESOODCLASSES
#include "initsood.h"
#endif
int main( int narg, char** argv )
{
#ifdef USEQT3
    QWidget* myWindow = SoQt::init( argv[0] );
#else
    QWidget* myWindow = SoQt::init( narg, argv, argv[0] );
#endif

#ifdef USESOODCLASSES
    SoOD::initStdClasses();
#endif

    if ( myWindow==NULL ) return 1;

    BufferString filename;
    if ( narg==2 )
	filename = argv[1];

    while ( filename.isEmpty() || !File_exists( filename.buf() ) )
    {
	uiFileDialog dlg( myWindow, uiFileDialog::ExistingFile, true, 0,
			  "*.iv", "Select file to view" );
	if ( !dlg.go() )
	    return 1;

	filename = dlg.fileName();
    }

    myViewer->setTitle( filename.buf() );

    SoInput mySceneInput;

    //SoInput::addDirectoryFirst( "." ); // Add additional directories.
    SbStringList dirlist = SoInput::getDirectories();
    for ( int idx=0; idx<dirlist.getLength(); idx++ )
	printf( "Looking for \"%s\" in %s\n", argv[1],
		dirlist[idx]->getString() );

    if ( !mySceneInput.openFile( argv[1] ))
	return 1;

    SoSeparator* myGraph = SoDB::readAll(&mySceneInput);
    if ( !myGraph ) return 1;
    mySceneInput.closeFile();

    SoQtExaminerViewer* myViewer = new SoQtExaminerViewer(myWindow);
    myViewer->setTransparencyType( SoGLRenderAction::SORTED_OBJECT_BLEND );

    myViewer->setSceneGraph( myGraph );
    myViewer->show();

    SoQt::show(myWindow);
    SoQt::mainLoop();

    return 0;
}
