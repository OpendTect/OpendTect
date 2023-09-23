/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "thread.h"
#include "testprog.h"
#include "math2.h"
#include "genc.h"


#define mRunTest( func ) \
    if ( (func)==false ) \
    { \
	handleTestResult( false, #func ); \
	return false; \
    } \
    else \
    { \
	handleTestResult( true, #func ); \
    }

#define mTestVal 100


bool testBits()
{
    mRunTest( Math::SetBits(0x0000, 0x0001, false )==0x0000 )
    mRunTest( Math::SetBits(0x0000, 0x0001, true )==0x0001 )
    mRunTest( Math::SetBits(0x0000, 0x0200, false )==0x0000 )
    mRunTest( Math::SetBits(0x0000, 0x0200, true )==0x0200 )
    mRunTest( Math::SetBits(0x0020, 0x0001, false )==0x0020 )
    mRunTest( Math::SetBits(0x0020, 0x0001, true )==0x0021 )
    mRunTest( Math::SetBits(0x0020, 0x0200, false )==0x0020 )
    mRunTest( Math::SetBits(0x0020, 0x0200, true )==0x0220 )
    mRunTest( Math::SetBits(0x0021, 0x0001, false )==0x0020 )
    mRunTest( Math::SetBits(0x0021, 0x0001, true )==0x0021 )
    mRunTest( Math::SetBits(0x0220, 0x0200, false )==0x0020 )
    mRunTest( Math::SetBits(0x0220, 0x0200, true )==0x0220 )

    mRunTest( Math::AreBitsSet(0x0220, 0x0200, true )==true )
    mRunTest( Math::AreBitsSet(0x0220, 0x0200, false )==true )
    mRunTest( Math::AreBitsSet(0x0220, 0x0220, true )==true )
    mRunTest( Math::AreBitsSet(0x0200, 0x0220, true )==false )
    mRunTest( Math::AreBitsSet(0x0200, 0x0220, false )==true )
    mRunTest( Math::AreBitsSet(0x0220, 0x0002, true )==false )
    mRunTest( Math::AreBitsSet(0x0220, 0x0002, false )==false )

    return true;
}


bool testNiceNumber()
{
    mRunStandardTest(Math::NiceNumber(0.f)==0.f, "NiceNumber(0.f)");
    mRunStandardTest(Math::NiceNumber(14.f)==10.f, "NiceNumber(14.f)");
    mRunStandardTest(Math::NiceNumber(-14.f)==-10.f, "NiceNumber(-14.f)");
    mRunStandardTest(Math::NiceNumber(14.f, false)==20.f,
						     "NiceNumber(14.f, false)");
    mRunStandardTest(Math::NiceNumber(-14.f, false)==-20.f,
						 "NiceNumber(-14.f, false)");
    mRunStandardTest(Math::NiceNumber(0.14f)==0.1f, "NiceNumber(0.14f)");
    mRunStandardTest(Math::NiceNumber(-0.14f)==-0.1f, "NiceNumber(-0.14f)");
    mRunStandardTest(Math::NiceNumber(0.14f, false)==0.2f,
						 "NiceNumber(0.14f, false)");
    mRunStandardTest(Math::NiceNumber(-0.14f, false)==-0.2f,
						 "NiceNumber(-0.14f, false)");

    mRunStandardTest(Math::NiceNumber(0.0)==0.0, "NiceNumber(0.0)");
    mRunStandardTest(Math::NiceNumber(14.0)==10.0, "NiceNumber(14.0)");
    mRunStandardTest(Math::NiceNumber(-14.0)==-10.0, "NiceNumber(-14.0)");
    mRunStandardTest(Math::NiceNumber(14.0, false)==20.0,
						     "NiceNumber(14.0, false)");
    mRunStandardTest(Math::NiceNumber(-14.0, false)==-20.0,
						 "NiceNumber(-14.0, false)");
    mRunStandardTest(Math::NiceNumber(0.14)==0.1, "NiceNumber(0.14)");
    mRunStandardTest(Math::NiceNumber(-0.14)==-0.1, "NiceNumber(-0.14)");
    mRunStandardTest(Math::NiceNumber(0.14, false)==0.2,
						 "NiceNumber(0.14, false)");
    mRunStandardTest(Math::NiceNumber(-0.14, false)==-0.2,
						 "NiceNumber(-0.14, false)");
    return true;
}


bool testNrSignificantDecimals()
{
    double stepms = 0.1;
    double steps = stepms / 1000.;
    mRunStandardTest(Math::NrSignificantDecimals(stepms)==1,
		     "nrDec(stepms=0.1)");
    mRunStandardTest(Math::NrSignificantDecimals(steps*1000.)==1,
		     "nrDec(steps*1000.)");
    mRunStandardTest(Math::NrSignificantDecimals(stepms/10.)==2,
		     "nrDec(stepms/10.)");
    mRunStandardTest(Math::NrSignificantDecimals(stepms/100.)==3,
		     "nrDec(stepms/100.)");
    mRunStandardTest(Math::NrSignificantDecimals(stepms/1000.)==4,
		     "nrDec(stepms/1000.)");

    mRunStandardTest(Math::NrSignificantDecimals(12345)==0,"nrDec(12345)");
    mRunStandardTest(Math::NrSignificantDecimals(12345.99999)==0,
		     "nrDec(12345.99999)");
    mRunStandardTest(Math::NrSignificantDecimals(1234.599999)==1,
		     "nrDec(1234.599999)");
    mRunStandardTest(Math::NrSignificantDecimals(123.4599999)==2,
		     "nrDec(123.4599999)");
    mRunStandardTest(Math::NrSignificantDecimals(12.34599999)==3,
		     "nrDec(12.34599999)");
    mRunStandardTest(Math::NrSignificantDecimals(1.234599999)==4,
		     "nrDec(1.234599999)");
    mRunStandardTest(Math::NrSignificantDecimals(0.1234599999)==5,
		     "nrDec(0.1234599999)");
    mRunStandardTest(Math::NrSignificantDecimals(0.01234599999)==6,
		     "nrDec(0.01234599999)");

    mRunStandardTest(Math::NrSignificantDecimals(0.123000012)==3,
		     "nrDec(0.123000012)");
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testBits() ||
	 !testNiceNumber() ||
	 !testNrSignificantDecimals()
       )
	return 1;

    return 0;
}
