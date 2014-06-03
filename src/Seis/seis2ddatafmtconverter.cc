/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "bufstringset.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "typeset.h"
#include "seis2dline.h"
#include "seiscbvs2d.h"
#include "seisioobjinfo.h"
#include "survgeom.h"
#include "survinfo.h"

#define mCapChar "^"


static const char* getSurvDefAttrName()
{
    mDefineStaticLocalObject( BufferString, ret,
	   = SI().getPars().find(IOPar::compKey(sKey::Default(),
			SeisTrcTranslatorGroup::sKeyDefaultAttrib())) );
    if ( ret.isEmpty() ) ret = "Seis";
    return ret.buf();
}


class OD_FileListCopier : public Executor
{
public:
OD_FileListCopier( const BufferStringSet& fromlist,
		   const BufferStringSet& tolist, BufferString& errmsg )
    : Executor( "2D data conversion" )
    , fromlist_(fromlist), tolist_(tolist)
    , errmsg_(errmsg),curidx_(0)
{
}

od_int64 totalNr() const	{ return mCast(od_int64,fromlist_.size()); }
od_int64 nrDone() const		{ return mCast(od_int64,curidx_); }
uiStringCopy uiNrDoneText() const	{ return "Nr files done"; }
uiStringCopy uiMessage() const	{ return "Converting 2D Seismic data"; }

int nextStep()
{
    if ( !fromlist_.validIdx(curidx_) || !tolist_.validIdx(curidx_) )
	return Finished();

    const BufferString& src = fromlist_.get( curidx_ );
    const BufferString& dest = tolist_.get( curidx_ );
    if ( File::exists(src) && !File::exists(dest) )
    {
	if ( !File::copy(src,dest,&errmsg_) )
	    return ErrorOccurred();
    }

    FilePath srcfp( src );
    FilePath destfp( dest );
    srcfp.setExtension( "par" );
    destfp.setExtension( "par" );
    if ( File::exists(srcfp.fullPath()) && !File::exists(destfp.fullPath()) )
    {
	if ( !File::copy(srcfp.fullPath(),destfp.fullPath(),&errmsg_) )
	    return ErrorOccurred();
    }

    curidx_++;
    return MoreToDo();
}

    const BufferStringSet&	fromlist_;
    const BufferStringSet&	tolist_;
    BufferString&		errmsg_;
    int				curidx_;
};


class OD_2DLineSetTo2DDataSetConverter
{mODTextTranslationClass(OD_2DLineSetTo2DDataSetConverter);
public:

			    OD_2DLineSetTo2DDataSetConverter()	    {}
			    ~OD_2DLineSetTo2DDataSetConverter();

    void		    doConversion(uiString& errmsg,TaskRunner*);

protected:

    void		    makeListOfLineSets(ObjectSet<IOObj>&);
    void		    fillIOParsFrom2DSFile(const ObjectSet<IOObj>&);
    void		    getCBVSFilePaths(BufferStringSet&);
    bool		    copyData(BufferStringSet&,uiString&,TaskRunner*);
    void		    update2DSFilesAndAddToDelList(
						    ObjectSet<IOObj>& ioobjlist,
						    BufferStringSet&);
    void		    removeDuplicateData(BufferStringSet&);
    

    BufferString	    getAttrFolderPath(IOPar&) const;

    	    
    ObjectSet<ObjectSet<IOPar> > all2dseisiopars_;
};


/* 0=No old 2D data, 1=First time conversion, 2=Incremental conversion. */
mGlobal(Seis) int OD_Get_2D_Data_Conversion_Status()
{
    IOObjContext oldctxt( mIOObjContext(SeisTrc) );
    oldctxt.toselect.allowtransls_ = "2D";
    const IODir oldiodir( oldctxt.getSelKey() );
    const IODirEntryList olddel( oldiodir, oldctxt );
    if ( olddel.isEmpty() )
	return 0;

    IOObjContext newctxt( mIOObjContext(SeisTrc) );
    newctxt.toselect.allowtransls_ = "TwoD DataSet";
    const IODir newiodir( newctxt.getSelKey() );
    const IODirEntryList newdel( newiodir, newctxt );
    return newdel.isEmpty() ? 1 : 2;
}

