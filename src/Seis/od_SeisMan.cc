/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "serverprogtool.h"

#include "applicationdata.h"
#include "ascbinstream.h"
#include "commandlineparser.h"
#include "ctxtioobj.h"
#include "cubesubsel.h"
#include "dbman.h"
#include "dbdir.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "od_iostream.h"
#include "cubedata.h"
#include "posinfo2d.h"
#include "prog.h"
#include "segydirectdef.h"
#include "segyfiledata.h"
#include "segyfiledef.h"
#include "seisioobjinfo.h"
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "seisstorer.h"
#include "seistrc.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "transl.h"


static const int cProtocolNr = 1;

static const char* sListCubesCmd	= "list-cubes";
static const char* sListLinesCmd	= "list-lines";
static const char* sListPS3DCmd		= "list-ps3d";
static const char* sListPS2DCmd		= "list-ps2d";
static const char* sListAttribs2DCmd	= "list-2d-attributes";
static const char* sInfoCmd		= ServerProgTool::sInfoUsrCmd();
static const char* sGeomInfoCmd		= "geometry-info";
static const char* sReadCmd		= "read";
static const char* sWriteCubeCmd	= "write-cube";
static const char* sWriteLineCmd	= "write-line";
static const char* sWritePS3DCmd	= "write-ps3d";
static const char* sWritePS2DCmd	= "write-ps2d";
static const char* sWriteSEGYDefCmd	= "write-segydef";

static const char* sAsciiArg		= "ascii";
static const char* sAllArg		= "all";
static const char* sComponentArg	= "component";
static const char* sEncodingArg		= "encoding";
static const char* sFormatArg		= "format";
static const char* sFileNameArg		= "filename";
static const char* sFileNamesArg	= "filenames";
static const char* sGeometryArg		= "geometry";
static const char* sGeomIDArg		= "geomid";
static const char* sLineNameArg		= "linename";
static const char* sNrFilesArg		= "nrfiles";
static const char* sRangeArg		= "range";
static const char* sTypeArg		= "type";

mUseType( CubeSubSel, pos_steprg_type );
mUseType( Pos::ZSubSel, z_steprg_type );


class SeisServerTool : public ServerProgTool
{
public:

    mUseType( Pos,	GeomID );
    mUseType( Seis,	GeomType );
    mUseType( Seis,	Provider );
    mUseType( Seis,	Storer );
    mUseType( PosInfo,	CubeData );
    mUseType( PosInfo,	LineData );

			SeisServerTool(int,char**);

    void		listCubes();
    void		listLines(const char* attr);
    void		listPS2D();
    void		listPS3D();
    void		listAttribs2D();
    void		provideGenInfo();
    void		provideGeometryInfo();
    void		read();
    void		writeCube(const char*);
    void		writeLine(const char*);
    void		writePS3D(const char*);
    void		writePS2D(const char*);
    void		writeSEGYDef(const char*);

protected:

    BufferString	getSpecificUsage() const override;

    IOObjContext*	ctxt_	    = nullptr;
    Storer*		storer_	    = nullptr;
    Provider*		prov_	    = nullptr;
    bool		ascii_	    = false;
    BufferString	translname_;
    BufferString	typetag_;
    int			encoding_   = 0;
    GeomID		geomid_;
    BufferString	linename_;
    int			lineidx_    = -1;

    void		getCtxt(GeomType,bool forread=true);
    void		listObjs(GeomType);
    BufferString	getObjName(const char*);
    void		mkGenObjInfo(const char*);
    void		set3DPos(int,int,int);
    void		set3DGeomInfo(bool);
    void		set2DGeomInfo(bool);
    void		writeObj(GeomType,const char*);
    void		finishWrite();
    void		writeData(GeomType);
    void		getProvider(const DBKey&);
    int			getTranslIdx(const char*) const;
    GeomType		geomTypeFromCL() const;
    bool		getLineFromCL(bool mandatory);
    bool		getFileNamesFromCL(BufferStringSet&) const;
    pos_steprg_type	posRgFromCL(int,const char*) const;
    z_steprg_type	zRgFromCL(int) const;
    void		slurpSEGYIndexingData(GeomType,SEGY::FileDataSet&);
    void		selectComponent();
    void		setReadRange();
    void		pushTrcHeader(ascbinostream&,SeisTrcInfo&);
    void		pushTrc(ascbinostream&,SeisTrc&);
    void		getNextTrace(SeisTrc&,ascbinostream&);

};


