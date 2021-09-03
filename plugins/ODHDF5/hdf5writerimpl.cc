/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5writerimpl.h"

#include "datachar.h"
#include "envvars.h"
#include "hdf5readerimpl.h"
#include "uistrings.h"
#include "arrayndimpl.h"
#include "iopar.h"

static unsigned gzip_pixels_per_block = 16;
		    // can be an even number [2,32]
static int gzip_encoding_status = -1;

#define mCatchErrDuringWrite() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringWrite(fileName()) )


HDF5::WriterImpl::WriterImpl()
    : AccessImpl(*this)
{
    if ( gzip_encoding_status < 0 )
    {
	gzip_encoding_status = H5Zfilter_avail( H5Z_FILTER_DEFLATE ) ? 1 : 0;
	if ( gzip_encoding_status == 1 )
	{
	    unsigned int filter_info;
	    H5Zget_filter_info( H5Z_FILTER_DEFLATE, &filter_info );
	    if ( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ||
		 !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) )
		gzip_encoding_status = 0;
	}
    }
}


HDF5::WriterImpl::~WriterImpl()
{
    closeFile();
}


HDF5::Reader* HDF5::WriterImpl::createCoupledReader() const
{
    return file_ ? new HDF5::ReaderImpl( *const_cast<H5::H5File*>(file_) ) : 0;
}


void HDF5::WriterImpl::openFile( const char* fnm, uiRetVal& uirv, bool edit )
{
    try
    {
	H5::H5File* newfile = new H5::H5File( fnm, edit ? H5F_ACC_RDWR
							: H5F_ACC_TRUNC );
	closeFile();
	myfile_ = true;
	file_ = newfile;
    }
    mCatchAdd2uiRv( uiStrings::phrCannotOpenForWrite(fnm) )
}


H5::Group* HDF5::WriterImpl::ensureGroup( const char* grpnm, uiRetVal& uirv )
{
    if ( !selectGroup(grpnm) )
    {
	try {
	    group_ = file_->createGroup( grpnm );
	}
	mCatchAdd2uiRv( tr("Cannot create Group '%1'").arg(grpnm) )
    }

    return &group_;
}


H5::DataSet* HDF5::WriterImpl::crDS( const DataSetKey& dsky,
				     const ArrayNDInfo& info, ODDataType dt,
				     uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return nullptr;

    nrdims_ = info.nrDims();
    TypeSet<hsize_t> dims, chunkdims;
    hsize_t maxdim = 0;
    hsize_t maxchunkdim = 0;
    const bool editable = dsky.isEditable();
    bool mustchunk = editable;
    for ( ArrayNDInfo::dim_idx_type idim=0; idim<nrdims_; idim++ )
    {
	const auto dimsz = info.getSize( idim );
	if ( dimsz > maxdim )
	    { maxdim = dimsz; }
	dims += dimsz;
    }

    TypeSet<hsize_t> maxszs; bool hasmaxsz = false;
    od_uint64 totsz = 1;
    for ( ArrayNDInfo::dim_idx_type idim=0; idim<nrdims_; idim++ )
    {
	const hsize_t dimsz = dims[idim];
	const int chunksz = dsky.chunkSz( idim );
	const int maxchunkzs = dsky.maxDimSz( idim );
	hsize_t chunkdim = dimsz < chunksz ? dimsz : chunksz;
	const bool hasmaxresize = !mIsUdf(maxchunkzs);
	if ( mustchunk && hasmaxresize )
	{
	    chunkdim = maxchunkzs;
	    hasmaxsz = true;
	}
	if ( maxchunkdim < chunkdim )
	    maxchunkdim = chunkdim;
	chunkdims += chunkdim;
	maxszs += hasmaxresize ? H5S_UNLIMITED : dimsz;
	totsz *= chunkdim;
    }

    const DataCharacteristics dc( dt );
    totsz *= dc.nrBytes();
    static od_uint64 maxhdf5chunksz = 4294967296ULL;
    if ( totsz >= maxhdf5chunksz && !chunkdims.isEmpty() )
	// More than 4GB, needs further chunking
    {
	// Will try to split the largest dimension
	int maxdimidx = 0;
	for ( int idim=1; idim<nrdims_; idim++ )
	{
	    if ( chunkdims[idim] > chunkdims[maxdimidx] )
		maxdimidx = idim;
	}

	const double chunkratio = double(totsz) / (maxhdf5chunksz-1);
	const double new1dsz = double(chunkdims[maxdimidx]) / chunkratio;
	if ( new1dsz > 1. )
	{
	    maxchunkdim = Math::Floor( new1dsz );
	    if ( maxchunkdim > 0 )
	    {
		maxszs[maxdimidx] = H5S_UNLIMITED;
		chunkdims[maxdimidx] = maxchunkdim;
		hasmaxsz = true;
		mustchunk = true;
		const_cast<DataSetKey&>( dsky ).setMaximumSize( maxdimidx,
								maxchunkdim );
	    }
	}
    }

    if ( mustchunk && !hasmaxsz )
    {
	/*No specific axis provided. Net to set
	    them all as unlimited */
	for ( auto& maxsz : maxszs )
	    maxsz = H5S_UNLIMITED;
    }

    mDefineStaticLocalObject(bool, allowzip,
		= GetEnvVarYN("OD_HDF5_ALLOWZIP",false) );
    mDefineStaticLocalObject(bool, allowshuffle,
		= GetEnvVarYN("OD_HDF5_ALLOWSHUFFLE",true) );

    const bool wantchunk = maxdim > maxchunkdim;
    const bool canzip = allowzip && (mustchunk || wantchunk) &&
		     gzip_encoding_status>0
		     && maxdim >= gzip_pixels_per_block;

    const H5DataType h5dt = h5DataTypeFor( dt );
    try
    {
	H5::DataSpace dataspace( nrdims_, dims.arr(), maxszs.arr() );
	H5::DSetCreatPropList proplist;
	if ( mustchunk || wantchunk )
	    proplist.setChunk( nrdims_, chunkdims.arr() );
	if ( canzip )
	{
	    proplist.setDeflate( compressionlvl_ );
	    if ( allowshuffle && compressionlvl_ > 0 )
		proplist.setShuffle();
	}
	dataset_ = group_.createDataSet( dsky.dataSetName(), h5dt,
					 dataspace, proplist );
    }
    mCatchErrDuringWrite()
    return &dataset_;
}


