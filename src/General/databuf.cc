/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 6-9-1995
 * FUNCTION : Data buffer implementation
-*/
 
static const char* rcsID = "$Id: databuf.cc,v 1.2 1999-12-10 11:33:07 bert Exp $";
 
#include "databuf.h"
#include <malloc.h>


DataBuffer::DataBuffer( int n, int byts, bool init )
	: data(0)
	, nelem(n)
	, bytes(byts)
{
    if ( nelem )
    {
	data = mMALLOC(nelem*bytes,unsigned char);
	if ( init ) clear();
    }
}


DataBuffer::~DataBuffer()
{
    if ( data ) free( data );
}


DataBuffer& DataBuffer::operator=( const DataBuffer& tb )
{
    if ( &tb != this )
    {
	reSize( tb.size() );
	bytes = tb.bytes;
	if ( data ) memcpy( data, tb.data, nelem*bytes );
    }

    return *this;
}


void DataBuffer::reSize( int n )
{
    if ( n == nelem ) return;

    if ( data )
    {
	if ( n )
	    data = mREALLOC(data,n*bytes,unsigned char);
	else
	    { free( data ); data = 0; }
    }
    else if ( n )
	data = mMALLOC(n*bytes,unsigned char);

    nelem = data ? n : 0;
}


void DataBuffer::reByte( int n )
{
    if ( n == bytes ) return;

    bytes = n;
    n = nelem;
    nelem = -1;
    reSize( n );
}


void DataBuffer::clear()
{
    if ( data ) memset( (char*)data, 0, nelem*bytes );
}
