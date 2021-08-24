/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/


#include "helpview.h"
#include "keystrs.h"
#include "separstr.h"


mImplFactory( HelpProvider, HelpProvider::factory );

void HelpProvider::provideHelp( const HelpKey& key )
{
    PtrMan<HelpProvider> provider = factory().create( key.providername_ );
    if ( !provider )
	return;

    provider->provideHelp( key.argument_ );
}


bool HelpProvider::hasHelp( const HelpKey& key )
{
    PtrMan<HelpProvider> provider = factory().create( key.providername_ );
    if ( !provider )
	return false;

    return provider->hasHelp( key.argument_ );
}


uiString HelpProvider::description( const HelpKey& key )
{
    PtrMan<HelpProvider> provider = factory().create( key.providername_ );
    if ( !provider )
	return uiString::emptyString();

    return provider->description( key.argument_ );
}


uiString HelpProvider::description( const char* arg ) const
{
    return toUiString( "%1 - %2" ).arg(factory().currentName()).arg(arg);
}


// HelpKey
bool HelpKey::operator==( const HelpKey& oth ) const
{
    return providername_==oth.providername_ && argument_==oth.argument_;
}


bool HelpKey::isEmpty() const
{
    return !providername_ || !*providername_;
}


HelpKey HelpKey::emptyHelpKey()
{ return HelpKey( nullptr , sKey::EmptyString() ); }


HelpKey HelpKey::makeFromString( const char* str )
{
    HelpKey ret = emptyHelpKey();
    const FixedString inpstr( str );
    if ( inpstr.isEmpty() || !inpstr.contains(':') )
	return ret;

    SeparStringSB sepstr( str, ':' );
    if ( sepstr.size() != 2 )
	return ret;

    ret.providername_ = sepstr[0];
    ret.argument_ = sepstr[1];

    return ret;
}
