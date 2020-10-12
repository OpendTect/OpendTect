/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2020
________________________________________________________________________

-*/


#include "dbkey.h"

#include "surveydisklocation.h"


DBKey::~DBKey()
{}


void DBKey::setSurveyDiskLocation( const SurveyDiskLocation& sdl )
{
    delete survloc_;
    if ( sdl.isCurrentSurvey() )
        survloc_ = nullptr;
    else
        survloc_ = new SurveyDiskLocation( sdl );
}


const SurveyDiskLocation& DBKey::surveyDiskLocation() const
{
    static const SurveyDiskLocation emptysdl_( 0, 0 );
    return survloc_ ? *survloc_ : emptysdl_;
}


void DBKey::clearSurveyDiskLocation()
{
    delete survloc_;
    survloc_ = nullptr;
}


BufferString DBKey::toString( bool withsurvloc ) const
{
    BufferString ret = buf();
    if ( withsurvloc && survloc_ )
	ret.add( "`" ).add( surveyDiskLocation().fullPath() );

    return ret;
}
