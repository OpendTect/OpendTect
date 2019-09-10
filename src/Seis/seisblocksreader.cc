/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksreader.h"
#include "seisblocksbackend.h"
#include "seisseldata.h"
#include "seistrc.h"
#include "ascstream.h"
#include "cubedata.h"
#include "datachar.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "keystrs.h"
#include "od_istream.h"
#include "posidxpairdataset.h"
#include "scaler.h"
#include "separstr.h"
#include "survgeom3d.h"
#include "uistrings.h"
#include "zdomain.h"
#include "zsubsel.h"


#define mSeisBlocksReaderInitList() \
      fileidtbl_(*new FileIDTable) \
    , backend_(0) \
    , scaler_(0) \
    , interp_(0) \
    , curcdpos_(*new PosInfo::CubeDataPos) \
    , seldata_(0) \
    , nrcomponentsintrace_(0) \
    , depthinfeet_(false) \
    , lastopwasgetinfo_(false)



Seis::Blocks::Reader::Reader( const char* inp )
    : mSeisBlocksReaderInitList()
{
    initFromFileName( inp );
}


Seis::Blocks::Reader::Reader( od_istream& strm )
    : mSeisBlocksReaderInitList()
{
    initFromFileName( strm.fileName() );
    backend_ = new StreamReadBackEnd( *this, strm );
}


void Seis::Blocks::Reader::initFromFileName( const char* inp )
{
    const BufferString datafnm( findDataFileName(inp) );
    if ( !File::exists(datafnm) )
    {
	if ( datafnm.isEmpty() )
	    state_.set( tr("No input specified") );
	else
	    state_.set( uiStrings::phrFileDoesNotExist(datafnm) );
	return;
    }

    basepath_.set( datafnm );
    basepath_.setExtension( 0 );

    od_istream strm( infoFileName() );
    if ( !strm.isOK() )
    {
	state_.set( uiStrings::phrCannotOpenForRead( strm.fileName() ) );
	strm.addErrMsgTo( state_ );
	return;
    }

    readInfoFile( strm );
}


Seis::Blocks::Reader::~Reader()
{
    closeBackEnd();
    delete seldata_;
    delete &curcdpos_;
    delete &fileidtbl_;
}


BufferString Seis::Blocks::Reader::findDataFileName( const char* inp ) const
{
    File::Path fp( inp );
    fp.setExtension( 0 );
    const BufferString basepth( fp.fullPath() );
    const BufferString expectedfnm( dataFileNameFor(basepth,usehdf_) );
    if ( File::exists(expectedfnm) )
	return expectedfnm;

    BufferString altfnm( dataFileNameFor(basepth,!usehdf_) );
    if ( File::exists(altfnm) )
    {
	usehdf_ = !usehdf_;
	return altfnm;
    }

    return expectedfnm;
}


void Seis::Blocks::Reader::close()
{
    closeBackEnd();
    fileidtbl_.clear();
    needreset_ = true;
}


void Seis::Blocks::Reader::closeBackEnd()
{
    delete backend_;
    backend_ = 0;
}


void Seis::Blocks::Reader::readInfoFile( od_istream& strm )
{
    ascistream astrm( strm );
    if ( !astrm.isOfFileType(sKeyFileType()) )
    {
	state_.set( tr("%1\nhas wrong file type").arg(strm.fileName()) );
	return;
    }

    bool havegensection = false, havepossection = false, haveoffsection = false;
    while ( !havepossection )
    {
	BufferString sectnm;
	if ( !strm.getLine(sectnm) )
	    break;

	if ( !sectnm.startsWith(sKeySectionPre()) )
	{
	    state_.set( tr("%1\n'%2' keyword not found")
			.arg(strm.fileName()).arg(sKeySectionPre()) );
	    return;
	}

	bool failed = false;
	if ( sectnm == sKeyPosSection() )
	{
	    failed = !cubedata_.read( strm, true );
	    havepossection = true;
	}
	else if ( sectnm == sKeyGenSection() )
	{
	    IOPar iop;
	    iop.getFrom( astrm );
	    failed = !getGeneralSectionData( iop );
	    havegensection = true;
	}
	else if ( sectnm == sKeyOffSection() || sectnm == sKeyFileIDSection() )
	{
	    IOPar iop;
	    iop.getFrom( astrm );
	    failed = !getOffsetSectionData( iop );
	    haveoffsection = true;
	}
	else
	{
	    IOPar* iop = new IOPar;
	    iop->getFrom( astrm );
	    iop->setName( sectnm.str() + FixedString(sKeySectionPre()).size() );
	    auxiops_ += iop;
	}
	if ( failed )
	{
	    state_.set( tr("%1\n'%2' section is invalid").arg(strm.fileName())
			.arg(sectnm) );
	    return;
	}
    }

    if ( !havegensection || !havepossection || !haveoffsection )
	state_.set( tr("%1\nlacks '%2' section").arg(strm.fileName())
	       .arg( !havegensection ? "General"
		  : (!havepossection ? "Positioning"
				     : usehdf_ ? "HDF ID" : "File Offset") ) );
}


