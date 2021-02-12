static const char* rcsID = "$Id$";

#include <iostream>
#include "tableconvimpl.h"
#include "strmprov.h"
#include "prog.h"

#define mRet(rv) { return 0; }


int mProgMainFnName( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: " << argv[0] << " in_csv out_sql" << std::endl;
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
	std::cerr << "Cannot open output sql file" << std::endl;
	mRet( 1 );
    }

    CSVTableImportHandler csvimp;

    SQLInsertTableExportHandler sqlexp;
    sqlexp.tblname_ = "contacts_cstm";
    sqlexp.indexcolnm_ = "id_c";
    sqlexp.colnms_.add( "newsletter_c" );

    TableConverter tc( *sdin.istrm, csvimp, *sdout.ostrm, sqlexp );
    tc.selcols_ += 15 - 1; // comment

    tc.execute( &std::cout );

    mRet( 0 );
}
