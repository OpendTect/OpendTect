/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "typeset.h"
#include "objectset.h"
#include "testprog.h"
#include "refcount.h"


class MacroReferencedClass
{ mRefCountImpl(MacroReferencedClass);
public:
    MacroReferencedClass(bool* delflag )
	: deleteflag_( delflag )
    {}


    bool*		deleteflag_;
};


MacroReferencedClass::~MacroReferencedClass()
{
    *deleteflag_ = true;
}


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

template <class T>
bool testRefCount()
{
    bool deleted = false;
    T* refclass = new T( &deleted );

    mRunTest( , refclass->refIfReffed()==false, false, 0 );
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

    refclass = new T( &deleted );
    RefMan<T> rptr = refclass;
    mRunTest( refPtr(refclass), true, false, 2 );
    mRunTest( refPtr(refclass), true, false, 3 );
    mRunTest( unRefPtr(refclass), true, false, 2 );
    mRunTest( unRefPtr(refclass), true, false, 1 );

    return true;
}


template <class T>
bool testObsPtr()
{
    bool deleted = false;
    T* refclass = new T( &deleted );

    ObsPtr<T> obsptr = refclass;
    mRunStandardTest( obsptr.get().ptr()==0,
		      "Setting unreffed class should give NULL");

    RefMan<T> refman1 = new T( &deleted );
    obsptr = refman1;

    mRunStandardTest( obsptr.get().ptr(), "ObsPtr is set" );

    refman1 = 0;

    mRunStandardTest( !obsptr.get().ptr(), "ObsPtr is is unset on last unref" );

    refman1 = new T( &deleted );
    obsptr = refman1;

    RefMan<T> refman2 = new T( &deleted );
    obsptr = refman2;

    refman1 = 0;
    mRunStandardTest( obsptr.get().ptr(), "ObsPtr updates to new object." );

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRefCount<MacroReferencedClass>() ||
	 !testRefCount<ReferencedClass>() ||
	 !testObsPtr<ReferencedClass>() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
