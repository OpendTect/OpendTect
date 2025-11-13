/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellselection.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"


namespace Well
{

SelInfo::SelInfo( const Well::Data& wd )
    : wd_(&wd)
{
}


SelInfo::~SelInfo()
{
}


MultiID SelInfo::wellID() const
{
    return wd_->multiID();
}


const char* SelInfo::wellName() const
{
    return wd_->name();
}



// MultiSelSpec
MultiSelSpec::MultiSelSpec()
{}


MultiSelSpec::~MultiSelSpec()
{}


void MultiSelSpec::clear()
{
    wellkeys_.erase();
    lognms_.erase();
    mnemonicnms_.erase();
    markernms_.erase();
}



// SubSelData
SubSelData::SubSelData( const SelInfo& info )
{
    const LoadReqs lreqs( Inf, Mrkrs, LogInfos );
    wd_ = MGR().get( info.wellID(), lreqs );
    init( info );
}


SubSelData::~SubSelData()
{
}


void SubSelData::init( const SelInfo& info )
{
    mdrg_ = info.getMDRange();
    lognms_ = info.selectedLogs();
    markernms_ = info.selectedMarkers();
}


bool SubSelData::isOK() const
{
    return wd_.ptr();
}


MultiID SubSelData::wellID() const
{
    return wd_->multiID();
}


const char* SubSelData::wellName() const
{
    return wd_->name();
}

} // namespace Well
