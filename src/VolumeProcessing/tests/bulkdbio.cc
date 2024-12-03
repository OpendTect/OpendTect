/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "moddepmgr.h"
#include "testprog.h"

#include "ioman.h"
#include "ioobj.h"
#include "seistrctr.h"
#include "welltransl.h"

MultiID			singleentryaddedid_		= MultiID::udf();
TypeSet<MultiID>	singletypeentriesaddedid_;
TypeSet<MultiID>	allentriesaddedid_;

bool testNewSingleEntry( const IOObj* obj )
{
    if ( !obj )
	return false;

    IOObj* ioobj = obj->clone();
    const IOObjContext ctxt
			= TranslatorGroup::getGroup(ioobj->group()).ioCtxt();
    CtxtIOObj ctio( ctxt, ioobj );
    IOM().getNewEntry( ctio );
    singleentryaddedid_ = ctio.ioobj_->key();
    mRunStandardTestWithError( !singleentryaddedid_.isUdf(),
			       "Entry added success", "Udf MultiID" );
    return true;
}


bool testRemoveSingleEntry()
{
    uiRetVal uirv;
    mRunStandardTestWithError( IOM().implRemove(singleentryaddedid_,true,&uirv),
			       "Remove operation done", uirv.getText() );
    mRunStandardTestWithError( !IOM().isPresent(singleentryaddedid_),
			       "New Entry removed",
			       "Entry still present: " );
    singleentryaddedid_.setUdf();
    return true;
}


bool testNewEntriesSingleType( const ObjectSet<IOObj> objs )
{
    ManagedObjectSet<CtxtIOObj> ctios;
    for ( const auto* obj : objs )
    {
	IOObj* ioobj = obj->clone();
	const IOObjContext ctxt
		= TranslatorGroup::getGroup(ioobj->group()).ioCtxt();
	auto* ctio = new CtxtIOObj( ctxt, ioobj );
	ctios.add( ctio );
    }

    BufferString errmsg( "Current ctio set size: ", ctios.size() );
    errmsg.add( "; Expected size: " ).add( objs.size() );
    mRunStandardTestWithError( ctios.size()==objs.size(),
			       "CtxtIOObjs created successfully", errmsg );
    IOM().getNewEntries( ctios );
    for ( const auto* ctio : ctios )
	singletypeentriesaddedid_ += ctio->ioobj_->key();

    errmsg.setEmpty();
    errmsg.add( "Added ids current size: " )
	  .add( singletypeentriesaddedid_.size() )
	  .add( "; Expected size: " ).add( ctios.size() );
    const bool entriesok = !singletypeentriesaddedid_.isEmpty() &&
			   singletypeentriesaddedid_.size() == ctios.size();
    mRunStandardTestWithError( entriesok,
			       "Entries added successfully", errmsg );
    for ( const auto& id : singletypeentriesaddedid_ )
	mRunStandardTestWithError( !id.isUdf(),
				   "Entry OK", "Udf MultiID" );

    return true;
}


bool testRemoveEntriesSingleType()
{
    uiRetVal uirv;
    mRunStandardTestWithError( IOM().implRemove(singletypeentriesaddedid_,
						true,&uirv),
			       "All new  entries removed", uirv.getText() );
    for ( const auto& id : singletypeentriesaddedid_ )
    {
	mRunStandardTestWithError( !IOM().isPresent(id),
				   "New Entry removed",
				   "Entry still present" );
    }

    singletypeentriesaddedid_.setEmpty();
    return true;
}


bool testCloneObjSet( const ObjectSet<IOObj> objs, const char* type )
{
    mRunStandardTestWithError( !objs.isEmpty() && objs.size() == 2,
			       "Seis object set is OK",
			       BufferString("Seis object set size: ",
					    objs.size()) );
    for ( const auto* obj : objs )
    {
	BufferString errmsg( "Object type: ", obj->group() );
	errmsg.add( "; Expected type:").add( type );
	mRunStandardTestWithError( obj->group() == StringView(type),
				   "Correct Type", errmsg );
    }

    return true;
}


bool testNewEntries( const ObjectSet<IOObj>& objs )
{
    ManagedObjectSet<CtxtIOObj> ctios;
    for ( const auto* obj : objs )
    {
	IOObj* ioobj = obj->clone();
	const IOObjContext ctxt
	    = TranslatorGroup::getGroup(ioobj->group()).ioCtxt();
	auto* ctio = new CtxtIOObj( ctxt, ioobj );
	ctios.add( ctio );
    }

    BufferString errmsg( "Current ctio set size: ", ctios.size() );
    errmsg.add( "Actual size: " ).add( objs.size() );
    mRunStandardTestWithError( ctios.size()==objs.size(),
			       "CtxtIOObjs created successfully", errmsg );
    IOM().getNewEntries( ctios );
    for ( const auto* ctio : ctios )
	allentriesaddedid_ += ctio->ioobj_->key();

    errmsg.setEmpty();
    errmsg.add( "Added ids current size: " )
	.add( allentriesaddedid_.size() )
	.add( "; Expected size: " ).add( ctios.size() );
    const bool entriesok = !allentriesaddedid_.isEmpty() &&
	allentriesaddedid_.size() == ctios.size();
    mRunStandardTestWithError( entriesok,
	"Entries added successfully", errmsg );
    for ( const auto& id : allentriesaddedid_ )
	mRunStandardTestWithError( !id.isUdf(),
	    "Entry OK", "Udf MultiID" );

    return true;
}


