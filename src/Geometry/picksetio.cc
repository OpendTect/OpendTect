/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "picksetio.h"
#include "picksettr.h"
#include "picksetmanager.h"
#include "executor.h"
#include "uistrings.h"
#include "ioman.h"
#include "iodir.h"

mDefineInstanceCreatedNotifierAccess(Pick::SetSaver)


namespace Pick
{

/*\brief Makes sure actual Pick::Set's Category is also in IOObj's.

  This makes sure that:
  1) Selection can be done on Category by IOObjContext requirements
  2) Old versions of OpendTect will not have problems with it

*/

class SetCategoryFromTypeInOMFPutter : public CallBacker
{
public:

SetCategoryFromTypeInOMFPutter()
{
    doWork( 0 );
    mAttachCB( IOM().surveyChanged, SetCategoryFromTypeInOMFPutter::doWork );
}

void doWork( CallBacker* )
{
    const IODir iodir( MultiID( mIOObjContext(PickSet)
				.getStdDirData(IOObjContext::Loc)->id_ ) );
    for ( int idx=0; idx<iodir.size(); idx++ )
    {
	const IOObj& ioobj = *iodir.get( idx );
	if ( ioobj.group() != mTranslGroupName(PickSet) )
	    continue;

	BufferString ioobjcat( ioobj.pars().find(sKey::Category()) );
	BufferString realcat( PickSetTranslator::getCategory(ioobj) );
	if ( realcat != ioobjcat )
	{
	    IOObj* replioobj = ioobj.clone();
	    replioobj->pars().set( sKey::Category(), realcat );
	    IOM().commitChanges( *replioobj );
	    delete replioobj;
	}
    }
}

}; // class SetCategoryFromTypeInOMFPutter

static SetCategoryFromTypeInOMFPutter* cat_in_omf_putter = 0;

void startSetCategoryFromTypeInOMFPutter()
{
    cat_in_omf_putter = new SetCategoryFromTypeInOMFPutter;
}

} // namespace Pick


Pick::SetSaver::SetSaver( const Set& ps )
    : Saveable(ps)
{
    mTriggerInstanceCreatedNotifier();
}


Pick::SetSaver::SetSaver( const SetSaver& oth )
    : Saveable(oth)
{
    copyAll( oth );
    mTriggerInstanceCreatedNotifier();
}


Pick::SetSaver::~SetSaver()
{
    sendDelNotif();
}


mImplMonitorableAssignment(Pick::SetSaver,Saveable)

void Pick::SetSaver::copyClassData( const SetSaver& oth )
{
}


ConstRefMan<Pick::Set> Pick::SetSaver::pickSet() const
{
    return ConstRefMan<Set>( static_cast<const Set*>( object() ) );
}


void Pick::SetSaver::setPickSet( const Set& ps )
{
    setObject( ps );
}


bool Pick::SetSaver::doStore( const IOObj& ioobj ) const
{
    ConstRefMan<Set> ps = pickSet();
    if ( !ps )
	return true;

    // Try to be real fast, only lock the set during in-mem copy
    RefMan<Set> copiedset = new Set( *ps );
    if ( !PickSetTranslator::store(*copiedset,&ioobj,errmsg_) )
	return false;

    if ( storekey_ == ioobj.key() )
	ps.getNonConstPtr()->setName( ioobj.name() );

    return true;
}


Pick::SetLoader::SetLoader( const MultiID& ky )
{
    toload_ += ky;
}


Pick::SetLoader::SetLoader( const TypeSet<MultiID>& kys )
    : toload_(kys)
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
    , curidx_(-1)
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
    curidx_++;
    if ( curidx_ >= loader_.toload_.size() )
	return Finished();

    Pick::SetManager& psmgr = Pick::SetMGR();
    const MultiID id = loader_.toload_[curidx_];
    if ( psmgr.isLoaded(id) )
    {
	loader_.available_ += id;
	return MoreToDo();
    }

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
    {
	pErrMsg( "Required ID not in IOM. Probably not OK" );
	return MoreToDo();
    }

    Pick::Set* ps = new Pick::Set( 0, loader_.category_ );
    uiString errmsg;
    if ( PickSetTranslator::retrieve(*ps,ioobj,errmsg) )
    {
	if ( psmgr.isLoaded(id) ) // check, someone may have beat me to it
	    ps->unRef();
	else
	    psmgr.add( *ps, id, &ioobj->pars(), true );

	loader_.available_ += id;
    }
    else
    {
	ps->unRef();
	loader_.errmsgs_.add(
		uiStrings::phrJoinStrings( ioobj->uiName(), errmsg ) );
    }

    return MoreToDo();
}


Executor* Pick::SetLoader::getLoader() const
{
    return new SetLoaderExec( *this );
}


bool Pick::SetLoader::load() const
{
    Executor* exec = getLoader();
    exec->execute();
    delete exec;
    return allOK();
}
