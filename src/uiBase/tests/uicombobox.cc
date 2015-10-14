/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

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


int main( int argc, char** argv )
{
    mInitTestProg();

    QApplication( argc, argv );


    if ( !testEnums() )
        ExitProgram( 1 );

    return ExitProgram( 0 );
}