mGlobal(Seis) void OD_Convert_2DLineSets_To_2DDataSets( uiString& errmsg,
							TaskRunner* taskrnr )
{
    mDefineStaticLocalObject( OD_2DLineSetTo2DDataSetConverter, converter, );
    converter.doConversion( errmsg, taskrnr );
}


OD_2DLineSetTo2DDataSetConverter::~OD_2DLineSetTo2DDataSetConverter()
{}

#include <iostream>
void OD_2DLineSetTo2DDataSetConverter::doConversion( uiString& errmsg,
						     TaskRunner* taskrnr )
{
    ObjectSet<IOObj> all2dsfiles;
    makeListOfLineSets( all2dsfiles );
    fillIOParsFrom2DSFile( all2dsfiles );
    BufferStringSet filepathsofold2ddata, filestobedeleted;
    getCBVSFilePaths( filepathsofold2ddata );
    copyData( filepathsofold2ddata, errmsg, taskrnr );
    update2DSFilesAndAddToDelList( all2dsfiles, filestobedeleted );
    removeDuplicateData( filestobedeleted );
    deepErase( all2dseisiopars_ );
    return;
}


void OD_2DLineSetTo2DDataSetConverter::makeListOfLineSets( 
						   ObjectSet<IOObj>& ioobjlist )
{
    BufferStringSet lsnms; TypeSet<MultiID> lsids;
    SeisIOObjInfo::get2DLineInfo( lsnms, &lsids );
    if ( lsnms.isEmpty() ) return;

    for ( int idx=0; idx<lsids.size(); idx++ )
    {
	IOObj* ioobj = IOM().get( lsids[idx] );
	if ( !ioobj ) continue;
	ioobjlist += ioobj;
	ObjectSet<IOPar>* obset = new ObjectSet<IOPar>();
	all2dseisiopars_ += obset;
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
	    IOPar* iop = new IOPar( lineset.getInfo(lineidx) );
	    iop->add( sKey::GeomID(), Survey::GM().getGeomID(lineset.name(),
						lineset.lineName(lineidx)) );
	    *all2dseisiopars_[idx] += iop;
	}
    }

    return;
}


BufferString OD_2DLineSetTo2DDataSetConverter::getAttrFolderPath( 
							IOPar& iop ) const
{
    const IOObjContext& iocontext = mIOObjContext(SeisTrc);
    if ( !IOM().to(iocontext.getSelKey()) ) return BufferString::empty();
    CtxtIOObj ctio( iocontext );
    ctio.ctxt.deftransl.add( TwoDDataSeisTrcTranslator::translKey() );
    if ( iop.find(sKey::DataType()) )
    {
	BufferString datatype;
	iop.get( sKey::DataType(), datatype );
	ctio.ctxt.toselect.require_.set( sKey::Type(), datatype );
    }

    FixedString attribnm = iop.find( sKey::Attribute() );
    if ( attribnm.isEmpty() ) attribnm = "Seis";
    ctio.ctxt.setName( attribnm );
    if ( ctio.fillObj() == 0 ) return BufferString::empty();
    IOObj* ioobj = ctio.ioobj;
    if ( ioobj->translator() != ctio.ctxt.deftransl )
    {
	BufferString nm = ioobj->name();
	nm.add( "[2D]" );
	ctio.ioobj = 0;
	ctio.ctxt.setName( nm );
	if ( ctio.fillObj() == 0 ) return BufferString::empty();
	FilePath fp( ctio.ioobj->fullUserExpr() );
	nm = fp.fileName();
	iop.set( sKey::Attribute(), nm.buf() );
    }

    const FixedString survdefattr( getSurvDefAttrName() );
    const bool issurvdefset = SI().pars().find( IOPar::compKey(sKey::Default(),
				SeisTrcTranslatorGroup::sKeyDefault2D()) );
    if ( !issurvdefset && ctio.ioobj && survdefattr == attribnm )
	ctio.ioobj->setSurveyDefault();

    return BufferString( ctio.ioobj ? ctio.ioobj->fullUserExpr() : "" );
}