bool testRemoveEntries()
{
    uiRetVal uirv;
    mRunStandardTestWithError( IOM().implRemove(allentriesaddedid_,
						true,&uirv),
			       "All new entries removed", uirv.getText() );
    for ( const auto& id : allentriesaddedid_ )
    {
	mRunStandardTestWithError( !IOM().isPresent(id),
				   "New Entry removed",
				   "Entry still present" );
    }

    allentriesaddedid_.setEmpty();
    return true;
}


#define mIOObjAbsent(obj,id,msg,objtyp) \
    PtrMan<IOObj> obj = IOM().get( id ); \
    msg.setEmpty(); \
    msg.add( "Valid obj: " ) \
       .add( objtyp ); \
    mRunStandardTestWithError( obj.ptr(), msg, IOM().message() )

#define mIOObjClone(outobj,inobj,msg,objtyp) \
    PtrMan<IOObj> outobj = inobj->clone(); \
    msg.setEmpty(); \
    msg.add( "Clone success: " ) \
       .add( objtyp ); \
    mRunStandardTestWithError( outobj.ptr(), msg, IOM().message() ) \
    BufferString outobj##nm( outobj->name(), "_copy" ); \
    outobj->setName( outobj##nm ); \
    msg.setEmpty(); \
    msg.add( "Rename success: " ) \
       .add( objtyp ); \
    mRunStandardTestWithError(outobj##nm==outobj->name(),msg,IOM().message()) \
    cloneobjset.add( outobj.ptr() ); \
    cloneobjnms.add( outobj->name() );


mLoad3Modules( "General", "Well", "Seis" )

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();

    MultiID seis1id, seis2id, well1id, well2id;
    mRunStandardTest(pars().get("Seis1", seis1id),
		     "First seismic multiID found")
    mRunStandardTest(pars().get("Seis2", seis2id),
		     "Second seismic multiID found")
    mRunStandardTest( pars().get( "Well1", well1id ),
		     "First well multiID found")
    mRunStandardTest(pars().get("Well2", well2id),
		     "Second well multiID found")

    BufferString msg;
    mIOObjAbsent(seis1obj,seis1id,msg,"Seis 1")
    mIOObjAbsent(seis2obj,seis2id,msg,"Seis 2")
    mIOObjAbsent(well1obj,well1id,msg,"Well 1")
    mIOObjAbsent(well2obj,well2id,msg,"Well 2")

    ObjectSet<IOObj> cloneobjset;
    BufferStringSet cloneobjnms;
    mIOObjClone(s1cloneobj,seis1obj,msg,"Seis 1")
    mIOObjClone(s2cloneobj,seis2obj,msg,"Seis 2")
    mIOObjClone(w1cloneobj,well1obj,msg,"Well 1")
    mIOObjClone(w2cloneobj,well2obj,msg,"Well 2")

    msg.setEmpty();
    msg.add( "Clone objects objectset: Current size: ")
       .add( cloneobjset.size() ).add( "; Actual size: " ).add( 4 );
    mRunStandardTestWithError( (!cloneobjset.isEmpty()
				    && cloneobjset.size()==4),
			       "Valid clone objects set.", msg );
    msg.setEmpty();
    msg.add( "Clone objects names: Current size: ")
	.add( cloneobjset.size() ).add( "; Actual size: " ).add( 4 );
    mRunStandardTestWithError( (!cloneobjnms.isEmpty()
				    && cloneobjnms.size()==4),
			       "Valid clone object names.", msg );
    if ( !testNewSingleEntry(w1cloneobj.ptr()) )
	return false;
    if ( !testRemoveSingleEntry() )
	return false;
    if ( !testNewSingleEntry(s1cloneobj.ptr()) )
	return false;
    if ( !testRemoveSingleEntry() )
	return false;

    ObjectSet<IOObj> wellcloneobjs;
    wellcloneobjs.add( w1cloneobj.ptr() );
    wellcloneobjs.add( w2cloneobj.ptr() );

    ObjectSet<IOObj> seiscloneobjs;
    seiscloneobjs.add( s1cloneobj.ptr() );
    seiscloneobjs.add( s2cloneobj.ptr() );

    if ( !testCloneObjSet(wellcloneobjs,"Well") )
	return false;
    if ( !testNewEntriesSingleType(wellcloneobjs) )
	return false;
    if ( !testRemoveEntriesSingleType() )
	return false;
    if ( !testCloneObjSet(seiscloneobjs,"Seismic Data") )
	return false;
    if ( !testNewEntriesSingleType(seiscloneobjs) )
	return false;
    if ( !testRemoveEntriesSingleType() )
	return false;

    if ( !testNewEntries(cloneobjset) )
	return false;
    if ( !testRemoveEntries() )
	return false;

    return true;
}
