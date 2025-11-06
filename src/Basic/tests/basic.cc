/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "datapack.h"
#include "zdomain.h"


namespace ZDomain
{

static Def& testDomainDef()
{
    static Def ret( "TestDef", ::toUiString("TestDef"), "um", 1e6 );
    return ret;
}


static Info& testDomainInfo()
{
    static Info ret( testDomainDef(), nullptr );
    return ret;
}

} // namespace ZDomain


static bool testPointerCast()
{
    float val;
    float* ptr = &val;

    void* voidptr = ptr;

    float* newptr = reinterpret_cast<float*>(voidptr);

    mRunStandardTest( newptr==ptr, "Pointer cast" );

    return true;
}


static bool testUndefineds()
{
    const short sval = 32766;
    const unsigned short usval = 65534;
    const od_int32 i32val = 2109876543;
    const od_uint32 ui32val = 2109876543;
    const od_int64 i64val = 9223344556677889900LL;
    const od_uint64 ui64val = 9223344556677889900ULL;
    const float fval = 1e30f;
    const double dval = 1e30;

    mRunStandardTest( mIsUdf(sval), "Short is undefined" );
    mRunStandardTest( mIsUdf(usval), "Unsigned short is undefined" );
    mRunStandardTest( mIsUdf(i32val), "Signed int is undefined");
    mRunStandardTest( mIsUdf(ui32val), "Unsigned int is undefined" );
    mRunStandardTest( mIsUdf(i64val), "long long int is undefined");
    mRunStandardTest( mIsUdf(ui64val), "Unsigned long long int is undefined" );
    mRunStandardTest( mIsUdf(fval), "Single-precision float is undefined");
    mRunStandardTest( mIsUdf(dval), "Double-precision float is undefined");

    return true;
}


static bool testNotUndefineds()
{
    const short psval =  31245;
    const short msval = -31245;
    const unsigned short usval = 51235;
	//Must be below __mUndefIntVal = 2109876543
    const od_int32 pi32val =  2097483640;
    const od_int32 mi32val = -2097483640;
    const od_uint32 ui32val = 2097483640;
	//Must be below __mUndefIntVal64 = 9223344556677889900LL
    const od_int64 pi64val =  9223322036854775703LL;
    const od_int64 mi64val = -9223322036854775703LL;
    const od_uint64 ui64val = 9223322036854775703ULL;
    const float fval = 12345.6789f;
    const double dval = 12345.6789;

    mRunStandardTest( !mIsUdf(psval), "Short positive is not undefined" );
    mRunStandardTest( !mIsUdf(msval), "Short negative is not undefined" );
    mRunStandardTest( !mIsUdf(usval), "Unsigned short is not undefined" );
    mRunStandardTest( !mIsUdf(pi32val), "Positive signed int is not undefined");
    mRunStandardTest( !mIsUdf(mi32val), "Negative signed int is not undefined");
    mRunStandardTest( !mIsUdf(ui32val), "Unsigned int is not undefined" );
    mRunStandardTest( !mIsUdf(pi64val),
		      "Positive signed long long int is not undefined" );
    mRunStandardTest( !mIsUdf(mi64val),
		      "Negative signed long long int is not undefined" );
    mRunStandardTest( !mIsUdf(ui64val),
		      "Unsigned long long int as od_uint64 is not undefined" );
    mRunStandardTest( !mIsUdf(fval), "Single-precision float is not undefined");
    mRunStandardTest( !mIsUdf(dval), "Double-precision float is not undefined");

    return true;
}


static bool testPointerAlignment()
{
    char buffer[] = { 0, 0, 0, 0, 1, 1, 1, 1 };

    char* ptr0 = buffer;
    char* ptr1 = buffer+1;

    od_uint32* iptr0 = reinterpret_cast<od_uint32*>( ptr0 );
    od_uint32* iptr1 = reinterpret_cast<od_uint32*>( ptr1 );

    od_uint32 ival0 = *iptr0;
    od_uint32 ival1 = *iptr1;

    char* ptr0_mod = (char*) iptr0;
    char* ptr1_mod = (char*) iptr1;

    union IVal1Reference
    {
    public:
			IVal1Reference()
		{
                            charval_[0] = 0;
                            charval_[1] = 0;
                            charval_[2] = 0;
                            charval_[3] = 1;
                        }
        char		charval_[4];
        int		intval_;
    }  val1ref;

    mRunStandardTest( ptr0_mod==ptr0 && ptr1_mod==ptr1 &&
		      ival0==0 && ival1 == val1ref.intval_,
		       "Pointer alignment" );

    return true;
}


