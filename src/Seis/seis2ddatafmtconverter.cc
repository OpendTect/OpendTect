/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";


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

class OD_2DLineSetTo2DDataSetConverter
{
public:

			    OD_2DLineSetTo2DDataSetConverter()	    {}
			    ~OD_2DLineSetTo2DDataSetConverter();

    void		    doConversion();

protected:

    void		    makeListOfLineSets(ObjectSet<IOObj>&) const;
    void		    fillIOParsFrom2DSFile(const ObjectSet<IOObj>&);
    void		    getCBVSFilePaths(BufferStringSet&);
    bool		    copyDataAndAddToDelList(BufferStringSet&,
						    BufferStringSet&,
						    BufferString&);
    void		    update2DSFiles(ObjectSet<IOObj>& ioobjlist);
    void		    removeDuplicateData(BufferStringSet&);
    

    BufferString	    getAttrFolderPath(const IOPar&) const;

    	    
    ObjectSet<IOPar>	    all2dseisiopars_;
};


mGlobal(Seis) void OD_Convert_2DLineSets_To_2DDataSets();
mGlobal(Seis) void OD_Convert_2DLineSets_To_2DDataSets()
{
    mDefineStaticLocalObject( OD_2DLineSetTo2DDataSetConverter, converter, );
    converter.doConversion();
}


OD_2DLineSetTo2DDataSetConverter::~OD_2DLineSetTo2DDataSetConverter()
{ deepErase( all2dseisiopars_ ); }


void OD_2DLineSetTo2DDataSetConverter::doConversion()
{
    ObjectSet<IOObj> all2dsfiles;
    makeListOfLineSets( all2dsfiles );
    fillIOParsFrom2DSFile( all2dsfiles );
    BufferStringSet filepathsofold2ddata, filestobedeleted;
    getCBVSFilePaths( filepathsofold2ddata );
    BufferString errmsg;
    copyDataAndAddToDelList( filepathsofold2ddata, filestobedeleted, errmsg );
    update2DSFiles( all2dsfiles );
    removeDuplicateData( filestobedeleted );
    return;
}


void OD_2DLineSetTo2DDataSetConverter::makeListOfLineSets( 
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


void OD_2DLineSetTo2DDataSetConverter::fillIOParsFrom2DSFile(
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


BufferString OD_2DLineSetTo2DDataSetConverter::getAttrFolderPath( 
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


void OD_2DLineSetTo2DDataSetConverter::getCBVSFilePaths( 
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


bool OD_2DLineSetTo2DDataSetConverter::copyDataAndAddToDelList( 
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


void OD_2DLineSetTo2DDataSetConverter::update2DSFiles( 
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


void OD_2DLineSetTo2DDataSetConverter::removeDuplicateData( 
						  BufferStringSet& oldfilepath )
{
    for ( int idx=0; idx<oldfilepath.size(); idx++ )
	File::remove( oldfilepath.get(idx) );

    return;
}

