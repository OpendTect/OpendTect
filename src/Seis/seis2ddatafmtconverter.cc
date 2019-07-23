/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2013
-*/



#include "ascstream.h"
#include "bufstringset.h"
#include "dirlist.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "dbdir.h"
#include "dbman.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2dsurv.h"
#include "typeset.h"
#include "safefileio.h"
#include "seiscbvs2d.h"
#include "seisioobjinfo.h"
#include "seispsioprov.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include <iostream>
#define mCapChar "^"

static void convert2DPSData()
{
    const DBDirEntryList olddel( mIOObjContext(SeisPS2D) );
    BufferStringSet lsnms;
    S2DPOS().getLineSets( lsnms );
    for ( int idx=0; idx<olddel.size(); idx++ )
    {
	const IOObj& psobj = olddel.ioobj( idx );
	const BufferString psdir = psobj.mainFileName();
	if ( psdir.isEmpty() || !File::isDirectory(psdir) )
	    continue;

	const DirList flist( psdir, File::FilesInDir, "*.*" );
	for ( int fidx=0; fidx<flist.size(); fidx++ )
	{
	    const File::Path fp( flist.fullPath(fidx) );
	    const BufferString lnm = fp.baseName();
	    if ( lnm.contains('^') )
		continue;	//Already converted.

	    Pos::GeomID geomid = SurvGeom::getGeomID( lnm );
	    int lsidx=0;
	    while( mIsUdfGeomID(geomid) && lsidx<lsnms.size() )
	    {
		const BufferString oldlnm( lsnms.get(lsidx++), "-", lnm );
		geomid = SurvGeom::getGeomID( oldlnm );
	    }
	    if ( mIsUdfGeomID(geomid) )
		continue;

	    File::Path newfp( psdir );
	    const BufferString newfnm( newfp.fileName(), "^",
				       toString(geomid) );
	    newfp.add( newfnm );
	    newfp.setExtension( fp.extension(), false );
	    File::rename( fp.fullPath().buf(), newfp.fullPath().buf() );
	}
    }
}


static void convertSeis2DTranslators()
{
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    if ( !dbdir )
	return;

    ObjectSet<IOObj> newioobjs;
    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( ioobj.translator() == TwoDDataSeisTrcTranslator::translKey() )
	{
	    PtrMan<IOObj> duplobj = dbdir->getEntryByName( ioobj.name(),
						mTranslGroupName(SeisTrc2D) );
	    if ( duplobj )
		continue;

	    IOObj* edioobj = ioobj.clone();
	    edioobj->setGroup( mTranslGroupName(SeisTrc2D) );
	    edioobj->setTranslator( CBVSSeisTrc2DTranslator::translKey() );
	    newioobjs += edioobj;
	}
    }
    iter.retire(); // essential: lifts lock on dbdir

    for ( int idx=0; idx<newioobjs.size(); idx++ )
	newioobjs[idx]->commitChanges();
}


static const char* getSurvDefAttrName()
{
    mDefineStaticLocalObject( BufferString, ret,
	   = SI().getDefaultPars().find(IOPar::compKey(sKey::Default(),
			SeisTrcTranslatorGroup::sKeyDefaultAttrib())) );
    if ( ret.isEmpty() )
	ret = "Seis";
    return ret.buf();
}


class OD_FileListCopier : public Executor
{ mODTextTranslationClass(OD_FileListCopier);
public:
OD_FileListCopier( const BufferStringSet& fromlist,
		   const BufferStringSet& tolist, uiString& errmsg )
    : Executor( "2D data conversion" )
    , fromlist_(fromlist), tolist_(tolist)
    , errmsg_(errmsg),curidx_(0)
{
}

od_int64 totalNr() const	{ return mCast(od_int64,fromlist_.size()); }
od_int64 nrDone() const		{ return mCast(od_int64,curidx_); }
uiString nrDoneText() const	{ return tr("Nr files done"); }
uiString message() const	{ return tr("Converting 2D Seismic data"); }

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

    File::Path srcfp( src );
    File::Path destfp( dest );
    srcfp.setExtension( sParFileExtension() );
    destfp.setExtension( sParFileExtension() );
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
    uiString&			errmsg_;
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
    bool		    update2DSFilesAndAddToDelList(
						    ObjectSet<IOObj>& ioobjlist,
						    BufferStringSet&);
    void		    removeDuplicateData(BufferStringSet&);


