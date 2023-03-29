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
    : logs_(*new LogSet)
    , markers_(*new MarkerSet)
{
    wd_ = MGR().get( info.wellID(), LoadReqs(Inf,LogInfos,Mrkrs) );
    init( info );
}


SubSelData::~SubSelData()
{
    logs_.setEmpty( false );
    delete &logs_;
    delete &markers_;
}


void SubSelData::init( const SelInfo& info )
{
    mdrg_ = info.getMDRange();

    for ( auto* lognm : info.selectedLogs() )
    {
	const Well::Log* log = wd_->getLog( lognm->buf() );
	if ( log )
	    logs_.add( cCast(Log*,log) );
    }

    for ( auto* lognm : info.selectedMarkers() )
    {
	const Well::Marker* marker = wd_->markers().getByName( lognm->buf() );
	if ( marker )
	    markers_.addSameWell( *marker );
    }
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
