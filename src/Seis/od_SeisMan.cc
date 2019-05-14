/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "serverprogtool.h"
#include "seisprovider.h"
#include "seisstorer.h"
#include "commandlineparser.h"
#include "dbdir.h"
#include "keystrs.h"
#include "prog.h"


static const int cProtocolNr = 1;

static const char* sListCubesCmd	= "list-cubes";
static const char* sListLinesCmd	= "list-lines";
static const char* sListPS3DCmd		= "list-ps3d";
static const char* sListPS2DCmd		= "list-ps2d";
static const char* sListAttribs2DCmd	= "list-2d-attributes";

class SeisServerTool : public ServerProgTool
{
public:

    mUseType( Seis, GeomType );

		    SeisServerTool(int,char**);

    void	    listCubes();
    void	    listLines(const char* attr);
    void	    listPS2D();
    void	    listPS3D();
    void	    listAttribs2D();

protected:

    BufferString    getSpecificUsage() const override;
    IOObjContext*   ctxt_	    = nullptr;

    void	    getCtxt(GeomType,bool forread=true);
    void	    listObjs(GeomType);

};


SeisServerTool::SeisServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"Seis")
{
    initParsing( cProtocolNr );
}


void SeisServerTool::getCtxt( GeomType gt, bool forread )
{
    ctxt_ = Seis::getIOObjContext( gt, forread );
}


void SeisServerTool::listObjs( GeomType gt )
{
    getCtxt( gt );

    DBKeySet ids;
    BufferStringSet nms;
    BufferStringSet types;
    DBDirEntryList del( *ctxt_ );

    for ( int idx=0; idx<del.size(); idx++ )
    {
	ids.add( del.key(idx) );
	nms.add( del.name(idx) );
	types.add( del.ioobj(idx).pars().find(sKey::Type()) );
    }

    setSize( ids.size() );
    set( sKey::ID(mPlural), ids );
    set( sKey::Name(mPlural), nms );
    set( sKey::Type(mPlural), types );
    respondInfo( true );
}


void SeisServerTool::listCubes()
{
    listObjs( Seis::Vol );
}


void SeisServerTool::listLines( const char* attr )
{
    set( sKey::TODO(), "TODO" );
    respondInfo( true );
}


void SeisServerTool::listPS3D()
{
    listObjs( Seis::VolPS );
}


void SeisServerTool::listPS2D()
{
    listObjs( Seis::LinePS );
}


void SeisServerTool::listAttribs2D()
{
    listObjs( Seis::Line );
}


BufferString SeisServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sListCubesCmd, "" );
    addToUsageStr( ret, sListLinesCmd, "[attrib_name=Seis]" );
    addToUsageStr( ret, sListPS3DCmd, "" );
    addToUsageStr( ret, sListPS2DCmd, "" );
    addToUsageStr( ret, sListAttribs2DCmd, "" );
    return ret;
}


int main( int argc, char** argv )
{
    SeisServerTool st( argc, argv );
    auto& clp = st.clp();

    if ( clp.hasKey(sListCubesCmd) )
	st.listCubes();
    else if ( clp.hasKey(sListPS3DCmd) )
	st.listPS3D();
    else if ( clp.hasKey(sListPS2DCmd) )
	st.listPS2D();
    else if ( clp.hasKey(sListAttribs2DCmd) )
	st.listAttribs2D();
    else if ( clp.hasKey(sListLinesCmd) )
    {
	BufferStringSet normargs;
	clp.getNormalArguments( normargs );
	BufferString attrnm( "Seis" );
	if ( !normargs.isEmpty() )
	    attrnm.set( normargs.get(0) );
	st.listLines( attrnm );
    }

    pFreeFnErrMsg( "Should not reach" );
    return ExitProgram( 0 );
}
