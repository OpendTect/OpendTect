/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/


#include "helpview.h"


mImplClassFactory( HelpProvider, factory );

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


HelpKey HelpKey::emptyHelpKey()
{ return HelpKey( 0 , OD::EmptyString() ); }
