/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2013
-*/

#include "testprog.h"
#include "dbman.h"
#include "dbdir.h"
#include "segydirecttr.h"
#include "seiscbvs.h"
#include "separstr.h"
#include "bufstringset.h"
#include "moddepmgr.h"

#include <iostream>

mExternC(Seis) void od_Seis_initStdClasses();

static void addToList( const IOObj& ioobj, BufferStringSet& strs )
{
    BufferString toadd( ioobj.name(), " - ", ioobj.group() );
    toadd.add( " (" ).add( ioobj.translator() ).add( ")" );
    strs.add( toadd );
}


static int runDBM( const IOObjContext& ctxt,
		   const BufferStringSet& allowedtransls,
		   BufferStringSet& strs )
{
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( ctxt );
    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	const char* translnm = ioobj.translator();
	if ( !allowedtransls.isPresent( translnm ) )
	    continue;
	const FixedString res = ioobj.pars().find( sKey::Type() );
	if ( res && res == sKey::Steering() )
	    continue;

	addToList( ioobj, strs );
    }
    return strs.size();
}


static int runEntryList( const IOObjContext& ctxt, BufferStringSet& strs )
{
    DBDirEntryList entrylist( ctxt );
    for ( int idx=0; idx<entrylist.size(); idx++ )
	addToList( entrylist.ioobj(idx), strs );
    return strs.size();
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("Seis");

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    ctxt.forread_ = true;
    BufferStringSet allowedtransls;
    allowedtransls.add( CBVSSeisTrcTranslator::translKey() );
    allowedtransls.add( SEGYDirectSeisTrcTranslator::translKey() );
    ctxt.toselect_.dontallow_.set( sKey::Type(), sKey::Steering() );

    BufferStringSet strsdbm;
    const int nritemsdbm = runDBM( ctxt, allowedtransls, strsdbm );
    strsdbm.sort();

    FileMultiString fms( allowedtransls.get(0) );
    fms += allowedtransls.get(1);
    ctxt.toselect_.allowtransls_ = fms;
    ctxt.toselect_.dontallow_.set( sKey::Type(), sKey::Steering() );

    BufferStringSet strsel;
    const int nritemsel = runEntryList( ctxt, strsel );
    strsel.sort();

    od_cout() << "Nr 3D Vol entries: " << nritemsel << od_endl;
    for ( int idx=0; idx<nritemsel; idx++ )
    {
	const BufferString str( strsel.get(idx) );
	od_cout() << str << od_endl;
    }

    od_cout() << "\nNr other entries: " << nritemsdbm - nritemsel << od_endl;
    for ( int idx=0; idx<nritemsdbm; idx++ )
    {
	const BufferString str( strsdbm.get(idx) );
	if ( !strsel.isPresent(str) )
	    od_cout() << str << od_endl;
    }

    return 0;
}
