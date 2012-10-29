/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID mUsedVar = "$Id$";

#include "attriblinebuffer.h"

#include "attribdataholder.h"
#include "position.h"

namespace Attrib
{

DataHolderLineBuffer::DataHolderLineBuffer() {}


DataHolderLineBuffer::~DataHolderLineBuffer()
{
    for ( int idx=inlinedata_.size()-1; idx>=0; idx-- )
	deepErase( *inlinedata_[idx] );

    deepErase( inlinedata_ );
    deepErase( crossliness_ );
}


DataHolder* DataHolderLineBuffer::createDataHolder( const BinID& bid,
						    int z0, int nrsamples )
{
    int lineidx = inlines_.indexOf(bid.inl);
    if ( lineidx==-1 )
    {
	inlinedata_ += new ObjectSet<DataHolder>;
	crossliness_ += new TypeSet<int>;
	lineidx = inlinedata_.size()-1;
	inlines_ += bid.inl;
    }

    const int traceidx = crossliness_[lineidx]->indexOf(bid.crl);
    if ( traceidx==-1 )
    {
	DataHolder* res = new DataHolder( z0, nrsamples );
	(*inlinedata_[lineidx]) += res;
	(*crossliness_[lineidx]) += bid.crl;
	return res;
    }

    DataHolder* trcdata = (*inlinedata_[lineidx])[traceidx];
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
    const int lineidx = inlines_.indexOf(bid.inl);
    if ( lineidx==-1 ) return 0;

    const int traceidx = crossliness_[lineidx]->indexOf(bid.crl);
    if ( traceidx==-1 ) return 0;
    return const_cast<DataHolder*>( (*inlinedata_[lineidx])[traceidx] );
}


void DataHolderLineBuffer::removeDataHolder( const BinID& bid )
{
    const int lineidx = inlines_.indexOf(bid.inl);
    if ( lineidx==-1 ) return;

    const int traceidx = crossliness_[lineidx]->indexOf(bid.crl);
    if ( traceidx==-1 ) return;

    delete (*inlinedata_[lineidx])[traceidx];
    inlinedata_[lineidx]->remove( traceidx );
    crossliness_[lineidx]->remove(traceidx);

    if ( !inlinedata_[lineidx]->size() )
	removeInline( lineidx );
}


#define removeWithOp( op )\
{\
    for ( int idx=0; idx<inlines_.size(); idx++ )\
    {\
	bool removeline = false;\
	if ( direction.inl*inlines_[idx] op direction.inl*bid.inl )\
	    removeline = true;\
	else if ( inlines_[idx]==bid.inl )\
	{\
	    TypeSet<int>& crosslines = *crossliness_[idx];\
	    for ( int idy=crosslines.size()-1; idy>=0; idy-- )\
	    {\
		if ( direction.crl*crosslines[idy] op direction.crl*bid.crl )\
		{ \
		    delete inlinedata_[idx]->removeSingle(idy);\
		    crosslines.removeSingle(idy);\
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
    deepErase( *inlinedata_[lineidx] );
    delete inlinedata_.removeSingle( lineidx );
    delete crossliness_.removeSingle( lineidx );

    inlines_.removeSingle(lineidx);
}


}; //namespace
