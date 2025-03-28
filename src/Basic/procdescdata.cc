/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "procdescdata.h"

#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odinst.h"

mDefineEnumUtils(ProcDesc::DataEntry,Type,"Type")
{ ProcDesc::DataEntry::sKeyODv6(), ProcDesc::DataEntry::sKeyPython(), 0 };

template <>
void EnumDefImpl<ProcDesc::DataEntry::Type>::init()
 {
     uistrings_ += ::toUiString( "ODv6" );
     uistrings_ += ::toUiString( "Python" );
 }


static const char* sCMDAdd() { return "add"; }
static const char* sCMDRemove() { return "rm"; }
static const char* sCMDAddAndRemove() { return "all"; }
static const char* sFlagODPath() { return "odpath"; }
static const char* sFlagPyPath() { return "pypath"; }

mDefineEnumUtils(ProcDesc::DataEntry,ActionType,"ActionType")
{
    "Add", "Remove", "AddNRemove", 0
};
 template <>
 void EnumDefImpl<ProcDesc::DataEntry::ActionType>::init()
 {
     uistrings_ += tr("Add Firewall Rules");
     uistrings_ += tr("Remove Firewall Rules");
     uistrings_ += tr("Add or Remove Firewall Rules");
 }


 ProcDesc::DataEntry::DataEntry()
 {}


 ProcDesc::DataEntry::~DataEntry()
 {}


const char* ProcDesc::DataEntry::getCMDActionKey( const ActionType acttype )
{
    if ( acttype == Add )
	return sCMDAdd();
    else if ( acttype == Remove )
	return sCMDRemove();
    else
	return sCMDAddAndRemove();
}


ProcDesc::DataEntry::ActionType ProcDesc::DataEntry::getActionTypeForCMDKey(
						   const BufferString& actcmd )
{
    if ( actcmd.isEqual(sCMDAdd()) )
	return Add;
    else if ( actcmd.isEqual(sCMDRemove()) )
	return Remove;
    else
	return AddNRemove;
}


const char* ProcDesc::DataEntry::getTypeFlag( const Type type )
{
    if ( type == Python )
	return sFlagPyPath();
    else
	return sFlagODPath();
}


bool ProcDesc::DataEntry::isCMDOK( const BufferString& cmd )
{
    if ( cmd.isEqual(sCMDAdd()) ||
	    cmd.isEqual(sCMDRemove()) ||
		cmd.isEqual(sCMDAddAndRemove()) )
	return true;

    return false;
}


ProcDesc::Data& ePDD()
{
    mDefineStaticLocalObject( PtrMan<ProcDesc::Data>, theinstance, = nullptr );
    return *theinstance.createIfNull();
}


const ProcDesc::Data& PDD()
{
    return ePDD();
}


void ProcDesc::Data::setPath( const BufferString& path )
{
    path_ = path;
}


bool ProcDesc::Data::hasWorkToDo( const BufferString& pypath, bool toadd )
 {
    ProcDesc::DataEntry::ActionType acttyp =
		toadd ? ProcDesc::DataEntry::Add : ProcDesc::DataEntry::Remove;

    BufferStringSet pyprocnms;
    uiStringSet pyprocdescs;
    ePDD().getProcData( pyprocnms, pyprocdescs,
					ProcDesc::DataEntry::Python, acttyp );


    FilePath fp( pypath );
    fp.add("envs");
    for ( int idx=pyprocnms.size()-1; idx>=0; idx-- )
    {
	const FilePath pyexefp( fp.fullPath(), pyprocnms.get(idx),
							    "python.exe" );
	if ( !File::exists(pyexefp.fullPath()) )
	{
	    pyprocnms.removeSingle( idx );
	    pyprocdescs.removeSingle( idx );
	    continue;
	}
    }

    uiStringSet odprocdescs;
    BufferStringSet odprocnms;
    ePDD().getProcData( odprocnms, odprocdescs, ProcDesc::DataEntry::OD,
								    acttyp );
    if ( odprocdescs.isEmpty() && pyprocdescs.isEmpty() )
	return false;

    const ProcDesc::DataEntry::ActionType availacttype = ePDD().getActionType();
    if ( availacttype == ProcDesc::DataEntry::AddNRemove ||
			acttyp == ProcDesc::DataEntry::AddNRemove )
	return true;

    return acttyp == availacttype;
 }


ProcDesc::Data& ProcDesc::Data::add( ProcDesc::DataEntry* pdde )
{
    *this += pdde;
    return *this;
}


void ProcDesc::Data::setEmpty()
{
    addedodprocs_.setEmpty();
    addedpyprocs_.setEmpty();
    addedprocnms_.setEmpty();
}


ProcDesc::Data& ProcDesc::Data::add( const char* nm, const uiString& desc,
						ProcDesc::DataEntry::Type typ )
{
    ProcDesc::DataEntry* pdde = new ProcDesc::DataEntry();
    pdde->desc_ = desc;
    pdde->execnm_ = nm;
    pdde->type_ = typ;

    return add( pdde );
}

#define mGetData(type) \

IOPar& ProcDesc::Data::readPars()
{
    if ( path_.isEmpty() )
	path_ = GetSoftwareDir( false );

    FilePath fp( path_, "data", "FirewallExceptionList" );

    pars_.setEmpty();

    if ( File::exists(fp.fullPath()) )
	pars_.read( fp.fullPath(), 0 );
    else
	return pars_;

    setEmpty();
    pars_.get( ProcDesc::DataEntry::sKeyODv6(), addedodprocs_ );
    pars_.get( ProcDesc::DataEntry::sKeyPython(), addedpyprocs_ );

    addedprocnms_.add( addedodprocs_, true );
    addedprocnms_.add( addedpyprocs_, true );

    return pars_;
}


