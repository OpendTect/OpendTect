/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2001
 * FUNCTION : Binary data descritpion
-*/

static const char* rcsID = "$Id: bindatadesc.cc,v 1.3 2001-12-09 09:29:30 bert Exp $";

#include "bindatadesc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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


void BinDataDesc::dump( unsigned char& c, unsigned char& ) const
{
    _BDD_union bdd;
    bdd.c = 0;
    bdd.b.isint = isint ? 1 : 0;
    bdd.b.issigned = issigned ? 1 : 0;
    bdd.b.bytepow = 0; int nb = nrbytes;
    while ( nb > 1 ) { bdd.b.bytepow++; nb /= 2; }

    c = bdd.c;
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
