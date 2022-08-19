/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprocchain.h"
#include "volprocvolreader.h"
#include "volprocbodyfiller.h"

#include "testprog.h"


static bool testConnectionSetup()
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



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return testConnectionSetup() ? 0 : 1;
}
