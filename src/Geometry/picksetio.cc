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
#include "dbman.h"
#include "dbdir.h"
#include "keystrs.h"

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
    mAttachCB( DBM().surveyChanged, SetCategoryFromTypeInOMFPutter::doWork );
}

void doWork( CallBacker* )
{
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Loc );
    if ( !dbdir )
	return;

    ObjectSet<IOObj> toset;
    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( ioobj.group() != mTranslGroupName(PickSet) )
	    continue;

	BufferString ioobjcat( ioobj.pars().find(sKey::Category()) );
	BufferString realcat( PickSetTranslator::getCategory(ioobj) );
	if ( realcat != ioobjcat )
	{
	    IOObj* replioobj = ioobj.clone();
	    replioobj->pars().set( sKey::Category(), realcat );
	    toset += replioobj;
	}
    }
    iter.retire(); // needed, otherwise deadlock

    for ( int idx=0; idx<toset.size(); idx++ )
	toset[idx]->commitChanges();
    deepErase( toset );
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
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Pick::SetSaver::~SetSaver()
{
    sendDelNotif();
}


mImplMonitorableAssignmentWithNoMembers(Pick::SetSaver,Saveable)


ConstRefMan<Pick::Set> Pick::SetSaver::pickSet() const
{
    return ConstRefMan<Set>( static_cast<const Set*>( object() ) );
}


void Pick::SetSaver::setPickSet( const Set& ps )
{
    setObject( ps );
}


uiRetVal Pick::SetSaver::doStore( const IOObj& ioobj,
				  const TaskRunnerProvider& trprov ) const
{
    uiRetVal uirv;
    ConstRefMan<Set> ps = pickSet();
    if ( !ps )
	return uiRetVal::OK();

    RefMan<Set> copiedset = new Set( *ps );
    uirv = PickSetTranslator::store( *copiedset, ioobj );
    if ( uirv.isOK() && storeIsSave(ioobj) )
	ps.getNonConstPtr()->setName( ioobj.name() );

    return uirv;
}


Pick::SetLoader::SetLoader( const DBKey& ky )
{
    toload_ += ky;
}


Pick::SetLoader::SetLoader( const DBKeySet& kys )
    : toload_(kys)
{
}


Pick::Set* Pick::SetLoader::getSingleSet( const IOObj& ioobj, uiRetVal& uirv,
					  const char* cat )
{
    Pick::Set* ps = new Pick::Set( 0, cat );
    uirv = PickSetTranslator::retrieve( *ps, ioobj );
    if ( !uirv.isOK() )
	unRefAndZeroPtr( ps );
    return ps;
}


namespace Pick
{

class SetLoaderExec : public Executor
{ mODTextTranslationClass(SetLoaderExec)
public:

SetLoaderExec( const Pick::SetLoader& ldr )
    : Executor("PointSet Loader")
    , loader_(ldr)
    , curidx_(-1)
{
    loader_.available_.setEmpty();
    loader_.uirv_.setEmpty();
}

virtual od_int64 nrDone() const
{
    return curidx_;
}

virtual od_int64 totalNr() const
{
    return loader_.toload_.size();
}

virtual uiString message() const
{
    return uiStrings::phrLoading( uiStrings::sPointSet() );
}

virtual uiString nrDoneText() const
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
    const DBKey id = loader_.toload_[curidx_];
    if ( psmgr.isLoaded(id) )
    {
	loader_.available_ += id;
	return MoreToDo();
    }

    PtrMan<IOObj> ioobj = getIOObj( id );
    if ( !ioobj )
	return MoreToDo();

    uiRetVal uirv;
    Pick::Set* ps = SetLoader::getSingleSet( *ioobj, uirv, loader_.category_);
    if ( !ps )
	loader_.uirv_.add( uirv );
    else
    {
	if ( psmgr.isLoaded(id) ) // check, someone may have beat me to it
	    ps->unRef();
	else
	    psmgr.addNew( *ps, id, &ioobj->pars(), true );

	loader_.available_ += id;
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
