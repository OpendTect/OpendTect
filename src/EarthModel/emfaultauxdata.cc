/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emfaultauxdata.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "binidvalset.h"
#include "emfault3d.h"
#include "emioobjinfo.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "parametricsurface.h"
#include "ptrman.h"
#include "safefileio.h"
#include "settings.h"
#include "strmprov.h"
#include "varlenarray.h"

namespace EM
{

FaultAuxData::DataInfo::DataInfo()
{
    username.setEmpty();
    filename.setEmpty();
    data = 0;
    policy = OD::UsePtr;
}


FaultAuxData::DataInfo::~DataInfo()
{
    if ( policy!=OD::UsePtr )
	delete data;
}


bool FaultAuxData::DataInfo::operator==( const DataInfo& di )
{ 
    return username==di.username && filename==di.filename && 
	   data==di.data && policy==di.policy; 
}


FaultAuxData::FaultAuxData( const Fault3D& flt )
    : faultmid_(flt.multiID())
{
    fltfullnm_.setEmpty();
    init();
}


FaultAuxData::FaultAuxData( const MultiID& mid )
    : faultmid_(mid)
{
    fltfullnm_.setEmpty();
    init();
}


FaultAuxData::~FaultAuxData()
{ deepErase( dataset_ ); }


bool FaultAuxData::init()
{
    if ( !dataset_.isEmpty() )
	return true; //already called

    PtrMan<IOObj> ioobj = IOM().get( faultmid_ );
    IOObjInfo ioinfo( ioobj );
    if ( !ioobj || ioinfo.type()!=IOObjInfo::Fault ) 
	return false;
    
    FilePath fp( ioobj->fullUserExpr(true) );
    fp.setExtension( "" );
    fltfullnm_ = fp.fullPath();

    deepErase( dataset_ );
    
    ObjectSet<IOPar> pars;
    readSDInfoFile( pars );
    for ( int idx=0; idx<pars.size(); idx++ )
    {
	DataInfo* di = new DataInfo();
	pars[idx]->get( sKey::Name(), di->username );
	pars[idx]->get( sKey::FileName(), di->filename );
	
	dataset_ += di;
    }
    deepErase( pars );
    return true;
}


int FaultAuxData::dataIndex( const char* nm ) const
{
    for ( int idx=0; idx<dataset_.size(); idx++ )
    {
	if ( dataset_[idx]->username==nm )
	    return idx;
    }

    return -1; 
}


void FaultAuxData::setSelected( const TypeSet<int>& sl )
{
    selected_.erase();
    selattribnames_.erase();
    for ( int idx=0; idx<sl.size(); idx++ )
    {
	if ( !dataset_.validIdx(sl[idx]) )
	    continue;

	selected_ += sl[idx];
	selattribnames_.add( dataset_[sl[idx]]->username );
    }
}


void FaultAuxData::getAuxDataList( BufferStringSet& list ) const
{
    list.erase();
    for ( int idx=0; idx<dataset_.size(); idx++ )
	list.add( dataset_[idx]->username );
}


int FaultAuxData::setData( const char* name, const Array2D<float>* data, 
			   OD::PtrPolicy policy )
{
    if ( !data )
	return -1;

    const int sdidx = dataIndex( name );
    if ( sdidx<0 )
    {
	DataInfo* di = new DataInfo();
	di->data = data;
	di->policy = policy;

	dataset_ += di;
	const int cursdidx = dataset_.size()-1;
    	updateDataFiles( SetName, cursdidx, name );
	return cursdidx;
    }

    setData( sdidx, data, policy );
    return sdidx;
}


void FaultAuxData::setData( int sdidx, const Array2D<float>* data, 
			   OD::PtrPolicy policy )
{
    if ( !data || !dataset_.validIdx(sdidx) )
	return;

    if ( dataset_[sdidx]->policy != OD::UsePtr )
	delete dataset_[sdidx]->data;
    
    dataset_[sdidx]->data = data;
    dataset_[sdidx]->policy = policy;
}


void FaultAuxData::setDataName( int sdidx, const char* name )
{ updateDataFiles( SetName, sdidx, name ); }


void FaultAuxData::setDataName( const char* oldname, const char* newname )
{
    const int sdidx = dataIndex( oldname );
    setDataName( sdidx, newname );
}


void FaultAuxData::removeData( int sdidx )
{ updateDataFiles( Remove, sdidx, 0 ); }


void FaultAuxData::removeData( const char* name )
{
    const int sdidx = dataIndex( name );
    removeData( sdidx );
}


void FaultAuxData::removeAllData()
{
    const int datasize = dataset_.size();
    if ( !datasize || fltfullnm_.isEmpty() )
	return;

    deepErase( dataset_ );

    for ( int idx=0; idx<datasize; idx++ )
    {
	const BufferString attrnm = createFltDataName( fltfullnm_, idx );
    	File::remove( attrnm );
    }

    FilePath fp( fltfullnm_ );
    fp.setExtension( sKeyExtension() );
    File::remove( fp.fullPath() );
}


void FaultAuxData::renameFault( const char* fltnewname )
{
    if ( fltfullnm_.isEmpty() )
	return;

    const BufferString oldfltfulnm = fltfullnm_;
    FilePath fp( fltfullnm_ );
    fp.setFileName( fltnewname );
    fltfullnm_ = fp.fullPath();
    
    for ( int idx=0; idx<dataset_.size(); idx++ )
    {
	const BufferString newname = createFltDataName( fltfullnm_, idx );
	const BufferString oldname = createFltDataName( oldfltfulnm, idx );

	FilePath fn( newname );
	dataset_[idx]->filename = fn.fileName(); 
    	File::rename( oldname, newname );
    }

    FilePath oldfp( oldfltfulnm );
    oldfp.setExtension( sKeyExtension() );
    File::remove( oldfp.fullPath() );
    for ( int idx=0; idx<dataset_.size(); idx++ )
	updateDataFiles( SetName, idx, dataset_[idx]->username );
}


void FaultAuxData::updateDataFiles( Action act, int sdidx, const char* nm )
{
    if ( !dataset_.validIdx(sdidx) ) 
	return;

    FilePath fp( fltfullnm_ );
    fp.setExtension( sKeyExtension() );
    FilePath backupfp(fp);
    backupfp.setExtension(".old");

    if ( File::exists(fp.fullPath()) )
        File::rename(fp.fullPath(), backupfp.fullPath());

    SafeFileIO sfio( fp.fullPath(), true );
    if ( !sfio.open(false,true) )
    {
	File::remove( backupfp.fullPath() );
	return;
    }

    ascostream astream( sfio.ostrm() );
    astream.putHeader( sKeyFaultAuxData() );
    
    FilePath fpnm( fltfullnm_ );
    BufferString filenm( fpnm.fileName() );
    cleanupString( filenm.buf(), false, false, false );

    if ( act==Remove )
    {
	delete dataset_.removeSingle( sdidx );
    
	BufferString prevnm = createFltDataName( fltfullnm_, sdidx );
	BufferString nextnm = createFltDataName( fltfullnm_, sdidx+1 );
    	File::remove( prevnm );
	for ( int idx=sdidx; idx<dataset_.size(); idx++ )
	{
	    File::rename( nextnm, prevnm );
	    prevnm = nextnm;
	    nextnm = createFltDataName( fltfullnm_, idx+2 );
	}
    }
    else 
	dataset_[sdidx]->username = nm;

    ObjectSet<IOPar> pars;
    for ( int idx=0; idx<dataset_.size(); idx++ )
    {
	IOPar* par = new IOPar();
	par->set( sKey::Name(), dataset_[idx]->username );
    
	const BufferString attrfilenm = createFltDataName( filenm,  idx ); 
	par->set( sKey::FileName(), attrfilenm );
	pars += par;
    }

    astream.put( "Number of surface data", pars.size() );
    astream.newParagraph();
    
    IOPar allpars;
    for ( int pidx=0; pidx<pars.size(); pidx++ )
	allpars.mergeComp( *pars[pidx], toString(pidx) );
    allpars.putTo( astream );
    
    deepErase( pars );
    File::remove( backupfp.fullPath() );
    sfio.closeSuccess();
}


void FaultAuxData::readSDInfoFile( ObjectSet<IOPar>& sdnmpars )
{
    deepErase( sdnmpars );
    FilePath fp( fltfullnm_ );
    fp.setExtension( sKeyExtension() );

    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	return;

    ascistream astrm( sfio.istrm(), true );
    if ( !astrm.isOfFileType( sKeyFaultAuxData() ) )
    {
	sfio.closeSuccess();
	return;
    }
    
    while ( !atEndOfSection(astrm.next()) )
	continue;

    IOPar allpars;
    allpars.getFrom( astrm );
    
    int idx = 0;
    while ( true )
    {
	IOPar* par = allpars.subselect( idx );
	if ( !par ) break;
	sdnmpars += par;
	idx++;
    }
    sfio.closeSuccess();
}


BufferString FaultAuxData::createFltDataName( const char* base, int sdidx )
{
    BufferString res( base );
    res += "^"; res += (sdidx+1); res += ".flt";
    return res;
}


#define mErrRtn(msg)	{ errmsg_ = msg; return false; }

bool FaultAuxData::storeData( int sdidx, bool binary )
{
    errmsg_.setEmpty();
    
    if ( !dataset_.validIdx(sdidx) || !dataset_[sdidx]->data ) 
	mErrRtn("No valid surface data to store");

    const BufferString fltsdnm = createFltDataName( fltfullnm_, sdidx );
    FilePath fp( fltsdnm );

    FilePath backupfp(fp);
    backupfp.setExtension(".old");
    if ( File::exists(fp.fullPath()) )
	File::copy( fp.fullPath(), backupfp.fullPath() );

    SafeFileIO sfio( fp.fullPath(), true );
    if ( !sfio.open(false,true) )
    {
	File::remove( backupfp.fullPath() );
	mErrRtn("Cannot open file to write, check your disk I/O permission");
    }

    ascostream astream( sfio.ostrm() );
    astream.putHeader( sKeyFaultAuxData() );

    IOPar sdinfo;
    sdinfo.setYN( sKey::Binary(), binary );
    FilePath fpnm( fltfullnm_ );
    sdinfo.set( "RowSize", dataset_[sdidx]->data->info().getSize(0) );
    sdinfo.set( "ColSize", dataset_[sdidx]->data->info().getSize(1) );
    sdinfo.putTo( astream );

    od_ostream& strm = astream.stream();
    const float* vals = dataset_[sdidx]->data->getData();
    if ( vals )
    {
	const od_uint64 datasz = dataset_[sdidx]->data->info().getTotalSz();
	for ( od_int64 idy=0; idy<datasz; idy++ )
	{
	    if ( binary )
		strm.addBin( &vals[idy], sizeof(float) );
	    else
		strm << vals[idy] << od_tab;
	}
    }
    else
    {
	const int isz = dataset_[sdidx]->data->info().getSize( 0 );
	const int jsz = dataset_[sdidx]->data->info().getSize( 1 );
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int jdx=0; jdx<jsz; jdx++ )
	    {
		const float val = dataset_[sdidx]->data->get( idx, jdx );
    		if ( binary )
    		    strm.addBin( &val, sizeof(float) );
    		else
    		    strm << val << od_tab;
	    }
	}
    }
	
    if ( !binary ) 
	strm << od_newline;

    File::remove( backupfp.fullPath() );
    return sfio.closeSuccess();
}


