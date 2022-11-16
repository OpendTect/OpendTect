/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribsteering.h"

#include "math2.h"


namespace Attrib
{


BinID getSteeringPosition( int targetidx )
{
    if ( !targetidx ) return BinID::noStepout();

    int radius = 1;
    int idx = 0;

    while ( true )
    {
	int inl, crl;

	inl = -radius;

	for ( crl=-radius; crl<radius; crl++ )
	{
	    idx++;
	    if ( idx==targetidx ) return BinID( inl, crl );
	}

	for ( ; inl<radius; inl++ )
	{
	    idx++;
	    if ( idx==targetidx ) return BinID( inl, crl );
	}

	for ( ; crl>-radius; crl-- )
	{
	    idx++;
	    if ( idx==targetidx ) return BinID( inl, crl );
	}

	for ( ; inl>-radius; inl-- )
	{
	    idx++;
	    if ( idx==targetidx ) return BinID( inl, crl );
	}

	radius++;
    }

    return BinID::noStepout();
}

static int *map = NULL;
const  int MAXRADIUS = 200;

static int& mapAt( int inl, int crl )
{
    return map[(inl+MAXRADIUS)*(2*MAXRADIUS+1)+crl+MAXRADIUS];
}

void fillSteeringMap()
{
    mapAt(0,0) = 0;//    if ( !targetidx ) return BinID::noStepout();

    int radius = 1;
    int idx = 0;

    while ( radius < MAXRADIUS )
    {
	int inl, crl;
	inl = -radius;

	for ( crl=-radius; crl<radius; crl++ )
	{
	    idx++;
	    mapAt(inl,crl)=idx; //if ( idx==targetidx ) return BinID(inl,crl);
	}

	for ( ; inl<radius; inl++ )
	{
	    idx++;
	    mapAt(inl,crl)=idx; // if ( idx==targetidx ) return BinID(inl,crl);
	}

	for ( ; crl>-radius; crl-- )
	{
	    idx++;
	    mapAt(inl,crl)=idx; // if ( idx==targetidx ) return BinID(inl,crl);
	}

	for ( ; inl>-radius; inl-- )
	{
	    idx++;
	    mapAt(inl,crl)=idx; //if ( idx==targetidx ) return BinID(inl,crl);
	}

	radius++;
    }
}

int getSteeringIndex( const BinID& bid )
{
    if ( map == NULL )
    {
	map = new int[(2*MAXRADIUS+1)*(2*MAXRADIUS+1)];
	fillSteeringMap();
    }

    if ( Math::Abs(bid.inl()) > MAXRADIUS || Math::Abs(bid.crl()) > MAXRADIUS )
	return 0;

    return mapAt(bid.inl(), bid.crl());
}


} // namespace Attrib
