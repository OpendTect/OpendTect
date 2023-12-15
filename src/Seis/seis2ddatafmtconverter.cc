/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ascstream.h"
#include "bufstringset.h"
#include "dirlist.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2dsurv.h"
#include "typeset.h"
#include "safefileio.h"
#include "seiscbvs2d.h"
#include "seisioobjinfo.h"
#include "seispsioprov.h"
#include "survgeom.h"
#include "survinfo.h"
#include <iostream>
#define mCapChar "^"

static void convert2DPSData()
{
    IOObjContext oldctxt( mIOObjContext(SeisPS2D) );
    const IODir oldiodir( oldctxt.getSelKey() );
    const IODirEntryList olddel( oldiodir, oldctxt );
    BufferStringSet lsnms;
    S2DPOS().getLineSets( lsnms );
    for ( int idx=0; idx<olddel.size(); idx++ )
    {
	const IOObj* psobj = olddel[idx]->ioobj_;
	if ( !psobj ) continue;
	const StringView psdir = psobj->fullUserExpr( true );
	if ( psdir.isEmpty() || !File::isDirectory(psdir) )
	    continue;

	const DirList flist( psdir, File::FilesInDir, "*.*" );
	for ( int fidx=0; fidx<flist.size(); fidx++ )
	{
	    const FilePath fp( flist.fullPath(fidx) );
	    const BufferString lnm = fp.baseName();
	    if ( lnm.contains('^') )
		continue;	//Already converted.

	    Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	    int lsidx=0;
	    while( geomid==Survey::GM().cUndefGeomID() && lsidx<lsnms.size() )
		geomid = Survey::GM().getGeomID(lsnms.get(lsidx++), lnm.buf());

	    if ( geomid==Survey::GM().cUndefGeomID() )
		continue;

	    FilePath newfp( psdir );
	    const BufferString newfnm( newfp.fileName(), "^",
				       toString(geomid.asInt()) );
	    newfp.add( newfnm );
	    newfp.setExtension( fp.extension(), false );
	    File::rename( fp.fullPath().buf(), newfp.fullPath().buf() );
	}
    }
}


static void convertSeis2DTranslators()
{
    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    IODir iodir( ctxt.getSelKey() );
    const ObjectSet<IOObj>& oldobjs = iodir.getObjs();
    for ( int idx=0; idx<oldobjs.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = oldobjs[idx] ? oldobjs[idx]->clone() : 0;
	if ( !ioobj ) continue;

	if ( ioobj->translator() == TwoDDataSeisTrcTranslator::translKey() )
	{
	    PtrMan<IOObj> duplobj = IOM().getLocal( ioobj->name().buf(),
						mTranslGroupName(SeisTrc2D) );
	    if ( duplobj )
		continue;

	    ioobj->setGroup( mTranslGroupName(SeisTrc2D) );
	    ioobj->setTranslator( CBVSSeisTrc2DTranslator::translKey() );
	    iodir.commitChanges( ioobj );
	}
    }

    IOM().toRoot();
}


static const BufferString getSurvDefAttrName()
{
    mDefineStaticLocalObject( BufferString, ret,
	   = SI().getPars().find(IOPar::compKey(sKey::Default(),
			SeisTrcTranslatorGroup::sKeyDefaultAttrib())) );
    if ( ret.isEmpty() )
	ret = "Seis";

    return ret;
}


class OD_FileListCopier : public Executor
{ mODTextTranslationClass(OD_FileListCopier);
public:
OD_FileListCopier( const BufferStringSet& fromlist,
		   const BufferStringSet& tolist, BufferString& errmsg )
    : Executor( "2D data conversion" )
    , fromlist_(fromlist), tolist_(tolist)
    , errmsg_(errmsg),curidx_(0)
{
}

od_int64 totalNr() const override
{
    return mCast(od_int64,fromlist_.size());
}

od_int64 nrDone() const override	{ return mCast(od_int64,curidx_); }
uiString uiNrDoneText() const override	{ return tr("Nr files done"); }

uiString uiMessage() const override
{
    return tr("Converting 2D Seismic data");
}

int nextStep() override
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

    bool		    makeListOfLineSets(ObjectSet<IOObj>&);
    void		    fillIOParsFrom2DSFile(const ObjectSet<IOObj>&);
    void		    getCBVSFilePaths(BufferStringSet&);
    bool		    copyData(BufferStringSet&,uiString&,TaskRunner*);
    bool		    removeLineSetsAndAddFilesToDelList(
						ObjectSet<IOObj>& ioobjlist,
						BufferStringSet&);
    void		    removeDuplicateData(BufferStringSet&);


    BufferString	    getAttrFolderPath(IOPar&) const;


    ObjectSet<ObjectSet<IOPar> > all2dseisiopars_;
};