    BufferString	    getAttrFolderPath(IOPar&) const;


    ObjectSet<ObjectSet<IOPar> > all2dseisiopars_;
};


/* 0=No old 2D data, 1=First time conversion, 2=Incremental conversion. */
mGlobal(Seis) int OD_Get_2D_Data_Conversion_Status()
{
    bool hasold2d = false;
    bool has2dps = false;
    if ( DBM().isBad() )
	return 0;

    convertSeis2DTranslators();
    IOObjContext oldctxt( mIOObjContext(SeisTrc) );
    oldctxt.fixTranslator( TwoDSeisTrcTranslator::translKey() );
    oldctxt.toselect_.allownonuserselectable_ = true;
    const DBDirEntryList olddel( oldctxt );
    if ( !olddel.isEmpty() )
	hasold2d = true;

    IOObjContext psctxt( mIOObjContext(SeisPS2D) );
    psctxt.fixTranslator( CBVSSeisPS2DTranslator::translKey() );
    const DBDirEntryList psdel( psctxt );
    if ( !psdel.isEmpty() )
	has2dps = true;

    if ( !hasold2d && !has2dps )
	return 0;

    File::Path geom2dfp( DBM().survDir(), "2DGeom", "idx.txt" );
    if ( !has2dps && !File::exists(geom2dfp.fullPath()) )
	return 3; //TODO: Pre 4.2 surveys, extract geometry from cbvs.

    IOObjContext newctxt( mIOObjContext(SeisTrc2D) );
    newctxt.toselect_.allowtransls_ = CBVSSeisTrc2DTranslator::translKey();
    const DBDirEntryList newdel( newctxt );
    return hasold2d && newdel.isEmpty() ? 1 : 2;
}


mGlobal(Seis) void OD_Convert_2DLineSets_To_2DDataSets( uiString& errmsg,
							TaskRunner* taskrnr )
{
    OD_2DLineSetTo2DDataSetConverter converter;
    converter.doConversion( errmsg, taskrnr );
}


OD_2DLineSetTo2DDataSetConverter::~OD_2DLineSetTo2DDataSetConverter()
{
    deepErase( all2dseisiopars_ );
}


void OD_2DLineSetTo2DDataSetConverter::doConversion( uiString& errmsg,
						     TaskRunner* taskrnr )
{
    convert2DPSData();
    ObjectSet<IOObj> all2dsfiles;
    makeListOfLineSets( all2dsfiles );
    fillIOParsFrom2DSFile( all2dsfiles );
    BufferStringSet filepathsofold2ddata, filestobedeleted;
    getCBVSFilePaths( filepathsofold2ddata );
    if ( !copyData(filepathsofold2ddata,errmsg,taskrnr) ||
	    !update2DSFilesAndAddToDelList(all2dsfiles,filestobedeleted) )
    {
	errmsg = tr("Failed to update 2D database. Most probably"
		    " the survey or its 'Seismics' folder are not writable");
	return;
    }

    removeDuplicateData( filestobedeleted );
}


void OD_2DLineSetTo2DDataSetConverter::makeListOfLineSets(
						   ObjectSet<IOObj>& ioobjlist )
{
    IOObjContext oldctxt( mIOObjContext(SeisTrc) );
    oldctxt.fixTranslator( TwoDSeisTrcTranslator::translKey() );
    oldctxt.toselect_.allownonuserselectable_ = true;
    const DBDirEntryList olddel( oldctxt );
    if ( olddel.isEmpty() )
	return;

    for ( int idx=0; idx<olddel.size(); idx++ )
    {
	IOObj* ioobj = olddel.ioobj( idx ).clone();
	ioobjlist += ioobj;
	ObjectSet<IOPar>* obset = new ObjectSet<IOPar>();
	all2dseisiopars_ += obset;
    }

    return;
}

