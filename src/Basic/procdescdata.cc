/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : September 2019
-*/

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "procdescdata.h"
#include "uistrings.h"


mDefineEnumUtils(ProcDesc::DataEntry,Type,"Type")
{ ProcDesc::DataEntry::sKeyODv6(),
    ProcDesc::DataEntry::sKeyODv7(), ProcDesc::DataEntry::sKeyPython(), 0 };

template <>
void EnumDefImpl<ProcDesc::DataEntry::Type>::init()
 {
     uistrings_ += ::toUiString( "ODv6" );
     uistrings_ += ::toUiString( "ODv7" );
     uistrings_ += ::toUiString( "Python" );
 }


mDefineEnumUtils(ProcDesc::DataEntry,ActionType,"ActionType")
{
    "Add", "Remove", "AddNRemove", 0
};
 template <>
 void EnumDefImpl<ProcDesc::DataEntry::ActionType>::init()
 {
     uistrings_ += uiStrings::sAdd();
     uistrings_ += uiStrings::sRemove();
     uistrings_ += tr("Not sure");
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


ProcDesc::Data& ProcDesc::Data::add( ProcDesc::DataEntry* pdde )
{
    *this += pdde;
    return *this;
}


void ProcDesc::Data::setEmpty()
{
    addedodv6procs_.setEmpty();
    addedodv7procs_.setEmpty();
    addedpyprocs_.setEmpty();
    nrprocadded_ = 0;
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
	path_ = GetExecPlfDir();

    FilePath fp( path_, "data", "FirewallExceptionList" );

    pars_.setEmpty();

    if ( File::exists(fp.fullPath()) )
	pars_.read( fp.fullPath(), 0 );
    else
	return pars_;
    setEmpty();
    pars_.get( ProcDesc::DataEntry::sKeyODv6(), addedodv6procs_ );
    pars_.get( ProcDesc::DataEntry::sKeyODv7(), addedodv7procs_ );
    pars_.get( ProcDesc::DataEntry::sKeyPython(), addedpyprocs_ );

    nrprocadded_ = addedodv6procs_.size() + addedodv7procs_.size() +
						    addedpyprocs_.size();

    return pars_;
}


bool ProcDesc::Data::writePars( const IOPar& pars )
{
    const FilePath exceptionfp( path_, "data", "FirewallExceptionList" );
    BufferStringSet v6procs;
    BufferStringSet v7procs;
    BufferStringSet pyprocs;

    pars.get( ProcDesc::DataEntry::sKeyODv6(), v6procs );
    v6procs.append( addedodv6procs_ );
    pars.get( ProcDesc::DataEntry::sKeyODv7(), v7procs );
    v7procs.append( addedodv7procs_ );
    pars.get( ProcDesc::DataEntry::sKeyPython(), pyprocs );
    pyprocs.append( addedpyprocs_ );

    IOPar wrpars;
    wrpars.set( ProcDesc::DataEntry::sKeyODv6(), v6procs );
    wrpars.set( ProcDesc::DataEntry::sKeyODv7(), v7procs );
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
	BufferStringSet targetset = type == DataEntry::ODv6 ? addedodv6procs_ :
	type == DataEntry::ODv7 ? addedodv7procs_ : addedpyprocs_;
	for ( int idx=0; idx<ePDD().size(); idx++ )
	{
	    ProcDesc::DataEntry* pdde = ePDD()[idx];
	    if ( pdde->type_ == type )
	    {
		bool toadd = true;
		for ( int jidx=0; jidx<targetset.size(); jidx++ )
		{
		    BufferString nm = targetset.get(jidx);
		    if ( pdde->execnm_ == targetset.get(jidx) )
		    {
			toadd = false;
			break;
		    }

		}
		if ( toadd )
		{
		    nms.add( pdde->execnm_ );
		    descs.add( pdde->desc_ );
		}
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

    BufferStringSet targetset = type == DataEntry::ODv6 ? addedodv6procs_ :
	type == DataEntry::ODv7 ? addedodv7procs_ : addedpyprocs_;

    for ( int idx=0; idx<ePDD().size(); idx++ )
    {
	ProcDesc::DataEntry* pdde = ePDD()[idx];
	if ( pdde->type_ == type )
	{
	    for ( int jidx=0; jidx<targetset.size(); jidx++ )
	    {
		if ( pdde->execnm_ == targetset.get(jidx) )
		{
		    nms.add( pdde->execnm_ );
		    descs.add( pdde->desc_ );
		    break;
		}
	    }
	}
    }

}


ProcDesc::DataEntry::ActionType ProcDesc::Data::getActionType()
{
    readPars();
    const int sz = ePDD().size();
    if ( nrprocadded_ == 0 )
	return ProcDesc::DataEntry::Add;
    else if ( nrprocadded_ == ePDD().size() )
	return ProcDesc::DataEntry::Remove;
    else
	return ProcDesc::DataEntry::AddNRemove;
}
