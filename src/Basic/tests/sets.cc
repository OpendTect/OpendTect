/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "arrayndimpl.h"
#include "manobjectset.h"
#include "typeset.h"


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
    return 1; \
}

#define mRetAllOK() \
    if ( !quiet_ ) { \
    od_cout() << "All OK.\n" << od_endl; } \
    return 0;

#define mPrintFunc des[idx].print()


static int testTypeSetFind()
{
    od_cout() << od_endl;
    TypeSet<DataElem> des( 6, DataElem() );
    des[0] = DataElem( 1, 0.1 );
    des[1] = DataElem( 2, 0.2 );
    des[2] = DataElem( 3, 0.3 );
    des[3] = DataElem( 1, 0.4 );
    des[4] = DataElem( 7, 0.1 );
    des[5] = DataElem( 8, 0.1 );
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

    if ( des.indexOf( des0, false, 2 ) != 0 )
	mErrRet("backward indexOf with offset");

    if ( !des.isPresent(des3) )
	mErrRet("isPresent fails for 3rd element" );

    if ( des.isPresent( DataElem(12,0.1) ) )
	mErrRet("isPresent returns true for non-existing");

    mRetAllOK()
}


static int testTypeSetSetFns()
{
    od_cout() << od_endl;
    TypeSet<DataElem> des;
    des += DataElem( 1, 0.1 );
    des += DataElem( 2, 0.2 );
    des += DataElem( 3, 0.3 );
    des += DataElem( 4, 0.4 );
    const DataElem des0( des[0] );
    const DataElem des1( des[1] );
    const DataElem des2( des[2] );
    const DataElem des3( des[3] );
    mPrElems("testTypeSetSetFns")

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

static int testObjSetFind()
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

    des.swap( 0, 4 );

    if ( des.indexOf( des4 ) != 0 )
	mErrRet("indexOf swapped elem3 != 0");

    while ( des.size() )
    {
	DataElem* elem = des.removeSingle(0);
	while ( des.isPresent(elem) )
	    des.removeSingle( des.indexOf(elem) );

	delete elem;
    }

    mRetAllOK()
}

