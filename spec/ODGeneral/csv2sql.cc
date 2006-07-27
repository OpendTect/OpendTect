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
    sqlexp.tblname_ = "contacts";
    sqlexp.indexcolnm_ = "id";
    sqlexp.extracolnms_.add( "date_entered" );
    sqlexp.extracolvals_.add( "2006-07-27" );
    sqlexp.extracolnms_.add( "date_modified" );
    sqlexp.extracolvals_.add( "2006-07-27" );
    sqlexp.extracolnms_.add( "modified_user_id" );
    sqlexp.extracolvals_.add( "1" );
    sqlexp.extracolnms_.add( "created_by" );
    sqlexp.extracolvals_.add( "1" );
    sqlexp.colnms_.add( "salutation" );
    sqlexp.colnms_.add( "first_name" );
    sqlexp.colnms_.add( "last_name" );
    sqlexp.colnms_.add( "portal_name" ); // to hold company name
    sqlexp.colnms_.add( "portal_app" ); // to hold company city
    sqlexp.colnms_.add( "title" );
    sqlexp.colnms_.add( "department" );
    sqlexp.colnms_.add( "alt_address_street" );
    sqlexp.colnms_.add( "phone_work" );
    sqlexp.colnms_.add( "email1" );
    sqlexp.colnms_.add( "email2" );
    sqlexp.colnms_.add( "phone_home" );
    sqlexp.colnms_.add( "lead_source" );
    sqlexp.colnms_.add( "description" );

    TableConverter tc( *sdin.istrm, csvimp, *sdout.ostrm, sqlexp );
    tc.selcols_ += 1 - 1; // Mr , Mrs, Dr etc
    tc.selcols_ += 2 - 1; // first name
    tc.selcols_ += 4 - 1; // last name
    tc.selcols_ += 5 - 1; // comp
    tc.selcols_ += 6 - 1; // city
    tc.selcols_ += 7 - 1; // position
    tc.selcols_ += 8 - 1; // dept
    tc.selcols_ += 9 - 1; // home address
    tc.selcols_ += 10 - 1; // direct phone
    tc.selcols_ += 11 - 1; // e-mail
    tc.selcols_ += 12 - 1; // e-mail 2
    tc.selcols_ += 13 - 1; // home phone
    tc.selcols_ += 16 - 1; // status
    tc.selcols_ += 17 - 1; // comments

    tc.execute( &std::cout );

    mRet( 0 );
}
