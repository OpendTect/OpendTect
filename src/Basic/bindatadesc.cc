/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2001
 * FUNCTION : Binary data descritpion
-*/

static const char* rcsID = "$Id: bindatadesc.cc,v 1.2 2001-02-22 08:21:20 bert Exp $";

#include "bindatadesc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


union _BDD_union
{
    unsigned short c;
    struct bits {
	unsigned char	bytepow:3;	// nrbytes_per_sample == 2^bytepow
	unsigned char	isint:1;	// integer == 1
	unsigned char	issigned:1;	// signed data == 1
	unsigned char	rest:3;		// For who knows what
   } b;
};


void BinDataDesc::set( unsigned short c )
{
    _BDD_union bdd; bdd.c = c;

    isint = bdd.b.isint;
    issigned = bdd.b.issigned;
    int nb = 1;
    while( bdd.b.bytepow ) { bdd.b.bytepow--; nb *= 2; }
    nrbytes = (BinDataDesc::ByteCount)nb;
};


void BinDataDesc::set( const char* s )
{
    if ( !s || !*s ) return;

    const char* ptr = strchr( s, '`' );
    isint = *s != 'F' && *s != 'f';
    if ( !ptr ) return;

    s = ptr + 1;
    ptr = strchr( s, '`' );
    issigned = *s == 'S' || *s == 's';
    if ( ptr )
	nrbytes = nearestByteCount( isint, atoi( ptr+1 ) );
}


unsigned short BinDataDesc::dump() const
{
    _BDD_union bdd;
    bdd.b.isint = isint ? 1 : 0;
    bdd.b.issigned = issigned ? 1 : 0;
    bdd.b.bytepow = 0; int nb = nrbytes;
    while ( nb > 1 ) { bdd.b.bytepow++; nb /= 2; }

    return bdd.c;
}


void BinDataDesc::toString( char* buf ) const
{
    if ( !buf ) return;

    sprintf( buf, "%s`%s`%d",
		  isint ? "Integer" : "Float",
		  issigned ? "Signed" : "Unsigned",
		  (int)nrbytes );
}


bool BinDataDesc::convertsWellTo( const BinDataDesc& dd ) const
{
    if ( (int)nrbytes > (int)dd.nrbytes ) return true;
    if ( (int)nrbytes < (int)dd.nrbytes ) return false;
    if ( !dd.isint ) return true;
    if ( !isint ) return false;
    return dd.issigned == issigned;
}
