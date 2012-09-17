// No multiple inclusion protection!

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2001
 RCS:           $Id: i_datainterp.h,v 1.2 2009/07/22 16:01:32 cvsbert Exp $
________________________________________________________________________

-*/

#ifndef mTheType
#error To include __FILE__, set mTheType
#endif

#define mDICB(fntyp,fn) \
((DataInterpreter<mTheType>::fntyp)(&DataInterpreter<mTheType>::fn))

#define mDefGetPut(fnnm) \
{ \
    getfn = needswap ? mDICB(GetFn,get##fnnm##swp) : mDICB(GetFn,get##fnnm); \
    putfn = needswap ? mDICB(PutFn,put##fnnm##swp) : mDICB(PutFn,put##fnnm); \
}

#define mDefGetPutNoSwap(fnnm) \
{ getfn = mDICB(GetFn,get##fnnm); putfn = mDICB(PutFn,put##fnnm); }


template <>
void DataInterpreter<mTheType>::set( const DataCharacteristics& dc,
				     bool ignend )
{
    swpfn = dc.nrBytes() == BinDataDesc::N1 ? mDICB(SwapFn,swap0)
	: ( dc.nrBytes() == BinDataDesc::N8 ? mDICB(SwapFn,swap8)
	: ( dc.nrBytes() == BinDataDesc::N4 ? mDICB(SwapFn,swap4)
					    : mDICB(SwapFn,swap2) ) );

    getfn = mDICB(GetFn,get0);
    putfn = mDICB(PutFn,put0);
    bool needswap = !ignend && dc.needSwap();

    if ( !dc.isInteger() )
    {
	if ( !dc.isIeee() )
	{
	    if ( dc.nrBytes() == BinDataDesc::N4 )
		mDefGetPut(FIbm)
	}
	else
	{
	    if ( dc.nrBytes() == BinDataDesc::N4 )
		mDefGetPut(F)
	    else if ( dc.nrBytes() == BinDataDesc::N8 )
		mDefGetPut(D)
	}
    }
    else
    {
	if ( dc.isSigned() )
	{
	    if ( !dc.isIeee() )
	    {
		switch ( dc.nrBytes() )
		{
		case BinDataDesc::N1: mDefGetPutNoSwap(S1)	break;
		case BinDataDesc::N2: mDefGetPut(S2Ibm)		break;
		case BinDataDesc::N4: mDefGetPut(S4Ibm)		break;
		}
	    }
	    else
	    {
		switch ( dc.nrBytes() )
		{
		case BinDataDesc::N1: mDefGetPutNoSwap(S1)	break;
		case BinDataDesc::N2: mDefGetPut(S2)		break;
		case BinDataDesc::N4: mDefGetPut(S4)		break;
		case BinDataDesc::N8: mDefGetPut(S8)		break;
		}
	    }
	}
	else if ( dc.isIeee() )
	{
	    switch ( dc.nrBytes() )
	    {
	    case BinDataDesc::N1: mDefGetPutNoSwap(U1)		break;
	    case BinDataDesc::N2: mDefGetPut(U2)		break;
	    case BinDataDesc::N4: mDefGetPut(U4)		break;
	    }
	}
    }
}


template <>
DataInterpreter<mTheType>::DataInterpreter(
		const DataCharacteristics& dc, bool ignend )
{
    set( dc, ignend );
}


template <>
DataInterpreter<mTheType>::DataInterpreter(
				const DataInterpreter& di )
	: swpfn(di.swpfn)
	, getfn(di.getfn)
	, putfn(di.putfn)
{
}


template <>
DataInterpreter<mTheType>& DataInterpreter<mTheType>::operator=(
			    const DataInterpreter<mTheType>& di )
{
    if ( &di != this )
    {
	swpfn = di.swpfn;
	getfn = di.getfn;
	putfn = di.putfn;
    }
    return *this;
}


template <>
int DataInterpreter<mTheType>::nrBytes() const
{
    return swpfn == &DataInterpreter<mTheType>::swap2 ?	2
	: (swpfn == &DataInterpreter<mTheType>::swap4 ?	4
	: (swpfn == &DataInterpreter<mTheType>::swap8 ?	8
	:						1  ));
}


#define mSet(nb,isint,frmt,iss,swpd) \
	dc = DataCharacteristics( isint, iss, (BinDataDesc::ByteCount)nb, \
		DataCharacteristics::frmt, __islittle__ != swpd );

template <>
DataCharacteristics DataInterpreter<mTheType>::dataChar() const
{
    DataCharacteristics dc;
    switch ( nrBytes() )
    {

    case 2: {
	if ( getfn == &DataInterpreter<mTheType>::getS2 )
	    mSet(2,true,Ieee,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getU2 )
	    mSet(2,true,Ieee,false,false)
	else if ( getfn == &DataInterpreter<mTheType>::getS2Ibm )
	    mSet(2,true,Ibm,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getS2swp )
	    mSet(2,true,Ieee,true,true)
	else if ( getfn == &DataInterpreter<mTheType>::getU2swp )
	    mSet(2,true,Ieee,false,true)
	else if ( getfn == &DataInterpreter<mTheType>::getS2Ibmswp )
	    mSet(2,true,Ibm,true,true)
    }

    case 4:
    {
	if ( getfn == &DataInterpreter<mTheType>::getS4 )
	    mSet(4,true,Ieee,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getU4 )
	    mSet(4,true,Ieee,false,false)
	else if ( getfn == &DataInterpreter<mTheType>::getF )
	    mSet(4,false,Ieee,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getS4Ibm )
	    mSet(4,true,Ibm,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getFIbm )
	    mSet(4,false,Ibm,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getS4swp )
	    mSet(4,true,Ieee,true,true)
	else if ( getfn == &DataInterpreter<mTheType>::getU4swp )
	    mSet(4,true,Ieee,false,true)
	else if ( getfn == &DataInterpreter<mTheType>::getFswp )
	    mSet(4,false,Ieee,true,true)
	else if ( getfn == &DataInterpreter<mTheType>::getS4Ibmswp )
	    mSet(4,true,Ibm,true,true)
	else if ( getfn == &DataInterpreter<mTheType>::getFIbmswp )
	    mSet(4,false,Ibm,true,true)
    }

    case 8:
    {
	if ( getfn == &DataInterpreter<mTheType>::getD )
	    mSet(8,false,Ieee,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getDswp )
	    mSet(8,false,Ieee,true,true)
	else if ( getfn == &DataInterpreter<mTheType>::getS8 )
	    mSet(8,true,Ieee,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getS8swp )
	    mSet(8,true,Ieee,true,true)
    }

    default:
    {
	if ( getfn == &DataInterpreter<mTheType>::getS1 )
	    mSet(1,true,Ieee,true,false)
	else if ( getfn == &DataInterpreter<mTheType>::getU1 )
	    mSet(1,true,Ieee,false,false)
    }

    }

    return dc;
}

#undef mSet


template <>
bool DataInterpreter<mTheType>::needSwap() const
{
    switch ( nrBytes() )
    {
    case 2: return getfn == &DataInterpreter<mTheType>::getS2swp
		|| getfn == &DataInterpreter<mTheType>::getU2swp
		|| getfn == &DataInterpreter<mTheType>::getS2Ibmswp;

    case 4: return getfn == &DataInterpreter<mTheType>::getS4swp
		|| getfn == &DataInterpreter<mTheType>::getU4swp
		|| getfn == &DataInterpreter<mTheType>::getFswp
		|| getfn == &DataInterpreter<mTheType>::getS4Ibmswp
		|| getfn == &DataInterpreter<mTheType>::getFIbmswp;

    case 8: return getfn == &DataInterpreter<mTheType>::getDswp
	    	|| getfn == &DataInterpreter<mTheType>::getS8swp;

    }
    return false;
}


#define mDoChgSwp(typ) \
    if ( getfn == &DataInterpreter<mTheType>::get##typ ) \
    { \
	getfn = &DataInterpreter<mTheType>::get##typ##swp; \
	putfn = &DataInterpreter<mTheType>::put##typ##swp; \
    } \
    else if ( getfn == &DataInterpreter<mTheType>::get##typ##swp ) \
    { \
	getfn = &DataInterpreter<mTheType>::get##typ; \
	putfn = &DataInterpreter<mTheType>::put##typ; \
    }


template <>
void DataInterpreter<mTheType>::swpSwap()
{
    switch ( nrBytes() )
    {
    case 2: {
	mDoChgSwp(S2)
	else mDoChgSwp(U2)
	else mDoChgSwp(S2Ibm)
    }

    case 4:
    {
	mDoChgSwp(S4)
	else mDoChgSwp(U4)
	else mDoChgSwp(F)
	else mDoChgSwp(S4Ibm)
	else mDoChgSwp(FIbm)
    }

    case 8:
    {
	mDoChgSwp(D)
	else mDoChgSwp(S8)
    }

    }
}
