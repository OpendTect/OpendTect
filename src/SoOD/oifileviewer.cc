/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>
#include "SoOD.h"

/*! \brief
oifileviewer is a program that lets the user display oi files. These can 
be generated via the 'dump OI' option in the debug-version of dTect.

The syntax is

oifileviewer <filename>
*/


int main( int narg, char** argv )
{
    QWidget* myWindow = SoQt::init(argv[0]);
    SoOD::init();
    if ( myWindow==NULL ) return 1;

    if ( narg != 2  )
    {
	printf("Syntax:\noifileviewer <filename>\n\n");
	return 1;
    }

    SoInput mySceneInput;
    if ( !mySceneInput.openFile( argv[1] ))
	return 1;

    SoSeparator* myGraph = SoDB::readAll(&mySceneInput);
    if ( !myGraph ) return 1;
    mySceneInput.closeFile();

    SoQtExaminerViewer* myViewer= new SoQtExaminerViewer(myWindow);
    myViewer->setTransparencyType(
	    	SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND );

    myViewer->setSceneGraph( myGraph );
    myViewer->show();

    SoQt::show(myWindow);
    SoQt::mainLoop();

    return 0;
}