/* 0=No conversion needed, 1=First time conversion, 2=Incremental conversion. */
mGlobal(Seis) int Seis_Get_2D_Data_Conversion_Status()
{
    if ( !SI().has2D() )
       return 0;

    bool islegacy = false;
    if ( !Survey::GM().has2D() )
    {
	//Check Seismics omf version
	const FilePath seisomffilepath(
		IOObjContext::getDataDirName(IOObjContext::Seis), ".omf" );
	od_istream omfstrm( seisomffilepath.fullPath() );
	ascistream astrm( omfstrm );
	if ( astrm.majorVersion()<4 ||
		(astrm.majorVersion()==4 && astrm.minorVersion()<2) )
	    islegacy = true;
	else
	    return 0; // There are no 2D data.
    }

    const FilePath old2dgeomfp( IOM().rootDir(), "2DGeom", "idx.txt" );
    if ( !islegacy && !File::exists(old2dgeomfp.fullPath()) )
       return 0; // All required conversions have already been done.

    bool hasold2d = false;
    bool has2dps = false;
    convertSeis2DTranslators();
    IOObjContext oldctxt( mIOObjContext(SeisTrc) );
    oldctxt.fixTranslator( TwoDSeisTrcTranslator::translKey() );
    oldctxt.toselect_.allownonuserselectable_ = true;
    const IODir oldiodir( oldctxt.getSelKey() );
    if ( !oldiodir.isBad() )
    {
	const IODirEntryList olddel( oldiodir, oldctxt );
	if ( !olddel.isEmpty() )
	    hasold2d = true;
    }

    IOObjContext psctxt( mIOObjContext(SeisPS2D) );
    psctxt.fixTranslator( CBVSSeisPS2DTranslator::translKey() );
    const IODir psiodir( psctxt.getSelKey() );
    if ( !psiodir.isBad() )
    {
	const IODirEntryList psdel( psiodir, psctxt );
	if ( !psdel.isEmpty() )
	    has2dps = true;
    }

    if ( !hasold2d && !has2dps )
	return 0;

    if ( islegacy )
	return 3; // Has unsupported 2D data.

    IOObjContext newctxt( mIOObjContext(SeisTrc2D) );
    newctxt.toselect_.allowtransls_ = CBVSSeisTrc2DTranslator::translKey();
    const IODir newiodir( newctxt.getSelKey() );
    const IODirEntryList newdel( newiodir, newctxt );
    return hasold2d && newdel.isEmpty() ? 1 : 2;
}

mGlobal(Seis) void Seis_Convert_2DLineSets_To_2DDataSets( uiString& errmsg,
							  TaskRunner* taskrnr )
{
    OD_2DLineSetTo2DDataSetConverter converter;
    converter.doConversion( errmsg, taskrnr );
}


OD_2DLineSetTo2DDataSetConverter::~OD_2DLineSetTo2DDataSetConverter()
{
    for ( int idx=0; idx<all2dseisiopars_.size(); idx++ )
	deepErase( *all2dseisiopars_[idx] );
    deepErase( all2dseisiopars_ );
}


void OD_2DLineSetTo2DDataSetConverter::doConversion( uiString& errmsg,
						     TaskRunner* taskrnr )
{
    convert2DPSData();
    ObjectSet<IOObj> all2dsfiles;
    if ( !makeListOfLineSets(all2dsfiles) )
	return; // Nothing left to convert

    fillIOParsFrom2DSFile( all2dsfiles );
    BufferStringSet filepathsofold2ddata, filestobedeleted;
    getCBVSFilePaths( filepathsofold2ddata );
    if ( !copyData(filepathsofold2ddata,errmsg,taskrnr) ||
	    !removeLineSetsAndAddFilesToDelList(all2dsfiles,filestobedeleted) )
    {
	errmsg = tr( "Failed to update 2D database. Most probably"
		     " the survey or its 'Seismics' folder is not writable" );
	return;
    }

    removeDuplicateData( filestobedeleted );
}


bool OD_2DLineSetTo2DDataSetConverter::makeListOfLineSets(
						   ObjectSet<IOObj>& ioobjlist )
{
    IOObjContext oldctxt( mIOObjContext(SeisTrc) );
    oldctxt.fixTranslator( TwoDSeisTrcTranslator::translKey() );
    oldctxt.toselect_.allownonuserselectable_ = true;
    const IODir oldiodir( oldctxt.getSelKey() );
    const IODirEntryList olddel( oldiodir, oldctxt );
    if ( olddel.isEmpty() )
	return false;

    for ( int idx=0; idx<olddel.size(); idx++ )
    {
	IOObj* ioobj = olddel[idx]->ioobj_ ? olddel[idx]->ioobj_->clone() : 0;
	if ( !ioobj ) continue;
	ioobjlist += ioobj;
	ObjectSet<IOPar>* obset = new ObjectSet<IOPar>();
	all2dseisiopars_ += obset;
    }

    return !all2dseisiopars_.isEmpty();
}

