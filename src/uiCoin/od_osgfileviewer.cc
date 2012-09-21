/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Jaap Glas
 * DATE     : Sep 2012
-*/
static const char* __rcsID mUnusedVar = "$Id$";

#include <VolumeViz/nodes/SoVolumeRendering.h>

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/SoInput.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/nodes/SoSeparator.h>

#ifdef USESOODCLASSES
# include "moddepmgr.h"
# include "file.h"
# include "uifiledlg.h"
# ifdef __msvc__
#  include "winmain.h"
# endif
#endif

int main( int narg, char** argv )
{
    QWidget* myWindow = SoQt::init( narg, argv, argv[0] );

#ifdef USESOODCLASSES
    OD::ModDeps().ensureLoaded( "SoOD" );
#endif

    if ( myWindow==NULL ) return 1;

    const char* filename = 0;
    if ( narg==2 )
	filename = argv[1];
#ifdef USESOODCLASSES
    BufferString filebuf = filename;

    while ( filebuf.isEmpty() || !File::exists( filebuf.buf() ) )
    {
	uiFileDialog dlg( 0, uiFileDialog::ExistingFile, 0,
			  "IV files (*.iv)", "Select file to view" );
	dlg.setAllowAllExts( true );
	if ( !dlg.go() )
	    return 1;

	filename = filebuf = dlg.fileName();
    }
#endif

    if ( !filename || !*filename )
    {
	printf( "No filename given" );
	exit( 1 );
    }

    SoInput mySceneInput;
    //SoInput::addDirectoryFirst( "." ); // Add additional directories.
    SbStringList dirlist = SoInput::getDirectories();
    for ( int idx=0; idx<dirlist.getLength(); idx++ )
	printf( "Looking for \"%s\" in %s\n", filename,
		dirlist[idx]->getString() );

    if ( !mySceneInput.openFile(filename) )
	return 1;

    SoSeparator* myGraph = SoDB::readAll( &mySceneInput );
    if ( !myGraph ) return 1;
    mySceneInput.closeFile();

    SoQtExaminerViewer* myViewer = new SoQtExaminerViewer( myWindow );
    myViewer->setTitle( filename );
    myViewer->setTransparencyType( SoGLRenderAction::SORTED_OBJECT_BLEND );
    myViewer->setBackgroundColor( SbColor( 0.5, 0.5, 0.5) );

    myViewer->setSceneGraph( myGraph );
    myViewer->show();

    SoQt::show( myWindow );
    SoQt::mainLoop();

    return 0;
}
