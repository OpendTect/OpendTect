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
#include "ptrman.h"


class ReferencedClass
{ mRefCountImpl(ReferencedClass);
public:
    ReferencedClass(bool* delflag )
	: deleteflag_( delflag )
    {}


    bool*		deleteflag_;
};


ReferencedClass::~ReferencedClass()
{
    *deleteflag_ = true;
}

#define mRunTest( voiddo, test, delstatus, rc ) \
deleted = false; \
voiddo; \
if ( !(test) || delstatus!=deleted || (rc>=0 && rc!=refclass->nrRefs() )) \
{ \
    od_cout() << "Test " << #voiddo << " " << #test << " FAILED\n"; \
    ExitProgram( 1 ); \
} \
else if ( !quiet ) \
{ \
    od_cout() << "Test " << #voiddo << " " << #test << " - SUCCESS\n"; \
}


int main( int argc, char** argv )
{
    mInitTestProg();

    bool deleted = false;
    ReferencedClass* refclass = new ReferencedClass( &deleted );

    mRunTest( , refclass->refIfReffed()==false, false, 0 );
    mRunTest( refclass->ref(), true, false, 1 );
    mRunTest( , refclass->refIfReffed()==true, false, 2 );
    mRunTest( refclass->unRef(), true, false, 1 );
    mRunTest( refclass->unRefNoDelete(), true, false, 0 );
    mRunTest( refclass->ref(), true, false, 1 );
    mRunTest( unRefAndZeroPtr( refclass ), refclass==0, true, mInvalidRefCount );

    //Test null pointers
    mRunTest( refPtr(refclass), true, false, mInvalidRefCount );
    mRunTest( unRefPtr(refclass), true, false, mInvalidRefCount );

    refclass = new ReferencedClass( &deleted );
    RefMan<ReferencedClass> rptr = refclass;
    mRunTest( refPtr(refclass), true, false, 2 );
    mRunTest( refPtr(refclass), true, false, 3 );
    mRunTest( unRefPtr(refclass), true, false, 2 );
    mRunTest( unRefPtr(refclass), true, false, 1 );

    return ExitProgram( 0 );
}
