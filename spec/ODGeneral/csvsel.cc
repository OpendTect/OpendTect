static const char* rcsID = "$Id$";

#include <iostream>
#include "tableconvimpl.h"
#include "strmprov.h"
#include "prog.h"

#define mRet(rv) { ExitProgram( 0 ); return 0; }


int main( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: " << argv[0] << " in_csv out_csv" << std::endl;
	mRet( 1 );
    }
    StreamData sdin = StreamProvider(argv[1]).makeIStream();
    if ( !sdin.usable() )
    {
	std::cerr << "Cannot open input csv file" << std::endl;
	mRet( 1 );
    }

    StreamData sdout = StreamProvider(argv[2]).makeOStream();
    if ( !sdout.usable() )
    {
	std::cerr << "Cannot open output csv file" << std::endl;
	mRet( 1 );
    }

    CSVTableImportHandler csvimp;
    csvimp.nlreplace_ = '|';

    CSVTableExportHandler csvexp;

    TableConverter tc( *sdin.istrm, csvimp, *sdout.ostrm, csvexp );
    tc.execute( &std::cout );

    mRet( 0 );
}
