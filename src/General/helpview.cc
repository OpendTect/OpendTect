/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "helpview.h"

mImplFactory( HelpProvider, HelpProvider::factory );

void HelpProvider::provideHelp( const HelpKey& helpkey )
{
    PtrMan<HelpProvider> provider = factory().create( helpkey.providername_ );
    if ( !provider )
	return;

    provider->provideHelp( helpkey.argument_ );
}


bool HelpProvider::hasHelp( const HelpKey& helpkey )
{
    PtrMan<HelpProvider> provider = factory().create( helpkey.providername_ );
    if ( !provider )
	return false;

    return provider->hasHelp( helpkey.argument_ );
}


bool HelpKey::isEmpty() const
{
    return !providername_ || !*providername_;
}