H5::DataSet* HDF5::WriterImpl::crTxtDS( const DataSetKey& dsky, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return nullptr;

    hsize_t dims[1]; dims[0] = 1;
    const char data = '\0';
    const H5DataType h5dt = H5::PredType::C_S1;
    try
    {
	dataset_ = group_.createDataSet( dsky.dataSetName(),
					 h5dt, H5::DataSpace(1,dims) );
	dataset_.write( &data, h5dt );
    }
    mCatchErrDuringWrite()
    return &dataset_;
}


void HDF5::WriterImpl::reSzDS( const ArrayNDInfo& info, H5::DataSet& h5ds,
			       uiRetVal& uirv )
{
    PtrMan<Reader> rdr = createCoupledReader();
    if ( !rdr )
	return;

    const DataSetKey inpscope = scope();
    const nr_dims_type nrdims = rdr->getNrDims( inpscope, uirv );
    if ( nrdims != info.nrDims() )
	mPutInternalInUiRv( uirv, "HDF5 write: dim chg requested", return )

    TypeSet<hsize_t> newdims;
    bool havechg = false;
    for ( dim_idx_type idim=0; idim<nrdims; idim++ )
    {
	const size_type curdimsz = rdr->dimSize( inpscope, idim, uirv );
	const size_type newdimsz = info.getSize( idim );
	if ( curdimsz != newdimsz )
	    havechg = true;
	newdims += newdimsz;
    }

    rdr = nullptr;
    if ( !havechg )
	return;

    try {
	h5ds.extend( newdims.arr() );
    }
    mCatchErrDuringWrite();
}


