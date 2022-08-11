/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : March 2018
-*/

#include "arrayndimpl.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hdf5arraynd.h"
#include "iopar.h"
#include "od_istream.h"
#include "settings.h"
#include "uistrings.h"

mImplFactory( HDF5::AccessProvider, HDF5::AccessProvider::factory );

#define mRetNoFileInUiRv() mRetInternalInUiRv( uirv, sOpenFileFirst() )
#define mRetNoDataInUiRv() mRetInternalInUiRv( uirv, sNoDataPassed() )
#define mRetDataSpaceBad() { uirv.add( sBadDataSpace() ); return uirv; }

const char* HDF5::Access::sOpenFileFirst()
{ return "HDF5: Access not open. Use/check open()"; }
const char* HDF5::Access::sNeedScope()
{ return "HDF5: No valid scope set for data retrieval. Use/check setScope()"; }
const char* HDF5::Access::sNoDataPassed()
{ return "HDF5: Null data passed"; }


HDF5::DataSetKey::DataSetKey( const char* grpnm, const char* dsnm )
    : dsnm_(dsnm)
{
    setGroupName( grpnm );
}


BufferString HDF5::DataSetKey::fullDataSetName() const
{
    FilePath fp;
    if ( !grpnm_.isEmpty() )
	fp.add( grpnm_ );
    if ( !dsnm_.isEmpty() )
	fp.add( dsnm_ );

    return fp.fullPath( FilePath::Unix );
}


bool HDF5::DataSetKey::hasGroup( const char* reqnm ) const
{
    const bool rootgrpreq = !reqnm || !*reqnm || StringView(reqnm) == "/";
    const bool haverootgrp = grpnm_.isEmpty() || grpnm_ == "/";
    if ( rootgrpreq || haverootgrp )
	return rootgrpreq == haverootgrp;

    const char* grpnm = grpnm_.str();
    if ( *reqnm == '/' )
	reqnm++;
    if ( *grpnm == '/' )
	grpnm++;

    return StringView(grpnm) == StringView(reqnm);
}


HDF5::DataSetKey HDF5::DataSetKey::groupKey( const HDF5::DataSetKey& parentgrp,
					     const char* subgrpnm )
{
    return groupKey( parentgrp.fullDataSetName(), subgrpnm );
}


HDF5::DataSetKey HDF5::DataSetKey::groupKey( const char* fullparentnm,
					     const char* grpnm )
{
    const FilePath grpfp( fullparentnm, grpnm );
    BufferString grpkynm = grpfp.fullPath( FilePath::Unix );
    grpkynm.insertAt( 0, "/" );
    return DataSetKey( grpkynm );
}


void HDF5::DataSetKey::setChunkSize( int idim, int sz )
{
    setChunkSize( &sz, 1, idim );
}


void HDF5::DataSetKey::setChunkSize( const int* szs, int nrdims, int from )
{
    if ( !szs )
    {
	chunkszs_.setEmpty();
	return;
    }

    const int to = from + nrdims - 1;
    for ( int idx=chunkszs_.size(); idx<=to; idx++ )
	chunkszs_ += mUdf(int);
    for ( int i=from, idx=0; i<=to; i++, idx++ )
	chunkszs_[i] = szs[idx] > 0 ? szs[idx] : mUdf(int);
}


void HDF5::DataSetKey::setMaximumSize( int idim, int maxsz )
{
    if ( idim < 0 )
    {
	maxsizedim_.setEmpty();
	return;
    }
    else if ( maxsz < 1 )
	return;

    editable_ = true;
    for ( int idx=maxsizedim_.size(); idx<=idim; idx++ )
	maxsizedim_ += mUdf(int);
    maxsizedim_[idim] = maxsz;
}


int HDF5::DataSetKey::chunkSz( int idim ) const
{
    return chunkszs_.validIdx(idim) ? chunkszs_[idim] : mUdf(int);
}


int HDF5::DataSetKey::maxDimSz(int idim ) const
{
    return maxsizedim_.validIdx(idim) ? maxsizedim_[idim] : mUdf(int);
}



bool HDF5::Access::isEnvBlocked( const char* typ )
{
    if ( GetEnvVarYN("OD_NO_HDF5") )
	return true;
    if ( !typ || !*typ )
	return false;

    const BufferString envvar( "OD_NO_HDF5_", BufferString(typ).toUpper() );
    return GetEnvVarYN( envvar );
}


bool HDF5::Access::isEnabled( const char* typ )
{
    if ( !HDF5::isAvailable() || isEnvBlocked(typ)
      || !Settings::common().isTrue(sSettingsEnabKey()) )
	return false;

    if ( StringView(typ).isEmpty() )
	return true;

    const BufferString settky( sSettingsEnabKey(), ".", typ );
    return !Settings::common().isFalse( settky );
}


