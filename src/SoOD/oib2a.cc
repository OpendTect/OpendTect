/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/
static const char* rcsID mUsedVar = "$Id$";

#include <VolumeViz/nodes/SoVolumeRendering.h>

#include <Inventor/SoInput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/nodes/SoSeparator.h>

/*! \brief
oib2a is a program that reads in a binary iv file and dumps it as a
ascii iv-file.

The syntax is

oifileviewer <filename> [outfile]
*/

extern "C" void od_SoOD_initStdClasses();

int main( int narg, char** argv )
{
    SoDB::init();
    SoVolumeRendering::init();
    od_SoOD_initStdClasses();

    if ( narg<2 || narg>3)
    {
	printf("Syntax:\noi2ba <inputfile> [outputfile]\n\n");
	return 1;
    }


    SoInput mySceneInput;

    //SoInput::addDirectoryFirst( "." ); // Add additional directories.
    SbStringList dirlist = SoInput::getDirectories();
    for ( int idx=0; idx<dirlist.getLength(); idx++ )
	printf( "Looking for \"%s\" in %s\n", argv[1],
		dirlist[idx]->getString() );

    printf( "Reading input ..." );
    if ( !mySceneInput.openFile( argv[1] ))
	return 1;

    SoSeparator* myGraph = SoDB::readAll(&mySceneInput);
    if ( !myGraph ) return 1;
    mySceneInput.closeFile();

    SoWriteAction writeaction;
    if ( !writeaction.getOutput()->openFile( narg==3 ? argv[2] : argv[1]) )
    {
	printf( "Cannot open output file" );
	return false;
    }

    printf( "Writing output ..." );
    writeaction.getOutput()->setBinary( false );
    writeaction.apply( myGraph );
    writeaction.getOutput()->closeFile();

    return 0;
}
