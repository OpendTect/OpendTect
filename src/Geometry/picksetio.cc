/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "picksetio.h"
#include "picksettr.h"
#include "picksetmgr.h"
#include "executor.h"
#include "uistrings.h"
#include "ioman.h"


Pick::SetSaver::SetSaver( const Pick::Set& ps )
    : OD::AutoSaveable(ps)
{
}


BufferString Pick::SetSaver::getFingerPrint() const
{
    BufferString ret;
    const Pick::Set& ps = pickSet();
    ret.set( ps.size() );
    //TODO size only is a bit crude, need some real stuff here
    return ret;
}


bool Pick::SetSaver::doStore( const IOObj& ioobj ) const
{
    return PickSetTranslator::store( pickSet(), &ioobj, errmsg_ );
}


Pick::SetLoader::SetLoader( const MultiID& ky )
    : setmgr_(0)
{
    toload_ += ky;
}


Pick::SetLoader::SetLoader( const TypeSet<MultiID>& kys )
    : toload_(kys)
    , setmgr_(0)
{
}


namespace Pick
{

class SetLoaderExec : public Executor
{ mODTextTranslationClass(SetLoaderExec)
public:

SetLoaderExec( const Pick::SetLoader& ldr )
    : Executor("Pick Set Loader")
    , loader_(ldr)
    , curidx_(0)
{
    loader_.available_.setEmpty();
    loader_.errmsgs_.setEmpty();
}

virtual od_int64 nrDone() const
{
    return curidx_;
}

virtual od_int64 totalNr() const
{
    return loader_.toload_.size();
}

virtual uiString uiMessage() const
{
    return uiStrings::phrLoading( uiStrings::sPickSet() );
}

virtual uiString uiNrDoneText() const
{
    return tr("Sets loaded");
}

    virtual int		nextStep();

    const SetLoader&	loader_;
    int			curidx_;

};

} // namespace Pick


int Pick::SetLoaderExec::nextStep()
{
    if ( curidx_ >= loader_.toload_.size() )
	return Finished();

    const MultiID id = loader_.toload_[curidx_];
    curidx_++;
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
    {
	pErrMsg( "Required ID not in IOM. Probably not OK" );
	return MoreToDo();
    }

    Pick::SetMgr& psmgr = loader_.getSetMgr();
    if ( psmgr.indexOf(id) >= 0 )
    {
	loader_.available_ += id;
	return MoreToDo(); // Already loaded, maybe changed, cannot load again!
    }

    Pick::Set* ps = new Pick::Set;
    uiString errmsg;
    if ( PickSetTranslator::retrieve(*ps,ioobj,errmsg) )
    {
	psmgr.set( ioobj->key(), ps );
	loader_.available_ += id;
    }
    else
    {
	delete ps;
	psmgr.set( id, 0 ); //Remove from Mgr if present.
	loader_.errmsgs_.add(
		uiStrings::phrJoinStrings( ioobj->uiName(), errmsg ) );
    }

    return MoreToDo();
}


Executor* Pick::SetLoader::getLoader() const
{
    return new SetLoaderExec( *this );
}


Pick::SetMgr& Pick::SetLoader::getSetMgr() const
{
    return setmgr_ ? *setmgr_ : Pick::Mgr();
}


bool Pick::SetLoader::load() const
{
    Executor* exec = getLoader();
    exec->execute();
    delete exec;
    return allOK();
}
