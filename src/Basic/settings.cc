/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/
 
static const char* rcsID = "$Id: settings.cc,v 1.2 2000-04-17 14:56:41 bert Exp $";

#include "settings.h"
#include "filegen.h"
#include "ascstream.h"
#include <fstream.h>


Settings* Settings::common_ = 0;


class SettItem : public UserIDObject
{
public:
		SettItem( const char* nm, const char* s, int maj )
		: UserIDObject(nm) , str(0), ismajor(maj) { set(s); }
		~SettItem()				  { delete [] str; }

    void	set( const char* s )
		{
		    delete [] str;
		    str = new char[ s ? strlen(s)+1 : 1 ];
		    strcpy( str, s ? s : "" );
		}
    char* str;
    int   ismajor;
};

class SettItemList : public UserIDObjectSet<SettItem>
{ public: SettItemList(const char* nm=0) : UserIDObjectSet<SettItem>(nm) {} };

#define list() ((SettItemList&)(*list_))

static const char* sKey = "Default settings";


Settings& Settings::common()
{
    if ( !common_ )
    {
	FileNameString setupf( getenv( "HOME" ) );
	setupf = File_getFullPath( setupf, ".dgbSettings" );
	const char* ptr = getenv( "dGB_USER" );
	if ( ptr )
	{
	    setupf += ".";
	    setupf += ptr;
	}
	common_ = new Settings( setupf );
    }
    return *common_;
}


Settings::Settings( const char* strmopen )
	: list_(0)
{
    list_ = (PtrUserIDObjectSet)new SettItemList(sKey);

    fname = strmopen;
    istream* strm = new ifstream( fname );
    if ( !strm || !strm->good() )
	strm = new ifstream( GetDataFileName( "dgbSettings" ) );
    if ( !strm || !strm->good() )
    {
	cerr << "Cannot find system's dgbSettings file" << endl;
	delete strm;
	return;
    }

    ascistream stream( *strm );
    if ( !stream.isOfFileType( sKey ) )
	{ delete strm; return; }

    while ( !atEndOfSection( stream.next() ) )
    {
	const char* ptr = stream.keyWord();
	int ismajor = NO;
	if ( *ptr == '#' ) continue;
	else if ( *ptr == '*' ) { ismajor = YES; ptr++; }
	list() += new SettItem( ptr, stream.value(), ismajor );
    }

    delete strm;
}


const char* Settings::get( const char* key ) const
{
    if ( !list_ ) return 0;
    SettItem* val = list()[key];
    return val ? val->str : 0;
}


int Settings::is( const char* key, const char* s ) const
{
    const char* val = get( key );
    return val && !strcmp(val,s) ? YES : NO;
}


void Settings::set( const char* key, const char* s )
{
    if ( !list_ ) return;
    SettItem* val = list()[key];
    if ( !val )
	list() += new SettItem( key, s, NO );
    else
	val->set( s );
}


int Settings::isMajor( const char* key ) const
{
    SettItem* val = list()[key];
    if ( val && val->ismajor ) return YES;
    return NO;
}


Settings::~Settings()
{
    if ( list_ )
    {
	list().deepErase();
	delete list_;
    }
}


int Settings::write() const
{
    if ( !list_ ) return NO;
    ostream* strm = new ofstream( fname );
    if ( !strm ) return NO;

    ascostream stream( *strm );
    stream.putHeader( sKey );
    for ( int idx=0; idx<list().size(); idx++ )
    {
	SettItem* si = list()[idx];
	if ( !isMajor( si->name() ) )
	    stream.put( si->name(), si->str );
	else
	{
	    UserIDString key( "*" );
	    key += si->name();
	    stream.put( key, si->str );
	}
    }

    delete strm;
    return YES;
}
