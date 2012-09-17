/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.59 2012/01/26 13:20:17 cvsbert Exp $";

#include "stratunitrepos.h"
#include "stratreftree.h"
#include "safefileio.h"
#include "ioman.h"

const char* Strat::RepositoryAccess::fileNameBase() 	{ return "StratUnits"; }

namespace Strat
{

class RefTreeMgr : public CallBacker
{
public:

RefTreeMgr()
{
    IOM().surveyChanged.notify( mCB(this,RefTreeMgr,doNull) );
}

~RefTreeMgr()
{
    doNull( 0 );
}

void doNull( CallBacker* )
{
    deepErase( rts_ );
}

void createTree()
{
    RepositoryAccess ra;
    RefTree* rt = ra.readTree();
    if ( !rt )
    {
	rt = new RefTree;
	rt->src_ = Repos::Survey;
    }
    rts_ += rt;
}

RefTree& curTree()
{
    if ( rts_.isEmpty() )
	createTree();
    return *rts_[rts_.size()-1];
}

    ObjectSet<RefTree> rts_;

};

} // namespace


static Strat::RefTreeMgr& refTreeMgr()
{ static Strat::RefTreeMgr mgr; return mgr; }
const Strat::RefTree& Strat::RT()
{ return refTreeMgr().curTree(); }
void Strat::pushRefTree( Strat::RefTree* rt )
{ refTreeMgr().rts_ += rt; }
void Strat::popRefTree()
{ delete refTreeMgr().rts_.remove( refTreeMgr().rts_.size()-1 ); }


void Strat::setRT( RefTree* rt )
{
    if ( rt )
	delete refTreeMgr().rts_.replace( 0, rt );
}


Strat::RefTree* Strat::RepositoryAccess::readTree()
{
    Repos::FileProvider rfp( fileNameBase(), true );
    while ( rfp.next() )
    {
	src_ = rfp.source();
	RefTree* rt = readTree( src_ );
	if ( !rt || !rt->hasChildren() )
	    delete rt;
	else
	    return rt;
    }

    msg_ = "New, empty stratigraphic tree created";
    src_ = Repos::Survey;
    return new RefTree;
}


#define mAddFilenameToMsg(fnm) msg_.add(" '").add(fnm).add("'")

Strat::RefTree* Strat::RepositoryAccess::readTree( Repos::Source src )
{
    src_ = src;
    Repos::FileProvider rfp( fileNameBase() );
    const BufferString fnm( rfp.fileName(src) );
    SafeFileIO sfio( fnm );
    if ( !sfio.open(true) )
	{ msg_ = "Cannot open"; mAddFilenameToMsg(fnm); return 0; }

    RefTree* rt = new RefTree;
    if ( !rt->read(sfio.istrm()) )
    {
	delete rt;
	msg_ = "Error during read of"; mAddFilenameToMsg(fnm);
	sfio.closeFail(); return 0;
    }

    sfio.closeSuccess();
    msg_ = "Stratigraphic tree read from"; mAddFilenameToMsg(fnm);
    return rt;
}


bool Strat::RepositoryAccess::writeTree( const Strat::RefTree& rt,
					 Repos::Source src )
{
    src_ = src;
    Repos::FileProvider rfp( fileNameBase() );
    const BufferString fnm( rfp.fileName(src) );
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
	{ msg_ = "Cannot write to"; mAddFilenameToMsg(fnm); return 0; }

    if ( !rt.write(sfio.ostrm()) )
    {
	msg_ = "Error during write to"; mAddFilenameToMsg(fnm);
	sfio.closeFail(); return false;
    }

    sfio.closeSuccess();
    msg_ = "Stratigraphic tree written to"; mAddFilenameToMsg(fnm);
    return true;
}
