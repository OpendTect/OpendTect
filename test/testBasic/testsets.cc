/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id: testsvnversion.cc 27628 2012-11-25 16:04:42Z kristofer.tingdahl@dgbes.com $";

#include "typeset.h"
#include "objectset.h"

#include <iostream>

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

    void	print() const	{ std::cerr << '['<< id_<<": "<<val_<<']'; }

};

#define mPrElems(msg) { \
    std::cerr << msg << std::endl << '\t'; \
    for ( int idx=0; idx<des.size(); idx++ ) \
	{ des[idx].print(); std::cerr << " | "; } \
    std::cerr << std::endl; }

#define mErrRet(msg) \
{ \
    mPrElems("-> Failure ...") \
    std::cerr << msg << " failed.\n"; \
    return 1; \
}

#define mRetAllOK() \
    std::cerr << "All OK.\n" << std::endl; \
    return 0;
	

static int testFindFns()
{
    std::cerr << std::endl;
    TypeSet<DataElem> des( 6, DataElem() );
    des[0] = DataElem( 1, 0.1 );
    des[1] = DataElem( 2, 0.2 );
    des[2] = DataElem( 3, 0.3 );
    des[3] = DataElem( 1, 0.4 );
    des[4] = DataElem( 7, 0.1 );
    des[5] = DataElem( 8, 0.1 );
    const DataElem& des0 = des[0];
    const DataElem& des3 = des[3];
    mPrElems("testFindFns")

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


static int testSetFns()
{
    std::cerr << std::endl;
    TypeSet<DataElem> des;
    des += DataElem( 1, 0.1 );
    des += DataElem( 2, 0.2 );
    des += DataElem( 3, 0.3 );
    des += DataElem( 4, 0.4 );
    const DataElem des0( des[0] );
    const DataElem des1( des[1] );
    const DataElem des2( des[2] );
    const DataElem des3( des[3] );
    mPrElems("testSetFns")

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



int main( int narg, char** argv )
{
    SetProgramArgs( narg, argv );

    int res = testFindFns();
    res += testSetFns();

    return res;
}
