/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5readerimpl.h"
#include "uistrings.h"
#include "arrayndinfo.h"
#include "file.h"
#include "iopar.h"
#include "odjson.h"

#define mCatchErrDuringRead() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fileName()) )


HDF5::ReaderImpl::ReaderImpl()
    : AccessImpl(*this)
{
}


HDF5::ReaderImpl::ReaderImpl( const H5::H5File& h5file )
    : AccessImpl(*this)
{
    myfile_ = false;
    file_ = const_cast<H5::H5File*>( &h5file );
}


HDF5::ReaderImpl::~ReaderImpl()
{
    closeFile();
}


void HDF5::ReaderImpl::openFile( const char* fnm, uiRetVal& uirv, bool )
{
    if ( !File::exists(fnm) )
	{ uirv.add( uiStrings::phrCannotOpenForRead( fnm ) ); return; }

    try
    {
	H5::H5File* newfile = new H5::H5File( fnm, H5F_ACC_RDONLY );
	closeFile();
	myfile_ = true;
	file_ = newfile;
    }
    mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fnm) )
}


template <class H5Dir>
void HDF5::ReaderImpl::listObjs( const H5Dir& dir, BufferStringSet& nms,
				 bool wantgroups ) const
{
    const bool islevel0 = nms.isEmpty();
    try
    {
	const int nrobjs = mCast(int,dir.getNumObjs());
	for ( int iobj=0; iobj<nrobjs; iobj++ )
	{
	    const std::string nmstr = dir.getObjnameByIdx( iobj );
	    const BufferString nm( nmstr.c_str() );
	    const H5O_type_t h5objtyp = dir.childObjType( nm.buf() );
	    if ( (wantgroups  && h5objtyp != H5O_TYPE_GROUP)
	      || (!wantgroups && h5objtyp != H5O_TYPE_DATASET) )
		continue;

	    nms.add( wantgroups && islevel0 ? BufferString("/",nm) : nm );

	    if ( wantgroups )
	    {
		H5::Group grp = dir.openGroup( nm );
		BufferStringSet subnms;
		listObjs( grp, subnms, true );
		for ( int idx=0; idx<subnms.size(); idx++ )
		    nms.add( BufferString( nm.str(), "/", subnms.get(idx) ) );
	    }
	}
    }
    mCatchUnexpected( return );
}


void HDF5::ReaderImpl::getGroups( BufferStringSet& nms ) const
{
    nms.setEmpty();
    if ( file_ )
	listObjs( *file_, nms, true );
}


void HDF5::ReaderImpl::getDataSets( const char* grpnm,
				    BufferStringSet& nms ) const
{
    const H5::H5Object* group = selectGroup( grpnm );
    if ( !group )
	return;

    nms.setEmpty();
    listObjs( *group, nms, false );
}


const HDF5::ReaderImpl::H5DataType& HDF5::ReaderImpl::h5DataType(
					const H5::DataSet& h5ds ) const
{
    // makes sure we get data one of our data types
    return h5DataTypeFor( gtDataType(h5ds) );
}


HDF5::ODDataType HDF5::ReaderImpl::gtDataType( const H5::DataSet& h5ds ) const
{
    ODDataType ret = OD::F32;
    if ( !file_ )
	mRetNoFile( return ret )

    try
    {
	const H5::DataType& dt = h5ds.getDataType();
	bool issigned = true, isfp = true;
	if ( dt.getClass() == H5T_INTEGER )
	{
	    isfp = false;
	    issigned = h5ds.getIntType().getSign() != H5T_SGN_NONE;
	}
	ret = OD::GetDataRepType( isfp, issigned, dt.getSize() );
    }
    mCatchUnexpected( return ret );

    return ret;
}


