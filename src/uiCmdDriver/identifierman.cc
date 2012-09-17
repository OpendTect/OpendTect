/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		March 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: identifierman.cc,v 1.1 2012-09-17 12:37:41 cvsjaap Exp $";

#include "identifierman.h"

#include "cmddriverbasics.h"
#include "searchkey.h"

#include "oddirs.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "typeset.h"

namespace CmdDrive
{

static const char* getSpecDir( const char* envvar, const char* defdir )
{
    static BufferString retdir = GetEnvVar( envvar );
    if ( retdir.isEmpty() || !File::isDirectory(retdir) )
    {
	retdir = FilePath( GetDataDir(), defdir ).fullPath();
	File::createDir( retdir );
    }
    return retdir.buf();
}

#define getSnapshotsDir() getSpecDir( "DTECT_SNAPSHOTS_DIR", "Snapshots" )
#define getExportDir() getSpecDir( "DTECT_EXPORT_DIR", "Export" )
#define getImportDir() getSpecDir( "DTECT_IMPORT_DIR", "Import" )


IdentifierManager::IdentifierManager()
    : lastlinkedidentstr_("", ' ')
{
    reInit();
}


IdentifierManager::~IdentifierManager()
{
    while ( !identifiers_.isEmpty() )
	raiseScopeLevel( false );
}


void IdentifierManager::raiseScopeLevel( bool up )
{
    if ( up )
    {
	identifiers_ += new ObjectSet<Identifier>;
    }
    else if ( !identifiers_.isEmpty() )
    {
	deepErase( *identifiers_[identifiers_.size()-1] );
	delete identifiers_.remove( identifiers_.size()-1 );
	curident_ = 0;
	curlevel_ = -1;
    }
}


#define mPreset( name, val ) \
{ set(name,val); curident_->predefined_ = true; }

void IdentifierManager::reInit()
{
    while ( !identifiers_.isEmpty() )
	raiseScopeLevel( false );

    raiseScopeLevel( true );

    mPreset( "FALSE", false );
    mPreset( "TRUE", true );
    mPreset( "PI", M_PI );
    mPreset( "UNDEF", mUdf(double) );

    mPreset( "FAILURE", 0 );
    mPreset( "SUCCESS", 1 );
    mPreset( "WARNING", -1 );

    mPreset( "OFF", 0 );
    mPreset( "ON", 1 );
    mPreset( "UNSWITCHABLE", -1 );

    setFilePathPlaceholder( "FILEIDX", "1000" );

    lastlinkedidentstr_.setEmpty();

    curident_ = 0;
    curlevel_ = -1;
}


bool IdentifierManager::findCurIdent( const char* name, bool followlinks,
				      bool singlescope )
{
    lastlinkedidentstr_ = name;

    curlevel_ = identifiers_.size() - 1;

    while ( true ) 
    {
	mUnscope( name, unscopedname );
	if ( name != unscopedname )
	    curlevel_ = 0;

	curident_ = 0;
	for ( int idx=0; idx<identifiers_[curlevel_]->size(); idx++ )
	{ 
	    curident_ = (*identifiers_[curlevel_])[idx];

	    if ( SearchKey(unscopedname,false).isMatching(curident_->name_) )
	    {
		identifiers_[curlevel_]->insertAt( curident_, 0 );
		identifiers_[curlevel_]->remove( idx+1 );

		if ( !followlinks || !curident_->islink_ )
		    return true; 

		name = curident_->val_.buf();

		lastlinkedidentstr_ += "->";
		lastlinkedidentstr_ += name;
		break;
	    }

	    curident_ = 0;
	}

	if ( curlevel_==0 || (singlescope && !curident_) )
	    break;

	const bool isprocdef = FileMultiString(name).size() > 1;
	curlevel_ = curident_ || isprocdef ? curlevel_-1 : 0;
    }

    return false;
}


void IdentifierManager::set( const char* name, const char* val, bool islink )
{
    if ( findCurIdent(name, !islink, true) )
    {
	curident_->val_ = val;
	curident_->predefined_ = false;
	curident_->refresh_ = false;
	curident_->islink_ = islink;
    }
    else
    {
	name = lastlinkedidentstr_[lastlinkedidentstr_.size()-1];
	mUnscope( name, unscopedname );
	curident_ = new Identifier( unscopedname, val, islink );
	identifiers_[curlevel_]->insertAt( curident_, 0 );
    }
}


void IdentifierManager::set( const char* name, int val )
{ set( name, toString(val) ); }


void IdentifierManager::set( const char* name, double val )
{ set( name, toString(val) ); }


void IdentifierManager::unset( const char* name, bool followlinks )
{
    if ( findCurIdent(name, followlinks) )
    {
	delete identifiers_[curlevel_]->remove( 0 );
	curident_ = 0;
    }
}


#define mRefreshPlaceholder( idm, name, scopedkey, functioncall ) \
\
    if ( mMatchCI(name, scopedkey) || mMatchCI(name, scopedkey+1) ) \
    { \
	idm->setFilePathPlaceholder( scopedkey, functioncall ); \
	return true; \
    }

bool IdentifierManager::doesExist( const char* name ) const
{
    IdentifierManager* idm = const_cast<IdentifierManager*>( this );
    const bool isdef = idm->findCurIdent( name );

    if ( isdef && !curident_->refresh_ )
	return true;

    mRefreshPlaceholder( idm, name, "@BASEDIR", GetBaseDataDir() );
    mRefreshPlaceholder( idm, name, "@DATADIR", GetDataDir() );
    mRefreshPlaceholder( idm, name, "@PROCDIR", GetProcFileName(0) );
    mRefreshPlaceholder( idm, name, "@APPLDIR", GetSoftwareDir(0) );
    mRefreshPlaceholder( idm, name, "@USERDIR", GetPersonalDir() );
    mRefreshPlaceholder( idm, name, "@SCRIPTSDIR", GetScriptsDir(0) );
    mRefreshPlaceholder( idm, name, "@SNAPSHOTSDIR", getSnapshotsDir() );
    mRefreshPlaceholder( idm, name, "@IMPORTDIR", getImportDir() );
    mRefreshPlaceholder( idm, name, "@EXPORTDIR", getExportDir() );
    
    return isdef;
}


const char* IdentifierManager::getValue( const char* name ) const
{ return doesExist(name) ? curident_->val_.buf() : 0; }


bool IdentifierManager::getInteger( const char* name, int& val ) const
{ return StringProcessor(getValue(name)).convertToInt( &val ); }


bool IdentifierManager::getDouble( const char* name, double& val ) const
{ return StringProcessor(getValue(name)).convertToDouble( &val ); }


int IdentifierManager::substitute( const char* src, BufferString& dest )
{
    const char* ptr = src;
    dest.setEmpty();
    int nrfailed = 0;
    int nrsuccessful = 0;

    TypeSet<int> filepathpositions;

    while ( *ptr )
    {
	if ( *ptr=='$' && !StringProcessor(src).isEscapedSymbol(ptr) )
	{
	    BufferString name;
	    if ( StringProcessor(ptr).parseBracketed(name, '$', '$') )
	    {
		if ( mMatchCI(name, "FILEIDX") )
		{
		    int fileidx;
		    if ( getInteger(name, fileidx) && curlevel_==0 )
			set( name, fileidx+1 );
		}

		const char* val = getValue( name );
		if ( val )
		{
		    if ( curident_->filepathplaceholder_ )
			filepathpositions += dest.size();

		    dest += val;
		    nrsuccessful++;
		}
		else
		{
		    dest += "$"; dest += name; dest += "$";
		    nrfailed++;
		}
		ptr += name.size() + 2;
	    }
	    else
	    {
		mAddCharToBufStr( dest, *ptr++ );
		nrfailed++;
	    }
	}
	else
	    mAddCharToBufStr( dest, *ptr++ );
    }

    for ( int idx=0; idx<filepathpositions.size(); idx++ )
	StringProcessor(dest).makeDirSepIndep( filepathpositions[idx] );

    return nrfailed ? -nrfailed : nrsuccessful;
}


void IdentifierManager::setFilePathPlaceholder( const char* name,
						const char* val )
{
    mPreset( name, val );
    curident_->filepathplaceholder_ = true;
    curident_->refresh_ = true;
}


bool IdentifierManager::isPredefined( const char* name ) const
{ return doesExist(name) ? curident_->predefined_ : false; }


const char* IdentifierManager::lastLinkedIdentStr() const
{ return lastlinkedidentstr_.unescapedStr(); }


}; // namespace CmdDrive