bool Seis::Blocks::Reader::getGeneralSectionData( const IOPar& iop )
{
    int ver = version_;
    iop.get( sKeyFmtVersion(), ver );
    version_ = (version_type)ver;
    iop.get( sKeyCubeName(), cubename_ );
    if ( cubename_.isEmpty() )
	cubename_ = basepath_.fileName();
    iop.get( sKeySurveyName(), survname_ );

    hgeom_.getMapInfo( iop );
    hgeom_.setName( cubename_ );
    zdomain_ = ZDomain::Def::get( iop );
    iop.get( sKey::ZRange(), zgeom_ );
    iop.getYN( sKeyDepthInFeet(), depthinfeet_ );

    DataCharacteristics::getUserTypeFromPar( iop, datarep_ );
    interp_ = DataInterp::create( DataCharacteristics(datarep_), true );
    Scaler* scl = Scaler::get( iop );
    mDynamicCast( LinScaler*, scaler_, scl );

    int i1 = dims_.inl(), i2 = dims_.crl(), i3 = dims_.z();
    if ( !iop.get(sKeyDimensions(),i1,i2,i3) )
    {
	state_.set( tr("%1\nlacks block dimension info").arg(infoFileName()) );
	return false;
    }
    dims_.inl() = size_type(i1); dims_.crl() = size_type(i2);
    dims_.z() = size_type(i3);

    FileMultiString fms( iop.find(sKeyComponents()) );
    const int nrcomps = fms.size();
    if ( nrcomps < 1 )
    {
	compnms_.add( "Component 1" );
	compsel_ += true;
    }
    else
    {
	for ( int icomp=0; icomp<nrcomps; icomp++ )
	{
	    compnms_.add( fms[icomp] );
	    compsel_ += true;
	}
    }

    datatype_ = dataTypeOf( iop.find( sKeyDataType() ) );
    return true;
}


bool Seis::Blocks::Reader::getOffsetSectionData( const IOPar& iop )
{
    for ( int idx=0; idx<iop.size(); idx++ )
    {
	BufferString kw( iop.getKey(idx) );
	char* ptr = kw.find( '.' );
	if ( !ptr )
	    continue;

	*ptr++ = '\0';
	const HGlobIdx globidx( (idx_type)toInt(kw), (idx_type)toInt(ptr) );
	const od_stream_Pos pos = toInt64( iop.getValue(idx) );
	fileidtbl_[globidx] = pos;
    }

    return !fileidtbl_.empty();
}


void Seis::Blocks::Reader::setSelData( const SelData* sd )
{
    if ( seldata_ != sd )
    {
	delete seldata_;
	if ( sd )
	    seldata_ = sd->clone();
	else
	    seldata_ = 0;
	needreset_ = true;
    }
}


bool Seis::Blocks::Reader::isSelected( const CubeDataPos& pos ) const
{
    return pos.isValid()
	&& (!seldata_ || seldata_->isOK(cubedata_.binID(pos)));
}


bool Seis::Blocks::Reader::advancePos( CubeDataPos& pos ) const
{
    while ( true )
    {
	if ( !cubedata_.toNext(pos) )
	{
	    pos.toPreStart();
	    return false;
	}
	if ( isSelected(pos) )
	    return true;
    }
}


uiRetVal Seis::Blocks::Reader::skip( int nrpos ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );
    for ( int idx=0; idx<nrpos; idx++ )
    {
	if ( !advancePos(curcdpos_) )
	{
	    uirv.set( tr("Failed skipping %1 positions").arg(idx+1) );
	    return uirv;
	}
    }
    return uirv;
}


