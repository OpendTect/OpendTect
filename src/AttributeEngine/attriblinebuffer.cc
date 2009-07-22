/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriblinebuffer.cc,v 1.11 2009-07-22 16:01:29 cvsbert Exp $";

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
						    int z0, int nrsamples )
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
	DataHolder* res = new DataHolder( z0, nrsamples );
	(*inlinedata[lineidx]) += res;
	(*crossliness[lineidx]) += bid.crl;
	return res;
    }

    DataHolder* trcdata = (*inlinedata[lineidx])[traceidx];
    if ( trcdata->nrsamples_ != nrsamples )
    {
	removeDataHolder( bid );
	return createDataHolder( bid, z0, nrsamples );
    }

    trcdata->z0_ = z0;
    return trcdata;
}


DataHolder* DataHolderLineBuffer::gtDataHolder( const BinID& bid ) const
{
    const int lineidx = inlines.indexOf(bid.inl);
    if ( lineidx==-1 ) return 0;

    const int traceidx = crossliness[lineidx]->indexOf(bid.crl);
    if ( traceidx==-1 ) return 0;
    return const_cast<DataHolder*>( (*inlinedata[lineidx])[traceidx] );
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


#define removeWithOp( op )\
{\
    for ( int idx=0; idx<inlines.size(); idx++ )\
    {\
	bool removeline = false;\
	if ( direction.inl*inlines[idx] op direction.inl*bid.inl )\
	    removeline = true;\
	else if ( inlines[idx]==bid.inl )\
	{\
	    TypeSet<int>& crosslines = *crossliness[idx];\
	    for ( int idy=crosslines.size()-1; idy>=0; idy-- )\
	    {\
		if ( direction.crl*crosslines[idy] op direction.crl*bid.crl )\
		{\
		    delete (*inlinedata[idx])[idy];\
		    inlinedata[idx]->remove(idy);\
		    crosslines.remove(idy);\
		}\
	    }\
\
	    if ( crosslines.isEmpty() )\
		removeline = true;\
	}\
\
	if ( !removeline )\
	    continue;\
\
	removeInline( idx-- );\
    }\
}


void DataHolderLineBuffer::removeBefore( const BinID& bid, 
					const BinID& direction )
{
    removeWithOp( < );
}


void DataHolderLineBuffer::removeAllExcept( const BinID& bid )
{
    const BinID direction(1,1);
    removeWithOp( != );
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
