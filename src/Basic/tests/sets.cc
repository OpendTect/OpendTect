/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "typeset.h"
#include "testprog.h"
#include "bufstringset.h"


class DataElem
{
public:

		DataElem( int i=0, float v=0 )
		    : id_(i), val_(v)	{}
    bool	operator ==( const DataElem& de ) const
		{ return id_ == de.id_; }
    bool	operator !=( const DataElem& de ) const
		{ return !(*this == de); }
    bool	operator <( const DataElem& de ) const
		{ return val_ < de.val_; }

    int		id_;
    float	val_;

    void	print() const	{ od_cout() << '['<< id_<<": "<<val_<<']'; }

};

#define mPrElems(msg ) if ( !quiet_ ) { \
    od_cout() << msg << od_endl << '\t'; \
    for ( int idx=0; idx<des.size(); idx++ ) \
	{ mPrintFunc; od_cout() << " | "; } \
    od_cout() << od_endl; }

#define mErrRet(msg ) \
{ \
    if ( !quiet_ ) { \
    mPrElems("-> Failure ..." ) \
    od_cout() << msg << " failed.\n"; } \
    return false; \
}

#define mRetAllOK() \
    { logStream() << "All OK.\n" << od_endl; } \
    return true;

#define mPrintFunc des[idx].print()


static bool testTypeSetFind()
{
    od_cout() << od_endl;
    TypeSet<DataElem> des( 6, DataElem() );
    des[0] = DataElem( 1, 0.1f );
    des[1] = DataElem( 2, 0.2f );
    des[2] = DataElem( 3, 0.3f );
    des[3] = DataElem( 1, 0.4f );
    des[4] = DataElem( 7, 0.1f );
    des[5] = DataElem( 8, 0.1f );
    const DataElem& des0 = des[0];
    const DataElem& des3 = des[3];
    mPrElems("testTypeSetFind")

    if ( des.count(des0) != 2 )
	mErrRet("count" );

    if ( des.indexOf( des0, true, -1 ) != 0 )
	mErrRet("indexOf elem0 != 0");

    if ( des.indexOf( des0, true, 1 ) != 3 )
	mErrRet("indexOf 2nd match not right");

    if ( des.indexOf( des0, false, -1 ) != 3 )
	mErrRet("backward indexOf");

    if ( des.indexOf( des0, false, 1 ) != 0 )
	mErrRet("backward indexOf with offset");

    if ( !des.isPresent(des3) )
	mErrRet("isPresent fails for 3rd element" );

    if ( des.isPresent( DataElem(12,0.1) ) )
	mErrRet("isPresent returns true for non-existing");

    mRetAllOK()
}


static bool testTypeSetSetFns()
{
    od_cout() << od_endl;
    TypeSet<DataElem> org;
    TypeSet<DataElem> des;
    org += DataElem( 1, 0.1 );
    org += DataElem( 2, 0.2 );
    org += DataElem( 3, 0.3 );
    org += DataElem( 4, 0.4 );

    des.swap( org );
    if ( des.size() != 4 || !org.isEmpty() )
	mErrRet("swap()" );

    const DataElem des0( des[0] );
    const DataElem des1( des[1] );
    const DataElem des2( des[2] );
    const DataElem des3( des[3] );
    mPrElems("testTypeSetSetFns")

    int curidx = 0;
    for ( auto elem=des.cbegin(); elem!=des.cend(); ++elem )
    {
	if ( des.getIdx(elem) != curidx )
	    mErrRet("getIdx()" );
	curidx++;
    }

    DataElem el( des.pop() );
    if ( des.size() != 3 || des[0] != des0 || des[1] != des1 || des[2] != des2 )
	mErrRet("pop()" );
    if ( el != des3 )
	mErrRet("return value of pop()" );
    des += des3;

    des -= des2;
    if ( des.size() != 3 || des[0] != des0 || des[1] != des1 || des[2] != des3 )
	mErrRet("operator -=()" );
    des.insert( 2, des2 );
    if ( des.size() != 4 || des[0] != des0 || des[1] != des1 || des[2] != des2
			 || des[3] != des3 )
	mErrRet("insert()" );

    des.pop(); des.pop();
    des.setAll( des0 );
    if ( des.size() != 2 || des[0] != des0 || des[1] != des0 )
	mErrRet("setAll() or previous pop()'s" );


    mRetAllOK()
}

