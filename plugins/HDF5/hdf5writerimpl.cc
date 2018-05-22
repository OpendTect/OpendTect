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

unsigned szip_options_mask = H5_SZIP_EC_OPTION_MASK; // entropy coding
		     // nearest neighbour coding: H5_SZIP_NN_OPTION_MASK
unsigned szip_pixels_per_block = 16;
		    // can be an even number [2,32]
static int szip_encoding_status = -1;


#define mCatchErrDuringWrite() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringWrite(fileName()) )


HDF5::WriterImpl::WriterImpl()
    : AccessImpl(*this)
    , chunksz_(64)
{
    if ( szip_encoding_status < 0 )
    {
	unsigned int filter_config_flags;
	H5Zget_filter_info( H5Z_FILTER_SZIP, &filter_config_flags );
	if ( (filter_config_flags & H5Z_FILTER_CONFIG_ENCODE_ENABLED) == 0 )
	    szip_encoding_status = 0;
	else
	    szip_encoding_status = 1;
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


void HDF5::WriterImpl::openFile( const char* fnm, uiRetVal& uirv )
{
    try
    {
	H5::H5File* newfile = new H5::H5File( fnm, H5F_ACC_TRUNC );
	closeFile();
	myfile_ = true;
	file_ = newfile;
    }
    mCatchAdd2uiRv( uiStrings::phrCannotOpen(fnm) )
    // mCatchAdd2uiRv( uiStrings::phrCannotOpenForWrite(fnm) )
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


void HDF5::WriterImpl::crDS( const DataSetKey& dsky, const ArrayNDInfo& info,
			       ODDataType dt, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

    nrdims_ = info.nrDims();
    TypeSet<hsize_t> dims, chunkdims;
    int maxdim = 0;
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	const auto dimsz = info.getSize( idim );
	if ( dimsz > maxdim )
	    maxdim = dimsz;
	dims += dimsz;
	chunkdims += chunksz_ < dimsz ? chunksz_ : dimsz;
    }

    const bool canchunk = maxdim > chunksz_;
    const bool canzip = szip_encoding_status>0
		     && maxdim >= szip_pixels_per_block;

    const H5DataType h5dt = h5DataTypeFor( dt );
    try
    {
	H5::DataSpace dataspace( nrdims_, dims.arr() );
	H5::DSetCreatPropList proplist;
	if ( canchunk )
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
	    putAttrib( dataset_, iop, uirv );
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
		uirv.add( uiStrings::phrInternalErr(
			    "Use createDataSet first, then putInfo") );
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

    putAttrib( *useds, iop, uirv );
}


static void getWriteStr( const char* inpstr, int nrchars, char* buf )
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


void HDF5::WriterImpl::putAttrib( H5::DataSet& h5ds, const IOPar& iop,
				  uiRetVal& uirv )
{
    const H5DataType h5dt = H5::PredType::C_S1;
    const int nrchars = iop.maxContentSize( false ) + 1;
    hsize_t dims[1]; dims[0] = nrchars;
    mDeclareAndTryAlloc( char*, writestr, char [ nrchars ] );

    try
    {
	H5::DataSpace dataspace( 1, dims );
	for ( int idx=0; idx<iop.size(); idx++ )
	{
	    H5::Attribute attribute = h5ds.createAttribute(
				iop.getKey(idx), h5dt, dataspace );
	    getWriteStr( iop.getValue(idx), nrchars, writestr );
	    attribute.write( h5dt, writestr );
	}
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptStrings( const DataSetKey& dsky,
				  const BufferStringSet& bss, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

    const int nrstrs = bss.size();
    char** strs = new char* [ nrstrs ];
    ArrPtrMan<char*> deleter = strs;

    for ( int istr=0; istr<nrstrs; istr++ )
	strs[istr] = (char*)bss.get( istr ).buf();
    hsize_t dims[1] = { (hsize_t)nrstrs };

    try
    {
	const H5::StrType strtyp( 0, H5T_VARIABLE );
	dataset_ = group_.createDataSet( dsky.dataSetName(),
					 strtyp, H5::DataSpace(1,dims) );
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
	H5::DataSpace dataspace = dataset_.getSpace();
	dataspace.selectAll();
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
	H5::DataSpace memdataspace( (NrDimsType)spec.size(), counts.arr() );
	dataset_.write( data, dataset_.getDataType(), memdataspace,
			filedataspace );
    }
    mCatchErrDuringWrite()
}