static int testObjSetEqual()
{
    ObjectSet<DataElem> s1;
    ObjectSet<DataElem> s2; s2.allowNull();
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


static bool testTypeSetCapacity()
{
    TypeSet<int> vec;
    mRunStandardTest( vec.setCapacity( 4, true ) &&
		      vec.getCapacity()==4 && vec.isEmpty(),
		      "TypeSet capacity 4");

    mRunStandardTest( vec.setCapacity( 5, true ) &&
		      vec.getCapacity()==8 && vec.isEmpty(),
		      "TypeSet capacity 5" );

    mRunStandardTest( vec.setCapacity( 4, true ) &&
		      vec.getCapacity()==8 && vec.isEmpty(),
		      "re-TypeSet capacity 4" );
    return true;
}


static bool testObjectSetCapacity()
{
    ObjectSet<Array2D<float> > vec;
    mRunStandardTest( vec.setCapacity( 4, true ) &&
		      vec.getCapacity() == 4 && vec.isEmpty(),
		      "ObjectSet capacity 4" );

    mRunStandardTest( vec.setCapacity( 5, true ) &&
		      vec.getCapacity() == 8 && vec.isEmpty(),
		      "ObjectSet capacity 5" );

    mRunStandardTest( vec.setCapacity( 4, true ) &&
		      vec.getCapacity() == 8 && vec.isEmpty(),
		      "re-ObjectSet capacity 4" );

    mRunStandardTest( vec.setSize( 10 ) && vec.size() == 10 &&
		      !vec.get(9), "ObjectSize size 10 with nulls" );

    mRunStandardTest( vec.setSize( 4 ) && vec.size() == 4 &&
		      !vec.get(3), "ObjectSize re-size 4 with nulls" );

    vec.setCapacity( 5 , true );
    delete vec.replace( 1, new Array2DImpl<float>(3,5) );
    delete vec.replace( 3, new Array2DImpl<float>(2,6) );
    mRunStandardTest( vec.size() == 4 && vec.getCapacity() == 8 &&
		      !vec.get(0) && vec.get(1) && !vec.get(2) && vec.get(3),
		      "Vector with some nulls and some non-nulls" );
    deepErase( vec );

    mRunStandardTest( vec.isEmpty() && vec.getCapacity() == 0,
		      "Erased ObjectSet without size and capacity 0" );

    return true;
}


class TestClass
{
    public:
	TestClass(bool& deletedflag)
	    : deleted_( deletedflag )
	{}

	~TestClass()
	{
	    deleted_ = true;
	}

    bool& deleted_;
};


static bool testManagedObjectSet()
{
    bool delflag = false;
    {
	ManagedObjectSet<ManagedObjectSet<TestClass> > set1;

	auto* tc = new TestClass( delflag );
	auto* set2 = new ManagedObjectSet<TestClass>();
	set2->push( tc );
	set1.push( set2 );

	set1.erase();
	mRunStandardTest( delflag, "Erasing nested managed objectsets" );

	delflag = false;
	tc = new TestClass( delflag );
	set2 = new ManagedObjectSet<TestClass>();
	set2->push( tc );
	set1.push( set2 );
    }

    mRunStandardTest( delflag, "Deleting nested managed objectsets" );
    return true;
}


static bool testMove()
{
    TypeSet<int> vals;
    vals += 1;
    vals += 2;
    vals += 3;
    vals += 4;

    vals.move( 0, 0 );
    vals.move( 1, 1 );
    vals.move( 2, 2 );
    vals.move( 3, 3 );
    mRunStandardTest( vals[0] == 1 && vals[1] == 2 &&
		      vals[2] == 3 && vals[3] == 4, "void move operation" );

    vals.move( 0, 1 );
    mRunStandardTest( vals[0] == 2 && vals[1] == 1 &&
		      vals[2] == 3 && vals[3] == 4, "move 0->1 operation" );
    vals.move( 1, 2 );
    mRunStandardTest( vals[0] == 2 && vals[1] == 3 &&
		      vals[2] == 1 && vals[3] == 4, "move 1->2 operation" );
    vals.move( 2, 3 );
    mRunStandardTest( vals[0] == 2 && vals[1] == 3 &&
		      vals[2] == 4 && vals[3] == 1, "move 2->3 operation" );
    vals.move( 0, 2 );
    mRunStandardTest( vals[0] == 3 && vals[1] == 4 &&
		      vals[2] == 2 && vals[3] == 1, "move 0->2 operation" );
    vals.move( 1, 3 );
    mRunStandardTest( vals[0] == 3 && vals[1] == 2 &&
		      vals[2] == 1 && vals[3] == 4, "move 1->3 operation" );
    vals.move( 0, 3 );
    mRunStandardTest( vals[0] == 2 && vals[1] == 1 &&
		      vals[2] == 4 && vals[3] == 3, "move 0->3 operation" );
    vals.move( 3, 0 );
    mRunStandardTest( vals[0] == 3 && vals[1] == 2 &&
		      vals[2] == 1 && vals[3] == 4, "move 3->0 operation" );
    vals.move( 3, 1 );
    mRunStandardTest( vals[0] == 3 && vals[1] == 4 &&
		      vals[2] == 2 && vals[3] == 1, "move 3->1 operation" );
    vals.move( 2, 0 );
    mRunStandardTest( vals[0] == 2 && vals[1] == 3 &&
		      vals[2] == 4 && vals[3] == 1, "move 2->0 operation" );
    vals.move( 3, 2 );
    mRunStandardTest( vals[0] == 2 && vals[1] == 3 &&
		      vals[2] == 1 && vals[3] == 4, "move 3->2 operation" );
    vals.move( 2, 1 );
    mRunStandardTest( vals[0] == 2 && vals[1] == 1 &&
		      vals[2] == 3 && vals[3] == 4, "move 2->1 operation" );
    vals.move( 1, 0 );
    mRunStandardTest( vals[0] == 1 && vals[1] == 2 &&
		      vals[2] == 3 && vals[3] == 4, "move 1->0 operation" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    int res = testTypeSetFind();
    res += testTypeSetSetFns();
    res += testObjSetFind();
    res += testObjSetEqual();
    res += testTypeSetCapacity() ? 0 : 1;
    res += testObjectSetCapacity() ? 0 : 1;
    res += testManagedObjectSet() ? 0 : 1;
    res += testMove() ? 0 : 1;

    return res;
}
