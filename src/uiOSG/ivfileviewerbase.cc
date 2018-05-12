/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/

#include <VolumeViz/nodes/SoVolumeRendering.h>

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/SoInput.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/nodes/SoSeparator.h>

#ifdef USESOODCLASSES
# include "moddepmgr.h"
# include "genc.h"
# include "file.h"
# include "uifileselector.h"
# include "odruncontext.h"
# ifdef __msvc__
#  include "winmain.h"
# endif
#endif

int main( int narg, char** argv )
{
#ifdef USESOODCLASSES
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( narg, argv );
#endif

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
	uiFileSelector::Setup fssu;
	fssu.setFormat( toUiString("IV files"), "iv" );
	uiFileSelector uifs( 0, fssu );
	if ( !uifs.go() )
	    return 1;

	filename = filebuf = uifs.fileName();
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