#undef mPrintFunc
#define mPrintFunc od_cout().addPtr( des[idx] )

static bool testObjSetFind()
{
    od_cout() << od_endl;
    ObjectSet<DataElem> des;
    des += new DataElem( 1, 0.1 );
    des += new DataElem( 2, 0.2 );
    des += new DataElem( 3, 0.3 );
    des += new DataElem( 1, 0.4 );
    des += new DataElem( 7, 0.1 );
    des += new DataElem( 8, 0.1 );
    DataElem* des0 = des[0];
    DataElem* des4 = des[3];

    des.insertAt( des0, 3 );

    mPrElems("testObjSetFind")

    if ( des.indexOf( des0 ) != 0 )
	mErrRet("indexOf elem0 != 0");

    if ( !des.isPresent(des4) )
	mErrRet("isPresent fails for 4th element" );

    if ( des.isPresent( 0 ) )
	mErrRet("isPresent returns true for non-existing");

    des.swapItems( 0, 4 );

    if ( des.indexOf( des4 ) != 0 )
	mErrRet("indexOf swapped elem3 != 0");

    while ( des.size() )
    {
	DataElem* elem = des.removeSingle(0);
	while ( des.isPresent(elem) )
	{
	    des.removeSingle( des.indexOf(elem) );
	}

	delete elem;
    }

    mRetAllOK()
}

static bool testObjSetEqual()
{
    ObjectSet<DataElem> s1;
    ObjectSet<DataElem> s2; s2.setNullAllowed();
    ObjectSet<DataElem>& des = s1;
    mPrElems("testObjSetEqual")

    if ( !equalContents(s1,s2) )
	mErrRet("empty sets not equal");

    s1 += new DataElem( 1, 0.1 );
    if ( equalContents(s1,s2) )
	mErrRet("different sizes equal");

    s2 += 0;
    if ( equalContents(s1,s2) )
	mErrRet("null ptr equal real element");

    delete s2.replace( 0, new DataElem( 1, 0.2 ) );
    if ( !equalContents(s1,s2) )
	mErrRet("Single-element sets not equal");

    s1 += new DataElem( 2, 0.2 );
    s2 += new DataElem( 2, 0.2 );
    if ( !equalContents(s1,s2) )
	mErrRet("Multi-element sets not equal");

    delete s1.replace( 0, new DataElem( 3, 0.2 ) );
    if ( equalContents(s1,s2) )
	mErrRet("Multi-element sets equal");

    deepErase( s1 );
    deepErase( s2 );

    mRetAllOK()
}


static bool testTypeSetSort()
{
    TypeSet<int> vals;
    vals.add( 10 ).add( 0 ).add( 20 ).add( 5 ).add( 0 ).add( -4 ).add( 15 );
    // 5 1 4 3 0 6 2
    int* idxarr = getSortIndexes( vals );
    TypeSet<int> idxs( idxarr, vals.size() );
    delete [] idxarr;
    TypeSet<int> expected;
    expected.add( 5 ).add( 1 ).add( 4 ).add( 3 ).add( 0 ).add( 6 ).add( 2 );
    mRunStandardTest( idxs == expected, "Sort Indexes" )

    TypeSet<int> sortedvals( vals );
    sortedvals.useIndexes( idxs.arr() );
    expected.setEmpty();
    expected.add( -4 ).add( 0 ).add( 0 ).add( 5 ).add( 10 ).add( 15 ).add( 20 );
    mRunStandardTest( sortedvals == expected, "Sort Values" )

    return true;
}