bool HDF5::Access::isHDF5File( const char* fnm )
{
    if ( !File::exists(fnm) )
	return false;

    od_istream strm( fnm );
    od_int64 magicnumb = 0;
    strm.getBin( magicnumb );
    if ( __islittle__ )
	SwapBytes( &magicnumb, sizeof(magicnumb) );
    return magicnumb == 0x894844460d0a1a0a;
}


HDF5::AccessProvider* HDF5::AccessProvider::mkProv( int idx )
{
    if ( idx<0 || idx>=factory().size() )
	idx = factory().size()-1;
    if ( idx<0 )
	return 0;

    return factory().create( factory().getNames().get(idx) );
}


HDF5::Reader* HDF5::AccessProvider::mkReader( int idx )
{
    AccessProvider* prov = mkProv( idx );
    Reader* rdr = 0;
    if ( prov )
    {
	rdr = prov->getReader();
	delete prov;
    }
    return rdr;
}


HDF5::Writer* HDF5::AccessProvider::mkWriter( int idx )
{
    AccessProvider* prov = mkProv( idx );
    Writer* wrr = 0;
    if ( prov )
    {
	wrr = prov->getWriter();
	delete prov;
    }
    return wrr;
}


HDF5::Access::Access()
    : file_(0)
    , myfile_(true)
{
}


HDF5::Access::~Access()
{
    // cannot do closeFile(), the reader or writer has already been destructed
}


uiRetVal HDF5::Access::open( const char* fnm )
{
    closeFile();

    uiRetVal uirv;
    openFile( fnm, uirv, false );
    myfile_ = true;

    return uirv;
}


bool HDF5::Access::hasGroup( const char* grpnm ) const
{
    const DataSetKey dsky( grpnm );
    return getGrpScope( &dsky );
}


bool HDF5::Access::hasDataSet( const DataSetKey& dsky ) const
{
    return getDSScope( dsky );
}


uiString HDF5::Access::sHDF5PackageDispName()
{
    return tr("HDF5 File Access");
}


uiString HDF5::Access::sHDF5NotAvailable()
{
    return tr("No HDF5 installation found");
}


uiString HDF5::Access::sHDF5FileNoLongerAccessibe()
{
    return tr("HDF5 file no longer accesible");
}


uiString HDF5::Access::sHDF5NotAvailable( const char* fnm )
{
    return sHDF5NotAvailable().append( tr("Needed to access '%1'").arg(fnm) );
}


uiString HDF5::Access::sHDF5Err( const uiString& err )
{
    uiString ret( tr("HDF5 Error") );
    if ( !err.isEmpty() )
	ret.addMoreInfo( err );
    return ret;
}


uiString HDF5::Access::sNotHDF5File( const char* fnm )
{
    return tr("'%1' is not an HDF5 file").arg( fnm );
}


uiString HDF5::Access::sDataSetNotFound( const DataSetKey& dsky )
{
    return sHDF5Err( tr("Could not find DataSet '%1'")
			.arg( dsky.fullDataSetName() ) );
}


uiString HDF5::Access::sCantSetScope( const DataSetKey& dsky ) const
{
    if ( isReader() )
	return sDataSetNotFound( dsky );

    return sHDF5Err( tr("Could not create DataSet '%1' in '%2'")
			.arg( dsky.fullDataSetName() )
			.arg(fileName()) );
}


uiString HDF5::Access::sCannotReadDataSet( const DataSetKey& dsky )
{
    return sHDF5Err( tr("Could not read DataSet '%1'")
			.arg( dsky.fullDataSetName() ) );
}


uiString HDF5::Access::sFileNotOpen()
{
    return sHDF5Err( tr("Could not open file") );
}



HDF5::ODDataType HDF5::Reader::getDataType( const DataSetKey& dsky,
				      uiRetVal& uirv ) const
{
    const H5::DataSet* dsscope = getDSScope( dsky );
    if ( !dsscope )
	return OD::F32;

    return gtDataType( *dsscope );
}


ArrayNDInfo* HDF5::Reader::getDataSizes( const DataSetKey& dsky,
					 uiRetVal& uirv ) const
{
    const H5::DataSet* dsscope = getDSScope( dsky );
    if ( !dsscope )
    {
	uirv = sCantSetScope( dsky );
	return nullptr;
    }

    return gtDataSizes( *dsscope );
}


HDF5::Access::nr_dims_type HDF5::Reader::getNrDims( const DataSetKey& dsky,
						    uiRetVal& uirv ) const
{
    const H5::DataSet* dsscope = getDSScope( dsky );
    if ( !dsscope )
    {
	uirv = sCantSetScope( dsky );
	return -1;
    }

    return nrDims();
}


