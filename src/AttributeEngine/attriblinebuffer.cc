/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriblinebuffer.cc,v 1.1 2005-01-28 16:30:53 kristofer Exp $";

#include "attriblinebuffer.h"

#include "attribdataholder.h"
#include "position.h"

namespace Attrib
{

DataHolderLineBuffer::DataHolderLineBuffer() {}


DataHolderLineBuffer::~DataHolderLineBuffer()
{
    for ( int idx=inlinedata.size()-1; idx>=0; idx-- )
	deepErase( *inlinedata[idx] );

    deepErase( inlinedata );
    deepErase( crossliness );
}


DataHolder* DataHolderLineBuffer::createDataHolder( const BinID& bid,
						    int t1, int nrsamples )
{
    int lineidx = inlines.indexOf(bid.inl);
    if ( lineidx==-1 )
    {
	inlinedata += new ObjectSet<DataHolder>;
	crossliness += new TypeSet<int>;
	lineidx = inlinedata.size()-1;
	inlines += bid.inl;
    }

    const int traceidx = crossliness[lineidx]->indexOf(bid.crl);
    if ( traceidx==-1 )
    {
	DataHolder* res = new DataHolder( t1, nrsamples );
	(*inlinedata[lineidx]) += res;
	(*crossliness[lineidx]) += bid.crl;
	return res;
    }

    if ( (*inlinedata[lineidx])[traceidx]->nrsamples!=nrsamples )
    {
	deepErase( *(*inlinedata[lineidx])[traceidx] );
	(*inlinedata[lineidx])[traceidx]->nrsamples = nrsamples;
    }

    (*inlinedata[lineidx])[traceidx]->t1 = t1;
    return (*inlinedata[lineidx])[traceidx];
}


DataHolder* DataHolderLineBuffer::getDataHolder( const BinID& bid ) const
{
    const int lineidx = inlines.indexOf(bid.inl);
    if ( lineidx==-1 ) return 0;

    const int traceidx = crossliness[lineidx]->indexOf(bid.crl);
    if ( traceidx==-1 ) return 0;
    return (*inlinedata[lineidx])[traceidx];
}


void DataHolderLineBuffer::removeDataHolder( const BinID& bid )
{
    const int lineidx = inlines.indexOf(bid.inl);
    if ( lineidx==-1 ) return;

    const int traceidx = crossliness[lineidx]->indexOf(bid.crl);
    if ( traceidx==-1 ) return;

    delete (*inlinedata[lineidx])[traceidx];
    inlinedata[lineidx]->remove( traceidx );
    crossliness[lineidx]->remove(traceidx);

    if ( !inlinedata[lineidx]->size() )
	removeInline( lineidx );
}


void DataHolderLineBuffer::removeBefore( const BinID& bid )
{
    for ( int idx=0; idx<inlines.size(); idx++ )
    {
	bool removeline = false;
	if ( inlines[idx]<bid.inl )
	    removeline = true;
	else if ( inlines[idx]==bid.inl )
	{
	    TypeSet<int>& crosslines = *crossliness[idx];
	    for ( int idy=crosslines.size()-1; idy>=0; idy-- )
	    {
		if ( crosslines[idy]<bid.crl )
		{
		    delete (*inlinedata[idx])[idy];
		    inlinedata[idx]->remove(idy);
		    crosslines.remove(idy);
		}
	    }

	    if ( !crosslines.size() )
		removeline = true;
	}

	if ( !removeline )
	    continue;

	removeInline( idx-- );
    }
}


void DataHolderLineBuffer::removeInline( int lineidx )
{
    deepErase( *inlinedata[lineidx] );
    delete inlinedata[lineidx];
    inlinedata.remove( lineidx );

    delete crossliness[lineidx];
    crossliness.remove( lineidx );

    inlines.remove(lineidx);
}


}; //namespace
