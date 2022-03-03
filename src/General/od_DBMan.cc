/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "serverprogtool.h"

#include "applicationdata.h"
#include "commandlineparser.h"
#include "dbdir.h"
#include "dbman.h"
#include "filepath.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odjson.h"
#include "prog.h"
#include "surveydisklocation.h"


static const int cProtocolNr = 1;

static const char* sStatusCmd		= "status";
static const char* sListCmd		= ServerProgTool::sListUsrCmd();
static const char* sListSurvCmd		= "list-surveys";
static const char* sExistsCmd		= ServerProgTool::sExistsUsrCmd();
static const char* sInfoCmd		= ServerProgTool::sInfoUsrCmd();
static const char* sAllCmd		= ServerProgTool::sAllUsrCmd();
static const char* sCreateCmd		= "create";
static const char* sRemoveCmd		= "remove";
static const char* sFileNameArg		= "filename";
static const char* sTrlGrpArg		= "trl-grp";

class DBManServerTool : public ServerProgTool
{
public:
		    DBManServerTool(int,char**);

    void	    listSurveys();
    void	    listObjs();
    void	    checkExists();
    void	    provideInfo();
    void	    provideInfo(const IOObj&,bool all);
    void	    removeObj();
    void	    createObj();

protected:

    BufferString    getSpecificUsage() const override;

};


DBManServerTool::DBManServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"General")
{
    initParsing( cProtocolNr, false );
}


void DBManServerTool::listSurveys()
{
    BufferString dataroot
	    = getKeyedArgStr( CommandLineParser::sDataRootArg(), false );
    if ( dataroot.isEmpty() )
	dataroot = GetBaseDataDir();
    BufferStringSet dirnms;
    SurveyDiskLocation::listSurveys( dirnms, dataroot );
    set( sKey::DataRoot(), dataroot );
    if ( !dirnms.isEmpty() )
    {
	set( sKey::Size(), dirnms.size() );
	set( sKey::Name(mPlural), dirnms );
    }

    respondInfo( true );
}


void DBManServerTool::listObjs()
{
    const BufferString trgrpnm = getKeyedArgStr( sListCmd );
    const bool alltrlgrps = clp().hasKey( sAllCmd );
    auto dbdir = DBM().findDir( trgrpnm );
    BufferStringSet nms, types, trls, trlgrps; DBKeySet ids;
    bool havetype = false;
    if ( dbdir )
    {
	DBDirIter it( *dbdir );
	while ( it.next() )
	{
	    if ( !it.ioObj().isTmp() &&
		 ( alltrlgrps ||
		   (!alltrlgrps && it.ioObj().group() == trgrpnm) ) )
	    {
		nms.add( it.ioObj().name() );
		ids.add( it.ioObj().key() );
		trls.add( it.ioObj().translator() );
		if ( alltrlgrps )
		    trlgrps.add( it.ioObj().group() );

		BufferString typ;
		if ( it.ioObj().pars().get(sKey::Type(),typ) && !typ.isEmpty() )
		    havetype = true;
		types.add( typ );
	    }
	}
    }

    set( sKey::Size(), ids.size() );
    set( sKey::ID(mPlural), ids );
    set( sKey::Name(mPlural), nms );
    set( sKey::Format(mPlural), trls );
    if ( !trlgrps.isEmpty() )
	set( ServerProgTool::sKeyTransGrp(mPlural), trlgrps );
    if ( havetype )
	set( sKey::Type(mPlural), types );

    respondInfo( true );
}


void DBManServerTool::checkExists()
{
    const BufferString objnm = getKeyedArgStr( sExistsCmd );
    if ( objnm.isEmpty() )
    {
	respondError( "Incorrect usage: no object name provided" );
	return;
    }

    const BufferString trlgrpnm = getKeyedArgStr( sTrlGrpArg, false );
    if ( trlgrpnm.isEmpty() )
    {
	const EnumDefImpl<IOObjContext::StdSelType>& stdseltypimpl =
						IOObjContext::StdSelTypeDef();
	for ( int idx=0; idx<stdseltypimpl.size()-1; idx++ )
	{
	    auto dbdir = DBM().fetchDir( stdseltypimpl.getEnumForIndex(idx) );
	    if ( !dbdir )
		continue;
	    DBDirIter it( *dbdir );
	    while ( it.next() )
	    {
		if ( it.ioObj().name() != objnm )
		    continue;
		provideInfo( it.ioObj(), true );
		respondInfo( true );
		return;
	    }
	}
    }
    else
    {
	auto dbdir = DBM().findDir( trlgrpnm );
	if ( !dbdir )
	{
	    respondError( "No database directory for object found" );
	    return;
	}

	DBDirIter it( *dbdir );
	while ( it.next() )
	{
	    if ( it.ioObj().name() != objnm )
		continue;
	    provideInfo( it.ioObj(), true );
	    respondInfo( true );
	    return;
	}
    }

    set( "message", "Input object key not found" );
    respondInfo( true );
}