const Array2D<float>* FaultAuxData::loadIfNotLoaded( const char* sdname )
{ return loadIfNotLoaded( dataIndex(sdname) ); }


const Array2D<float>* FaultAuxData::loadIfNotLoaded( int sdidx )
{ 
    if ( !dataset_.validIdx(sdidx) )
	return 0;

    if ( !dataset_[sdidx]->data )
	loadData( sdidx );

    return dataset_[sdidx]->data; 
}


bool FaultAuxData::loadData( int sdidx )
{
    errmsg_.setEmpty();
    if ( !dataset_.validIdx(sdidx) ) 
	mErrRtn("Surface data does not exist");

    const BufferString fltsdnm = createFltDataName( fltfullnm_, sdidx );
    FilePath fp( fltsdnm );

    if ( !File::exists(fp.fullPath()) )
	mErrRtn("Surface data does not exist");

    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	mErrRtn("Cannot open file, check your file read permission");

    ascistream astream( sfio.istrm(), true );
    if ( !astream.isOfFileType( sKeyFaultAuxData() ) )
    {
	sfio.closeSuccess();
	mErrRtn("Wrong file type, not fault aux data");
    }

    astream.next();
    IOPar sdinfo;
    sdinfo.getFrom( astream );

    bool binary = false;
    BufferString fltnm, username;
    int rowsize, colsize;
    sdinfo.getYN( sKey::Binary(), binary );
    sdinfo.get( "RowSize", rowsize );
    sdinfo.get( "ColSize", colsize );
    
    Array2DImpl<float>* newdata = new Array2DImpl<float>( rowsize, colsize );
    const od_int64 datasz = rowsize * colsize;
    float* arr = newdata->getData();

    od_istream& strm = astream.stream();
    for ( od_int64 idx=0; idx<datasz; idx++ )
    {
	float val;
	if ( binary )
	    strm.getBin( &val, sizeof(float) );
	else
	    strm >> val;
	arr[idx] = val;
    }

    dataset_[sdidx]->data = newdata;
    dataset_[sdidx]->policy = OD::CopyPtr;

    return sfio.closeSuccess();
}


}; //namespace