static const char* sKeyLSFileType = "2D Line Group Data";
static void read2DSFile( const IOObj& lsobj, ObjectSet<IOPar>& pars )
{
    SafeFileIO sfio( lsobj.mainFileName(), true );
    if ( !sfio.open(true) )
	return;

    ascistream astrm( sfio.istrm(), true );
    if ( !astrm.isOfFileType(sKeyLSFileType) )
    {
	sfio.closeSuccess( false );
	return;
    }

    while ( !atEndOfSection(astrm.next()) )
    {}

    while ( astrm.type() != ascistream::EndOfFile )
    {
	IOPar* newpar = new IOPar;
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.hasKeyword(sKey::Name()) )
	    {
		newpar->setName( astrm.value() );
		const BufferString oldlnm( lsobj.name(), "-", astrm.value() );
		newpar->set( sKey::GeomID(),
			     SurvGeom::getGeomID( oldlnm ) );
	    }
	    else if ( !astrm.hasValue("") )
		newpar->set( astrm.keyWord(), astrm.value() );
	}

	if ( newpar->size() < 2 )
	{
	    delete newpar;
	    continue;
	}

	pars += newpar;
    }

    sfio.closeSuccess( false );
}


void OD_2DLineSetTo2DDataSetConverter::fillIOParsFrom2DSFile(
					    const ObjectSet<IOObj>& ioobjlist )
{
    for ( int idx=0; idx<ioobjlist.size(); idx++ )
	read2DSFile( *ioobjlist[idx], *all2dseisiopars_[idx] );
}


BufferString OD_2DLineSetTo2DDataSetConverter::getAttrFolderPath(
							IOPar& iop ) const
{
    CtxtIOObj ctio( mIOObjContext(SeisTrc2D) );
    ctio.ctxt_.deftransl_ = CBVSSeisTrc2DTranslator::translKey();
    if ( iop.find(sKey::DataType()) )
    {
	BufferString datatype, zdomain;
	iop.get( sKey::DataType(), datatype );
	iop.get( ZDomain::sKey(), zdomain );
	ctio.ctxt_.toselect_.require_.set( sKey::Type(), datatype );
	ctio.ctxt_.toselect_.require_.set( ZDomain::sKey(), zdomain );
    }

    FixedString attribnm = iop.find( sKey::Attribute() );
    if ( attribnm.isEmpty() ) attribnm = "Seis";
    ctio.ctxt_.setName( attribnm );
    if ( ctio.fillObj() == 0 ) return BufferString::empty();
    IOObj* ioobj = ctio.ioobj_;
    if ( ioobj->group() != mTranslGroupName(SeisTrc2D) )
    {
	BufferString nm = ioobj->name();
	nm.add( "[2D]" );
	ctio.setObj( 0 );
	ctio.ctxt_.setName( nm );
	if ( ctio.fillObj() == 0 ) return BufferString::empty();
	File::Path fp( ctio.ioobj_->mainFileName() );
	nm = fp.fileName();
	iop.set( sKey::Attribute(), nm.buf() );
    }

    const FixedString survdefattr( getSurvDefAttrName() );
    const bool issurvdefset =
	SI().getDefaultPars().find( IOPar::compKey(sKey::Default(),
				SeisTrc2DTranslatorGroup::sKeyDefault()) );
    if ( !issurvdefset && ctio.ioobj_ && survdefattr == attribnm )
	ctio.ioobj_->setSurveyDefault();

    BufferString ret( ctio.ioobj_ ? ctio.ioobj_->mainFileName() : "" );
    ctio.setObj( 0 );
    return ret;
}


void OD_2DLineSetTo2DDataSetConverter::getCBVSFilePaths(
						    BufferStringSet& filepaths )
{
    for ( int idx=0; idx<all2dseisiopars_.size(); idx++ )
    {
	for ( int lineidx=0; lineidx<all2dseisiopars_[idx]->size(); lineidx++ )
	{
	    BufferString fnm;
	    (*all2dseisiopars_[idx])[lineidx]->get( sKey::FileName(), fnm );
	    File::Path oldfp( IOObjContext::getDataDirName(IOObjContext::Seis) );
	    oldfp.add( fnm );
	    filepaths.add( oldfp.fullPath() );
	}
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
	    File::Path oldfp( oldfilepaths.get(numberoflines) );
	    numberoflines++;
	    if ( !File::exists(oldfp.fullPath()) )
		continue;

	    File::Path newfp( getAttrFolderPath( *iop ) );
	    File::createDir( newfp.fullPath() );
	    BufferString newfn( newfp.fileName() );
	    newfn.add( mCapChar );
	    Pos::GeomID geomid;
	    if ( !iop->get(sKey::GeomID(),geomid) || !geomid.is2D() )
		continue;

	    newfn.add( geomid );
	    newfp.add( newfn ).setExtension( oldfp.extension(), false );
	    if ( oldfp == newfp )
		continue;

	    srclist.add( oldfp.fullPath() );
	    destlist.add( newfp.fullPath() );
	}
    }

    if ( srclist.isEmpty() )
	return true;

    uiString msg;
    OD_FileListCopier exec( srclist, destlist, msg );
    const bool res = TaskRunner::execute( taskrnr, exec );
    if ( !res )
	errmsg = tr("Error while converting 2D Seismic data").addMoreInfo(msg);

    return res;
}

