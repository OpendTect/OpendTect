/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/


#include "stratunitrepos.h"
#include "stratreftree.h"
#include "ioman.h"
#include "ioobj.h"
#include "safefileio.h"
#include "survinfo.h"
#include "uistrings.h"

namespace Strat
{

const char* RepositoryAccess::fileNameBase() 	{ return "StratUnits"; }

class RefTreeMgr : public CallBacker
{
public:

RefTreeMgr()
{
    mAttachCB( IOM().surveyChanged, RefTreeMgr::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, RefTreeMgr::surveyChangedCB );
    mAttachCB( IOM().afterSurveyChange, RefTreeMgr::afterSurveyChangeCB );
}

~RefTreeMgr()
{
    detachAllNotifiers();
}

void surveyChangedCB( CallBacker* )
{
    rts_.erase();
}

void afterSurveyChangeCB( CallBacker* )
{
    Strat::loadDefaultTree();
}

RefTree& curTree()
{
    Threads::Locker lkr( lock_ );
    if ( rts_.isEmpty() )
	createTree();

    return *rts_[rts_.size()-1];
}

void addTree( RefTree* rt )
{
    Threads::Locker lkr( lock_ );
    rts_ += rt;
}

void popTree()
{
    Threads::Locker lkr( lock_ );
    delete rts_.removeSingle( rts_.size()-1 );
}

void setTree( RefTree* rt )
{
    if ( !rt )
	return;

    Threads::Locker lkr( lock_ );
    if ( rts_.isEmpty() )
	rts_ += rt;
    else
	delete rts_.replace( rts_.size()-1, rt );
}

private:

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

    ManagedObjectSet<RefTree>	rts_;
    Threads::Lock		lock_;

};


static RefTreeMgr& refTreeMgr()
{
    mDefineStaticLocalObject( RefTreeMgr, mgr, );
    return mgr;
}


void init()
{
    refTreeMgr();
    loadDefaultTree();
}


const RefTree& RT()
{
    return refTreeMgr().curTree();
}


void pushRefTree( RefTree* rt )
{
    refTreeMgr().addTree( rt );
}


void popRefTree()
{
    refTreeMgr().popTree();
}


void setRT( RefTree* rt )
{
    refTreeMgr().setTree( rt );
}


const char* sKeyDefaultTree()
{ return "Default.Stratigraphic Framework"; }

bool loadDefaultTree()
{
    MultiID key = MultiID::udf();
    SI().pars().get( sKeyDefaultTree(), key );
    if ( key.isUdf() )
	return false;

    RepositoryAccess ra;
    RefTree* tree = ra.read( key );
    Strat::setRT( tree );

    Strat::LevelSet* levels = LevelSet::read( key );
    Strat::setLVLS( levels );
    return tree && levels;
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
    {
	msg_ = uiStrings::phrCannotOpen(toUiString(fnm));
	return nullptr;
    }

    auto* rt = new RefTree;
    if ( !rt->read(sfio.istrm()) )
    {
	delete rt;
	msg_ = tr("Error during read of %1").arg(fnm);
	sfio.closeFail();
	return nullptr;
    }

    sfio.closeSuccess();
    msg_ = tr("Stratigraphic tree read from %1").arg(fnm);
    return rt;
}


RefTree* RepositoryAccess::read( const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if	( !ioobj )
	return nullptr;

    RefTree* tree = readFromFile( ioobj->fullUserExpr() );
    if ( tree ) tree->name_ = IOM().nameOf( key );
    return tree;
}


RefTree* RepositoryAccess::readTree( Repos::Source src )
{
    src_ = src;
    Repos::FileProvider rfp( fileNameBase() );
    const BufferString fnm( rfp.fileName(src) );
    RefTree* rt = readFromFile( fnm );
    if ( rt )
	rt->src_ = src;

    return rt;
}


bool RepositoryAccess::writeToFile( const RefTree& rt, const char* fnm )
{
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
    {
	msg_ = tr("Cannot write to %1").arg(fnm);
	return false;
    }

    if ( !rt.write(sfio.ostrm()) )
    {
	msg_ = tr("Error during write to %1").arg(fnm);
	sfio.closeFail();
	return false;
    }

    sfio.closeSuccess();
    msg_ = tr("Stratigraphic tree written to %1").arg(fnm);
    return true;
}


bool RepositoryAccess::write( const RefTree& rt, const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return false;

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
