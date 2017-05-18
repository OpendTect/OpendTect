/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "stratunitrepos.h"
#include "stratreftree.h"
#include "safefileio.h"
#include "ioman.h"
#include "ioobj.h"
#include "uistrings.h"

namespace Strat
{

const char* RepositoryAccess::fileNameBase() 	{ return "StratUnits"; }

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


static RefTreeMgr& refTreeMgr()
{ mDefineStaticLocalObject( RefTreeMgr, mgr, ); return mgr; }
const RefTree& RT()
{ return refTreeMgr().curTree(); }
void pushRefTree( RefTree* rt )
{ refTreeMgr().rts_ += rt; }
void popRefTree()
{ delete refTreeMgr().rts_.removeSingle( refTreeMgr().rts_.size()-1 ); }


void setRT( RefTree* rt )
{
    if ( !rt ) return;

    if ( refTreeMgr().rts_.isEmpty() )
	refTreeMgr().rts_ += rt;
    else
    {
	const int currentidx = refTreeMgr().rts_.indexOf( &RT() );
	delete refTreeMgr().rts_.replace( currentidx < 0 ? 0 : currentidx, rt );
    }
}


// RepositoryAccess
RefTree* RepositoryAccess::readTree()
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

    msg_ = tr("New, empty stratigraphic tree created");
    src_ = Repos::Survey;
    return new RefTree;
}


RefTree* RepositoryAccess::readFromFile( const char* fnm )
{
    SafeFileIO sfio( fnm );
    if ( !sfio.open(true) )
	{ msg_ = uiStrings::phrCannotOpen(toUiString(fnm)); return 0; }

    RefTree* rt = new RefTree;
    if ( !rt->read(sfio.istrm()) )
    {
	delete rt;
	msg_ = tr("Error during read of %1").arg(fnm);
	sfio.closeFail(); return 0;
    }

    sfio.closeSuccess();
    msg_ = tr("Stratigraphic tree read from %1").arg(fnm);
    return rt;
}


RefTree* RepositoryAccess::read( const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if  ( !ioobj ) return 0;

    RefTree* tree = readFromFile( ioobj->fullUserExpr() );
    if ( tree ) tree->name_ = IOM().nameOf( key );
    return tree;
}


RefTree* RepositoryAccess::readTree( Repos::Source src )
{
    src_ = src;
    Repos::FileProvider rfp( fileNameBase() );
    const BufferString fnm( rfp.fileName(src) );
    return readFromFile( fnm );
}


bool RepositoryAccess::writeToFile( const RefTree& rt, const char* fnm )
{
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
	{ msg_ = tr("Cannot write to %1").arg(fnm); return 0; }

    if ( !rt.write(sfio.ostrm()) )
    {
	msg_ = tr("Error during write to %1").arg(fnm);
	sfio.closeFail(); return false;
    }

    sfio.closeSuccess();
    msg_ = tr("Stratigraphic tree written to %1").arg(fnm);
    return true;
}


bool RepositoryAccess::write( const RefTree& rt, const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    return writeToFile( rt, ioobj->fullUserExpr() );
}


bool RepositoryAccess::writeTree( const RefTree& rt, Repos::Source src )
{
    src_ = src;
    Repos::FileProvider rfp( fileNameBase() );
    const BufferString fnm( rfp.fileName(src) );
    return writeToFile( rt, fnm );
}

} // namespace Strat