bool testCompoundKey()
{
    mRunStandardTest( MultiID::udf().isUdf(), "Undefined multiid" );
    MultiID testid;
    mRunStandardTest( testid.isUdf(), "Empty multiid is undefined" );
    testid.fromString( "100010.2" );
    mRunStandardTest( testid.isDatabaseID() && !testid.isSyntheticID() &&
		      !testid.isInMemoryDPID(), "Valid database MultiID" );
    testid.fromString( "100050.999998" );
    const MultiID testid2( 100010, MultiID::cSyntheticObjID() );
    mRunStandardTest( testid.isSyntheticID() && !testid.isInMemoryDPID() &&
		      testid2.isSyntheticID() && !testid2.isInMemoryDPID(),
		      "Valid synthetic MultiID" );
    testid.setSubGroupID( 1 );
    mRunStandardTest( testid.isSyntheticID() && !testid.isInMemoryDPID(),
		      "Valid synthetic MultiID subgroup" );
    testid.fromString( "3.33" );
    mRunStandardTest( !testid.isDatabaseID() && !testid.isSyntheticID() &&
		       testid.isInMemoryDPID() &&
		       testid.groupID() == DataPackMgr::SeisID().asInt() &&
		       testid.toString() == "3.33",
		      "Valid in-memory MultiID" );

    return true;
}



static bool test64BitDetection()
{
#ifdef __win__
    mRunStandardTest( is64BitWindows(), "Detecting 64 bit Windows OS" );
#endif

    return true;
}


static bool testOSVersion()
{
    mRunStandardTest( GetOSIdentifier(), "GetOSIdentifier not returning null" );

    return true;
}

#define mFuncInMacro( dummy ) BufferString( __func__ )


static bool testFuncName()
{
    BufferString func = mFuncInMacro( "Hello" );
    mRunStandardTest( func=="testFuncName", "Function name macro" );

    return true;
}


