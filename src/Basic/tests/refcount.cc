/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "objectset.h"
#include "ptrman.h"
#include "refcount.h"
#include "testprog.h"
#include "typeset.h"


class ReferencedClass : public ReferencedObject
{
public:
ReferencedClass( bool* delflag )
    : deleteflag_(delflag)
{}

protected:
~ReferencedClass()
{
    *deleteflag_ = true;
}

private:

    bool*	deleteflag_;
};


#undef mRunTest
#define mRunTest( voiddo, test, delstatus, rc ) \
deleted = false; \
voiddo; \
mRunStandardTest( !((refclass && !(test)) || delstatus!=deleted || \
		   (rc>=0 && refclass && rc!=refclass->nrRefs()) ), \
		   BufferString(#voiddo, " ", #test).trimBlanks() )

bool testRefCount()
{
    bool deleted = false;
    auto* refclass = new ReferencedClass( &deleted );

    mRunTest( , refclass->refIfReffed()==false, false,
	      RefCount::Counter::cStartRefCount() );
    mRunTest( refclass->ref(), true, false, 1 );
    mRunTest( , refclass->refIfReffed()==true, false, 2 );
    mRunTest( refclass->unRef(), true, false, 1 );
    mRunTest( refclass->unRefNoDelete(), true, false, 0 );
    mRunTest( refclass->ref(), true, false, 1 );
    mRunTest( unRefAndNullPtr( refclass ), refclass==nullptr, true,
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
    auto* refclass = new ReferencedClass( &deleted );

    //This will cause a prog-error, but should still be
    //handled properly
    const bool oldstatus = DBG::setCrashOnProgError( false );
    WeakPtr<ReferencedClass> obsptr = refclass;
    mRunStandardTest( obsptr.get().ptr()==nullptr,
		      "Setting unreffed class should give NULL");
    DBG::setCrashOnProgError( oldstatus );

    RefMan<ReferencedClass> refman1 = new ReferencedClass( &deleted );
    obsptr = refman1;

    mRunStandardTest( obsptr.get().ptr(), "WeakPtr is set" );

    refman1 = nullptr;

    mRunStandardTest( !obsptr.get().ptr(),
		      "WeakPtr is is unset on last unref" );

    refman1 = new ReferencedClass( &deleted );
    obsptr = refman1;

    RefMan<ReferencedClass> refman2 = new ReferencedClass( &deleted );
    obsptr = refman2;

    refman1 = nullptr;
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
	auto* referenced = new ReferencedClass( &deleted1 );
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

	ref_os.swapItems( 0, 1 );
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
    {
	bool deleted1 = false, deleted2 = false;
	RefObjectSet<ReferencedClass> ref_os;
	ref_os += new ReferencedClass( &deleted1 );

	RefMan<ReferencedClass> holder2 = new ReferencedClass( &deleted2 );
	ref_os.replace( 0, holder2.ptr() );

	mRunStandardTest( deleted1 && !deleted2 && holder2->nrRefs()==2,
			 "Number of refs after RefObjectSet::replace");
    }
    {
	bool deleted1 = false, deleted2 = false;
	RefObjectSet<ReferencedClass> ref_os;
	ref_os += new ReferencedClass( &deleted1 );

	RefMan<ReferencedClass> holder2 = new ReferencedClass( &deleted2 );
	ref_os.insertAt( holder2.ptr(), 0 );

	mRunStandardTest( !deleted1 && !deleted2 && holder2->nrRefs()==2,
			 "Number of refs after RefObjectSet::insertAt");
    }
    {
	bool deleted1 = false, deleted2 = false;
	RefObjectSet<ReferencedClass> ref_os;
	ref_os += new ReferencedClass( &deleted1 );

	RefMan<ReferencedClass> holder2 = new ReferencedClass( &deleted2 );
	ref_os.insertAfter( holder2.ptr(), 0 );

	mRunStandardTest( !deleted1 && !deleted2 && holder2->nrRefs()==2,
			 "Number of refs after RefObjectSet::insertAfter");
    }


    return true;
}


class NotReferenced
{
public:
    virtual ~NotReferenced() {}
    virtual void init() {}
    int var = 0;
};

bool testSanityCheck()
{
    PtrMan<NotReferenced> ptr = new NotReferenced;
    mRunStandardTest(
	    !RefCount::Referenced::isSane((RefCount::Referenced*) ptr.ptr()),
	    "Sanity check of false \"Referenced\" pointers");
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRefCount() ||
	 !testWeakPtr() ||
	 !testRefObjectSet() ||
	 !testSanityCheck() )
	return 1;

    return 0;
}
