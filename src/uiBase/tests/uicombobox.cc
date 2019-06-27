/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

#include "uicombobox.h"
#include "testprog.h"
#include "dateinfo.h"
#include "applicationdata.h"

#include "QApplication"

bool testEnums()
{
    EnumDef def = DateInfo::DayOfWeekDef();

    PtrMan<uiComboBox> box = new uiComboBox( 0, def, "Enum test" );
    mRunStandardTest( box->size()==def.size(), "Not right number of items");

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    QApplication( argc, argv );


    if ( !testEnums() )
	return 1;

    return 0;
}