ArrayNDInfo* HDF5::ReaderImpl::gtDataSizes( const H5::DataSet& h5ds ) const
{
    ArrayNDInfo* ret = nullptr;
    if ( !file_ )
	mRetNoFile( return ret )
    else if ( nrdims_ < 1 )
	{ ErrMsg( "HDF5: Empty dataspace found" ); return nullptr; }

    try
    {
	const H5::DataSpace dataspace = h5ds.getSpace();
	mGetDataSpaceDims( dims, nrdims_, dataspace );

	ret = ArrayNDInfoImpl::create( nrdims_ );
	for ( dim_idx_type idim=0; idim<nrdims_; idim++ )
	    ret->setSize( idim, (int)dims[idim] );
    }
    mCatchUnexpected( return ret );

    return ret;
}


void HDF5::ReaderImpl::gtSlab( const H5::DataSet& h5ds, const SlabSpec& spec,
			       void* data, uiRetVal& uirv ) const
{
    TypeSet<hsize_t> counts;
    try
    {
	H5::DataSpace filedataspace = h5ds.getSpace();
	filedataspace.selectAll();
	selectSlab( filedataspace, spec, &counts ); // h5obj ?
	H5::DataSpace memdataspace( nrdims_, counts.arr() );
	const H5DataType& h5dt = h5DataType( h5ds );
	h5ds.read( data, h5dt, memdataspace, filedataspace );
    }
    mCatchErrDuringRead()
}


void HDF5::ReaderImpl::gtAll( const H5::DataSet& h5ds, void* data,
			      uiRetVal& uirv ) const
{
    try
    {
	h5ds.getSpace().selectAll();
	h5ds.read( data, h5DataType(h5ds) );
    }
    mCatchErrDuringRead()
}


void HDF5::ReaderImpl::gtStrings( const H5::DataSet& h5ds,
				  BufferStringSet& bss, uiRetVal& uirv ) const
{
    try
    {
	H5::DataSpace dataspace = h5ds.getSpace();
	mGetDataSpaceDims( dims, nrdims_, dataspace );
	const hsize_t nrstrs = dims[0];
	char** strs = new char* [ nrstrs ];
	ArrPtrMan<char*> deleter = strs;

	dataspace.selectAll();
	h5ds.read( strs, h5ds.getDataType() );

	for ( int istr=0; istr<nrstrs; istr++ )
	    bss.add( strs[istr] );
    }
    mCatchErrDuringRead()
}


void HDF5::ReaderImpl::gtValues( const H5::DataSet& h5ds,
				 const NDPosBufSet& posbufs, void* data,
				 uiRetVal& uirv ) const
{
    try
    {
	H5::DataSpace inputdataspace = h5ds.getSpace();
	const hsize_t nrpts = (hsize_t)posbufs.size();

	mAllocVarLenArr( hsize_t, hdfcoordarr, nrdims_ * nrpts );
	if ( !mIsVarLenArrOK(hdfcoordarr) )
	    { uirv.add( uiStrings::phrCannotAllocateMemory() ); return; }
	for ( NDPosBufSet::idx_type ipt=0; ipt<nrpts; ipt++ )
	{
	    const NDPosBuf& posbuf = posbufs[ipt];
	    const int arroffs = ipt * nrdims_;
	    for ( dim_idx_type idim=0; idim<nrdims_; idim++ )
		hdfcoordarr[arroffs + idim] = posbuf[idim];
	}
	inputdataspace.selectElements( H5S_SELECT_SET, nrpts, hdfcoordarr );

	H5::DataSpace outputdataspace( 1, &nrpts );
	h5ds.read( data, h5DataType( h5ds ), outputdataspace, inputdataspace);
    }
    mCatchErrDuringRead()
}


bool HDF5::ReaderImpl::hasAttribute( const char* attrnm,
				     const DataSetKey* dsky ) const
{
    const H5::H5Object* h5scope = getScope( dsky );
    if ( !h5scope )
	return false;
    try
    {
	return h5scope->attrExists( attrnm );
    }
    catch ( ... )
	{ return false; }
}


int HDF5::ReaderImpl::getNrAttributes( const DataSetKey* dsky ) const
{
    const H5::H5Object* h5scope = getScope( dsky );
    if ( !h5scope )
	return 0;
    try
    {
	return h5scope->getNumAttrs();
    }
    catch ( ... )
	{ return 0; }
}


