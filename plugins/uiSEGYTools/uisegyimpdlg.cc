/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegyimpdlg.h"

#include "ui2dgeomman.h"
#include "uisegydef.h"
#include "uiseistransf.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uiseparator.h"
#include "uifileinput.h"
#include "uibatchjobdispatchersel.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "segyhdr.h"
#include "seisioobjinfo.h"
#include "seisimporter.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "file.h"
#include "dirlist.h"
#include "ioman.h"
#include "iostrm.h"
#include "zdomain.h"
#include "segybatchio.h"
#include "keystrs.h"
#include "od_helpids.h"



uiSEGYImpDlg::uiSEGYImpDlg( uiParent* p,
			const uiSEGYReadDlg::Setup& su, IOPar& iop )
    : uiSEGYReadDlg(p,su,iop)
    , morebut_(0)
    , batchfld_(0)
{
    BufferString ttl( setup().dlgtitle_.getFullString() );
    if ( ttl.isEmpty() )
    {
	ttl.set( "Import " ).add( Seis::nameOf(setup_.geom_) );
	FileSpec fs; fs.usePar( iop );
	ttl.add( " " ).add( getLimitedDisplayString(fs.dispName(),40,0) );
    }
    setTitleText( tr(ttl) );

    uiSeparator* sep = optsfld_ ? new uiSeparator( this, "Hor sep" ) : 0;

    uiGroup* outgrp = new uiGroup( this, "Output group" );
    transffld_ = new uiSeisTransfer( outgrp, uiSeisTransfer::Setup(setup_.geom_)
				    .withnullfill(false)
				    .fornewentry(true) );
    outgrp->setHAlignObj( transffld_ );
    if ( sep )
    {
	sep->attach( stretchedBelow, optsfld_ );
	outgrp->attach( alignedBelow, optsfld_ );
	outgrp->attach( ensureBelow, sep );
    }

    uiSeisSel::Setup sssu( setup_.geom_ ); sssu.enabotherdomain( true );
    IOObjContext ctxt( uiSeisSel::ioContext( su.geom_, false ) );
    if ( su.geom_ != Seis::Line )
	ctxt.fixTranslator( "CBVS" );
    seissel_ = new uiSeisSel( outgrp, ctxt, sssu );
    seissel_->attach( alignedBelow, transffld_ );

    if ( setup_.geom_ != Seis::Line )
    {
	batchfld_ = new uiBatchJobDispatcherSel( outgrp, true,
						 Batch::JobSpec::SEGY );
	batchfld_->setJobName( "import SEG-Y" );
	Batch::JobSpec& js = batchfld_->jobSpec();
	js.pars_.set( SEGY::IO::sKeyTask(), SEGY::IO::sKeyImport() );
	js.pars_.setYN( SEGY::IO::sKeyIs2D(), Seis::is2D(setup_.geom_) );
	batchfld_->attach( alignedBelow, seissel_ );
    }
    else
    {
	morebut_ = new uiCheckBox( outgrp, tr("Import more, similar files") );
	morebut_->attach( alignedBelow, seissel_ );
    }

    if ( !optsfld_ )
    {
	uiToolButton* tb = new uiToolButton( this, "prescan",
				tr("Pre-scan file(s)"),
				mCB(this,uiSEGYImpDlg,preScanCB) );
	tb->attach( rightOf, outgrp->attachObj() );
    }
}


void uiSEGYImpDlg::use( const IOObj* ioobj, bool force )
{
    uiSEGYReadDlg::use( ioobj, force );
    if ( ioobj )
	transffld_->updateFrom( *ioobj );
}


MultiID uiSEGYImpDlg::outputID() const
{
    return seissel_->key( true );
}


class uiSEGYImpSimilarDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYImpSimilarDlg)
public:

uiSEGYImpSimilarDlg( uiSEGYImpDlg* p, const IOObj& iio, const IOObj& oio )
	: uiDialog(p,uiDialog::Setup(tr("2D SEG-Y multi-import"),
				     tr("Specify file details"),
                                     mODHelpKey(mSEGYImpSimilarDlgHelpID) ))
	, inioobj_(iio)
	, outioobj_(oio)
	, impdlg_(p)
{
    const BufferString fnm( inioobj_.fullUserExpr(true) );
    FilePath fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Imp "); setupnm += uiSEGYFileSpec::sKeyLineNmToken();

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken() );
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    uiString txt( tr("Input ('%1' will become line name)")
                                    .arg(uiSEGYFileSpec::sKeyLineNmToken()) );
    uiFileInput::Setup fisu( fp.fullPath() );
    fisu.forread( true ).objtype( uiStrings::sSEGY() );
    fnmfld_ = new uiFileInput( this, txt, fisu );
}