static bool testZDomain()
{
    ZDomain::Def::add( &ZDomain::testDomainDef() );
    getNonConst(*ZDomain::sAll().last()).pars_ =
						ZDomain::testDomainInfo().pars_;

    const ZDomain::Info& twtzdom = ZDomain::TWT();
    const ZDomain::Info& depthmzdom = ZDomain::DepthMeter();
    const ZDomain::Info& depthftzdom = ZDomain::DepthFeet();
    const ZDomain::Info& otherzdom = *ZDomain::sAll().last();

    mRunStandardTest( twtzdom.isTime(), "Time ZDomain::Info" );
    mRunStandardTest( depthmzdom.isDepthMeter() &&
				  depthmzdom.depthType() ==
				  ZDomain::DepthType::Meter,
		      "Depth-m ZDomain::Info" );
    mRunStandardTest( depthftzdom.isDepthFeet() &&
				  depthftzdom.depthType() ==
				  ZDomain::DepthType::Feet,
		      "Depth-ft ZDomain::Info" );
    mRunStandardTest( twtzdom != depthmzdom,
		      "Time vs Depth-m are not equal" );
    mRunStandardTest( twtzdom != depthftzdom,
		      "Time vs Depth-ft are not equal" );
    mRunStandardTest( depthmzdom != depthftzdom,
		      "Depth-m vs Depth-ft are not equal" );
    mRunStandardTest( otherzdom != twtzdom &&
		      otherzdom != depthmzdom &&
		      otherzdom != depthftzdom,
		      "Other ZDomain::Info object is neither time nor depth" );

    ZDomain::Info mytimezdom( ZDomain::Time(), nullptr );
    ZDomain::Info mydepthmzdom( ZDomain::Depth(), nullptr );
    mydepthmzdom.setDepthUnit( ZDomain::DepthType::Meter );
    ZDomain::Info mydepthftzdom( ZDomain::Depth(), nullptr );
    mydepthftzdom.setDepthUnit( ZDomain::DepthType::Feet );
    const MultiID mid( 100010, 30 );
    mytimezdom.setID( mid );
    mytimezdom.pars_.set( "Another key", "Value" );
    mydepthmzdom.setID( mid );
    mydepthmzdom.pars_.set( "Another key", "Value" );
    mydepthftzdom.setID( mid );
    mydepthftzdom.pars_.set( "Another key", "Value" );

    mRunStandardTest(  mytimezdom.isCompatibleWith( twtzdom ) &&
		      !mytimezdom.isCompatibleWith( depthmzdom ) &&
		      !mytimezdom.isCompatibleWith( depthftzdom ),
		      "Custom time ZDomain::Info compatibility" );
    mRunStandardTest( !mydepthmzdom.isCompatibleWith( twtzdom ) &&
		       mydepthmzdom.isCompatibleWith( depthmzdom ) &&
		      !mydepthmzdom.isCompatibleWith( depthftzdom ),
		      "Custom Depth-m ZDomain::Info compatibility" );
    mRunStandardTest( !mydepthftzdom.isCompatibleWith( twtzdom ) &&
		      !mydepthftzdom.isCompatibleWith( depthmzdom ) &&
		       mydepthftzdom.isCompatibleWith( depthftzdom ),
		      "Custom Depth-ft ZDomain::Info compatibility" );

    const ZDomain::Info* rettimezdom = &ZDomain::Info::getFrom( mytimezdom );
    const ZDomain::Info* retdepthmzdom = &ZDomain::Info::getFrom( mydepthmzdom);
    const ZDomain::Info* retdepthftzdom =&ZDomain::Info::getFrom(mydepthftzdom);
    mRunStandardTest( rettimezdom == &twtzdom,
		      "Retrieved time ZDomain::Info singleton" );
    mRunStandardTest( retdepthmzdom == &depthmzdom,
		      "Retrieved Depth-m ZDomain::Info singleton" );
    mRunStandardTest( retdepthftzdom == &depthftzdom,
		      "Retrieved Depth-ft ZDomain::Info singleton" );

    IOPar par;
    mytimezdom.fillPar( par );
    rettimezdom = ZDomain::Info::getFrom( par );
    mRunStandardTest( rettimezdom && rettimezdom == &twtzdom,
		      "Retrieved time ZDomain::Info singleton from IOPar" );

    par.setEmpty();
    mydepthmzdom.fillPar( par );
    mRunStandardTest( retdepthmzdom == &depthmzdom,
		      "Retrieved Depth-m ZDomain::Info singleton from IOPar" );

    par.setEmpty();
    mydepthftzdom.fillPar( par );
    mRunStandardTest( retdepthftzdom == &depthftzdom,
		      "Retrieved Depth-ft ZDomain::Info singleton from IOPar" );

    mRunStandardTest( twtzdom.nrDecimals(mUdf(float)) == 1, "TWT nrDec (1)" );
    mRunStandardTest( depthmzdom.nrDecimals(mUdf(float)) == 1,
		      "Depth-m nrDec (1)" );
    mRunStandardTest( depthftzdom.nrDecimals(mUdf(float)) == 1,
		      "Depth-ft nrDec (1)" );
    mRunStandardTest( twtzdom.nrDecimals(4e-6f), "uS nrDec (4)" );

    getNonConst(twtzdom).setPreferredNrDec( 3 );
    mRunStandardTest( twtzdom.nrDecimals(mUdf(float)) == 3, "TWT nrDec (3)" );
    mRunStandardTest( twtzdom.nrDecimals(4e-6f) == 3, "TWT nrDec (3)" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    // Main idea is to test things that are so basic they don't
    // really fit anywhere else.

    if (   !testPointerCast()
	|| !testUndefineds()
	|| !testNotUndefineds()
	|| !testCompoundKey()
	|| !testOSVersion()
        || !testPointerAlignment()
	|| !testFuncName()
	|| (__iswin__ && !test64BitDetection())
	|| !testZDomain() )
	return 1;

    return 0;
}