#define mRemoveProcsNUpdateList(finallist,orglist) \
    for ( int jidx=0; jidx<finallist.size(); jidx++ ) \
    { \
	const int idx = orglist.indexOf( finallist.get(jidx) ); \
	if ( idx < 0 ) \
	    continue; \
	orglist.removeSingle( idx ); \
    } \
    finallist.setEmpty(); \
    finallist.append( orglist );


bool ProcDesc::Data::writePars( const IOPar& pars, bool toadd )
{
    const FilePath exceptionfp( path_, "data", "FirewallExceptionList" );
    BufferStringSet v6procs;
    BufferStringSet v7procs;
    BufferStringSet pyprocs;

    pars.get( ProcDesc::DataEntry::sKeyODv6(), v6procs );
    pars.get( ProcDesc::DataEntry::sKeyPython(), pyprocs );

    if ( toadd )
    {
	v6procs.append( addedodprocs_ );
	pyprocs.append( addedpyprocs_ );
    }
    else
    {
	mRemoveProcsNUpdateList( v6procs, addedodprocs_ )
	mRemoveProcsNUpdateList( pyprocs, addedpyprocs_ )
    }

    IOPar wrpars;
    wrpars.set( ProcDesc::DataEntry::sKeyODv6(), v6procs );
    wrpars.set( ProcDesc::DataEntry::sKeyPython(), pyprocs );
    return wrpars.write( exceptionfp.fullPath(), 0 );
}


void ProcDesc::Data::getProcData( BufferStringSet& nms, uiStringSet& descs,
			DataEntry::Type type, DataEntry::ActionType acttyp )
{
    if ( acttyp == DataEntry::Add )
	getProcsToBeAdded( nms, descs, type );
    else if ( acttyp == DataEntry::Remove )
	getProcsToBeRemoved( nms, descs, type );

    if ( type == DataEntry::OD )
    {
	int idx = nms.indexOf( ODInst::sKeyODExecNm() );

	if ( idx <  0 )
	    return;

	nms.swap( idx, 0 );
	descs.swap( idx, 0 );
    }
}


void ProcDesc::Data::getProcsToBeAdded( BufferStringSet& nms,
				uiStringSet& descs, DataEntry::Type type )
{
    readPars();
    if ( pars_.isEmpty() )
    {
	 for ( int idx=0; idx<ePDD().size(); idx++ )
	{
	    ProcDesc::DataEntry* pdde = ePDD()[idx];
	    if ( pdde->type_ == type )
	    {
		nms.add( pdde->execnm_ );
		descs.add( pdde->desc_ );
	    }
	 }
	 return;
    }
    else
    {
	BufferStringSet targetset = type == DataEntry::OD ? addedodprocs_ :
							 addedpyprocs_;
	for ( int idx=0; idx<ePDD().size(); idx++ )
	{
	    ProcDesc::DataEntry* pdde = ePDD()[idx];
	    if ( pdde->type_ == type && targetset.indexOf(pdde->execnm_) < 0 )
	    {
		nms.add( pdde->execnm_ );
		descs.add( pdde->desc_ );
	    }
	}

    }
}


void ProcDesc::Data::getProcsToBeRemoved( BufferStringSet& nms,
				    uiStringSet& descs, DataEntry::Type type )
{
    readPars();
    if ( pars_.isEmpty() )
	return; // no process are added already hence none to be removed

    BufferStringSet targetset = type == DataEntry::OD ? addedodprocs_ :
								addedpyprocs_;

    for ( int idx=0; idx<ePDD().size(); idx++ )
    {
	ProcDesc::DataEntry* pdde = ePDD()[idx];
	if ( pdde->type_ == type && targetset.indexOf(pdde->execnm_) >= 0 )
	{
	    nms.add( pdde->execnm_ );
	    descs.add( pdde->desc_ );
	}
    }

}


ProcDesc::DataEntry::ActionType ProcDesc::Data::getActionType()
{
    readPars();
    BufferStringSet reqexentadded;
    int alreadyadded = 0;

    for ( int idx=0; idx<ePDD().size(); idx++ )
    {
	const BufferString requiredexecnm = ePDD()[idx]->execnm_;

	if ( addedprocnms_.indexOf(requiredexecnm,OD::CaseInsensitive) < 0 )
	    reqexentadded.add( requiredexecnm );
	else
	    alreadyadded++;
    }

    if ( alreadyadded == 0 && reqexentadded.size() > 0 )
	return ProcDesc::DataEntry::Add;
    else if ( alreadyadded == ePDD().size() && addedprocnms_.size() >= size() )
	return ProcDesc::DataEntry::Remove;
    else
	return ProcDesc::DataEntry::AddNRemove;
}


void gatherFireWallProcInf()
{
    const FilePath fp( GetSoftwareDir(false), "data", "Firewall" );
    const DirList filelist( fp.fullPath().buf(),
			    File::DirListType::FilesInDir );
    for ( int idx=0; idx<filelist.size(); idx++ )
    {
	const BufferString& filefp = filelist.fullPath( idx );
	IOPar par;
	par.read( filefp, nullptr );
	const int typeidx = ProcDesc::DataEntry::TypeDef().indexOf(
					    par.find(sKey::Type()).buf() );
	ePDD().add( par.find(sKey::Name()), toUiString(par.find("Def")),
	    ProcDesc::DataEntry::TypeDef().getEnumForIndex(typeidx) );
    }
}
