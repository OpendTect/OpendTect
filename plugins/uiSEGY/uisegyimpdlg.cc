/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/

#include "uisegyimpdlg.h"

#include "uibatchjobdispatchersel.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uiimpexp2dgeom.h"
#include "uisegydef.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"

#include "file.h"
#include "filepath.h"
#include "dirlist.h"
#include "ioobjctxt.h"
#include "iostrm.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "posinfo2dsurv.h"
#include "segybatchio.h"
#include "segyhdr.h"
#include "seisimporter.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seisstorer.h"
#include "survgeom2d.h"
#include "zdomain.h"


uiSEGYImpDlg::uiSEGYImpDlg( uiParent* p,
			const uiSEGYReadDlg::Setup& su, IOPar& iop )
    : uiSEGYReadDlg(p,su,iop)
    , morebut_(0)
    , batchfld_(0)
{
    uiString ttl = setup().dlgtitle_;
    if ( ttl.isEmpty() )
    {
	SEGY::FileSpec fs; fs.usePar( iop );
	ttl = toUiString( "%1 %2 %3" ).arg( uiStrings::sImport() )
	    .arg( Seis::nameOf(setup_.geom_) )
	    .arg( getLimitedDisplayString(fs.dispName(),40,0) );
    }
    setTitleText( ttl );

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


DBKey uiSEGYImpDlg::outputID() const
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
    const BufferString fnm( inioobj_.mainFileName() );
    File::Path fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Imp "); setupnm += uiSEGYFileSpec::sKeyLineNmToken();

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken() );
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    uiString txt( tr("Input ('%1' will become line name)")
				    .arg(uiSEGYFileSpec::sKeyLineNmToken()) );
    uiFileSel::Setup fssu( fp.fullPath() );
    fssu.objtype( uiStrings::sSEGY() );
    fnmfld_ = new uiFileSel( this, txt, fssu );
}


bool acceptOK()
{
    BufferString fnm = fnmfld_->fileName();
    File::Path fp( fnm );
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


bool doImp( const File::Path& fp )
{
    BufferString mask( fp.fileName() );
    mask.replace( uiSEGYFileSpec::sKeyLineNmToken(), "*" );
    File::Path maskfp( fp ); maskfp.setFileName( mask );
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
	File::Path newfp( maskfp );
	newfp.setFileName( dirlistfnm );
	const BufferString fullfnm( newfp.fullPath() );
	const int newlen = fullfnm.size();
	const int lnmlen = (newlen - orglen + 1) / nrtok;
	BufferString lnm( fullfnm.buf() + lnmoffs );
	*(lnm.getCStr() + lnmlen) = '\0';

	Pos::GeomID geomid = SurvGeom::getGeomID( lnm );
	if ( geomid.isValid() )
	{
	    if ( !overwritequestionasked )
	    {
		overwrite = uiMSG().askGoOn( tr("Do you want to overwrite the "
						"Geometry of the lines?") );
		overwritequestionasked = true;
	    }

	    if ( overwrite )
	    {
		const auto& geom2d = SurvGeom::get2D( geomid );
		geom2d.setEmpty();
	    }

	}

	IOObj* newioobj = getSubstIOObj( fullfnm );
	if ( !doWork( newioobj, lnm, idx > dl.size()-2, nofails ) )
	    return false;
    }

    return nofails;
}

    uiFileSel*		fnmfld_;
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
    BufferString lnm = is2d && transffld_->selFld2D() ?
		       transffld_->selFld2D()->selectedLine() : 0;
    if ( is2d && lnm.isEmpty() )
    {
	uiMSG().error( tr("Linename is empty. Please enter a line name") );
	return false;
    }

    const IOObj* useinioobj = &inioobj; IOObj* tmpioobj = 0;
    const bool outissidom = ZDomain::isSI( outioobj->pars() );
    if ( !outissidom )
    {
	tmpioobj = inioobj.clone();
	ZDomain::Def::get(outioobj->pars()).set( tmpioobj->pars() );
	useinioobj = tmpioobj;
    }

    bool retval;
    if ( !morebut_ || !morebut_->isChecked() )
    {
	Pos::GeomID geomid = SurvGeom::getGeomID( lnm );
	if ( is2d && geomid.isValid() )
	{
	    const bool overwrite =
		uiMSG().askGoOn( tr("Geometry of Line '%1' is already present."
				    "\n\nDo you want to overwrite?").arg(lnm) );
	    if ( overwrite )
	    {
		const auto& geom2d = SurvGeom::get2D( geomid );
		geom2d.setEmpty();
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

    if ( tmpioobj )
	tmpioobj->commitChanges();
    delete tmpioobj;
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
	ioobjinfo = new uiSeisIOObjInfo( this, outioobj );
	if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo(),true) )
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

    if ( is2d )
    {
	Pos::GeomID geomid = SurvGeom::getGeomID( linenm );
	if ( mIsUdfGeomID(geomid) )
	    geomid = Geom2DImpHandler::getGeomID( linenm );
	if ( mIsUdfGeomID(geomid) )
	    return false;
    }

    SEGY::TxtHeader::info2D() = is2d;
    PtrMan<Seis::Storer> storer = new Seis::Storer( outioobj );
    SeisStdImporterReader* rdr = new SeisStdImporterReader( inioobj, "SEG-Y" );
    rdr->removeNull( transffld_->removeNull() );
    rdr->setResampler( transffld_->getResampler() );
    rdr->setScaler( transffld_->getScaler() );
    rdr->setSelData( transffld_->getSelData() );

    PtrMan<SeisImporter> imp = new SeisImporter( rdr, *storer, setup_.geom_ );
    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( imp->name() );
	nm += " ("; nm += linenm; nm += ")";
	imp->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = TaskRunner::execute( &dlg, *imp );
    imp.erase();
    storer->close(); // closes output cube

    uiStringSet warns;
    uiSEGY::displayWarnings( this, warns, false, imp ? imp->nrSkipped() : 0 );
    if ( rv && !is2d && ioobjinfo )
	rv = ioobjinfo->provideUserInfo();

    SEGY::TxtHeader::info2D() = false;
    return rv;
}
