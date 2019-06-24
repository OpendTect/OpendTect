/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2016
-*/


#include "testprog.h"
#include "dbkey.h"
#include "surveydisklocation.h"

static bool checkCharacteristics( const char* strrep,
	      bool strvalid, bool kyvalid, bool isdir, bool hasaux )
{
    const bool isvalidstr = DBKey::isValidString( strrep );
    if ( isvalidstr == strvalid )
	tstStream( false ) << "'" << strrep << "' valid for strrep OK."
			  << od_endl;
    else
    {
	tstStream( true ) << "'" << strrep << "' valid fail for strrep."
			  << od_endl;
	return false;
    }

    const DBKey dbky( strrep );
    if ( dbky.isValid() == kyvalid )
	tstStream( false ) << "'" << strrep << "' valid for DBKey OK."
			  << od_endl;
    else
    {
	tstStream( true ) << "'" << strrep << "' valid fail for DBKey."
			  << od_endl;
	return false;
    }

    if ( dbky.hasValidObjID() == !isdir )
	tstStream( false ) << "'" << strrep << "' isdir OK." << od_endl;
    else
    {
	tstStream( true ) << "'" << strrep << "' isdir not as expected."
			  << od_endl;
	return false;
    }

    if ( dbky.hasAuxKey() == hasaux )
	tstStream( false ) << "'" << strrep << "' hasaux OK."
		<< "aux='" << dbky.auxKey() << "'" << od_endl;
    else
    {
	tstStream( true ) << "'" << strrep << "' hasaux not as expected."
			  << od_endl;
	return false;
    }

    return true;
}

static bool testToFromString()
{
    BufferString kystr1 = "-1";
    BufferString kystr2 = "";
    BufferString kystr3 = "-";
    BufferString kystr4 = "#100010.5";
    BufferString kystr5 = "123";
    BufferString kystr6 = "3.4";
    BufferString kystr7 = "|#auxky";
    BufferString kystr8 = "100010.5|#2.3";

	      // bool strvalid, bool kyvalid, bool isdir, bool hasaux
    if ( !checkCharacteristics(kystr1,true,false,true,false)
      || !checkCharacteristics(kystr2,false,false,true,false)
      || !checkCharacteristics(kystr3,false,false,true,false)
      || !checkCharacteristics(kystr4,false,false,true,false)
      || !checkCharacteristics(kystr5,true,true,true,false)
      || !checkCharacteristics(kystr6,true,true,false,false)
      || !checkCharacteristics(kystr7,true,false,true,true)
      || !checkCharacteristics(kystr8,true,true,false,true) )
	return false;

    DBKey dbky1; DBKey::DirID dirid; dirid.setInvalid();
    dbky1 = DBKey( kystr1 );
    DBKey dbky2 = DBKey( kystr2 );
    DBKey dbky3 = DBKey( kystr3 );
    DBKey dbky4 = DBKey( kystr4 );
    DBKey dbky5 = DBKey( kystr5 );
    DBKey dbky6 = DBKey( kystr6 );
    DBKey dbky7 = DBKey( kystr7 );
    DBKey dbky8 = DBKey( kystr8 );

    BufferString kystr1_2 = dbky1.toString(), kystr2_2 = dbky2.toString();
    BufferString kystr3_2 = dbky3.toString(), kystr4_2 = dbky4.toString();
    BufferString kystr5_2 = dbky5.toString(), kystr6_2 = dbky6.toString();
    BufferString kystr7_2 = dbky7.toString(), kystr8_2 = dbky8.toString();

	      // bool strvalid, bool kyvalid, bool isdir, bool hasaux
    if ( !checkCharacteristics(kystr1_2,true,false,true,false)
      || !checkCharacteristics(kystr2_2,true,false,true,false)
      || !checkCharacteristics(kystr3_2,true,false,true,false)
      || !checkCharacteristics(kystr4_2,true,false,true,false)
      || !checkCharacteristics(kystr5_2,true,true,true,false)
      || !checkCharacteristics(kystr6_2,true,true,false,false)
      || !checkCharacteristics(kystr7_2,true,false,true,true)
      || !checkCharacteristics(kystr8_2,true,true,false,true) )
	return false;

    return true;
}

#include "filepath.h"
static bool testSurvDBKey()
{
    File::Path sstr( "100010.5`/tmp/surveys/Apenoot" );
    DBKey ldbky = DBKey( "100010.5" );
    DBKey sdbky = DBKey( sstr.fullPath() );

    mRunStandardTest( ldbky.isInCurrentSurvey(), "DBKey in-survey" );
    mRunStandardTest( !sdbky.isInCurrentSurvey(), "DBKey off-survey" );

    const BufferString survdir( sdbky.surveyDiskLocation().fullPath() );
    File::Path fp( "/tmp/surveys/Apenoot" );
    BufferString str = fp.fullPath();
    mRunStandardTestWithError( survdir == fp.fullPath(),
	    "Correct survdir", BufferString("parsed: '",survdir,"'") );
    const BufferString dbkystr( sdbky.toString() );

    mRunStandardTestWithError( dbkystr == sstr.fullPath(),
	    "Correct toString", BufferString("got: '",dbkystr,"'") );

    return true;
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testToFromString() )
	return 1;
    else if ( !testSurvDBKey() )
	return 1;

    return 0;
}