void HDF5::WriterImpl::ptSlab( const SlabSpec& spec, const void* data,
			       H5::DataSet& h5ds, uiRetVal& uirv )
{
    TypeSet<hsize_t> counts, maxdims;
    try
    {
	H5::DataSpace filedataspace = h5ds.getSpace();
	selectSlab( filedataspace, spec, &counts );
	maxdims.setSize( counts.size(), H5S_UNLIMITED );
	filedataspace.getSimpleExtentDims( nullptr, maxdims.arr() );

	H5::DataSpace memdataspace( (nr_dims_type)spec.size(),
				    counts.arr(), maxdims.arr() );
	h5ds.write( data, h5ds.getDataType(), memdataspace, filedataspace );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptAll( const void* data, H5::DataSet& h5ds,
			      uiRetVal& uirv )
{
    try
    {
	h5ds.getSpace().selectAll();
	h5ds.write( data, h5ds.getDataType() );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptStrings( const BufferStringSet& bss,
				  H5::Group& grpobj, H5::DataSet* h5obj,
				  const char* dsnm, uiRetVal& uirv )
{
    const bool existing = h5obj;
    if ( existing )
    {
	const Array1DInfoImpl inf( bss.size() );
	reSzDS( inf, *h5obj, uirv );
	if ( !uirv.isOK() )
	    return;
    }

    const hsize_t nrstrs = bss.size();
    char** strs = new char* [ nrstrs ];
    ArrPtrMan<char*> deleter = strs;

    for ( int istr=0; istr<nrstrs; istr++ )
	strs[istr] = (char*)bss.get( istr ).buf();
    hsize_t dims[1] = { nrstrs };
    hsize_t maxdims[1] = { H5S_UNLIMITED };

    try
    {
	const H5::StrType strtyp( 0, H5T_VARIABLE );
	H5::DataSet newds;
	if ( !existing )
	{
	    H5::DataSpace dspace( 1, dims, maxdims );
	    H5::DSetCreatPropList proplist;
	    hsize_t chunkdims[1] = { 1 };
	    proplist.setChunk( 1, chunkdims );
	    newds = grpobj.createDataSet( dsnm, strtyp, dspace, proplist );
	    h5obj = &newds;
	}
	h5obj->write( strs, strtyp );
    }
    mCatchErrDuringWrite()
}


bool HDF5::WriterImpl::rmObj( const DataSetKey& dsky )
{
    const int res = H5Ldelete( file_->getId(), dsky.fullDataSetName(),
			       H5P_DEFAULT );
    return res >= 0;
}


void HDF5::WriterImpl::rmAttrib( const char* nm, H5::H5Object& h5obj )
{
    if ( !h5obj.attrExists(nm) )
	return;

    try {
	H5Adelete( h5obj.getId(), nm );
    }

    catch( ... ) {}
}


void HDF5::WriterImpl::rmAllAttribs( H5::H5Object& h5obj )
{
    const int nrattrs = h5obj.getNumAttrs();
    for ( int idx=nrattrs-1; idx>=0; idx-- )
    {
	try {
	    const H5::Attribute attr = h5obj.openAttribute( (unsigned int)idx );
	    const H5std_string attrnm = attr.getName();
	    H5Adelete( h5obj.getId(), attrnm.c_str() );
	}

	catch( ... ) { continue; }
    }
}


static bool findAttrib( H5::H5Object& h5obj, const char* ky,H5::Attribute& attr)
{
    if ( !h5obj.attrExists(ky) )
	return false;
    attr = h5obj.openAttribute( ky );
    return true;
}


void HDF5::WriterImpl::setAttribute( const char* ky, const char* val,
				     const DataSetKey* dsky )
{
    H5::H5Object* h5scope = setScope( dsky );
    if ( !h5scope )
	return;
    setAttribute( ky, val, *h5scope );
}


void HDF5::WriterImpl::setAttribute( const char* ky, const char* val,
				     H5::H5Object& h5scope )
{
    uiRetVal uirv;
    const H5::DataType reqstrtype = H5::StrType( H5::PredType::C_S1,
						 H5T_VARIABLE );
    const H5::DataSpace reqspace( H5S_SCALAR );
    try
    {
	H5::Attribute attr;
	const bool hasattrib = findAttrib( h5scope, ky, attr );
	bool neednew = !hasattrib;
	if ( hasattrib )
	{
	    if ( attr.getDataType() != reqstrtype ||
		 attr.getSpace().getId() != reqspace.getId() )
	    {
		h5scope.removeAttr( ky );
		neednew = true;
	    }
	}

	if ( neednew )
	    attr = h5scope.createAttribute( ky, reqstrtype, reqspace );
	H5std_string writestr;
	if ( val && *val )
	    writestr = H5std_string( val );
	attr.write( reqstrtype, writestr );
    }
    mCatchErrDuringWrite()
}


#define mSetNumAttr( attrnm, type, val ) \
void HDF5::WriterImpl::setAttribute( const char* attrnm, type val, \
				     const DataSetKey* dsky ) \
{ \
    const BufferString resstr( res ); \
    setAttribute( attrnm, resstr, dsky ); \
}
mSetNumAttr(attrnm,od_int16,res)
mSetNumAttr(attrnm,od_uint16,res)
mSetNumAttr(attrnm,od_int32,res)
mSetNumAttr(attrnm,od_uint32,res)
mSetNumAttr(attrnm,od_int64,res)
mSetNumAttr(attrnm,od_uint64,res)
mSetNumAttr(attrnm,float,res)
mSetNumAttr(attrnm,double,res)
#undef mSetNumAttr


void HDF5::WriterImpl::ptInfo( const IOPar& iop, H5::H5Object& h5obj,
			       uiRetVal& uirv )
{
    try
    {
	for ( int idx=0; idx<iop.size(); idx++ )
	{
	    const FixedString ky( iop.getKey(idx) );
	    const FixedString val( iop.getValue(idx) );
	    setAttribute( ky.str(), val.str(), h5obj );
	}
    }
    mCatchErrDuringWrite()
}
