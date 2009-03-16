/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/
static const char* __rcsID = "$Id: ivfileviewerbase.cc,v 1.4 2009-03-16 09:28:45 cvsranojay Exp $";

#include <VolumeViz/nodes/SoVolumeRendering.h>

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/SoInput.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/nodes/SoSeparator.h>

#include "filegen.h"
#include "uifiledlg.h"

#ifdef __msvc__
# include "winmain.h"
#endif

#ifdef USESOODCLASSES
#include "initsood.h"
#endif

int main( int narg, char** argv )
{
    QWidget* myWindow = SoQt::init( narg, argv, argv[0] );

#ifdef USESOODCLASSES
    SoOD::initStdClasses();
#endif

    if ( myWindow==NULL ) return 1;

    BufferString filename;
    if ( narg==2 )
	filename = argv[1];

    while ( filename.isEmpty() || !File_exists( filename.buf() ) )
    {
	uiFileDialog dlg( 0, uiFileDialog::ExistingFile, 0,
			  "IV files (*.iv)", "Select file to view" );
	dlg.setAllowAllExts( true );
	if ( !dlg.go() )
	    return 1;

	filename = dlg.fileName();
    }

    SoInput mySceneInput;
    //SoInput::addDirectoryFirst( "." ); // Add additional directories.
    SbStringList dirlist = SoInput::getDirectories();
    for ( int idx=0; idx<dirlist.getLength(); idx++ )
	printf( "Looking for \"%s\" in %s\n", filename.buf(),
		dirlist[idx]->getString() );

    if ( !mySceneInput.openFile(filename.buf()) )
	return 1;

    SoSeparator* myGraph = SoDB::readAll( &mySceneInput );
    if ( !myGraph ) return 1;
    mySceneInput.closeFile();

    SoQtExaminerViewer* myViewer = new SoQtExaminerViewer( myWindow );
    myViewer->setTitle( filename.buf() );
    myViewer->setTransparencyType( SoGLRenderAction::SORTED_OBJECT_BLEND );

    myViewer->setSceneGraph( myGraph );
    myViewer->show();

    SoQt::show( myWindow );
    SoQt::mainLoop();

    return 0;
}