bool acceptOK( CallBacker* )
{
    BufferString fnm = fnmfld_->fileName();
    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    if ( !File::isDirectory(dirnm) )
    {
	uiMSG().error( tr("Directory provided not usable") );
	return false;
    }
    fnm = fp.fullPath();
    if ( !fnm.contains(uiSEGYFileSpec::sKeyLineNmToken()) )
    {
	uiString msg = tr("The file name has to contain at least one '%1'\n"
			  "That will then become the line name")
		     .arg( uiSEGYFileSpec::sKeyLineNmToken());
	uiMSG().error( msg );
	return false;
    }

    IOM().to( outioobj_.key() );
    return doImp( fp );
}


IOObj* getSubstIOObj( const char* fullfnm )
{
    IOObj* newioobj = inioobj_.clone();
    newioobj->setName( fullfnm );
    mDynamicCastGet(IOStream*,iostrm,newioobj)
    iostrm->fileSpec().setFileName( fullfnm );
    return newioobj;
}


bool doWork( IOObj* newioobj, const char* lnm, bool islast, bool& nofails )
{
    bool res = impdlg_->impFile( *newioobj, outioobj_, lnm );
    delete newioobj;
    if ( !res )
    {
	nofails = false;
	if ( !islast && !uiMSG().askContinue(tr("Continue with next?")) )
	    return false;
    }
    return true;
}


bool doImp( const FilePath& fp )
{
    BufferString mask( fp.fileName() );
    mask.replace( uiSEGYFileSpec::sKeyLineNmToken(), "*" );
    FilePath maskfp( fp ); maskfp.setFileName( mask );
    const int nrtok = mask.count( '*' );
    DirList dl( fp.pathOnly(), File::FilesInDir, mask );
    if ( dl.size() < 1 )
    {
	uiMSG().error( tr("Cannot find any match for file name") );
	return false;
    }

    BufferString fullmaskfnm( maskfp.fullPath() );
    int lnmoffs = mCast( int, firstOcc( fullmaskfnm.buf(), '*' ) -
	                                   fullmaskfnm.buf() );
    const int orglen = fullmaskfnm.size();
    bool nofails = true;

    bool overwrite = false;
    bool overwritequestionasked = false;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString dirlistfnm( dl.get(idx) );
	FilePath newfp( maskfp );
	newfp.setFileName( dirlistfnm );
	const BufferString fullfnm( newfp.fullPath() );
	const int newlen = fullfnm.size();
	const int lnmlen = (newlen - orglen + 1) / nrtok;
	BufferString lnm( fullfnm.buf() + lnmoffs );
	*(lnm.getCStr() + lnmlen) = '\0';

	Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	if ( geomid != Survey::GeometryManager::cUndefGeomID() )
	{
	    if ( !overwritequestionasked )
	    {
		overwrite = uiMSG().askGoOn( tr("Do you want to overwrite the "
						"Geometry of the lines?") );
		overwritequestionasked = true;
	    }

	    if ( overwrite )
	    {
		Survey::Geometry* geom = Survey::GMAdmin().getGeometry(geomid );
		mDynamicCastGet(Survey::Geometry2D*,geom2d,geom);
		if ( geom2d ) geom2d->dataAdmin().setEmpty();
	    }

	}

	IOObj* newioobj = getSubstIOObj( fullfnm );
	if ( !doWork( newioobj, lnm, idx > dl.size()-2, nofails ) )
	    return false;
    }

    return nofails;
}

    uiFileInput*	fnmfld_;
    uiSEGYImpDlg*	impdlg_;

    const IOObj&	inioobj_;
    const IOObj&	outioobj_;

};