static const char* sKeyLSFileType = "2D Line Group Data";
static void read2DSFile( const IOObj& lsobj, ObjectSet<IOPar>& pars )
{
    SafeFileIO sfio( lsobj.fullUserExpr(), true );
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
		newpar->set( sKey::GeomID(),
			Survey::GM().getGeomID(lsobj.name(),astrm.value()) );
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
    const IOObjContext& iocontext = mIOObjContext(SeisTrc2D);
    if ( !IOM().to(iocontext.getSelKey()) )
	return BufferString::empty();

    CtxtIOObj ctio( iocontext );
    ctio.ctxt_.deftransl_ = CBVSSeisTrc2DTranslator::translKey();
    if ( iop.hasKey(sKey::DataType()) )
    {
	BufferString datatype;
	if ( iop.get(sKey::DataType(),datatype) && !datatype.isEmpty() )
	    ctio.ctxt_.requireType( datatype );

	const ZDomain::Info* zinfo = ZDomain::get( iop );
	if ( zinfo )
	    ctio.ctxt_.requireZDomain( *zinfo, *zinfo == SI().zDomainInfo() );
    }

    BufferString attribnm = iop.find( sKey::Attribute() );
    if ( attribnm.isEmpty() )
	attribnm = "Seis";

    ctio.ctxt_.setName( attribnm );
    if ( ctio.fillObj() == 0 )
	return BufferString::empty();

    IOObj* ioobj = ctio.ioobj_;
    if ( ioobj->group() != mTranslGroupName(SeisTrc2D) )
    {
	BufferString nm = ioobj->name();
	nm.add( "[2D]" );
	ctio.setObj( 0 );
	ctio.ctxt_.setName( nm );
	if ( ctio.fillObj() == 0 )
	    return BufferString::empty();

	FilePath fp( ctio.ioobj_->fullUserExpr() );
	nm = fp.fileName();
	iop.set( sKey::Attribute(), nm.buf() );
    }

    const StringView survdefattr( getSurvDefAttrName().buf() );
    const bool issurvdefset = SI().pars().hasKey(
				IOPar::compKey(sKey::Default(),
				SeisTrc2DTranslatorGroup::sKeyDefault()) );
    if ( !issurvdefset && ctio.ioobj_ && survdefattr == attribnm )
	ctio.ioobj_->setSurveyDefault();

    BufferString ret( ctio.ioobj_ ? ctio.ioobj_->fullUserExpr() : "" );
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
	    FilePath oldfp( IOObjContext::getDataDirName(IOObjContext::Seis) );
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
	    FilePath oldfp( oldfilepaths.get(numberoflines) );
	    numberoflines++;
	    if ( !File::exists(oldfp.fullPath()) )
		continue;

	    FilePath newfp( getAttrFolderPath( *iop ) );
	    File::createDir( newfp.fullPath() );
	    BufferString newfn( newfp.fileName() );
	    newfn.add( mCapChar );
	    Pos::GeomID geomid;
	    if ( !iop->get(sKey::GeomID(),geomid) || !geomid.isValid() )
		continue;

	    newfn.add( geomid.asInt() );
	    newfp.add( newfn ).setExtension( oldfp.extension(), false );

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


bool OD_2DLineSetTo2DDataSetConverter::removeLineSetsAndAddFilesToDelList(
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
	    FilePath oldfp( IOObjContext::getDataDirName(IOObjContext::Seis) );
	    oldfp.add( fnm );
	    const BufferString oldfullfnm( oldfp.fullPath() );
	    BufferString attrname;
	    iop.get(sKey::Attribute(), attrname);
	    if ( attrname.isEmpty() ) attrname = "Seis";
	    attrname.clean( BufferString::AllowDots );
	    BufferString newfile( attrname );
	    const Pos::GeomID geomid =
			Survey::GM().getGeomID( ioobjlist[idx]->name(),
						iop.name() );
	    newfile.add(mCapChar).add( geomid.asInt() );

	    FilePath newfp( IOObjContext::getDataDirName(IOObjContext::Seis),
			    attrname, newfile );
	    newfp.setExtension( oldfp.extension(), false );
	    const BufferString newfullfnm = newfp.fullPath();
	    if ( newfullfnm != oldfullfnm && File::exists(newfullfnm.buf()) &&
		    File::exists(oldfullfnm) )
		filestobedeleted.add( oldfullfnm );
	}

	ioobjlist[idx]->implRemove();
	IOM().permRemove( ioobjlist[idx]->key() );
    }

    return true;
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
}
