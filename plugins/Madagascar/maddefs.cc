/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "maddefs.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "dirlist.h"
#include "strmprov.h"
#include "executor.h"
#include "globexpr.h"
#include "ptrman.h"
#include "od_iostream.h"


ODMad::ProgInfo& ODMad::PI()
{
    mDefineStaticLocalObject( PtrMan<ODMad::ProgInfo>, pi,
			      = new ODMad::ProgInfo );
    return *pi;
}

ODMad::ProgInfo::ProgInfo()
	: scanned_(false)
{
    rsfroot_ = GetEnvVar( "RSFROOT" );
    doPreScanCheck();
}


void ODMad::ProgInfo::cleanUp()
{
    deepErase( defs_ );
    groups_.erase();
}


void ODMad::ProgInfo::doPreScanCheck()
{
    errmsg_ = "";

    if ( rsfroot_.isEmpty() )
    {
	errmsg_ = "RSFROOT not set - no program definitions available";
	return;
    }
    if ( !File::isDirectory(rsfroot_) )
    {
	errmsg_ = "$RSFROOT (Madagascar) is invalid:\n";
	errmsg_ += rsfroot_;
	errmsg_ += "\nThe directory does not exist or cannot be read";
	return;
    }

    FilePath fp( rsfroot_, "doc", "txt" );	// very old versions
    defdir_ = fp.fullPath();

    if ( !File::isDirectory(defdir_) )		// vesrion 1.0
    {
	fp = FilePath( rsfroot_, "share", "txt" );
	defdir_ = fp.fullPath();
    }

    if ( !File::isDirectory(defdir_) )		// version 1.2
    {
	fp = FilePath( rsfroot_, "share", "doc", "madagascar", "txt" );
	defdir_ = fp.fullPath();
    }

    if ( !File::isDirectory(defdir_) )
    {
	errmsg_ = "Madagascar installation not prepared. Directory:\n";
	errmsg_ += defdir_;
	errmsg_ += "\ndoes not exist. You need to issue the command:\n"
		   "$RSFROOT/bin/sfdoc -t $RSFROOT/doc/txt";
    }
}


namespace ODMad
{

class ProgInfoScanner : public Executor
{
public:

ProgInfoScanner( ODMad::ProgInfo& pi )
    : Executor("Scanning Madagascar program definitions")
    , pi_(pi)
    , dl_(0)
    , totnr_(-1)
    , curnr_(0)
{
    pi_.cleanUp(); pi_.doPreScanCheck();
    pi_.scanned_ = true;
    if ( !pi_.errmsg_.isEmpty() ) return;

    dl_ = new DirList( pi_.defdir_, DirList::FilesOnly, "*.txt" );
    totnr_ = dl_->size();
    if ( totnr_ < 1 )
    {
	pi_.errmsg_ = "Madagascar Definition directory:\n";
	pi_.errmsg_ += pi_.defdir_;
	pi_.errmsg_ += "\ncontains no definition files";
	delete dl_; dl_ = 0;
    }
}

~ProgInfoScanner()
{
    delete dl_;
}

const char* message() const
{
    if ( !dl_ )
	return pi_.errMsg();
    if ( curnr_ < 0 || curnr_ >= dl_->size() )
	return "Scanning files";

    msg_ = "Scanning "; msg_ += dl_->get( curnr_ );
    return msg_;
}

const char*	nrDoneText() const	{ return "Files scanned"; }
od_int64	nrDone() const		{ return curnr_; }
od_int64	totalNr() const		{ return totnr_; }

int nextStep()
{
    if ( !dl_ )
	return ErrorOccurred();
    if ( curnr_ >= totnr_ )
	return Finished();

    pi_.addEntry( dl_->fullPath(curnr_) );
    curnr_++;

    return curnr_ >= totnr_ ? Finished() : MoreToDo();
}

    ODMad::ProgInfo&	pi_;
    DirList*		dl_;
    int			totnr_;
    int			curnr_;
    mutable BufferString msg_;

};

}


Executor* ODMad::ProgInfo::getScanner() const
{
    return new ProgInfoScanner( const_cast<ODMad::ProgInfo&>( *this ) );
}


void ODMad::ProgInfo::search( const char* str,
			      ObjectSet<const ProgDef>& res ) const
{
    if ( !str || !*str || !scanned_ ) return;

    BufferString gestr;
    if ( firstOcc(str,'*') )
	gestr = str;
    else
	{ gestr = "*"; gestr += str; gestr += "*"; }

    const GlobExpr ge( gestr, false );
    for ( int ityp=0; ityp<3; ityp++ )
    {
	for ( int idx=0; idx<defs_.size(); idx++ )
	{
	    const ProgDef* def = defs_[idx];
	    const BufferString& matchstr( ityp == 0 ? def->name_
				       : (ityp == 1 ? def->shortdesc_
						    : def->comment_) );
	    if ( ge.matches(matchstr) && !res[def] )
		res += def;
	}
    }
}

#define mbuflen 2048

void ODMad::ProgInfo::addEntry( const char* fnm )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
	return;

    ODMad::ProgDef* def = new ODMad::ProgDef;

    BufferString line;
    strm.getLine( line );
    if ( !matchString("Program ",line.buf()) )
	return;

    char* ptr = line.buf(); ptr += 8; // Skip 'Program '
    char* word = ptr;
    mSkipNonBlanks( ptr );
    if ( !*ptr ) { delete def; return; }
    *ptr = '\0'; def->name_ = word;

    ptr += 3; def->shortdesc_ = ptr;

    bool buildingcomment = false;
    BufferString tmp;
    while ( strm.getLine(line) )
    {
	if ( line[0] != '[' )
	{
	    if ( buildingcomment )
		{ line += "\n"; def->comment_ += line; }
	    continue;
	}

	const bool iscomment = line == "[COMMENTS]";
	const bool isparams = line == "[PARAMETERS]";
	if ( isparams )
	    { def->comment_ += "\n\nParameters:\n"; }
	    //TODO use PARAMETERS properly
	buildingcomment = iscomment || isparams;
	if ( buildingcomment )
	    continue;

	if ( line == "[SYNOPSIS]" )
	{
	    strm.getLine( line );
	    def->synopsis_ = line;
	}
	else if ( line == "[DIRECTORY]" )
	{
	    strm.getLine( line );
	    groups_.addIfNew( line );
	    def->group_ = find( groups_, line );
	}
    }

    if ( !def->group_ )
    {
	groups_.addIfNew( "<unknown>" );
	def->group_ = find( groups_, "<unknown>" );
    }

    defs_ += def;
}
