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
    , chunksz_(64)
    , createeditable_(false)
{
    if ( szip_encoding_status < 0 )
	szip_encoding_status = H5Zfilter_avail( H5Z_FILTER_SZIP ) ? 1 : 0;
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


bool HDF5::WriterImpl::ensureGroup( const char* grpnm, uiRetVal& uirv )
{
    if ( !selectGroup(grpnm) )
    {
	try {
	    group_ = file_->createGroup( grpnm );
	}
	mCatchAdd2uiRv( tr("Cannot create Group '%1'").arg(grpnm) )
    }

    return uirv.isOK();
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


void HDF5::WriterImpl::crDS( const DataSetKey& dsky, const ArrayNDInfo& info,
			       ODDataType dt, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

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
	dataset_ = group_.createDataSet( dsky.dataSetName(),
					 h5dt, dataspace, proplist );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::reSzDS( const DataSetKey& dsky, const ArrayNDInfo& info,
			       uiRetVal& uirv )
{
    PtrMan<Reader> rdr = createCoupledReader();
    if ( !rdr || !rdr->setScope(dsky) )
	return;

    const nr_dims_type nrdims = rdr->nrDims();
    if ( nrdims != info.nrDims() )
	mPutInternalInUiRv( uirv, "HDF5 write: dim chg requested", return )

    TypeSet<hsize_t> newdims;
    bool havechg = false;
    for ( dim_idx_type idim=0; idim<nrdims; idim++ )
    {
	const size_type curdimsz = rdr->dimSize( idim );
	const size_type newdimsz = info.getSize( idim );
	if ( curdimsz != newdimsz )
	    havechg = true;
	newdims += newdimsz;
    }

    if ( !havechg || !stScope(dsky) )
	return;

    try {
	dataset_.extend( newdims.arr() );
    }
    mCatchErrDuringWrite();
}


bool HDF5::WriterImpl::rmObj( const DataSetKey& dsky )
{
    const int res = H5Ldelete( file_->getId(), dsky.fullDataSetName(),
			       H5P_DEFAULT );
    return res >= 0;
}


void HDF5::WriterImpl::ptInfo( const IOPar& iop, uiRetVal& uirv,
			       const DataSetKey* ptrdsky )
{
    if ( iop.isEmpty() )
	return;

    DataSetKey dsky;
    if ( ptrdsky )
	dsky = *ptrdsky;
    else
    {
	if ( haveScope() )
	{
	    putAttribs( dataset_, iop, uirv );
	    return;
	}
	// No datasetkey, nothing open yet: work with file level
    }

    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

    H5::DataSet* useds = &dataset_;
    H5::DataSet ds;
    if ( !atDataSet(dsky.dataSetName()) )
    {
	const bool dsempty = dsky.dataSetEmpty();
	bool notpresent = dsempty;
	if ( !dsempty )
	{
	    try
	    {
		ds = group_.openDataSet( dsky.dataSetName() );
	    }
	    mCatchAnyNoMsg( notpresent = true )
	}

	if ( notpresent )
	{
	    if ( !dsempty )
	    {
		uirv.add( mINTERNAL("Use createDataSet first, then putInfo") );
		return;
	    }
	    hsize_t dims[1]; dims[0] = 1;
	    const char data = '\0';
	    const H5DataType h5dt = H5::PredType::C_S1;
	    try
	    {
		ds = group_.createDataSet( DataSetKey::sGroupInfoDataSetName(),
					    h5dt, H5::DataSpace(1,dims) );
		ds.write( &data, h5dt );
	    }
	    mCatchErrDuringWrite()
	}
	useds = &ds;
    }

    putAttribs( *useds, iop, uirv );
}


static void getAttribWriteStr( const char* inpstr, int nrchars, char* buf )
{
    if ( !inpstr )
	inpstr = "";

    int termpos = FixedString(inpstr).size();
    if ( termpos > nrchars-2 )
	termpos = nrchars - 1;
    for ( int ichar=0; ichar<nrchars; ichar++ )
    {
	if ( ichar > termpos )
	    *buf = ' ';
	else
	{
	    *buf = *inpstr;
	    inpstr++;
	}
	buf++;
    }
}


static void removeAttribsNotInIOPar( H5::DataSet& h5ds, const IOPar& iop )
{
    const int nrattrs = h5ds.getNumAttrs();
    BufferStringSet torem;
    for ( int idx=0; idx<nrattrs; idx++ )
    {
	try {
	    const H5::Attribute attr = h5ds.openAttribute( (unsigned int)idx );
	    const std::string attrky = attr.getName();
	    if ( !iop.isPresent(attrky.c_str()) )
		torem.add( attrky.c_str() );
	}
	catch( ... ) { continue; }
    }

    for ( int idx=0; idx<torem.size(); idx++ )
	H5Adelete( h5ds.getId(), torem.get(idx).str() );
}


static bool findAttrib( H5::DataSet& h5ds, const char* ky, H5::Attribute& attr )
{
    const int nrattrs = h5ds.getNumAttrs();
    for ( int idx=0; idx<nrattrs; idx++ )
    {
	try {
	    attr = h5ds.openAttribute( (unsigned int)idx );
	    const std::string attrky = attr.getName();
	    if ( attrky == ky )
		return true;
	}
	catch( ... ) { continue; }
    }
    return false;
}


void HDF5::WriterImpl::putAttribs( H5::DataSet& h5ds, const IOPar& iop,
				   uiRetVal& uirv )
{
    removeAttribsNotInIOPar( h5ds, iop );

    const H5DataType h5dt = H5::PredType::C_S1;
    const int nrchars = iop.maxContentSize( false ) + 1;
    hsize_t dims[1]; dims[0] = nrchars;
    mDeclareAndTryAlloc( char*, writestr, char [ nrchars ] );

    try
    {
	H5::DataSpace dataspace( 1, dims );
	for ( int idx=0; idx<iop.size(); idx++ )
	{
	    const BufferString ky( iop.getKey(idx) );
	    H5::Attribute attr;
	    if ( !findAttrib(h5ds,ky,attr) )
		attr = h5ds.createAttribute( ky, h5dt, dataspace );
	    getAttribWriteStr( iop.getValue(idx), nrchars, writestr );
	    attr.write( h5dt, writestr );
	}
    }
    mCatchErrDuringWrite()
}



void HDF5::WriterImpl::ptStrings( const DataSetKey& dsky,
				  const BufferStringSet& bss, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

    bool existing = false;
    if ( stScope(dsky) )
    {
	Array1DInfoImpl inf( bss.size() );
	reSzDS( dsky, inf, uirv );
	if ( !uirv.isOK() )
	    return;
	existing = true;
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
	if ( !existing )
	{
	    H5::DataSpace dspace( 1, dims, mDSResizing );
	    H5::DSetCreatPropList proplist;
	    if ( createeditable_ )
	    {
		hsize_t chunkdims[1] = { getChunkSz4TwoChunks(nrstrs) };
		proplist.setChunk( 1, chunkdims );
	    }
	    dataset_ = group_.createDataSet( dsky.dataSetName(), strtyp,
					     dspace, proplist );
	}
	dataset_.write( strs, strtyp );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptAll( const void* data, uiRetVal& uirv )
{
    if ( !haveScope() )
	mRetNeedScopeInUiRv()

    try
    {
	dataset_.getSpace().selectAll();
	dataset_.write( data, dataset_.getDataType() );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptSlab( const SlabSpec& spec,
			       const void* data, uiRetVal& uirv )
{
    if ( !haveScope() )
	mRetNeedScopeInUiRv()

    TypeSet<hsize_t> counts;
    try
    {
	H5::DataSpace filedataspace = dataset_.getSpace();
	selectSlab( filedataspace, spec, &counts );
	H5::DataSpace memdataspace( (nr_dims_type)spec.size(), counts.arr(),
				    mDSResizing );
	dataset_.write( data, dataset_.getDataType(), memdataspace,
			filedataspace );
    }
    mCatchErrDuringWrite()
}
