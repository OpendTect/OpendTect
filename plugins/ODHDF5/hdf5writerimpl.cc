/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5writerimpl.h"
#include "hdf5readerimpl.h"
#include "uistrings.h"
#include "arrayndimpl.h"
#include "iopar.h"

static unsigned szip_options_mask = H5_SZIP_EC_OPTION_MASK; // entropy coding
		     // nearest neighbour coding: H5_SZIP_NN_OPTION_MASK
static unsigned szip_pixels_per_block = 16;
		    // can be an even number [2,32]
static int szip_encoding_status = -1;

#define mUnLim4 H5S_UNLIMITED, H5S_UNLIMITED, H5S_UNLIMITED, H5S_UNLIMITED
static hsize_t resizablemaxdims_[24] =
{ mUnLim4, mUnLim4, mUnLim4, mUnLim4, mUnLim4, mUnLim4 };
#define mDSResizing (createeditable_ ? resizablemaxdims_ : 0)

#define mCatchErrDuringWrite() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringWrite(fileName()) )


HDF5::WriterImpl::WriterImpl()
    : AccessImpl(*this)
{
    if ( szip_encoding_status < 0 )
	szip_encoding_status = 0;
/*	szip_encoding_status = H5Zfilter_avail( H5Z_FILTER_SZIP ) ? 1 : 0;
	TODO: enable if possible
*/
}


HDF5::WriterImpl::~WriterImpl()
{
    closeFile();
}


void HDF5::WriterImpl::setEditableCreation( bool yn )
{
    createeditable_ = yn;
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


void HDF5::WriterImpl::setChunkSize( int sz )
{
    chunksz_ = sz;
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


static hsize_t getChunkSz4TwoChunks( hsize_t dimsz )
{
    hsize_t ret = dimsz / 2;
    if ( dimsz % 2 )
	ret++;
    if ( ret < 1 )
	ret = 1;
    return ret;
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
    const bool mustchunk = createeditable_;
    bool havelargerdimthanchunk = false;
    ArrayNDInfo::dim_idx_type largestdim = 0;
    for ( ArrayNDInfo::dim_idx_type idim=0; idim<nrdims_; idim++ )
    {
	const auto dimsz = info.getSize( idim );
	if ( dimsz > maxdim )
	    { largestdim = idim; maxdim = dimsz; }
	dims += dimsz;
	if ( dimsz > chunksz_ )
	    havelargerdimthanchunk = true;
    }

    for ( ArrayNDInfo::dim_idx_type idim=0; idim<nrdims_; idim++ )
    {
	const hsize_t dimsz = dims[idim];
	hsize_t chunkdim = dimsz < chunksz_ ? dimsz : chunksz_;
	if ( mustchunk && !havelargerdimthanchunk && idim == largestdim )
	    chunkdim = getChunkSz4TwoChunks( chunkdim );
	if ( maxchunkdim < chunkdim )
	    maxchunkdim = chunkdim;
	chunkdims += chunkdim;
    }

    const bool wantchunk = maxdim > maxchunkdim;
    const bool canzip = szip_encoding_status>0
		     && maxdim >= szip_pixels_per_block;

    const H5DataType h5dt = h5DataTypeFor( dt );
    try
    {
	H5::DataSpace dataspace( nrdims_, dims.arr(), mDSResizing );
	H5::DSetCreatPropList proplist;
	if ( mustchunk || wantchunk )
	{
	    proplist.setChunk( nrdims_, chunkdims.arr() );
	    if ( canzip )
		proplist.setSzip( szip_options_mask, szip_pixels_per_block );
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
    TypeSet<hsize_t> counts;
    try
    {
	H5::DataSpace filedataspace = h5ds.getSpace();
	selectSlab( filedataspace, spec, &counts );
	H5::DataSpace memdataspace( (nr_dims_type)spec.size(), counts.arr(),
				    mDSResizing );
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

    try
    {
	const H5::StrType strtyp( 0, H5T_VARIABLE );
	H5::DataSet newds;
	if ( !existing )
	{
	    H5::DataSpace dspace( 1, dims, mDSResizing );
	    H5::DSetCreatPropList proplist;
	    if ( createeditable_ )
	    {
		hsize_t chunkdims[1] = { getChunkSz4TwoChunks(nrstrs) };
		proplist.setChunk( 1, chunkdims );
	    }
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


static void removeAttribsNotInIOPar( const IOPar& iop, H5::H5Object& h5obj )
{
    const int nrattrs = h5obj.getNumAttrs();
    BufferStringSet torem;
    for ( int idx=0; idx<nrattrs; idx++ )
    {
	try {
	    const H5::Attribute attr = h5obj.openAttribute( (unsigned int)idx );
	    const std::string attrky = attr.getName();
	    if ( !iop.isPresent(attrky.c_str()) )
		torem.add( attrky.c_str() );
	}
	catch( ... ) { continue; }
    }

    for ( int idx=0; idx<torem.size(); idx++ )
	H5Adelete( h5obj.getId(), torem.get(idx).str() );
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
    H5::H5Object* scope = setScope( dsky );
    if ( !scope )
	return;
    setAttribute( ky, val, *scope );
}


void HDF5::WriterImpl::setAttribute( const char* ky, const char* val,
				     H5::H5Object& scope )
{
    uiRetVal uirv;
    const H5::DataType reqstrtype = H5::StrType( H5::PredType::C_S1,
						 H5T_VARIABLE );
    const H5::DataSpace reqspace( H5S_SCALAR );
    try
    {
	H5::Attribute attr;
	const bool hasattrib = findAttrib( scope, ky, attr );
	bool neednew = !hasattrib;
	if ( hasattrib )
	{
	    if ( attr.getDataType() != reqstrtype ||
		 attr.getSpace().getId() != reqspace.getId() )
	    {
		scope.removeAttr( ky );
		neednew = true;
	    }
	}

	if ( neednew )
	    attr = scope.createAttribute( ky, reqstrtype, reqspace );
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
    removeAttribsNotInIOPar( iop, h5obj );
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
