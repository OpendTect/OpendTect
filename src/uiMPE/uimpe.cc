/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/

#include "uimpe.h"

namespace MPE
{

uiSetupGroup::uiSetupGroup( uiParent* p, const char* ref )
    : uiGroup( p, "Tracking Setup" )
    , helpref_(ref)
{}


bool uiSetupGroup::commitToTracker() const
{
    bool dummybool;
    return commitToTracker( dummybool );
}


void uiSetupGroupFactory::addFactory( uiSetupGrpCreationFunc f,
       				      const char* name )
{
   names_.add( name );
   funcs += f;
}


uiSetupGroup* uiSetupGroupFactory::create( const char* name, uiParent* p,
					   const char* typestr )
{
    int idx = names_.indexOf( name );
    if ( idx == -1 ) return 0;

    uiSetupGroup* res = funcs[idx](p,typestr);
    if ( res ) return res;

    return 0;
}


uiMPEEngine& uiMPE()
{
    mDefineStaticLocalObject( uiMPEEngine, uiengine,  );
    return uiengine;
}

} // namespace MPE