bool uiSEGYImpDlg::doWork( const IOObj& inioobj )
{
    const IOObj* outioobj = seissel_->ioobj();
    if ( !outioobj )
	return false;

    const bool is2d = Seis::is2D( setup_.geom_ );
    const BufferString lnm = transffld_->selectedLine();
    if ( is2d && lnm.isEmpty() )
    {
	uiMSG().error( tr("Linename is empty. Please enter a line name") );
	return false;
    }

    PtrMan<IOObj> useinioobj = inioobj.clone();
    if ( SeisStoreAccess::zDomain(outioobj).fillPar(useinioobj->pars()) )
	IOM().commitChanges( *useinioobj );

    bool retval;
    if ( !morebut_ || !morebut_->isChecked() )
    {
	Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	if ( is2d && Survey::is2DGeom(geomid) )
	{
	    const bool overwrite =
		uiMSG().askGoOn( tr("Geometry of Line '%1' is already present."
				    "\n\nDo you want to overwrite?").arg(lnm) );
	    if ( overwrite )
	    {
		Survey::Geometry* geom = Survey::GMAdmin().getGeometry(geomid );
		mDynamicCastGet(Survey::Geometry2D*,geom2d,geom);
		if ( geom2d )
		    geom2d->dataAdmin().setEmpty();
	    }
	}

	retval = impFile( *useinioobj, *outioobj, lnm );
	if ( is2d && retval )
	    uiMSG().message(tr("Successfully loaded %1")
			  .arg(useinioobj->fullUserExpr()));
    }
    else
    {
	uiSEGYImpSimilarDlg dlg( this, *useinioobj, *outioobj );
	retval = dlg.go();
    }

    return retval;
}


bool uiSEGYImpDlg::impFile( const IOObj& inioobj, const IOObj& outioobj,
			    const char* linenm )
{
    const bool isps = Seis::isPS( setup_.geom_ );
    const bool is2d = Seis::is2D( setup_.geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo;
    if ( !isps )
    {
	ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
	if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	    return false;
    }

    if ( batchfld_ && batchfld_->wantBatch() )
    {
	Batch::JobSpec& js = batchfld_->jobSpec();
	js.pars_.merge( pars_ );

	IOPar outpars;
	transffld_->fillPar( outpars );
	seissel_->fillPar( outpars );
	js.pars_.mergeComp( outpars, sKey::Output() );

	return batchfld_->start();
    }

    Pos::GeomID geomid;
    if ( is2d )
    {
	geomid = Survey::GM().getGeomID( linenm );
	if ( !Survey::isValidGeomID(geomid) )
	    geomid = Geom2DImpHandler::getGeomID( linenm );
	if ( !Survey::isValidGeomID(geomid) )
	    return false;
    }
    else
	geomid = Survey::default3DGeomID();

    PtrMan<SeisTrcWriter> wrr = new SeisTrcWriter( outioobj, geomid,
						   &setup_.geom_ );
    SeisStoreAccess::Setup ssasu( inioobj, &setup_.geom_ );
    ssasu.geomid( geomid );
    auto* rdr = new SeisStdImporterReader( ssasu, "SEG-Y" );
    rdr->removeNull( transffld_->removeNull() );
    rdr->setResampler( transffld_->getResampler() );
    rdr->setScaler( transffld_->getScaler() );
    PtrMan<Seis::SelData> sd = transffld_->getSelData();
    if ( !sd )
	return false;

    if ( !sd->isAll() )
    {
	rdr->setSelData( sd->clone() );
	wrr->setSelData( sd.release() );
    }

    PtrMan<SeisImporter> imp = new SeisImporter( rdr, *wrr, setup_.geom_ );
    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( imp->name() );
	nm += " ("; nm += linenm; nm += ")";
	imp->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = TaskRunner::execute( &dlg, *imp );
    BufferStringSet warns;
    if ( imp && imp->nrSkipped() > 0 )
	warns += new BufferString("[9] During import, ", imp->nrSkipped(),
				  " traces were rejected" );
    const SeisTrcTranslator* transl = rdr->reader().seisTranslator();
    if ( transl && transl->haveWarnings() )
	warns.add( transl->warnings(), false );
    imp.erase(); wrr.erase(); // closes output cube

    uiSEGY::displayWarnings( warns );
    if ( rv && !is2d && ioobjinfo )
	rv = ioobjinfo->provideUserInfo();

    return rv;
}
