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
#include "initsood.h"

/*! \brief
oifileviewer is a program that lets the user display oi files. These can 
be generated via the 'dump OI' option in the debug-version of dTect.

The syntax is

oifileviewer <filename>
*/


int main( int narg, char** argv )
{
#ifdef USEQT3
    QWidget* myWindow = SoQt::init( argv[0] );
#else
    QWidget* myWindow = SoQt::init( narg, argv, argv[0] );
#endif
    SoOD::initStdClasses();

    if ( myWindow==NULL ) return 1;

    if ( narg != 2  )
    {
	printf("Syntax:\noifileviewer <filename>\n\n");
	return 1;
    }


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

    SoQtExaminerViewer* myViewer= new SoQtExaminerViewer(myWindow);
    myViewer->setTransparencyType(
	    	SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND );

    myViewer->setSceneGraph( myGraph );
    if ( argv[1] )
	myViewer->setTitle( argv[1] );
    myViewer->show();

    SoQt::show(myWindow);
    SoQt::mainLoop();

    return 0;
}
