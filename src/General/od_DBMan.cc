/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "serverprogtool.h"

#include "applicationdata.h"
#include "commandlineparser.h"
#include "dbdir.h"
#include "dbman.h"
#include "filepath.h"
#include "iodirentry.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odjson.h"
#include "prog.h"
#include "surveydisklocation.h"


static const int cProtocolNr = 1;

static const char* sStatusCmd		= "status";
static const char* sListCmd		= ServerProgTool::sListUsrCmd();
static const char* sListSurveyCmd	= ServerProgTool::sListSurvCmd();
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
    : ServerProgTool(argc,argv,nullptr)
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
    set( sKey::DataRoot(), dataroot.buf() );
    if ( !dirnms.isEmpty() )
    {
	set( sKey::Size(), dirnms.size() );
	set( sKey::Names(), dirnms );
    }

    respondInfo( true );
}


void DBManServerTool::listObjs()
{
    const BufferString trgrpnm = getKeyedArgStr( sListCmd );
    const bool alltrlgrps = clp().hasKey( sAllCmd );
    const PtrMan<IODir> dbdir = IOM().getDir( trgrpnm.buf() );
    BufferStringSet nms, types, trls, trlgrps;
    DBKeySet ids;
    bool havetype = false;
    if ( dbdir )
    {
	for ( int idx=0; idx<dbdir->size(); idx++ )
	{
	    const IOObj* ioobj = dbdir->getObjs()[idx];
	    if ( !ioobj )
		continue;

	    if ( !ioobj->isTmp() &&
		 ( alltrlgrps ||
		   (!alltrlgrps && ioobj->group() == trgrpnm) ) )
	    {
		nms.add( ioobj->name() );
		ids.add( ioobj->key() );
		trls.add( ioobj->translator() );
		if ( alltrlgrps )
		    trlgrps.add( ioobj->group() );

		BufferString typ;
		if ( ioobj->pars().get(sKey::Type(),typ) && !typ.isEmpty() )
		    havetype = true;
		types.add( typ );
	    }
	}
    }

    set( sKey::Size(), ids.size() );
    set( sKey::IDs(), ids );
    set( sKey::Names(), nms );
    set( sKey::Format(2), trls );
    if ( !trlgrps.isEmpty() )
	set( ServerProgTool::sKeyTransGrp(2), trlgrps );
    if ( havetype )
	set( sKey::Types(), types );

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
	    IOObjContext::StdSelType tp = stdseltypimpl.getEnumForIndex( idx );
	    ConstPtrMan<IODir> dbdir = IOM().getDir( tp );
	    if ( !dbdir )
		continue;

	    for ( int idy=0; idy<dbdir->size(); idy++ )
	    {
		const IOObj* ioobj = dbdir->getObjs()[idy];
		if ( !ioobj || ioobj->name() != objnm )
		    continue;

		provideInfo( *ioobj, true );
		respondInfo( true );
		return;
	    }
	}
    }
    else
    {
	PtrMan<IODir> dbdir = IOM().getDir( trlgrpnm.buf() );
	if ( !dbdir )
	{
	    respondError( "No database directory for object found" );
	    return;
	}

	for ( int idx=0; idx<dbdir->size(); idx++ )
	{
	    const IOObj* ioobj = dbdir->getObjs()[idx];
	    if ( !ioobj || ioobj->name() != objnm )
		continue;

	    provideInfo( *ioobj, true );
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
    PtrMan<IOObj> ioobj = IOM().get( dbky );
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
    set( sKey::ID(), DBKey(ioobj.key()) );
    set( sKey::Name(), ioobj.name().buf() );
    set( sKey::Format(), ioobj.translator().buf() );
    if ( all )
	set( ServerProgTool::sKeyTransGrp(), ioobj.group().buf() );

    BufferString typ;
    if ( ioobj.pars().get(sKey::Type(),typ) && !typ.isEmpty() )
	set( sKey::Type(), typ.buf() );

    set( sKey::FileName(), ioobj.mainFileName().buf() );
}


void DBManServerTool::removeObj()
{
    const DBKey torem = getDBKey( sRemoveCmd );
    respondInfo( IOM().permRemove(torem) );
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

    const PtrMan<IODir> dbdir = IOM().getDir( MultiID(args.get(1).buf()) );
    if ( !dbdir )
    {
	respondError( "Invalid DBDir ID specified" );
	return;
    }

    IOStream iostrm( args.get(0) );
    iostrm.setKey( dbdir->getNewKey() );
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
    IODir* dbdirptr = mNonConst( dbdir.ptr() );
    if ( !dbdirptr->commitChanges(&iostrm) )
    {
	respondError( "Cannot commit new entry to data store" );
	return;
    }

    set( sKey::ID(), DBKey(iostrm.key()) );
    set( sKey::FileName(), iostrm.mainFileName().buf() );
    respondInfo( true );
}


BufferString DBManServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sStatusCmd, "" );
    addToUsageStr( ret, sListSurveyCmd, "" );
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
    DBManServerTool st( argc, argv );
    const CommandLineParser& clp = st.clp();

    if ( clp.hasKey(sListSurveyCmd) )
    {
	st.listSurveys();
	return app.exec();
    }

    const bool isbad = !IOMan::isOK();
    if ( isbad || clp.hasKey(sStatusCmd) )
    {
	if ( isbad )
	    st.respondError( "Data Store cannot be initialized" );
	else
	{
	    const FilePath rootdirfp( IOM().rootDir() );
	    st.set( sKey::DataRoot(), rootdirfp.pathOnly().buf() );
	    st.set( sKey::Survey(), IOM().surveyName() );
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
