#include <iostream>
#include "tableconvimpl.h"
#include "strmprov.h"
#include "prog.h"

#define mRet(rv) { ExitProgram( 0 ); return 0; }


int main( int argc, char** argv )
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
    sqlexp.tblname_ = "accounts";
    sqlexp.colnms_.add( "name" );
    sqlexp.colnms_.add( "billing_address_city" );
    sqlexp.colnms_.add( "billing_address_street" );
    sqlexp.colnms_.add( "shipping_address_street" );
    sqlexp.colnms_.add( "billing_address_state" );
    sqlexp.colnms_.add( "billing_address_country" );
    sqlexp.colnms_.add( "billing_address_postalcode" );
    sqlexp.colnms_.add( "description" );
    sqlexp.colnms_.add( "phone_office" );
    sqlexp.colnms_.add( "phone_fax" );
    sqlexp.colnms_.add( "website" );

    TableConverter tc( *sdin.istrm, csvimp, *sdout.ostrm, sqlexp );
    tc.execute( &std::cout );

    mRet( 0 );
}