static bool write2DSFile( const IOObj& lsobj, const ObjectSet<IOPar>& pars )
{
    SafeFileIO sfio( lsobj.mainFileName(), true );
    if ( !sfio.open(false,true) )
	return false;

    ascostream astrm( sfio.ostrm() );
    if ( !astrm.putHeader(sKeyLSFileType) )
    {
	sfio.closeSuccess( false );
	return false;
    }

    astrm.put( sKey::Name(), lsobj.name() );
    astrm.put( sKey::Type(), CBVSSeisTrc2DTranslator::translKey() );
    astrm.put( "Number of lines", pars.size() );
    astrm.newParagraph();

    for ( int ipar=0; ipar<pars.size(); ipar++ )
    {
	const IOPar& iopar = *pars[ipar];
	astrm.put( sKey::Name(), iopar.name() );
	for ( int idx=0; idx<iopar.size(); idx++ )
	{
	    const char* val = iopar.getValue(idx);
	    if ( !val || !*val ) continue;
	    astrm.put( iopar.getKey(idx), iopar.getValue(idx) );
	}

	astrm.newParagraph();
    }

    sfio.closeSuccess();
    return true;
}


bool OD_2DLineSetTo2DDataSetConverter::update2DSFilesAndAddToDelList(
		ObjectSet<IOObj>& ioobjlist,BufferStringSet& filestobedeleted )
{
    for ( int idx=0; idx<ioobjlist.size(); idx++ )
    {
	ObjectSet<IOPar>& lspars = *all2dseisiopars_[idx];
	for ( int lineidx=0; lineidx<lspars.size(); lineidx++ )
	{
	    IOPar& iop = *lspars[lineidx];
	    BufferString fnm;
	    iop.get( sKey::FileName(), fnm );
	    File::Path oldfp( IOObjContext::getDataDirName(IOObjContext::Seis) );
	    oldfp.add( fnm );
	    const BufferString oldfullfnm( oldfp.fullPath() );
	    BufferString attrname;
	    iop.get(sKey::Attribute(), attrname);
	    if ( attrname.isEmpty() ) attrname = "Seis";
	    attrname.clean( BufferString::AllowDots );
	    BufferString newfile( attrname );
	    const BufferString oldlnm( ioobjlist[idx]->name(), "-", iop.name());
	    newfile.add(mCapChar).add( SurvGeom::getGeomID( oldlnm ) );
	    File::Path newfp( IOObjContext::getDataDirName(IOObjContext::Seis),
			    attrname, newfile );
	    newfp.setExtension( oldfp.extension(), false );
	    const BufferString newfullfnm = newfp.fullPath();
	    if ( newfullfnm == oldfullfnm || !File::exists(newfullfnm.buf()) )
		continue;

	    File::Path newfnm( attrname );
	    newfnm.add( newfile );
	    newfnm.setExtension( oldfp.extension(), false );
	    iop.set( sKey::FileName(), newfnm.fullPath(File::Path::Unix) );
	    if ( File::exists(oldfullfnm) )
		filestobedeleted.add( oldfullfnm );
	}

	if ( !write2DSFile(*ioobjlist[idx],lspars) )
	    return false;
    }

    return true;
}


void OD_2DLineSetTo2DDataSetConverter::removeDuplicateData(
						  BufferStringSet& oldfilepath )
{
    for ( int idx=0; idx<oldfilepath.size(); idx++ )
    {
	File::Path fp( oldfilepath.get(idx) );
	File::remove( fp.fullPath() );
	fp.setExtension( sParFileExtension() );
	File::remove( fp.fullPath() );
    }
}
