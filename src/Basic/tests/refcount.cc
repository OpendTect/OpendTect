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
#include "manobjectset.h"



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

    mRunStandardTest( !obsptr.get().ptr(),
		      "WeakPtr is is unset on last unref" );

    refman1 = new ReferencedClass( &deleted );
    obsptr = refman1;

    RefMan<ReferencedClass> refman2 = new ReferencedClass( &deleted );
    obsptr = refman2;

    refman1 = 0;
    mRunStandardTest( obsptr.get().ptr(), "WeakPtr updates to new object." );

    return true;
}


bool testRefObjectSet()
{
    {
	bool deleted1 = false, deleted2 = false;;
	ObjectSet<ReferencedClass> normal_os;
	normal_os += new ReferencedClass( &deleted1 );
	normal_os += new ReferencedClass( &deleted2 );

	mRunStandardTest( !deleted1 && !deleted2,
			  "Normal objectsets not deleted");

	{
	    RefObjectSet<ReferencedClass> ref_os;
	    ref_os = normal_os;

	    mRunStandardTest( !deleted1 && !deleted2,
			     "Not unreffed after adding to RefObjectSet");
	}

	mRunStandardTest( deleted1 && deleted2,
			 "Unreffed after RefObjectSet goes out of scope");

	normal_os.erase();
    }
    {
	bool deleted1 = false, deleted2 = false;
	RefObjectSet<ReferencedClass> ref_os;
	ReferencedClass* referenced = new ReferencedClass( &deleted1 );
	ref_os += referenced;
	ref_os += new ReferencedClass( &deleted2 );

	ref_os -= referenced;

	mRunStandardTest( deleted1 && !deleted2, "Unreffed after -= operator");
    }
    {
	bool deleted1 = false, deleted2 = false;
	RefObjectSet<ReferencedClass> ref_os;

	ref_os += new ReferencedClass( &deleted1 );
	ref_os += new ReferencedClass( &deleted2 );

	ref_os.swap( 0, 1 );
	mRunStandardTest( !deleted1 && !deleted2,
			 "No unref during swap");

	ref_os.removeSingle( 0 );

	mRunStandardTest( !deleted1 && deleted2,
			  "Unref after swap and removeSingle");
    }
    {
	bool deleted1 = false, deleted2 = false;
	RefObjectSet<ReferencedClass> ref_os;

	ref_os += new ReferencedClass( &deleted1 );
	ref_os += new ReferencedClass( &deleted2 );

	ref_os.removeRange( 0, 1);

	mRunStandardTest( deleted1 && deleted2,
			 "Unref after removeRange");
    }
    {
        bool deleted1 = false, deleted2 = false;
        RefObjectSet<ReferencedClass> ref_os;
        
        ref_os += new ReferencedClass( &deleted1 );
        ref_os += new ReferencedClass( &deleted2 );

        ref_os = RefObjectSet<ReferencedClass>();
        
        mRunStandardTest( deleted1 && deleted2,
                         "Unref after whole set assignment");
    }


    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRefCount() ||
	!testWeakPtr() ||
	!testRefObjectSet() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