namespace HDF5
{

static void add_attr_valstr( H5::H5Object& loc, const H5std_string attr_name,
		       void* operator_data )
{
    auto* valstrs = reinterpret_cast<BufferStringSet*>( operator_data );
    valstrs->add( attr_name.c_str() );
}


static void add_attr_iop( H5::H5Object& loc, const H5std_string attr_name,
		       void* operator_data )
{
    auto* iop = reinterpret_cast<IOPar*>( operator_data );
    const H5::Attribute attr = loc.openAttribute( attr_name );
    std::string valstr;
    attr.read( attr.getDataType(), valstr );
    iop->set( attr_name.c_str(), valstr.c_str() );
}

} // namespace HDF5

void HDF5::ReaderImpl::gtAttribNames( const H5::H5Object& h5obj,
				      BufferStringSet& nms ) const
{
    H5::H5Object& h5objed = const_cast<H5::H5Object&>( h5obj );
    try {
	h5objed.iterateAttrs( add_attr_valstr, NULL, &nms );
    }
    catch( ... ) {}
}


bool HDF5::ReaderImpl::getAttribute( const char* attrnm,
				     BufferString& res,
				     const DataSetKey* dsky ) const
{
    if ( !hasAttribute(attrnm,dsky) )
	return false;

    const H5::Attribute attr = getScope( dsky )->openAttribute( attrnm );
    std::string valstr;
    attr.read( attr.getDataType(), valstr );
    res.set( valstr.c_str() );

    return true;
}


#define mGetIntAttr( attrnm, type, res ) \
bool HDF5::ReaderImpl::getAttribute( const char* attrnm, type& res, \
				     const DataSetKey* dsky ) const \
{ \
    BufferString resstr; \
    if ( !getAttribute(attrnm,resstr,dsky) ) \
	return false; \
    res = mCast(type, resstr.toInt() ); \
    return true; \
}
mGetIntAttr(attrnm,od_int16,res)
mGetIntAttr(attrnm,od_uint16,res)
mGetIntAttr(attrnm,od_int32,res)
mGetIntAttr(attrnm,od_uint32,res)
mGetIntAttr(attrnm,od_int64,res)
mGetIntAttr(attrnm,od_uint64,res)
#undef mGetIntAttr


bool HDF5::ReaderImpl::getAttribute( const char* attrnm, float& res,
				     const DataSetKey* dsky ) const
{
    BufferString resstr;
    if ( !getAttribute(attrnm,resstr,dsky) )
	return false;
    res = resstr.toFloat();
    return true;
}


bool HDF5::ReaderImpl::getAttribute( const char* attrnm, double& res,
				     const DataSetKey* dsky ) const
{
    BufferString resstr;
    if ( !getAttribute(attrnm,resstr,dsky) )
	return false;
    res = resstr.toDouble();
    return true;
}


void HDF5::ReaderImpl::gtInfo( const H5::H5Object& h5obj, IOPar& iop,
			       uiRetVal& uirv ) const
{
    iop.setEmpty();
    H5::H5Object& h5objed = const_cast<H5::H5Object&>( h5obj );
    try {
	h5objed.iterateAttrs( add_attr_iop, NULL, &iop );
    }
    mCatchUnexpected( return );
}


uiRetVal HDF5::ReaderImpl::readJSonAttribute( const char* attrnm,
					      OD::JSON::ValueSet& vs,
					      const DataSetKey* dsky ) const
{
    uiRetVal uirv;
    if ( !attrnm )
    {
	uirv.set( tr("Valid attribute name required") );
	return uirv;
    }

    if ( !hasAttribute(attrnm,dsky) )
    {
	uirv.set( tr("No attribute named: %1").arg(attrnm) );
	if ( dsky )
	    uirv.add( tr("In scope %1").arg(dsky->fullDataSetName()) );
	else
	    uirv.add( tr("In root scope") );
	return uirv;
    }

    vs.setEmpty();
    const H5::Attribute attr = getScope( dsky )->openAttribute( attrnm );
    std::string valstr;
    attr.read( attr.getDataType(), valstr );
    uirv = vs.parseJSon( const_cast<char*>( valstr.c_str() ),
							    valstr.size() );
    return uirv;
}
