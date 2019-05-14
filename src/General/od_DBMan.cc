/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "serverprogtool.h"
#include "commandlineparser.h"
#include "dbdir.h"
#include "dbman.h"
#include "filepath.h"
#include "iostrm.h"
#include "keystrs.h"
#include "odjson.h"
#include "prog.h"
#include "surveydisklocation.h"


static const int cProtocolNr = 1;

static const char* sStatusCmd		= "status";
static const char* sListCmd		= "list";
static const char* sListSurvCmd		= "list-surveys";
static const char* sInfoCmd		= "info";
static const char* sCreateCmd		= "create";
static const char* sRemoveCmd		= "remove";
static const char* sFileNameCmd		= "filename";

class DBManServerTool : public ServerProgTool
{
public:
		    DBManServerTool(int,char**);

    void	    listSurveys();
    void	    listObjs(const char* trgrpnm);
    void	    provideInfo(const DBKey&);
    void	    removeObj(const DBKey&);
    void	    createObj(const BufferStringSet&,const char* filenm);

protected:

    BufferString    getSpecificUsage() const override;

};


DBManServerTool::DBManServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"General")
{
    initParsing( cProtocolNr );
}


void DBManServerTool::listSurveys()
{
    BufferStringSet dirnms;
    const File::Path survfp( DBM().survDir() );
    const BufferString dataroot( survfp.pathOnly() );
    SurveyDiskLocation::listSurveys( dirnms, dataroot );
    set( sKey::DataRoot(), dataroot );
    if ( !dirnms.isEmpty() )
	set( sKey::Name(mPlural), dirnms );

    respondInfo( true );
}


void DBManServerTool::listObjs( const char* trgrpnm )
{
    BufferStringSet nms, types, trls; DBKeySet ids;
    bool havetype = false;
    auto dbdir = DBM().findDir( trgrpnm );
    if ( dbdir )
    {
	DBDirIter it( *dbdir );
	while ( it.next() )
	{
	    if ( !it.ioObj().isTmp() && it.ioObj().group() == trgrpnm )
	    {
		nms.add( it.ioObj().name() );
		ids.add( it.ioObj().key() );
		trls.add( it.ioObj().translator() );
		BufferString typ( it.ioObj().pars().find("Type") );
		typ.remove( ' ' );
		if ( !typ.isEmpty() )
		    { havetype = true; typ.replace( '`', '|' ); }
		types.add( typ );
	    }
	}
    }

    set( sKey::ID(mPlural), ids );
    set( sKey::Name(mPlural), nms );
    set( sKey::Format(mPlural), trls );
    if ( havetype )
	set( sKey::Type(mPlural), types );

    respondInfo( true );
}


void DBManServerTool::provideInfo( const DBKey& dbky )
{
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    if ( !ioobj )
	respondError( "Input object key not found" );

    set( sKey::ID(), ioobj->key() );
    set( sKey::Name(), ioobj->name() );
    set( sKey::Format(), ioobj->translator() );
    set( sKey::FileName(), ioobj->mainFileName() );
    const char* typstr = ioobj->pars().find( sKey::Type() );
    if ( typstr && *typstr )
	set( sKey::Type(), typstr );

    respondInfo( true );
}


void DBManServerTool::removeObj( const DBKey& dbky )
{
    respondInfo( DBM().removeEntry(dbky) );
}


void DBManServerTool::createObj( const BufferStringSet& args,
				 const char* filenm )
{
    if ( args.size() < 5 )
	respondError( "Specify at least name, dirid, trgrp, trl, ext. "
		      "Optional, type and/or --filename your_file_name." );

    auto dbdir = DBM().fetchDir( DBKey::DirID(toInt(args.get(1))) );
    if ( !dbdir )
	respondError( "Invalid DBDir ID specified" );

    IOStream iostrm( args.get(0) );
    iostrm.setKey( dbdir->newKey() );
    iostrm.setGroup( args.get(2) );
    iostrm.setTranslator( args.get(3) );
    iostrm.setExt( args.get(4) );
    if ( args.size() > 5 )
	iostrm.pars().set( sKey::Type(), args.get(5) );

    iostrm.setAbsDirectory( dbdir->dirName() );
    if ( filenm && *filenm )
	iostrm.fileSpec().setFileName( filenm );
    else
	iostrm.genFileName();

    iostrm.updateCreationPars();
    DBDir* dbdirptr = mNonConst( dbdir.ptr() );
    if ( !dbdirptr->commitChanges(iostrm) )
	respondError( "Cannot commit new entry to data store" );

    set( sKey::ID(), iostrm.key() );
    set( sKey::FileName(), iostrm.mainFileName() );
    respondInfo( true );
}


BufferString DBManServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sStatusCmd, "" );
    addToUsageStr( ret, sListCmd, "" );
    addToUsageStr( ret, sListSurvCmd, "" );
    addToUsageStr( ret, sInfoCmd, "object_id" );
    addToUsageStr( ret, sFileNameCmd, "object_id" );
    addToUsageStr( ret, sRemoveCmd, "object_id" );
    addToUsageStr( ret, sCreateCmd, "obj_name dir_id trl_group_name trl_name "
			    "extension [Type_in_omf] [--filename file_name]" );
    return ret;
}


int main( int argc, char** argv )
{
    DBManServerTool st( argc, argv );
    auto& clp = st.clp();

    if ( clp.hasKey( sListSurvCmd ) )
	st.listSurveys();

    const bool isbad = DBM().isBad();
    if ( isbad || clp.hasKey( sStatusCmd ) )
    {
	if ( isbad )
	    st.respondError( "Data Store cannot be initialised" );
	else
	{
	    File::Path fp( DBM().survDir() );
	    st.set( sKey::Survey(), fp.fileName() );
	    st.set( sKey::DataRoot(), fp.pathOnly() );
	}
	st.respondInfo( !isbad );
    }

    if ( clp.hasKey( sListCmd ) )
    {
	clp.setKeyHasValue( sListCmd, 1 );
	BufferString trgrpnm;
	clp.getVal( sListCmd, trgrpnm );
	st.listObjs( trgrpnm );
    }
    else if ( clp.hasKey( sInfoCmd ) )
    {
	clp.setKeyHasValue( sInfoCmd, 1 );
	DBKey dbky;
	clp.getDBKey( sInfoCmd, dbky );
	st.provideInfo( dbky );
    }
    else if ( clp.hasKey( sRemoveCmd ) )
    {
	clp.setKeyHasValue( sRemoveCmd, 1 );
	DBKey dbky;
	clp.getDBKey( sRemoveCmd, dbky );
	st.removeObj( dbky );
    }

    const int cridx = clp.indexOf( sCreateCmd );
    if ( cridx < 0 )
	st.exitWithUsage();

    clp.setKeyHasValue( sFileNameCmd, 1 );
    BufferString filenm;
    clp.getVal( sFileNameCmd, filenm );

    BufferStringSet normargs;
    clp.getNormalArguments( normargs );
    st.createObj( normargs, filenm );

    pFreeFnErrMsg( "Should not reach" );
    return ExitProgram( 0 );
}
