/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2000
 * FUNCTION : Interpret data buffers
-*/

static const char* rcsID = "$Id: datainterp.cc,v 1.22 2008-10-21 03:39:01 cvsnanne Exp $";

#include "datachar.h"
#include "separstr.h"

DefineEnumNames(DataCharacteristics,UserType,1,"Data storage") {
	"0 - auto",
	"1 - 8  bit signed",
        "2 - 8  bit unsigned",
        "3 - 16 bit signed",
        "4 - 16 bit unsigned",
        "5 - 32 bit signed",
        "6 - 32 bit unsigned",
        "7 - 32 bit floating point",
        "8 - 64 bit floating point",
        "9 - 64 bit signed",
	0 };


void DataCharacteristics::set( unsigned char c1, unsigned char c2 )
{
    // remember that the 'zero' member is always zero.
    littleendian = (c2 & 0x80) || (c2 & 0x01);
    setFrom( c1, littleendian );

    unsigned char f = (c2 & 0x0e) >> 1;
    if ( !f )
    {
	f = (c2 & 0x70) >> 4;
	unsigned char g = 0;
	if ( f&0x04 ) g |= 0x01;
	if ( f&0x02 ) g |= 0x02;
	if ( f&0x01 ) g |= 0x04;
	f = g;
    }
    fmt = (DataCharacteristics::Format)f;
};


void DataCharacteristics::set( const char* s )
{
    BinDataDesc::set( s );
    FileMultiString fms( s );
    const int sz = fms.size();
    if ( sz > 3 )
	fmt = matchStringCI( "ibm", fms[1] ) ? DataCharacteristics::Ibm
					     : DataCharacteristics::Ieee;
    if ( sz > 4 )
	littleendian = yesNoFromString( fms[4] );
}


DataCharacteristics::DataCharacteristics( DataCharacteristics::UserType ut )
	: BinDataDesc( ut!=F32 && ut!=F64, ut>UI32 || (int)ut % 2)
	, fmt(Ieee)
	, littleendian(__islittle__)
{
    if ( ut == Auto )
	*this = DataCharacteristics();
    else
	nrbytes = (BinDataDesc::ByteCount)
		  (ut < SI16 ? 1 : (ut < SI32 ? 2 : (ut > F32 ? 8 : 4) ) );
}


void DataCharacteristics::dump( unsigned char& c1, unsigned char& c2 ) const
{
    union _DC_union { unsigned char c;
	struct bits { unsigned char islittle:1; unsigned char fmt:3;
	    	      unsigned char zero:4; } b; };
    _DC_union dc; dc.c = 0;

    BinDataDesc::dump( c1, c2 );
    dc.b.fmt = isIeee() ? 0 : 1;
    dc.b.islittle = littleendian;
    c2 = dc.c;
}


void DataCharacteristics::toString( char* buf ) const
{
    if ( !buf ) return;

    BinDataDesc::toString( buf );

    FileMultiString fms( buf );
    fms += isIeee() ? "IEEE" : "IBMmf";
    fms += getYesNoString( littleendian );

    strcpy( buf, (const char*)fms );
}


DataCharacteristics::UserType DataCharacteristics::userType() const
{
    switch ( nrBytes() )
    {
    case N1: return isSigned() ? SI8 : UI8;
    case N2: return isSigned() ? SI16 : UI16;
    case N4: return isInteger() ? (isSigned() ? SI32 : UI32) : F32;
    case N8: return isInteger() ? SI64 : F64;
    }
    return Auto;
}
