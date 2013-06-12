/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "typeset.h"
#include "objectset.h"
#include "refcount.h"
#include "ptrman.h"
#include "commandlineparser.h"
#include "keystrs.h"
#include "debug.h"

#include <iostream>


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
std::cerr << "Test " << #voiddo << " " << #test << " FAILED\n"; \
return 1; \
} \
else if ( !quiet ) \
{ \
std::cerr << "Test " << #voiddo << " " << #test << " - SUCCESS\n"; \
}


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );

    CommandLineParser clparser;
    const bool quiet = clparser.hasKey( sKey::Quiet() );
    
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
    
    return 0;
}