void DBManServerTool::provideInfo()
{
    const DBKey dbky = getDBKey( sInfoCmd );
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    if ( !ioobj.ptr() )
    {
	respondError( "Input object key not found" );
	return;
    }

    provideInfo( *ioobj.ptr(), clp().hasKey(sAllCmd) );

    respondInfo( true );
}


void DBManServerTool::provideInfo( const IOObj& ioobj, bool all )
{
    set( sKey::ID(), ioobj.key() );
    set( sKey::Name(), ioobj.name() );
    set( sKey::Format(), ioobj.translator() );
    if ( all )
	set( ServerProgTool::sKeyTransGrp(), ioobj.group() );
    BufferString typ;
    if ( ioobj.pars().get(sKey::Type(),typ) && !typ.isEmpty() )
	set( sKey::Type(), typ );
    set( sKey::FileName(), ioobj.mainFileName() );
}


void DBManServerTool::removeObj()
{
    const DBKey torem = getDBKey( sRemoveCmd );
    respondInfo( DBM().removeEntry(torem) );
}


void DBManServerTool::createObj()
{
    const BufferString filenm = getKeyedArgStr( sFileNameArg, false );
    BufferStringSet args;
    clp().getNormalArguments( args );

    if ( args.size() < 5 )
    {
	respondError( "Specify at least name, dirid, trgrp, trl, ext. "
		      "Optional, type and/or --filename your_file_name." );
	return;
    }

    auto dbdir = DBM().fetchDir( DBKey::DirID(toInt(args.get(1))) );
    if ( !dbdir )
    {
	respondError( "Invalid DBDir ID specified" );
	return;
    }

    IOStream iostrm( args.get(0) );
    iostrm.setKey( dbdir->newKey() );
    iostrm.setGroup( args.get(2) );
    iostrm.setTranslator( args.get(3) );
    iostrm.setExt( args.get(4) );
    if ( args.size() > 5 )
	iostrm.pars().set( sKey::Type(), args.get(5) );

    iostrm.setAbsDirectory( dbdir->dirName() );
    if ( filenm.isEmpty() )
	iostrm.genFileName();
    else
	iostrm.fileSpec().setFileName( filenm );

    iostrm.updateCreationPars();
    DBDir* dbdirptr = mNonConst( dbdir.ptr() );
    if ( !dbdirptr->commitChanges(iostrm) )
    {
	respondError( "Cannot commit new entry to data store" );
	return;
    }

    set( sKey::ID(), iostrm.key() );
    set( sKey::FileName(), iostrm.mainFileName() );
    respondInfo( true );
}


BufferString DBManServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sStatusCmd, "" );
    addToUsageStr( ret, sListSurvCmd, "" );
    addToUsageStr( ret, sListCmd, "trl_group_name" );
    addToUsageStr( ret, sAllCmd, "output information for all groups" );
    addToUsageStr( ret, sExistsCmd, "obj_name [--trl-grp trl_group_name]" );
    addToUsageStr( ret, sInfoCmd, "object_id" );
    addToUsageStr( ret, sRemoveCmd, "object_id" );
    addToUsageStr( ret, sCreateCmd, "obj_name dir_id trl_group_name trl_name "
			    "extension [Type_in_omf] [--filename file_name]" );
    return ret;
}


int mProgMainFnName( int argc, char** argv )
{
    ApplicationData app;
    Threads::sleep( 30 );
    DBManServerTool st( argc, argv );
    CommandLineParser& clp = st.clp();

    if ( clp.hasKey(sListSurvCmd) )
    {
	st.listSurveys();
	return app.exec();
    }

    st.setDBMDataSource();

    const bool isbad = DBM().isBad();
    if ( isbad || clp.hasKey(sStatusCmd) )
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
	return app.exec();
    }

    if ( clp.hasKey(sListCmd) )
	st.listObjs();
    else if ( clp.hasKey(sExistsCmd) )
	st.checkExists();
    else if ( clp.hasKey(sInfoCmd) )
	st.provideInfo();
    else if ( clp.hasKey(sRemoveCmd) )
	st.removeObj();
    else if ( clp.hasKey(sCreateCmd) )
	st.createObj();
    else
	st.exitWithUsage();

    return app.exec();
}
