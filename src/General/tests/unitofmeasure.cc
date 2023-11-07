/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "unitofmeasure.h"


static double getUserValue( const char* unit, double sival )
{
    const UnitOfMeasure* uom = UoMR().get( unit );
    return uom ? uom->getUserValueFromSI( sival ) : mUdf(double);
}


static bool isEqual( const char* unit, double sival, double usrval )
{
    const double precision = 1e-5;
    return mIsEqual(getUserValue(unit,sival),usrval,precision);
}


static bool testStandardUnits()
{
    mRunStandardTest( UnitOfMeasure::secondsUnit(), "Seconds" );
    mRunStandardTest( UnitOfMeasure::millisecondsUnit(), "Milliseconds" );
    mRunStandardTest( UnitOfMeasure::meterUnit(), "Meter" );
    mRunStandardTest( UnitOfMeasure::feetUnit(), "Feet" );
    mRunStandardTest( UnitOfMeasure::meterSecondUnit(), "Meter/second" );
    mRunStandardTest( UnitOfMeasure::feetSecondUnit(), "Feet/second" );
    mRunStandardTest( UnitOfMeasure::radiansUnit(), "Radians" );
    mRunStandardTest( UnitOfMeasure::degreesUnit(), "Degrees" );

    return true;
}


static bool testDistance()
{
    mRunStandardTest( isEqual("um",1,1e6), "Micrometer" );
    mRunStandardTest( isEqual("mm",1,1e3), "Millimeter" );
    mRunStandardTest( isEqual("cm",1,1e2), "Centimeter" );
    mRunStandardTest( isEqual("km",1,1e-3), "Kilometer" );
    mRunStandardTest( isEqual("in",1,39.37007874), "Inch" );
    mRunStandardTest( isEqual("ft",1000,3280.839895), "Foot" );
    mRunStandardTest( isEqual("yd",10,10.936132983), "yard" );
    mRunStandardTest( isEqual("mi",10000,6.2137119224), "Mile" );
    return true;
}


static bool testArea()
{
    mRunStandardTest( isEqual("km2",1e6,1), "Square km" );
    mRunStandardTest( isEqual("ft2",1,10.763910417), "Square foot" );
    mRunStandardTest( isEqual("mi2",1e6,0.3861021585), "Square mile" );
    mRunStandardTest( isEqual("ac",1e3,0.2471053815), "Acre" );
    return true;
}


static bool testVolume()
{
    mRunStandardTest( isEqual("cm3",1e-2,1e4), "Cubic centimer" );
    mRunStandardTest( isEqual("L",1,1000), "Liter" );
    mRunStandardTest( isEqual("in3",1,61023.744095), "Cubic inch" );
    mRunStandardTest( isEqual("ft3",1,35.314666721), "Cubic foot" );
    mRunStandardTest( isEqual("mi3",1e10,2.3991275858), "Cubic mile" );
    mRunStandardTest( isEqual("gal",1,219.9692483), "Gallon [UK]" );
    mRunStandardTest( isEqual("usgal",1,264.17205236), "Gallon [US]" );
    mRunStandardTest( isEqual("bbl",1,6.2898107704), "Barrel" );
    mRunStandardTest( isEqual("Mbbl",1e3,6.2898107704), "Thousand Barrels" );
    mRunStandardTest( isEqual("MMbbl",1e6,6.2898107704), "Million Barrels" );
    mRunStandardTest( isEqual("bcf",1e9,35.314666721), "Billion cubic feet" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testStandardUnits() ||
	 !testDistance() ||
	 !testArea() ||
	 !testVolume() )
	return 1;

    return 0;
}