HDF5::Access::size_type HDF5::Reader::dimSize( const DataSetKey& dsky,
					       dim_idx_type idim,
					       uiRetVal& uirv ) const
{
    const ArrayNDInfo* inf = getDataSizes( dsky, uirv );
    const size_type ret = inf ? inf->getSize( idim ) : 0;
    delete inf;
    return ret;
}


#undef mRetNoScopeInUiRv
#define mRetNoScopeInUiRv() \
{ \
    uirv = sCantSetScope( dsky ); \
    return uirv; \
}

uiRetVal HDF5::Reader::getSlab( const DataSetKey& dsky, const SlabSpec& spec,
				void* data ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    const H5::DataSet* dsscope = getDSScope( dsky );
    if ( !dsscope )
	mRetNoScopeInUiRv()
    const auto nrdims = nrDims();
    if ( nrdims < 1 )
	mRetDataSpaceBad()
    if ( spec.size() != nrdims )
    {
	if ( spec.size() < nrdims )
	    mRetInternalInUiRv( uirv, "Specify all dimensions in SlabSpec" );
	pErrMsg( "Probably wrong: SlabSpec too big" );
    }

    gtSlab( *dsscope, spec, data, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::getAll( const DataSetKey& dsky, void* data ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    const H5::DataSet* dsscope = getDSScope( dsky );
    if ( !dsscope )
	mRetNoScopeInUiRv()
    const auto nrdims = nrDims();
    if ( nrdims < 1 )
	mRetDataSpaceBad()

    gtAll( *dsscope, data, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::get( const DataSetKey& dsky, BufferStringSet& bss ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    const H5::DataSet* dsscope = getDSScope( dsky );
    if ( !dsscope )
	mRetNoScopeInUiRv()
    const auto nrdims = nrDims();
    if ( nrdims != 1 )
	mRetDataSpaceBad()

    gtStrings( *dsscope, bss, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::getValue( const DataSetKey& dsky, NDPos pos,
				 void* data ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    if ( !pos )
	mRetInternalInUiRv( uirv, "No position provided" )
    const H5::DataSet* dsscope = getDSScope( dsky );
    if ( !dsscope )
	mRetNoScopeInUiRv()
    const auto nrdims = nrDims();
    if ( nrdims < 1 )
	mRetDataSpaceBad()

    NDPosBufSet pts;
    pts += mNDPosBufFromPos( pos, nrdims );
    gtValues( *dsscope, pts, data, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::getValues( const DataSetKey& dsky,
				  const NDPosBufSet& pts, void* data ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( pts.isEmpty() )
	return uirv;
    if ( !data )
	mRetNoDataInUiRv()
    const H5::DataSet* dsscope = getDSScope( dsky );
    if ( !dsscope )
	mRetNoScopeInUiRv()
    const auto nrdims = nrDims();
    if ( nrdims < 1 )
	mRetDataSpaceBad()

    gtValues( *dsscope, pts, data, uirv );
    return uirv;
}



uiRetVal HDF5::Writer::open4Edit( const char* fnm )
{
    closeFile();

    uiRetVal uirv;
    openFile( fnm, uirv, true );
    myfile_ = true;

    return uirv;
}


uiRetVal HDF5::Writer::createDataSet( const DataSetKey& dsky,
				      int sz, ODDataType dt )
{
    return createDataSet( dsky, Array1DInfoImpl(sz), dt );
}


uiRetVal HDF5::Writer::createDataSet( const DataSetKey& dsky,
				      const ArrayNDInfo& inf,
				      ODDataType dt )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    else if ( inf.getTotalSz() < 1 )
	{ pErrMsg("zero dims"); }

    crDS( dsky, inf, dt, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::createDataSetIfMissing( const DataSetKey& dsky,
					ODDataType dt,
					int addedsz, int changedir,
					int* existsnrsamples )
{
    PtrMan<ArrayNDInfo> existsinfo;
    uiRetVal uirv = createDataSetIfMissing( dsky, dt, Array1DInfoImpl(addedsz),
					    Array1DInfoImpl(changedir),
					    &existsinfo );
    if ( uirv.isOK() && existsnrsamples &&
	 existsinfo.ptr() && existsinfo->nrDims() > 0 )
	*existsnrsamples = existsinfo->getSize(0);

    return uirv;
}


uiRetVal HDF5::Writer::createDataSetIfMissing( const DataSetKey& dsky,
					ODDataType dt,
					const ArrayNDInfo& addedinf,
					const ArrayNDInfo& changedirs,
					PtrMan<ArrayNDInfo>* existsinfo )
{
    uiRetVal uirv;
    if ( !hasDataSet(dsky) )
    {
	uirv = createDataSet( dsky, addedinf, dt );
	if ( existsinfo )
	    existsinfo->set( nullptr, true );
	return uirv;
    }

    PtrMan<Reader> rdr = createCoupledReader();
    if ( !rdr )
    {
	uirv = mINTERNAL("Cannot create HDF5 reader from writer");
	return uirv;
    }

    const OD::DataRepType retdtyp = rdr->getDataType( dsky, uirv );
    if ( retdtyp != dt )
    {
	uirv.add( tr("Cannot open existing HDF5 dataset for write: "
		     "mismatching data type") );
	return uirv;
    }

    //TODO: check if the dataset is resizable

    const int rank = addedinf.nrDims();

    PtrMan<ArrayNDInfo> retinfo = rdr->getDataSizes( dsky, uirv );
    if ( !retinfo )
	return uirv;
    else if ( retinfo->nrDims() != rank )
    {
	uirv.add( tr("Cannot open existing HDF5 dataset for write: "
			    "mismatching array sizes") );
	return uirv;
    }

    PtrMan<ArrayNDInfo> newszs = ArrayNDInfoImpl::create( rank );
    for ( int idim=0; idim<rank; idim++ )
    {
	const int dir = changedirs.getSize(idim);
	const int newsz = retinfo->getSize(idim) +
			  dir * addedinf.getSize(idim);
	newszs->setSize( idim, newsz );
    }

    if ( existsinfo )
	*existsinfo = retinfo.release();

    return resizeDataSet( dsky, *newszs );
}


uiRetVal HDF5::Writer::createTextDataSet( const DataSetKey& dsky )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()

    crTxtDS( dsky, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::resizeDataSet( const DataSetKey& dsky,
				      const ArrayNDInfo& inf )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    else if ( inf.getTotalSz() < 1 )
	{ pErrMsg("zero dims"); }
    H5::DataSet* dsscope = setDSScope( dsky );
    if ( !dsscope )
	mRetNoScopeInUiRv()

    reSzDS( inf, *dsscope, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::putSlab( const DataSetKey& dsky, const SlabSpec& spec,
				const void* data )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    H5::DataSet* dsscope = setDSScope( dsky );
    if ( !dsscope )
	mRetNoScopeInUiRv()

    ptSlab( spec, data, *dsscope, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::putAll( const DataSetKey& dsky, const void* data )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    H5::DataSet* dsscope = setDSScope( dsky );
    if ( !dsscope )
	mRetNoScopeInUiRv()

    ptAll( data, *dsscope, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::put( const DataSetKey& dsky, const BufferStringSet& bss )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    H5::Group* grpscope = ensureGroup( dsky.groupName(), uirv );
    if ( !uirv.isOK() )
	return uirv;
    if ( !grpscope )
	mRetNoScopeInUiRv()

    if ( !bss.isEmpty() )
	ptStrings( bss, *grpscope, setDSScope(dsky), dsky.dataSetName(), uirv);

    return uirv;
}


bool HDF5::Writer::deleteObject( const DataSetKey& dsky )
{
    return file_ ? rmObj( dsky ) : true;
}


#undef mRetNoScopeInUiRv
#define mRetNoScopeInUiRv() \
{ \
    if ( dsky ) \
	uirv = sCantSetScope( *dsky ); \
    else \
	uirv = sCantSetScope( DataSetKey() ); \
    return uirv; \
}

uiRetVal HDF5::Reader::getAttributeNames( BufferStringSet& nms,
			const DataSetKey* dsky ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    const H5::H5Object* h5scope = getScope( dsky );
    if ( !h5scope )
	mRetNoScopeInUiRv()

    nms.setEmpty();
    gtAttribNames( *h5scope, nms );
    return uirv;
}

uiRetVal HDF5::Reader::get( IOPar& iop, const DataSetKey* dsky ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    const H5::H5Object* h5scope = getScope( dsky );
    if ( !h5scope )
	mRetNoScopeInUiRv()

    gtInfo( *h5scope, iop, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::set( const IOPar& iop, const DataSetKey* dsky )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    H5::H5Object* h5scope = setScope( dsky );
    if ( !h5scope )
	mRetNoScopeInUiRv()

    if ( !iop.isEmpty() )
	ptInfo( iop, *h5scope, uirv );

    return uirv;
}


uiRetVal HDF5::Writer::removeAttribute( const char* nm, const DataSetKey* dsky )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    H5::H5Object* h5scope = setScope( dsky );
    if ( !h5scope )
	mRetNoScopeInUiRv()

    if ( nm && *nm )
	rmAttrib( nm, *h5scope );

    return uirv;
}


uiRetVal HDF5::Writer::removeAllAttributes( const DataSetKey* dsky )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    H5::H5Object* h5scope = setScope( dsky );
    if ( !h5scope )
	mRetNoScopeInUiRv()

    rmAllAttribs( *h5scope );

    return uirv;
}