SeisServerTool::SeisServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"Seis")
{
    initParsing( cProtocolNr );
    ascii_ = clp().hasKey( sAsciiArg );
}


void SeisServerTool::getCtxt( GeomType gt, bool forread )
{
    ctxt_ = Seis::getIOObjContext( gt, forread );
}


BufferString SeisServerTool::getObjName( const char* ky )
{
    return getKeyedArgStr( ky, true );
}


void SeisServerTool::listObjs( GeomType gt )
{
    getCtxt( gt );

    DBKeySet ids; BufferStringSet nms; BufferStringSet types;
    DBDirEntryList del( *ctxt_ );

    for ( int idx=0; idx<del.size(); idx++ )
    {
	ids.add( del.key(idx) );
	nms.add( del.name(idx) );
	types.add( del.ioobj(idx).pars().find(sKey::Type()) );
    }

    set( sKey::Size(), ids.size() );
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
    getCtxt( Seis::Line );
    PtrMan<IOObj> ioobj = DBM().getByName( *ctxt_, attr );
    if ( !ioobj )
	respondError( BufferString( "Cannot find attribute '",attr,"'") );

    getProvider( ioobj->key() );
    TypeSet<int> gids; BufferStringSet lnms;
    for ( int idx=0; idx<prov_->nrGeomIDs(); idx++ )
    {
	const GeomID gid = prov_->geomID( idx );
	gids.add( gid.getI() );
	lnms.add( gid.name() );
    }
    set( sKey::Size(), gids.size() );
    set( sKey::GeomID(mPlural), gids );
    set( sKey::Name(mPlural), lnms );
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


void SeisServerTool::getProvider( const DBKey& dbky )
{
    uiRetVal uirv;
    prov_ = Provider::create( dbky, &uirv );
    if ( !uirv.isOK() )
	respondError( uirv );
}


void SeisServerTool::mkGenObjInfo( const char* cmd )
{
    const DBKey dbky = getDBKey( cmd );
    getProvider( dbky );

    set( sKey::ID(), dbky );
    set( sKey::Name(), prov_->name() );
    set( sKey::Geometry(), Seis::nameOf(prov_->geomType()) );
    set( sKey::ZDomain(), SeisIOObjInfo(dbky).zDomainDef().key() );
}


void SeisServerTool::provideGenInfo()
{
    mkGenObjInfo( sInfoCmd );

    BufferStringSet compnms;
    prov_->getComponentInfo( compnms );
    if ( compnms.size() > 1 )
	set( sKey::Component(mPlural), compnms );

    if ( prov_->isPS() )
	set( "Nr Offsets", prov_->nrOffsets() );

    if ( prov_->is2D() )
    {
	const auto& prov2d = *prov_->as2D();
	const auto nrlines = prov2d.nrLines();
	BufferStringSet lnms; TypeSet<int> gids;
	for ( int iln=0; iln<nrlines; iln++ )
	{
	    const GeomID gid = prov2d.geomID( iln );
	    lnms.add( gid.name() );
	    gids += gid.getI();
	}
	set( sKey::Size(), lnms.size() );
	set( sKey::Line(mPlural), lnms );
	set( sKey::GeomID(mPlural), gids );
    }

    respondInfo( true );
}


void SeisServerTool::set3DPos( int ptnr, int inl, int crl )
{
    const Coord coord( SI().transform(BinID(inl,crl)) );
    const BufferString baseky( "Corner.", ptnr );
    set( BufferString(baseky,".Inline"), inl );
    set( BufferString(baseky,".Crossline"), crl );
    set( BufferString(baseky,".X"), coord.x_ );
    set( BufferString(baseky,".Y"), coord.y_ );
}


void SeisServerTool::set3DGeomInfo( bool full )
{
    CubeData cd = prov_->as3D()->possibleCubeData();
    const auto nrtrcs = cd.totalSize();
    set( sKey::Size(), nrtrcs );
    if ( nrtrcs < 1 )
	return;

    if ( !full )
    {
	CubeData::pos_rg_type inlrg, crlrg;
	cd.getRanges( inlrg, crlrg );
	set3DPos( 0, inlrg.start, crlrg.start );
	set3DPos( 1, inlrg.stop, crlrg.start );
	set3DPos( 2, inlrg.stop, crlrg.stop );
	set3DPos( 3, inlrg.start, crlrg.stop );
    }
    else
    {
	TypeSet<int> inls, crls;
	PosInfo::CubeDataPos cdp;
	while ( cd.toNext(cdp) )
	{
	    const BinID bid( cd.binID( cdp ) );
	    inls += bid.inl();
	    crls += bid.crl();
	}
	set( sKey::Inline(mPlural), inls );
	set( sKey::Crossline(mPlural), crls );
    }
}


void SeisServerTool::set2DGeomInfo( bool full )
{
    LineData ld;
    prov_->as2D()->getLineData( lineidx_, ld );
    const auto nrtrcs = ld.size();
    set( sKey::Size(), nrtrcs );
    if ( nrtrcs < 1 )
	return;

    if ( !full )
    {
	const auto trcrg = ld.range();
	set( "First Trace", trcrg.start );
	set( "Last Trace", trcrg.stop );
	set( "Step Trace", ld.minStep() );
	Bin2D b2d( GeomID(ld.linenr_), trcrg.start );
	Coord coord( b2d.coord() );
	set( "First X", coord.x_ );
	set( "First Y", coord.y_ );
	b2d.trcNr() = trcrg.stop;
	coord = b2d.coord();
	set( "Last X", coord.x_ );
	set( "Last Y", coord.y_ );
    }
    else
    {
	TypeSet<int> trcnrs;
	TypeSet<double> xs, ys;
	PosInfo::LineDataPos ldp;
	const auto& survgeom = Survey::Geometry2D::get( GeomID(ld.linenr_) );
	while ( ld.toNext(ldp) )
	{
	    const auto trcnr = ld.pos( ldp );
	    trcnrs += trcnr;
	    const Coord coord( survgeom.getCoord(trcnr) );
	    xs += coord.x_;
	    ys += coord.y_;
	}
	set( sKey::Trace(mPlural), trcnrs );
	set( sKey::XCoord(mPlural), xs );
	set( sKey::YCoord(mPlural), ys );
    }
}


bool SeisServerTool::getLineFromCL( bool mandatory )
{
    const BufferString geomidnrstr = getKeyedArgStr( sGeomIDArg );
    if ( !geomidnrstr.isEmpty() )
    {
	geomid_.setI( geomidnrstr.toInt() );
	linename_ = geomid_.name();
    }
    else
    {
	linename_ = getKeyedArgStr( sLineNameArg );
	if ( !linename_.isEmpty() )
	    geomid_ = SurvGeom::getGeomID( linename_ );
    }

    const bool havegeomid = geomid_.isValid();
    if ( mandatory && !havegeomid )
	respondError( "Geometry ID not specified" );

    if ( havegeomid && prov_ )
    {
	const auto& prov2d = *prov_->as2D();
	lineidx_ = prov2d.lineIdx( geomid_ );
	if ( mandatory && lineidx_ < 0 )
	    respondError( BufferString("Line '",linename_,"' not present") );
    }

    if ( havegeomid )
    {
	set( sKey::GeomID(), geomid_.getI() );
	set( sKey::LineName(), linename_ );
    }

    return havegeomid;
}


void SeisServerTool::provideGeometryInfo()
{
    mkGenObjInfo( sGeomInfoCmd );

    const bool full = clp().hasKey( sAllArg );

    ZSampling zrg;
    if ( prov_->is2D() )
    {
	getLineFromCL( true );
	zrg = prov_->as2D()->zRange( lineidx_ );
	set2DGeomInfo( full );
    }
    else
    {
	set3DGeomInfo( full );
	zrg = prov_->zRange();
    }

    set( "First Z", zrg.start );
    set( "Last Z", zrg.stop );
    set( "Step Z", zrg.step );

    respondInfo( true );
}


pos_steprg_type SeisServerTool::posRgFromCL( int argidx0,
					     const char* what ) const
{
    pos_steprg_type ret;
    if ( !clp().getKeyedInfo(sRangeArg,ret.start,false,argidx0) )
	respondError( BufferString("Missing ",what," start") );
    if ( !clp().getKeyedInfo(sRangeArg,ret.stop,false,argidx0+1) )
	respondError( BufferString("Missing ",what," stop") );
    if ( !clp().getKeyedInfo(sRangeArg,ret.step,false,argidx0+2) )
	respondError( BufferString("Missing ",what," step") );
    if ( ret.start <= 0 ) ret.start = mUdf(Pos::Index_Type);
    if ( ret.stop <= 0 ) ret.stop = mUdf(Pos::Index_Type);
    if ( ret.step <= 0 ) ret.step = mUdf(Pos::Index_Type);
    return ret;
}


z_steprg_type SeisServerTool::zRgFromCL( int argidx0 ) const
{
    z_steprg_type ret;
    if ( !clp().getKeyedInfo(sRangeArg,ret.start,false,argidx0) )
	respondError( "Missing Z start" );
    if ( !clp().getKeyedInfo(sRangeArg,ret.stop,false,argidx0+1) )
	respondError( "Missing Z stop" );
    if ( !clp().getKeyedInfo(sRangeArg,ret.step,false,argidx0+2) )
	respondError( "Missing Z step" );
    if ( ret.step <= 0.f ) ret.step = mUdf(Pos::Z_Type);
    return ret;
}


void SeisServerTool::selectComponent()
{
    BufferString compstr = getKeyedArgStr( sComponentArg, false );
    int icomp = 0;
    if ( !compstr.isEmpty() )
    {
	BufferStringSet compnms;
	prov_->getComponentInfo( compnms );
	icomp = compstr.isNumber(true) ? compstr.toInt()
				       : compnms.indexOf( compstr );
	if ( icomp < 0 || icomp >= compnms.size() )
	    icomp = 0;
    }
    prov_->selectComponent( icomp );
}


void SeisServerTool::setReadRange()
{
    const bool is2d = prov_->is2D();
    if ( is2d )
	getLineFromCL( true );

    clp().setKeyHasValues( sRangeArg, is2d ? 6 : 9 );
    auto* rsd = is2d ? new Seis::RangeSelData( geomid_ )
		     : new Seis::RangeSelData;

    auto& fss = rsd->fullSubSel();
    if ( is2d )
    {
	const auto trcrg = posRgFromCL( 0, "Trace Number" );
	fss.setTrcNrRange( trcrg );
    }
    else
    {
	const auto inlrg = posRgFromCL( 0, "Inline" );
	fss.setInlRange( inlrg );
	const auto crlrg = posRgFromCL( 3, "Crossline" );
	fss.setCrlRange( crlrg );
    }
    const auto zrg = zRgFromCL( is2d ? 3 : 6 );
    rsd->zSubSel().setOutputZRange( zrg );
    prov_->setSelData( rsd );
}


void SeisServerTool::pushTrcHeader( ascbinostream& strm, SeisTrcInfo& ti )
{
    if ( prov_->is2D() )
	strm.add( ti.trcNr() ).add( ti.coord_.x_ ).add( ti.coord_.y_ );
    else
	strm.add( ti.inl() ).add( ti.crl() );
    if ( prov_->isPS() )
	strm.add( ti.offset_ );
}


void SeisServerTool::pushTrc( ascbinostream& strm, SeisTrc& trc )
{
    pushTrcHeader( strm, trc.info() );
    const auto ns = trc.size();
    for ( auto isamp=0; isamp<ns; isamp++ )
	strm.add( trc.get(isamp,0), isamp==ns-1 ? od_newline : od_tab );
    if ( !strm.stream().isOK() )
    {
	uiRetVal uirv;
	strm.stream().addErrMsgTo( uirv );
	respondError( uirv );
    }
}


void SeisServerTool::getNextTrace( SeisTrc& trc, ascbinostream& strm )
{
    const uiRetVal uirv = prov_->getNext( trc );
    if ( !uirv.isOK() )
    {
	SeisTrcInfo& ti = trc.info();
	ti.setTrcNr( 0 ); ti.setLineNr( 0 );
	ti.offset_ = 0.f; ti.coord_ = Coord( 0, 0 );
	pushTrcHeader( strm, ti );
	if ( isFinished(uirv) )
	    respondInfo( true );
	else
	    respondError( uirv );
    }
}


void SeisServerTool::read()
{
    const DBKey dbky = getDBKey( sReadCmd );
    getProvider( dbky );

    if ( clp().hasKey(sComponentArg) )
	selectComponent();
    if ( clp().hasKey(sRangeArg) )
	setReadRange();

    ascbinostream strm( outStream(), !ascii_ );
    SeisTrc trc; bool isfirst = true;
    while ( true )
    {
	getNextTrace( trc, strm );
	if ( isfirst )
	{
	    strm.add( trc.startPos() ).add( trc.stepPos() )
		.add( trc.size(), od_newline );
	    isfirst = false;
	}
	pushTrc( strm, trc );
    }
}


int SeisServerTool::getTranslIdx( const char* trnm ) const
{
    int ret = -1;
    const Translator* transl = ctxt_->trgroup_->getTemplate( trnm, true );
    if ( transl )
	ret = ctxt_->trgroup_->templates().indexOf( transl );
    return ret;
}


void SeisServerTool::writeObj( GeomType gt, const char* cmd )
{
    getCtxt( gt, false );

    int translidx = -1;
    bool issegy = false;
    if ( clp().hasKey(sFormatArg) )
    {
	BufferString trnm = getKeyedArgStr( sFormatArg, true );
	issegy = trnm.startsWith( "SEG" );
	translidx = getTranslIdx( trnm );
    }

    ctxt_->setName( getObjName(cmd) );
    CtxtIOObj ctio( *ctxt_ );
    auto res = ctio.fillObj( false, translidx );
    if ( !res )
	respondError( "Cannot create entry in Data Store" );

    bool ioobjchgd = false;
    if ( clp().hasKey(sTypeArg) )
    {
	BufferString typetag = clp().keyedString( sTypeArg, true );
	if ( !typetag.isEmpty() )
	{
	    ctio.ioobj_->pars().set( sKey::Type(), typetag );
	    ioobjchgd = true;
	}
    }

    if ( clp().hasKey(sEncodingArg) )
    {
	clp().setKeyHasValue( sEncodingArg );
	int encnr = 0;
	clp().getKeyedInfo( sEncodingArg, encnr );
	if ( encnr > 0 )
	{
	    if ( issegy )
		ctio.ioobj_->pars().set( "Number format", encnr );
	    else
		DataCharacteristics::putUserTypeToPar( ctio.ioobj_->pars(),
				(DataCharacteristics::UserType)encnr );
	    ioobjchgd = true;
	}
    }

    if ( ioobjchgd )
	ctio.ioobj_->commitChanges();

    storer_ = new Storer( *ctio.ioobj_ );
    ctio.setObj( nullptr );

    writeData( gt );
}


void SeisServerTool::finishWrite()
{
    const uiRetVal uirv = storer_->close();
    if ( !uirv.isOK() )
	respondError( uirv );
    else
	respondInfo( true );
}


void SeisServerTool::writeData( GeomType gt )
{
    respondInfo( true, false );

    ascbinistream strm( inStream(), !ascii_ );
    SeisTrc trc; SeisTrcInfo& ti = trc.info();
    int nrsamps;
    strm.get( ti.sampling_.start ).get( ti.sampling_.step ).get( nrsamps );
    if ( !strm.isOK() )
	respondError( "Failed to initialise seimic data slurp" );
    if ( SI().zIsTime() )
    {
	ti.sampling_.start *= 0.001f;
	ti.sampling_.step *= 0.001f;
    }

    trc.setSize( nrsamps );
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    int inl=0, crl=0, trcnr=0;

    while ( true )
    {
	if ( is2d )
	    strm.get( trcnr );
	else
	    strm.get( inl );

	if ( (is2d && trcnr==0) || (!is2d && inl==0) )
	    finishWrite();

	if ( !is2d )
	    strm.get( crl );
	else if ( !isps )
	    strm.get( ti.refnr_ ).get( ti.coord_.x_ ).get( ti.coord_.y_ );

	if ( isps )
	    strm.get( ti.offset_ ).get( ti.azimuth_ );

	if ( is2d )
	    ti.setPos( geomid_, trcnr );
	else
	    ti.setPos( BinID(inl,crl) );

	strm.getArr( trc.data().arr<float>(), nrsamps );
	if ( strm.isBad() )
	    finishWrite();

	const uiRetVal uirv = storer_->put( trc );
	if ( !uirv.isOK() )
	    respondError( uirv );
    }
}


void SeisServerTool::writeCube( const char* cmd )
{
    writeObj( Seis::Vol, cmd );
}


void SeisServerTool::writeLine( const char* cmd )
{
    // create entry in geometry manager first, obtaining geomid_
    respondError( "TODO" );
}


void SeisServerTool::writePS3D( const char* cmd )
{
    writeObj( Seis::VolPS, cmd );
}


void SeisServerTool::writePS2D( const char* cmd )
{
    writeObj( Seis::LinePS, cmd );
}


Seis::GeomType SeisServerTool::geomTypeFromCL() const
{
    BufferString gtstr = getKeyedArgStr( sGeometryArg, false );
    if ( gtstr.isEqual("2d",CaseInsensitive) )
	return Seis::Line;
    else if ( gtstr.isEqual("ps3d",CaseInsensitive) )
	return Seis::VolPS;
    else if ( gtstr.isEqual("ps2d",CaseInsensitive) )
	return Seis::LinePS;
    return Seis::Vol;
}


bool SeisServerTool::getFileNamesFromCL( BufferStringSet& fnms ) const
{
    clp().setKeyHasValue( sNrFilesArg );
    BufferString fnm;
    if ( clp().hasKey(sFileNameArg) )
    {
	fnm = getKeyedArgStr( sFileNameArg, true );
	fnms.add( fnm );
    }
    else if ( clp().hasKey(sFileNamesArg) )
    {
	clp().setKeyHasValue( sNrFilesArg );
	int nrfiles = 1;
	clp().getKeyedInfo( sNrFilesArg, nrfiles );
	clp().setKeyHasValues( sFileNamesArg, nrfiles );
	for ( int idx=0; idx<nrfiles; idx++ )
	{
	    clp().getKeyedInfo( sFileNamesArg, fnm, false, idx+1 );
	    fnms.add( fnm );
	}
    }
    return !fnms.isEmpty();
}


template <class T>
static T getFromClp( const CommandLineParser& clp, const char* clky, T defltval)
{
    T val = defltval;
    if ( clp.hasKey(clky) )
    {
	clp.setKeyHasValue( clky );
	clp.getKeyedInfo( clky, val );
    }
    return val;
}


void SeisServerTool::writeSEGYDef( const char* cmd )
{
    const GeomType gt = geomTypeFromCL();
    getCtxt( gt, false );
    ctxt_->setName( getObjName(cmd) );
    CtxtIOObj ctio( *ctxt_ );
    const int translidx = getTranslIdx( "SEGYDirect" );
    auto res = ctio.fillObj( false, translidx );
    if ( !res )
	respondError( "Cannot create SEGYDirect entry in Data Store" );

    IOPar fdspar;
    const int fmt = getFromClp( clp(), "format", 5 );
    const int ns = getFromClp( clp(), "ns", SI().zRange().nrSteps()+1 );
    const float z0 = getFromClp( clp(), "z0", SI().zRange().start );
    const float dz = getFromClp( clp(), "dz", SI().zRange().step );
    fdspar.set( SEGY::FilePars::sKeyNumberFormat(), fmt );
    fdspar.set( "Trace size", ns );
    fdspar.set( "Z sampling", z0, dz );
    Seis::putInPar( gt, fdspar );

    SEGY::FileDataSet fds( fdspar );
    BufferStringSet fnms;
    if ( !getFileNamesFromCL(fnms) )
	respondError( "Please specify the SEG-Y file name(s)" );
    for ( int idx=0; idx<fnms.size(); idx++ )
	fds.addFile( fnms.get(idx) );
    fds.getBaseDataFromPar( fdspar );

    SEGY::DirectDef def;
    def.setData( fds );
    const BufferString fnm( ctio.ioobj_->mainFileName() );
    if ( !def.writeHeadersToFile(fnm) )
	respondError( BufferString("Cannot open SEGYDirect data file ",fnm) );

    fds.setOutputStream( *def.getOutputStream() );
    respondInfo( true, false );
    slurpSEGYIndexingData( gt, fds );

    if ( !def.writeFootersToFile() )
	respondError( BufferString("Cannot finish SEGYDirect data file ",fnm) );

    respondInfo( true );
}


void SeisServerTool::slurpSEGYIndexingData( GeomType gt, SEGY::FileDataSet& fds)
{
    ascbinistream strm( inStream(), !ascii_ );

    int fileidx;
    od_int64 fileoffs;
    Seis::PosKey pk;
    Coord coord;
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );

    // We slurp:
	// file_number=signed_int32
	// binID_or_trace_number=signed_int32
	// [offset=float_32_native only for PreStack]
	// file_offset=signed_int64

    while ( strm.isOK() )
    {
	fileidx = -1;
	strm.get( fileidx );
	if ( fileidx < 0 )
	    return;

	if ( is2d )
	    strm.get( pk.trcNr() );
	else
	    strm.get( pk.binID().inl() ).get( pk.binID().crl() );

	if ( pk.trcNr() < 1 )
	    return;

	if ( isps )
	    strm.get( pk.offset() );

	strm.get( fileoffs );

	const bool res = fds.addTrace( fileidx, pk, coord, true );
	if ( !res )
	    respondError( "Error during data transfer" );
    }
}


