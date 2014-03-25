/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocchain.h"
#include "volprocvolreader.h"
#include "volprocbodyfiller.h"

#include "testprog.h"


bool testConnectionSetup()
{
    RefMan<VolProc::Chain> chain = new VolProc::Chain;

    VolProc::VolumeReader* volreader = new VolProc::VolumeReader;
    chain->addStep( volreader );

    VolProc::BodyFiller* bodyfiller = new VolProc::BodyFiller;
    chain->addStep( bodyfiller );

    chain->addConnection(
	VolProc::Chain::Connection( volreader->getID(),
				    volreader->getOutputSlotID(0),
				    bodyfiller->getID(),
				    bodyfiller->getInputSlotID(0) ) );


    return true;
}



int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testConnectionSetup() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
