/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bindatadesc.h"
#include "string2.h"
#include "separstr.h"


union _BDD_union
{
    unsigned char c;
    struct bits {
	unsigned char	bytepow:3;	// nrbytes_per_sample == 2^bytepow
	unsigned char	isint:1;	// integer == 1
	unsigned char	issigned:1;	// signed data == 1
	unsigned char	rest:3;		// For who knows what
   } b;
};


void BinDataDesc::set( unsigned char c, unsigned char )
{
    setFrom( c, __islittle__ );
}


void BinDataDesc::setFrom( unsigned char c, bool wronlittle )
{
    bool needswp = wronlittle != __islittle__;
    int nb = 1;
    if ( needswp )
    {
	union _BDD_union_swp
	{
	    unsigned char c;
	    struct bits {
		unsigned char	rest:3;
		unsigned char	issigned:1;
		unsigned char	isint:1;
		unsigned char	bytepow:3;
	   } b;
	};
	_BDD_union_swp bdd; bdd.c = c;
	isint_ = bdd.b.isint;
	issigned_ = bdd.b.issigned;
	while( bdd.b.bytepow ) { bdd.b.bytepow--; nb *= 2; }
    }
    else
    {
	_BDD_union bdd; bdd.c = c;
	isint_ = bdd.b.isint;
	issigned_ = bdd.b.issigned;
	while( bdd.b.bytepow ) { bdd.b.bytepow--; nb *= 2; }
    }
    nrbytes_ = sCast(BinDataDesc::ByteCount,nb);
};


void BinDataDesc::set( const char* s )
{
    if ( !s || !*s ) return;

    FileMultiString fms( s );
    StringView res = fms[0];
    isint_ = res.firstChar() != 'F' && res.firstChar() != 'f';
    res = fms[1];
    issigned_ = res.firstChar() == 'S' || res.firstChar() == 's';
    res = fms[2];
    if ( !res.isEmpty() )
	nrbytes_ = nearestByteCount( isint_, toInt(res.buf()) );
}


void BinDataDesc::dump( unsigned char& c, unsigned char&  b ) const
{
    _BDD_union bdd;
    bdd.c = 0;
    bdd.b.isint = isint_ ? 1 : 0;
    bdd.b.issigned = issigned_ ? 1 : 0;
    bdd.b.bytepow = 0; int nb = nrbytes_;
    while ( nb > 1 ) { bdd.b.bytepow++; nb /= 2; }

    c = bdd.c;
    b = 0;
}


void BinDataDesc::toString( BufferString& buf ) const
{
    FileMultiString fms;
    fms.add( isint_ ? "Integer" : "Float" )
	.add( issigned_ ? "Signed" : "Unsigned" )
	.add( nrbytes_ );
    buf = fms;
}


bool BinDataDesc::convertsWellTo( const BinDataDesc& dd ) const
{
    if ( nrbytes_ > dd.nrbytes_ )
	return true;

    if ( nrbytes_ < dd.nrbytes_ )
	return false;

    if ( !dd.isint_ ) return true;
    if ( !isint_ ) return false;
    return dd.issigned_ == issigned_;
}