BufferString SeisServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sListCubesCmd, "" );
    addToUsageStr( ret, sListLinesCmd, "[attrib_name]" );
    addToUsageStr( ret, sListPS3DCmd, "" );
    addToUsageStr( ret, sListPS2DCmd, "" );
    addToUsageStr( ret, sListAttribs2DCmd, "" );
    addToUsageStr( ret, sInfoCmd, "seis_id" );
    addToUsageStr( ret, sGeomInfoCmd, "seis_id [--geomid geomid]" );
#   define mAddWriteCmdToUsage( cmd ) \
	addToUsageStr( ret, cmd, \
	    "name [--ascii] [--format fmt] [--encoding nr] [--type tag]" )
    mAddWriteCmdToUsage( sWriteCubeCmd );
    mAddWriteCmdToUsage( sWriteLineCmd );
    mAddWriteCmdToUsage( sWritePS3DCmd );
    mAddWriteCmdToUsage( sWritePS2DCmd );
    addToUsageStr( ret, sWriteSEGYDefCmd,
		    "name --geometry 3D|2D|PS3D|PS3D more_args" );
    addToUsageStr( ret, sReadCmd,
	    "seis_id [--component comp_idx_or_name] [--ascii]"
	    "\n\t\t\t[--range trc_start_stop_step [crl_sss] z_sss]" );
    return ret;
}


int mProgMainFnName( int argc, char** argv )
{
    ApplicationData app;
    SeisServerTool st( argc, argv );
    CommandLineParser& clp = st.clp();

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
    else if ( clp.hasKey(sInfoCmd) )
	st.provideGenInfo();
    else if ( clp.hasKey(sGeomInfoCmd) )
	st.provideGeometryInfo();
    else if ( clp.hasKey(sReadCmd) )
	st.read();
    else if ( clp.hasKey(sWriteCubeCmd) )
	st.writeCube( sWriteCubeCmd );
    else if ( clp.hasKey(sWriteLineCmd) )
	st.writeLine( sWriteLineCmd );
    else if ( clp.hasKey(sWritePS3DCmd) )
	st.writePS3D( sWritePS3DCmd );
    else if ( clp.hasKey(sWritePS2DCmd) )
	st.writePS2D( sWritePS2DCmd );
    else if ( clp.hasKey(sWriteSEGYDefCmd) )
	st.writeSEGYDef( sWriteSEGYDefCmd );

    return app.exec();
}
