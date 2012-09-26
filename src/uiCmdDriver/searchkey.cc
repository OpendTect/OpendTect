/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		May 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "searchkey.h"

#include "cmddriverbasics.h"
#include "uimainwin.h"


namespace CmdDrive
{


SearchKey::SearchKey( const char* expr, bool cs )
    : searchexpr_(expr)
    , casesensitive_(cs)
{}


// Command Driver coding conventions for using special ASCII characters
// in order to guarantee no match with any scripted (sub)string:

static const char* programmedwildcard = "\a";
static const char* scriptedwildcard   = "\b";

// "\f*" : to be used as intermediate(s) in substring replacements 


bool SearchKey::isMatching( const char* nameptr ) const
{
    BufferString key( searchexpr_ );
    BufferString name( nameptr );

    replaceString( key.buf(), "@@", "\f1" );
    replaceString( key.buf(), "@*", "\f2" );
    replaceString( key.buf(), "\f1", "@@" );
    replaceString( key.buf(), "*", scriptedwildcard );
    replaceString( key.buf(), "\f2", "*" );

    StringProcessor(key).removeCmdFileEscapes();
    StringProcessor(key).cleanUp();
    StringProcessor(name).cleanUp();

    wildcardlist_.erase();
    if ( isMatch(key, name, name) )
    {
	wildcardlist_.add( name );
	wildcardlist_.reverse();
	return true;
    }

    BufferString stripname = nameptr;
    if ( !mStripOuterBrackets(stripname, sMatch) )
	return false;

    return isMatching( stripname );
}


bool SearchKey::isMatch( const char* keyptr, const char* nameptr,
			 const char* orgnameptr) const
{
    if ( !keyptr || !nameptr )
	return keyptr == nameptr;

    if ( !*nameptr )
    { 
	int nrwildcards = 0;
	while ( *keyptr )
	{ 
	    if ( *keyptr == *scriptedwildcard )
		nrwildcards++;
	    else if ( !isspace(*keyptr) && *keyptr!='.' )
		return false;

	    keyptr++;
	}
	while ( nrwildcards-- )
	    wildcardlist_.add( "" );

	return true;;
    }

    if ( !*keyptr )
	return false;

    if ( *keyptr!=*scriptedwildcard && *keyptr!=*programmedwildcard )
    {
	bool firstmatch = *keyptr == *nameptr;

	if ( !casesensitive_ )
	    firstmatch = toupper(*keyptr) == toupper(*nameptr);

	if ( firstmatch )
	    return isMatch(keyptr+1, nameptr+1, orgnameptr );

	return false;
    }

    for ( int nameoffset=0; nameoffset<=strlen(nameptr); nameoffset++ )
    {
	const int keyoffset = isspace(*(keyptr+1)) &&
			      nameptr+nameoffset==orgnameptr ? 2 : 1;

	if ( isMatch(keyptr+keyoffset, nameptr+nameoffset, orgnameptr) )
	{
	    if ( *keyptr == *programmedwildcard )
		return true;

	    BufferString wildcard( nameptr );
	    *(wildcard.buf()+nameoffset) = '\0';
	    wildcardlist_.add( wildcard );
	    return true;
	}
    }
    return false;
}


void SearchKey::getMatchingWindows( const uiMainWin* applwin,
				    ObjectSet<uiMainWin>& windowlist,
				    WildcardManager* wcm ) const
{
    windowlist.allowNull();
    if ( !wcm )
    {
	uiMainWin::getTopLevelWindows( windowlist );
	windowlist += 0;
    }

    for ( int idx=windowlist.size()-1; idx>=0; idx-- )
    {
	if ( wcm && idx )
	    continue;
	    
	if ( !isCmdDriverWindow(windowlist[idx]) )
	{
	    for ( int aliasnr=0; true; aliasnr++ )
	    {
		const char* alias =
			    windowTitle( applwin, windowlist[idx], aliasnr );
		if ( !alias )
		{
		    windowlist.remove( idx );
		    break;
		}
		if ( isMatching(alias) )
		{
		    if ( wcm )
			wcm->check( *this, alias );
		    break;
		}
	    }
	}
	else
	    windowlist.remove( idx );
    }
}


WildcardManager::WildcardManager()
{
    reInit();
}



WildcardManager::~WildcardManager()
{}


void WildcardManager::reInit()
{
    wildcards_.erase();
    wildcardstrings_.erase();
    newwildcards_.erase();
    newwildcardstrings_.erase();
}


void WildcardManager::check( const SearchKey& searchkey, const char* name,
			     bool addescapes )
{
    if ( !searchkey.isMatching(name) )
	return;

    const BufferStringSet& wildcardlist = searchkey.wildcardList();

    for ( int idx=1; idx<wildcardlist.size(); idx++ )
    {
	BufferString wildcardbuf = wildcardlist.get( idx );
	StringProcessor strproc( wildcardbuf );
	if ( addescapes )
	    strproc.addCmdFileEscapes( StringProcessor::sAllEscSymbols() );

	newwildcards_.add( wildcardbuf );

	wildcardbuf = wildcardlist.get( 0 );
	if ( addescapes )
	    strproc.addCmdFileEscapes( StringProcessor::sAllEscSymbols() );

	newwildcardstrings_.add( wildcardbuf );
    }
}


void WildcardManager::flush( bool yn )
{
    if ( yn && !newwildcards_.isEmpty() )
    {
	wildcards_ = newwildcards_;
	wildcardstrings_ = newwildcardstrings_;
    }

    newwildcards_.erase();
    newwildcardstrings_.erase();
}


const BufferString* WildcardManager::wildcard( int idx ) const
{ return idx<0 || idx>=wildcards_.size() ? 0 : wildcards_[idx]; }


const BufferString* WildcardManager::wildcardStr( int idx ) const
{ return idx<0 || idx>=wildcardstrings_.size() ? 0 : wildcardstrings_[idx]; }


}; // namespace CmdDrive