static bool testSetCapacity()
{
    TypeSet<int> vec;
    mRunStandardTest( vec.setCapacity( 4, true ) &&
		      vec.getCapacity()==4, "Set capacity 4" );

    mRunStandardTest( vec.setCapacity( 5, true ) &&
		      vec.getCapacity()==8, "Set capacity 5" );

    mRunStandardTest( vec.setCapacity( 4, true ) &&
		      vec.getCapacity()==8, "re-set capacity 4" );

    return true;
}


class TestClass
{
    public:
	TestClass(int& deletedflag)
	    : deleted_( deletedflag )
	{}

    ~TestClass()
    {
	deleted_++;
    }

    int& deleted_;
};


static bool testManagedObjectSet()
{

    int delcount = 0;
    {
	ManagedObjectSet<ManagedObjectSet<TestClass> > set1;

	TestClass* tc = new TestClass( delcount );
	ManagedObjectSet<TestClass>* set2 = new ManagedObjectSet<TestClass>();
	set2->push( tc );
	set1.push( set2 );

	set1.erase();
	mRunStandardTest( delcount, "Erasing nested managed objectsets" );

	delcount = 0;
	tc = new TestClass( delcount );
	set2 = new ManagedObjectSet<TestClass>();
	set2->push( tc );
	set1.push( set2 );
    }

    mRunStandardTest( delcount, "Deleting nested managed objectsets" );

    delcount = 0;
    {
	ManagedObjectSet<TestClass> set1;
	set1 += new TestClass( delcount );

	{
	    ManagedObjectSet<TestClass> set2( set1 );
	    mRunStandardTest( set1[0] != set2[0], "Copy constructor clones" );

	    ManagedObjectSet<TestClass> set3 = set1;
	    mRunStandardTest( set1[0] != set3[0],
			      "Assignment at construct clones" );

	    ManagedObjectSet<TestClass> set4;
	    set4 = set1;
	    mRunStandardTest( set1[0] != set4[0],
			      "Assignment operator clones" );

	}

	mRunStandardTest( delcount==3,
			  "Deleting after copy constructor - Part 1" );
    }
    mRunStandardTest( delcount==4,
		      "Deleting after copy constructor - Part 2" );

    delcount = 0;
    {
	ManagedObjectSet<TestClass> set1;
	set1 += new TestClass( delcount );

	{
	    ManagedObjectSet<TestClass> set2;
	    ObjectSet<TestClass>& set2ref = set2;
	    set2ref = set1;

	    mRunStandardTest( set1[0] != set2[0],
			      "ObjectSet cast clones" );
	}

	mRunStandardTest( delcount==1,
			  "Count after objectset cast construct" );
    }

    delcount = 0;
    {
	ManagedObjectSet<TestClass> set1;
	set1 += new TestClass( delcount );

	{
	    ManagedObjectSet<TestClass> set2;
	    ObjectSet<TestClass>& set1ref = set1;
	    set2 = set1ref;

	    mRunStandardTest( set1[0] != set2[0],
			      "Set operation clones" );
	}
    }

    return true;
}

static bool testStringSetFns()
{
    BufferStringSet bss;
    bss.add( "Aap" );
    bss.add( "Noot" );

    int idx = 0;
    for ( auto bsptr : bss )
    {
	if ( idx == 0 )
	    { mRunStandardTest( *bsptr == "Aap", "BufferStringSet iter 0" ) }
	else if ( idx == 1 )
	    { mRunStandardTest( *bsptr == "Noot", "BufferStringSet iter 1" ) }
	else
	    { mRunStandardTest( false, "BufferStringSet iter 2" ) }
	idx++;
    }

    return true;
}


#define mDoSetTest(nm) \
    if ( !nm() ) \
	return 1


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    mDoSetTest( testTypeSetSort );
    mDoSetTest( testTypeSetFind );
    mDoSetTest( testTypeSetSetFns );
    mDoSetTest( testObjSetFind );
    mDoSetTest( testObjSetEqual );
    mDoSetTest( testSetCapacity );
    mDoSetTest( testManagedObjectSet );
    mDoSetTest( testStringSetFns );

    return 0;
}
