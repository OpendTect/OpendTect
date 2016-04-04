/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "typeset.h"
#include "objectset.h"
#include "testprog.h"
#include "refcount.h"



class ReferencedClass : public RefCount::Referenced
{
public:
    ReferencedClass(bool* delflag )
	: deleteflag_( delflag )
    {}

protected:
    ~ReferencedClass() { *deleteflag_ = true; }

private:

    bool*		deleteflag_;
};


#define mRunTest( voiddo, test, delstatus, rc ) \
deleted = false; \
voiddo; \
if ( !(test) || delstatus!=deleted || (rc>=0 && rc!=refclass->nrRefs() )) \
{ \
    od_cout() << "Test " << #voiddo << " " << #test << " FAILED\n"; \
    return false; \
} \
else if ( !quiet ) \
{ \
    od_cout() << "Test " << #voiddo << " " << #test << " - SUCCESS\n"; \
}


bool testRefCount()
{
    bool deleted = false;
    ReferencedClass* refclass = new ReferencedClass( &deleted );

    mRunTest( , refclass->refIfReffed()==false, false, 
	      RefCount::Counter::cStartRefCount() );
    mRunTest( refclass->ref(), true, false, 1 );
    mRunTest( , refclass->refIfReffed()==true, false, 2 );
    mRunTest( refclass->unRef(), true, false, 1 );
    mRunTest( refclass->unRefNoDelete(), true, false, 0 );
    mRunTest( refclass->ref(), true, false, 1 );
    mRunTest( unRefAndZeroPtr( refclass ), refclass==0, true,
	      RefCount::Counter::cInvalidRefCount() );

    //Test null pointers
    mRunTest( refPtr(refclass), true, false,
	      RefCount::Counter::cInvalidRefCount() );
    mRunTest( unRefPtr(refclass), true, false,
	      RefCount::Counter::cInvalidRefCount() );

    refclass = new ReferencedClass( &deleted );
    RefMan<ReferencedClass> rptr = refclass;
    mRunTest( refPtr(refclass), true, false, 2 );
    mRunTest( refPtr(refclass), true, false, 3 );
    mRunTest( unRefPtr(refclass), true, false, 2 );
    mRunTest( unRefPtr(refclass), true, false, 1 );

    return true;
}


bool testWeakPtr()
{
    bool deleted = false;
    ReferencedClass* refclass = new ReferencedClass( &deleted );

    WeakPtr<ReferencedClass> obsptr = refclass;
    mRunStandardTest( obsptr.get().ptr()==0,
		      "Setting unreffed class should give NULL");

    RefMan<ReferencedClass> refman1 = new ReferencedClass( &deleted );
    obsptr = refman1;

    mRunStandardTest( obsptr.get().ptr(), "WeakPtr is set" );

    refman1 = 0;

    mRunStandardTest( !obsptr.get().ptr(), "WeakPtr is is unset on last unref" );

    refman1 = new ReferencedClass( &deleted );
    obsptr = refman1;

    RefMan<ReferencedClass> refman2 = new ReferencedClass( &deleted );
    obsptr = refman2;

    refman1 = 0;
    mRunStandardTest( obsptr.get().ptr(), "WeakPtr updates to new object." );

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRefCount() ||
	 !testWeakPtr() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
