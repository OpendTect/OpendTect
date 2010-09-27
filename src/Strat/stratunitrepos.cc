/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.56 2010-09-27 11:05:19 cvsbruno Exp $";

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
    : rt_(0)
{
    IOM().surveyChanged.notify( mCB(this,RefTreeMgr,doNull) );
}

void doNull( CallBacker* )
{
    rt_ = 0;
}

void getTree()
{
    RepositoryAccess ra;
    rt_ = ra.readTree();
    if ( !rt_ )
    {
	rt_ = new RefTree;
	rt_->src_ = Repos::Survey;
    }
}

    RefTree*	rt_;

};

} // namespace


const Strat::RefTree& Strat::RT()
{
    static Strat::RefTreeMgr mgr;
    if ( !mgr.rt_ )
	mgr.getTree();

    return *mgr.rt_;
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
    SafeFileIO sfio( fnm, true );
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