bool Seis::Blocks::Reader::reset( uiRetVal& uirv ) const
{
    needreset_ = false;
    lastopwasgetinfo_ = false;

    curcdpos_.toPreStart();
    if ( !advancePos(curcdpos_) )
    {
	uirv.set( tr("No selected positions found") );
	return false;
    }

    const BufferString fnm( findDataFileName(dataFileName()) );
    if ( backend_ )
	backend_->reset( fnm, uirv );
    else
    {
	Reader& self = *const_cast<Reader*>( this );
	if ( usehdf_ )
	    self.backend_ = new HDF5ReadBackEnd( self, fnm, uirv );
	else
	    self.backend_ = new StreamReadBackEnd( self, fnm, uirv );
    }
    if ( !uirv.isOK() )
	return false;

    int& nrcomps = const_cast<int&>( nrcomponentsintrace_ );
    nrcomps = 0;
    for ( int idx=0; idx<compsel_.size(); idx++ )
	if ( compsel_[idx] )
	    nrcomps++;

    mUseType( Pos, ZSubSel );
    mUseType( ZSubSel, z_steprg_type );
    auto& zrg = const_cast<z_steprg_type&>( zrgintrace_ );
    if ( !seldata_ )
	zrg = zgeom_;
    else
    {
	ZSubSel zss( zgeom_ );
	zss.setOutputZRange( seldata_->zRange() );
	zrg = zss.outputZRange();
    }

    return true;
}


bool Seis::Blocks::Reader::goTo( const BinID& bid ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );
    return doGoTo( bid, uirv );
}


uiRetVal Seis::Blocks::Reader::getTrcInfo( SeisTrcInfo& ti ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    if ( needreset_ )
    {
	if ( !reset(uirv) )
	    return uirv;
    }

    if ( lastopwasgetinfo_ )
	advancePos( curcdpos_ );
    lastopwasgetinfo_ = true;

    if ( !curcdpos_.isValid() )
	{ uirv.set( uiStrings::sFinished() ); return uirv; }

    const BinID bid = cubedata_.binID( curcdpos_ );
    fillInfo( bid, ti );

    return uirv;
}


uiRetVal Seis::Blocks::Reader::getTrcData( TraceData& td ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    doGet( nullptr, td, uirv );
    return uirv;
}


uiRetVal Seis::Blocks::Reader::get( const BinID& bid, SeisTrc& trc ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    if ( !doGoTo(bid,uirv) )
	return uirv;

    doGet( &trc.info(), trc.data(), uirv );
    return uirv;
}


uiRetVal Seis::Blocks::Reader::getNext( SeisTrc& trc ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    doGet( &trc.info(), trc.data(), uirv );
    return uirv;
}


bool Seis::Blocks::Reader::doGoTo( const BinID& bid, uiRetVal& uirv ) const
{
    if ( needreset_ )
    {
	if ( !reset(uirv) )
	    return false;
    }
    lastopwasgetinfo_ = false;

    PosInfo::CubeDataPos newcdpos = cubedata_.cubeDataPos( bid );
    if ( !newcdpos.isValid() )
    {
	uirv.set( tr("Position not present: %1/%2")
		.arg( bid.inl() ).arg( bid.crl() ) );
	return false;
    }

    curcdpos_ = newcdpos;
    return true;
}


void Seis::Blocks::Reader::fillInfo( const BinID& bid, SeisTrcInfo& ti ) const
{
    ti.sampling_.start = zrgintrace_.start;
    ti.sampling_.step = zrgintrace_.step;
    ti.setPos( bid );
    ti.coord_ = hgeom_.transform( bid );
}


void Seis::Blocks::Reader::doGet( SeisTrcInfo* ti, TraceData& td,
				  uiRetVal& uirv ) const
{
    lastopwasgetinfo_ = false;
    if ( needreset_ )
    {
	if ( !reset(uirv) )
	    return;
    }

    if ( !curcdpos_.isValid() )
	{ uirv.set( uiStrings::sFinished() ); return; }

    readTrace( ti, td, uirv );
    if ( !uirv.isError() )
	advancePos( curcdpos_ );
}


Seis::Blocks::Column* Seis::Blocks::Reader::getColumn(
		const HGlobIdx& globidx, uiRetVal& uirv ) const
{
    Column* column = findColumn( globidx );
    if ( !column )
    {
	column = backend_->createColumn( globidx, uirv );
	if ( uirv.isError() )
	    return 0;
	addColumn( column );
    }

    return column;
}


void Seis::Blocks::Reader::readTrace( SeisTrcInfo* ti, TraceData& td,
				      uiRetVal& uirv ) const
{
    const BinID bid = cubedata_.binID( curcdpos_ );
    const HGlobIdx globidx( Block::globIdx4Inl(hgeom_,bid.inl(),dims_.inl()),
			    Block::globIdx4Crl(hgeom_,bid.crl(),dims_.crl()) );

    Column* column = getColumn( globidx, uirv );
    if ( column )
    {
	backend_->fillTraceData( *column, bid, td, uirv );
	if ( ti )
	    fillInfo( bid, *ti );
    }
}


float Seis::Blocks::Reader::scaledVal( float val ) const
{
    return scaler_ ? (float)scaler_->scale(val) : val;
}
