/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "twodseisdataconverterfromod4tood5format.h"

#include "bufstringset.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "typeset.h"
#include "seis2dline.h"
#include "seiscbvs2d.h"
#include "seisioobjinfo.h"



TwoDSeisDataConverterFromOD4ToOD5Format::
    ~TwoDSeisDataConverterFromOD4ToOD5Format()
{ deepErase( all2dseisiopars_ ); }


bool TwoDSeisDataConverterFromOD4ToOD5Format::convertSeisData( 
							  BufferString& errmsg )
{
    ObjectSet<IOObj> allioobjsof2ddata;
    makeListOfLineSets( allioobjsof2ddata );
    fillIOParsFrom2DSFile( allioobjsof2ddata );
    BufferStringSet filepathsofold2ddata, filestobedeleted;
    getCBVSFilePaths( filepathsofold2ddata );
    copyDataAndAddToDelList( filepathsofold2ddata,filestobedeleted, errmsg );
    update2DSFiles( allioobjsof2ddata );
    removeDuplicateData( filestobedeleted );
    return errmsg.isEmpty() ? true : false;
}


void TwoDSeisDataConverterFromOD4ToOD5Format::makeListOfLineSets( 
					    ObjectSet<IOObj>& ioobjlist ) const
{
    BufferStringSet lsnms; TypeSet<MultiID> lsids;
    SeisIOObjInfo::get2DLineInfo( lsnms, &lsids );
    if ( lsnms.isEmpty() ) return;

    for ( int idx=0; idx<lsids.size(); idx++ )
    {
	IOObj* ioobj = IOM().get( lsids[idx] );
	if ( !ioobj ) continue;
	ioobjlist += ioobj;
    }

    return;
}


void TwoDSeisDataConverterFromOD4ToOD5Format::fillIOParsFrom2DSFile(
					    const ObjectSet<IOObj>& ioobjlist )
{
    for ( int idx=0; idx<ioobjlist.size(); idx++ )
    {
	const Seis2DLineSet lineset( *ioobjlist[idx] );
	for ( int lineidx=0; lineidx<lineset.nrLines(); lineidx++ )
	{
	    IOPar* iop = new IOPar(lineset.getInfo(lineidx) );
	    all2dseisiopars_ += iop;
	}
    }

    return;
}


BufferString TwoDSeisDataConverterFromOD4ToOD5Format::getAttrFolderPath( 
							const IOPar& iop ) const
{
    const IOObjContext& iocontext = mIOObjContext(SeisTrc);
    if ( !IOM().to(iocontext.getSelKey()) ) return BufferString::empty();
    CtxtIOObj ctio( iocontext );
    ctio.ctxt.deftransl.add( "TwoD DataSet" );
    ctio.ctxt.setName( iop.getValue( iop.indexOf( sKey::Attribute() ) ) );
    if ( ctio.fillObj() == 0 ) return BufferString::empty();
    PtrMan<IOObj> ioobj = ctio.ioobj;
    return BufferString( ioobj ? ioobj->fullUserExpr() : "" );
}


void TwoDSeisDataConverterFromOD4ToOD5Format::getCBVSFilePaths( 
						    BufferStringSet& filepaths )
{
    for ( int idx=0; idx<all2dseisiopars_.size(); idx++ )
	filepaths.add( SeisCBVS2DLineIOProvider::getFileName( 
						  *all2dseisiopars_[idx]) );

    return;
}


#define mCopyFileAndAddToDeleteList \
    if ( File::exists(oldfp.fullPath()) && !File::exists(newfp.fullPath()) ) \
    { \
	if ( File::copy(oldfp.fullPath(),newfp.fullPath(),&errmsg) ) \
	    filestobedeleted.add( oldfp.fullPath() ); \
	else \
	    return false; \
    }


bool TwoDSeisDataConverterFromOD4ToOD5Format::copyDataAndAddToDelList( 
	BufferStringSet& oldfilepaths, BufferStringSet& filestobedeleted,
	BufferString& errmsg )
{
    for ( int idx=0; idx<all2dseisiopars_.size(); idx++ )
    {
	IOPar* iop = all2dseisiopars_[idx];
	FilePath oldfp( oldfilepaths.get(idx) );
	FilePath newfp( getAttrFolderPath( *iop ) );
	File::createDir( newfp.fullPath() );
	newfp.add( oldfp.fileName() );
	if ( oldfp == newfp )
	    continue;

	mCopyFileAndAddToDeleteList
	oldfp.setExtension( "par" );
	newfp.setExtension( "par" );
	mCopyFileAndAddToDeleteList
    }

    return true;
}


void TwoDSeisDataConverterFromOD4ToOD5Format::update2DSFiles( 
						   ObjectSet<IOObj>& ioobjlist )
{
    for ( int idx=0; idx<ioobjlist.size(); idx++ )
    {
	Seis2DLineSet lineset( *ioobjlist[idx] );
	for ( int lineidx=0; lineidx<lineset.nrLines(); lineidx++ )
	{
	    IOPar* iop = new IOPar(lineset.getInfo(lineidx) );
	    FixedString attrname = iop->getValue( iop->indexOf(
							   sKey::Attribute()) );
	    int idxfnm = iop->indexOf( sKey::FileName() );
	    FilePath oldfnm( iop->getValue(idxfnm) );
	    BufferString oldfullfnm( SeisCBVS2DLineIOProvider::getFileName(
									*iop) );
	    if ( oldfnm.isAbsolute() || oldfnm.nrLevels()>1 )
	    {
		delete iop;
		continue;
	    }

	    BufferString newfnm( attrname );
	    newfnm.add(FilePath::dirSep(FilePath::Local)).add(
							    oldfnm.fullPath());
	    iop->set( sKey::FileName(), newfnm.buf() );
	    BufferString newfullfnm( SeisCBVS2DLineIOProvider::getFileName(
									*iop) );
	    if ( File::exists(newfullfnm.buf()) || !File::exists(
							    oldfullfnm.buf()) )
		delete lineset.linePutter( iop );
	}
    }

    return;
}


void TwoDSeisDataConverterFromOD4ToOD5Format::removeDuplicateData( 
						  BufferStringSet& oldfilepath )
{
    for ( int idx=0; idx<oldfilepath.size(); idx++ )
	File::remove( oldfilepath.get(idx) );

    return;
}

