/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "fontdata.h"


static bool testStyleFlagsRoundTrip()
{
    FontData fd( 12, "Arial", FontData::Bold, true );
    fd.setUnderline( true );
    fd.setStrikeOut( true );

    BufferString str;
    fd.putTo( str );

    FontData restored;
    mRunStandardTest( restored.getFrom(str.buf()), "Parse the font string" );
    mRunStandardTest( restored.isUnderline(), "Underline is stored" );
    mRunStandardTest( restored.isStrikeOut(), "Strikeout is stored" );
    mRunStandardTest( restored == fd, "The restored font equals the original" );

    return true;
}


static bool testOldFormat()
{
    FontData fd;
    mRunStandardTest( fd.getFrom("Arial`12`Bold`Yes`Bold"),
		      "Parse a font string without the style flags" );
    mRunStandardTest( !fd.isUnderline() && !fd.isStrikeOut(),
		      "No flags without the fields" );
    mRunStandardTest( fd.isItalic() && fd.weight()==FontData::Bold,
		      "The other fields are kept" );

    return true;
}


static bool testCopyAndCompare()
{
    FontData plain( 10, "Arial" );
    FontData underlined( plain );
    mRunStandardTest( underlined == plain, "A copy equals the original" );

    underlined.setUnderline( true );
    mRunStandardTest( underlined != plain, "Underline makes the font differ" );

    FontData struck( plain );
    struck.setStrikeOut( true );
    mRunStandardTest( struck != plain, "Strikeout makes the font differ" );

    FontData assigned;
    assigned = underlined;
    mRunStandardTest( assigned.isUnderline() && !assigned.isStrikeOut(),
		      "Assignment copies the flags" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testStyleFlagsRoundTrip() ||
	 !testOldFormat() ||
	 !testCopyAndCompare() )
	return 1;

    return 0;
}