void OD_2DLineSetTo2DDataSetConverter::getCBVSFilePaths( 
						    BufferStringSet& filepaths )
{
    for ( int idx=0; idx<all2dseisiopars_.size(); idx++ )
    {
	for ( int lineidx=0; lineidx<all2dseisiopars_[idx]->size(); lineidx++ )
	    filepaths.add( SeisCBVS2DLineIOProvider::getFileName(
				    *(*all2dseisiopars_[idx])[lineidx],false) );
    }

    return;
}


bool OD_2DLineSetTo2DDataSetConverter::copyData( BufferStringSet& oldfilepaths,
						 uiString& errmsg,
						 TaskRunner* taskrnr )
{
    int numberoflines = 0;
    BufferStringSet srclist, destlist;
    for ( int idx=0; idx<all2dseisiopars_.size(); idx++ )
    {
	for ( int lineidx=0; lineidx<all2dseisiopars_[idx]->size(); lineidx++ )
	{
	    IOPar* iop = (*all2dseisiopars_[idx])[lineidx];
	    FilePath oldfp( oldfilepaths.get(numberoflines) );
	    numberoflines++;
	    if ( !File::exists(oldfp.fullPath()) )
		continue;

	    FilePath newfp( getAttrFolderPath( *iop ) );
	    File::createDir( newfp.fullPath() );
	    BufferString newfn( newfp.fileName() );
	    newfn.add( mCapChar );
	    Pos::GeomID geomid;
	    if ( !iop->get(sKey::GeomID(),geomid) || geomid <= 0 )
		continue;
	    newfn.add( geomid );
	    newfp.add( newfn ).setExtension( oldfp.extension() );

	    if ( oldfp == newfp )
		continue;

	    srclist.add( oldfp.fullPath() );
	    destlist.add( newfp.fullPath() );
	}
    }

    if ( srclist.isEmpty() )
	return true;

    BufferString msg;
    OD_FileListCopier exec( srclist, destlist, msg );
    const bool res = TaskRunner::execute( taskrnr, exec );
    if ( !res )
	errmsg = tr( "Error while converting 2D Seismic data: %1" ).arg( msg );

    return res;
}


void OD_2DLineSetTo2DDataSetConverter::update2DSFilesAndAddToDelList( 
		ObjectSet<IOObj>& ioobjlist,BufferStringSet& filestobedeleted )
{
    for ( int idx=0; idx<ioobjlist.size(); idx++ )
    {
	Seis2DLineSet lineset( *ioobjlist[idx] );
	for ( int lineidx=0; lineidx<lineset.nrLines(); lineidx++ )
	{
	    IOPar* iop = new IOPar( lineset.getInfo(lineidx) );
	    BufferString fnm;
	    iop->get( sKey::FileName(), fnm );
	    FilePath oldfnm( fnm );
	    BufferString oldfullfnm( SeisCBVS2DLineIOProvider::getFileName(
								*iop,false) );
	    BufferString attrname;
	    (*all2dseisiopars_[idx])[lineidx]->get(sKey::Attribute(), attrname);
	    if ( attrname.isEmpty() ) attrname = "Seis";
	    attrname.clean( BufferString::AllowDots );
	    FilePath newfnm( attrname );
	    newfnm.add( BufferString(attrname)
		    .add(mCapChar)
		    .add( Survey::GM().getGeomID(lineset.name(),
						 lineset.lineName(lineidx)) ) );
	    newfnm.setExtension( oldfnm.extension() );
	    iop->set( sKey::FileName(), newfnm.fullPath(FilePath::Unix) );
	    BufferString newfullfnm( SeisCBVS2DLineIOProvider::getFileName(
									*iop) );
	    if ( newfullfnm == oldfullfnm )
		continue;

	    if ( File::exists(newfullfnm.buf()) )
	    {
		Seis2DLinePutter* ret = lineset.linePutter( iop );
		if ( ret )
		{
		    delete ret;
		    if ( File::exists(oldfullfnm) )
			filestobedeleted.add( oldfullfnm );
		}
	    }
	}
    }

    return;
}


void OD_2DLineSetTo2DDataSetConverter::removeDuplicateData( 
						  BufferStringSet& oldfilepath )
{
    for ( int idx=0; idx<oldfilepath.size(); idx++ )
    {
	FilePath fp( oldfilepath.get(idx) );
	File::remove( fp.fullPath() );
	fp.setExtension( "par" );
	File::remove( fp.fullPath() );
    }

    return;
}

