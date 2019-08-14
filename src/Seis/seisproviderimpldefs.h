#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2019
________________________________________________________________________

*/


#define mDefProvStdFns( basetyp, typ ) \
Seis::typ##Provider::typ##Provider() \
    : fetcher_(*new typ##Fetcher(*this)) \
{ \
} \
Seis::typ##Provider::typ##Provider( const DBKey& dbky, uiRetVal& uirv ) \
    : Provider##basetyp(dbky,uirv) \
    , fetcher_(*new typ##Fetcher(*this)) \
{ \
} \
Seis::typ##Provider::~typ##Provider() \
{ \
    delete &fetcher_; \
} \
Seis::Fetcher& Seis::typ##Provider::gtFetcher() \
{ \
    return fetcher_; \
}

#define mDefNonPSProvReqFns( typ ) \
void Seis::typ##Provider::gtTrc( TraceData& td, SeisTrcInfo& ti, \
				uiRetVal& uirv ) const \
{ \
    if ( !fetcher_.setPosition(trcpos_) ) \
	uirv.set( uiStrings::phrUnexpected(uiStrings::sPosition(), \
					    trcpos_.usrDispStr()) ); \
    else \
    { \
	fetcher_.getTrc( td, ti ); \
	uirv = fetcher_.uirv_; \
    } \
}

#define mDefPSProvReqFns( typ ) \
Seis::typ##Provider::size_type Seis::typ##Provider::gtNrOffsets() const \
{ \
    return fetcher_.nrOffsets(); \
} \
void Seis::typ##Provider::gtGather( SeisTrcBuf& tbuf, uiRetVal& uirv ) const \
{ \
    if ( !fetcher_.setPosition(trcpos_) ) \
	uirv.set( uiStrings::phrUnexpected(uiStrings::sPosition(), \
					   trcpos_.usrDispStr()) ); \
    else \
    { \
	fetcher_.getGather( tbuf ); \
	uirv = fetcher_.uirv_; \
    } \
}

#define mDefNonPSProvFns( basetyp, typ ) \
    mDefProvStdFns( basetyp, typ ) \
    mDefNonPSProvReqFns( typ )

#define mDefPSProvFns( basetyp, typ ) \
    mDefProvStdFns( basetyp, typ ) \
    mDefPSProvReqFns( typ )
