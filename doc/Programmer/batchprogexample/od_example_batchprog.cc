#include "batchprog.h"
#include "hostdata.h"
#include "progressmeter.h"
#include "iopar.h"

#include "progressmeter.h"
#include <iostream>


bool BatchProgram::go( std::ostream& strm )
{
    strm << "Processing on " << HostData::localHostName()  << '.' << std::endl;

    int nriter = 10000;
    pars().get( "Nr iterations", nriter );

    TextStreamProgressMeter progressmeter(strm);
    progressmeter.setTotalNr( nriter );

    for ( int idx=0; idx<nriter; idx++ )
    {
	// some useful stuff like processing here

	++progressmeter;
    }

    progressmeter.setFinished();
    return true;
}
