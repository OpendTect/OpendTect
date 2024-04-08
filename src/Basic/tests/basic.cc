/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "datapack.h"
#include "multiid.h"


bool testPointerCast()
{
    float val;
    float* ptr = &val;

    void* voidptr = ptr;

    float* newptr = reinterpret_cast<float*>(voidptr);

    mRunStandardTest( newptr==ptr, "Pointer cast" );

    return true;
}


bool testPointerAlignment()
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


bool testOSVersion()
{
    mRunStandardTest( GetOSIdentifier(), "GetOSIdentifier not returning null" );

    return true;
}

#define mFuncInMacro( dummy ) BufferString( __func__ )


bool testFuncName()
{
    BufferString func = mFuncInMacro( "Hello" );
    mRunStandardTest( func=="testFuncName", "Function name macro" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    // Main idea is to test things that are so basic they don't
    // really fit anywhere else.

    if ( !testPointerCast()
	|| !testCompoundKey()
	|| !testOSVersion()
        || !testPointerAlignment()
	|| !testFuncName() )
        return 1;

    return 0;
}
