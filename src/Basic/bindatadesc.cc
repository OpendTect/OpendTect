/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2000
 * FUNCTION : Interpret data buffers
-*/

static const char* rcsID = "$Id: bindatadesc.cc,v 1.1 2001-02-19 11:26:41 bert Exp $";

#include "bindatadesc.h"
#include "separstr.h"


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
    FileMultiString fms( s );
    const int sz = fms.size();
    if ( sz > 0 )
	isint = *fms[0] == 'F' || *fms[0] == 'f';
    if ( sz > 1 )
	issigned = *fms[1] == 'S' || *fms[1] == 's';
    if ( sz > 2 )
	nrbytes = nearestByteCount( isint, atoi( fms[2] ) );
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


BufferString BinDataDesc::toString() const
{
    FileMultiString fms( isint ? "Integer" : "Float" );
    fms += issigned ? "Signed" : "Unsigned";
    fms += (int)nrbytes;

    return BufferString( (const char*)fms );
}


bool BinDataDesc::convertsWellTo( const BinDataDesc& dd ) const
{
    if ( (int)nrbytes > (int)dd.nrbytes ) return true;
    if ( (int)nrbytes < (int)dd.nrbytes ) return false;
    if ( !dd.isint ) return true;
    if ( !isint ) return false;
    return dd.issigned == issigned;
}
