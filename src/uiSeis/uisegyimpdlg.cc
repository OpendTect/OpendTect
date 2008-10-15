/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.cc,v 1.8 2008-10-15 11:22:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyimpdlg.h"

#include "uisegydef.h"
#include "uisegyexamine.h"
#include "uiseistransf.h"
#include "uiseisfmtscale.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uitoolbar.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiseparator.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "keystrs.h"
#include "segytr.h"
#include "segyhdr.h"
#include "seisioobjinfo.h"
#include "seisimporter.h"
#include "seiswrite.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "filegen.h"
#include "dirlist.h"
#include "ioman.h"
#include "iostrm.h"


uiSEGYImpDlg::Setup::Setup( Seis::GeomType gt )
    : uiDialog::Setup("SEG-Y Import",mNoDlgTitle,"103.1.5")
    , geom_(gt) 
    , nrexamine_(0)     
    , rev_(uiSEGYRead::Rev0)
{
}

#define mAddButton(fnm,func,tip,toggle) \
	tb_->addButton( fnm, mCB(this,uiSEGYImpDlg,func), tip, toggle )

uiSEGYImpDlg::uiSEGYImpDlg( uiParent* p,
			const uiSEGYImpDlg::Setup& su, IOPar& iop )
    : uiDialog(p,su)
    , setup_(su)
    , pars_(iop)
    , optsfld_(0)
    , savesetupfld_(0)
    , morebut_(0)
    , ctio_(*uiSeisSel::mkCtxtIOObj(su.geom_))
    , readParsReq(this)
    , writeParsReq(this)
    , preScanReq(this)
{
    ctio_.ctxt.forread = false;
    if ( setup().dlgtitle_.isEmpty() )
    {
	BufferString ttl( "Import " );
	ttl += Seis::nameOf( setup_.geom_ );
	SEGY::FileSpec fs; fs.usePar( iop );
	ttl += " '"; ttl += fs.fname_; ttl += "'";
	setTitleText( ttl );
    }

    uiGroup* optsgrp = 0;
    if ( Seis::isPS(setup_.geom_) || setup_.rev_ != uiSEGYRead::Rev1 )
    {
	optsgrp = new uiGroup( this, "Opts group" );
	uiSEGYFileOpts::Setup osu( setup_.geom_, uiSEGYRead::Import,
				   setup_.rev_ );
	optsfld_ = new uiSEGYFileOpts( optsgrp, osu, &iop );
	optsfld_->readParsReq.notify( mCB(this,uiSEGYImpDlg,readParsCB) );
	optsfld_->preScanReq.notify( mCB(this,uiSEGYImpDlg,preScanCB) );

	savesetupfld_ = new uiGenInput( optsgrp, "On OK, save setup as" );
	savesetupfld_->attach( alignedBelow, optsfld_ );
	optsgrp->setHAlignObj( savesetupfld_ );
	uiLabel* lbl = new uiLabel( optsgrp, "(optional)" );
	lbl->attach( rightOf, savesetupfld_ );
    }

    uiSeparator* sep = optsgrp ? new uiSeparator( this, "Hor sep" ) : 0;

    uiGroup* outgrp = new uiGroup( this, "Output group" );
    transffld_ = new uiSeisTransfer( outgrp, uiSeisTransfer::Setup(setup_.geom_)
				    .withnullfill(false)
				    .fornewentry(true) );
    outgrp->setHAlignObj( transffld_ );
    if ( sep )
    {
	sep->attach( stretchedBelow, optsgrp );
	outgrp->attach( alignedBelow, optsgrp );
	outgrp->attach( ensureBelow, sep );
    }

    seissel_ = new uiSeisSel( outgrp, ctio_, uiSeisSel::Setup(setup_.geom_) );
    seissel_->attach( alignedBelow, transffld_ );

    if ( setup_.geom_ == Seis::Line )
    {
	morebut_ = new uiCheckBox( outgrp, "Import more, similar files" );
	morebut_->attach( alignedBelow, seissel_ );
    }

    finaliseDone.notify( mCB(this,uiSEGYImpDlg,setupWin) );
}


void uiSEGYImpDlg::setupWin( CallBacker* )
{
    if ( setup_.nrexamine_ < 1 ) return;

    uiSEGYExamine::Setup exsu( setup_.nrexamine_ );
    exsu.modal( false ); exsu.usePar( pars_ );
    uiSEGYExamine* dlg = new uiSEGYExamine( this, exsu );
    dlg->go();
}


void uiSEGYImpDlg::readParsCB( CallBacker* )
{
    readParsReq.trigger();
}


void uiSEGYImpDlg::preScanCB( CallBacker* )
{
    preScanReq.trigger();
}


uiSEGYImpDlg::~uiSEGYImpDlg()
{
}


void uiSEGYImpDlg::use( const IOObj* ioobj, bool force )
{
    if ( optsfld_ )
	optsfld_->use( ioobj, force );
    if ( ioobj )
	transffld_->updateFrom( *ioobj );
}


bool uiSEGYImpDlg::getParsFromScreen( bool permissive )
{
    return optsfld_ ? optsfld_->fillPar( pars_, permissive ) : true;
}


const char* uiSEGYImpDlg::saveObjName() const
{
    return savesetupfld_ ? savesetupfld_->text() : "";
}


bool uiSEGYImpDlg::rejectOK( CallBacker* )
{
    getParsFromScreen( true );
    return true;
}


class uiSEGYImpSimilarDlg : public uiDialog
{
public:

uiSEGYImpSimilarDlg( uiSEGYImpDlg* p, const IOObj& iio, const IOObj& oio,
		     const char* anm )
	: uiDialog(p,uiDialog::Setup("2D SEG-Y multi-import",
		    		     "Specify file details","103.0.6"))
	, inioobj_(iio)
	, outioobj_(oio)
	, impdlg_(p)
	, attrnm_(anm)
{
    const BufferString fnm( inioobj_.fullUserExpr(true) );
    FilePath fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Imp "); setupnm += uiSEGYFileSpec::sKeyLineNmToken;

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken );
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    BufferString txt( "Input ('" ); txt += uiSEGYFileSpec::sKeyLineNmToken;
    txt += "' will become line name)";
    fnmfld_ = new uiFileInput( this, txt,
		    uiFileInput::Setup(fp.fullPath()).forread(true) );
}


bool acceptOK( CallBacker* )
{
    BufferString fnm = fnmfld_->fileName();
    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    if ( !File_isDirectory(dirnm) )
    {
	uiMSG().error( "Directory provided not usable" );
	return false;
    }
    if ( !strstr(fp.fullPath().buf(),uiSEGYFileSpec::sKeyLineNmToken) )
    {
	BufferString msg( "The file name has to contain at least one '" );
	msg += uiSEGYFileSpec::sKeyLineNmToken; msg += "'\n";
	msg += "That will then become the line name";
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
    iostrm->setFileName( fullfnm );
    return newioobj;
}


bool doWork( IOObj* newioobj, const char* lnm, bool islast, bool& nofails )
{
    bool res = impdlg_->doWork( *newioobj, outioobj_, lnm, attrnm_ );
    delete newioobj;
    if ( !res )
    {
	nofails = false;
	if ( !islast && !uiMSG().askGoOn("Continue with next?") )
	    return false;
    }
    return true;
}


bool doImp( const FilePath& fp )
{
    BufferString mask( fp.fileName() );
    replaceString( mask.buf(), uiSEGYFileSpec::sKeyLineNmToken, "*" );
    FilePath maskfp( fp ); maskfp.setFileName( mask );
    const int nrtok = countCharacter( mask.buf(), '*' );
    DirList dl( fp.pathOnly(), DirList::FilesOnly, mask );
    if ( dl.size() < 1 )
    {
	uiMSG().error( "Cannot find any match for file name" );
	return false;
    }

    BufferString fullmaskfnm( maskfp.fullPath() );
    int lnmoffs = strstr( fullmaskfnm.buf(), "*" ) - fullmaskfnm.buf();
    const int orglen = fullmaskfnm.size();
    bool nofails = true;

    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString dirlistfnm( dl.get(idx) );
	FilePath newfp( maskfp );
	newfp.setFileName( dirlistfnm );
	const BufferString fullfnm( newfp.fullPath() );
	const int newlen = fullfnm.size();
	const int lnmlen = (newlen - orglen + 1) / nrtok;
	BufferString lnm( fullfnm.buf() + lnmoffs );
	*(lnm.buf() + lnmlen) = '\0';

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
    const char*		attrnm_;

};



bool uiSEGYImpDlg::acceptOK( CallBacker* )
{
    if ( !getParsFromScreen(false) )
	return false;
    if ( *saveObjName() )
	writeParsReq.trigger();

    if ( !seissel_->commitInput(true) )
    {
	uiMSG().error( "Please select the output data" );
	return false;
    }

    SEGY::FileSpec fs; fs.usePar( pars_ );
    PtrMan<IOObj> inioobj = fs.getIOObj();
    if ( !inioobj )
    {
	uiMSG().error( "Internal: cannot create SEG-Y object" );
	return false;
    }

    const IOObj& outioobj = *ctio_.ioobj;
    const bool is2d = Seis::is2D( setup_.geom_ );
    const char* attrnm = seissel_->attrNm();
    const char* lnm = is2d && transffld_->selFld2D() ?
		      transffld_->selFld2D()->selectedLine() : 0;

    if ( !morebut_ || !morebut_->isChecked() )
	return doWork( *inioobj, outioobj, lnm, attrnm );

    uiSEGYImpSimilarDlg dlg( this, *inioobj, outioobj, attrnm );
    return dlg.go();
}


bool uiSEGYImpDlg::doWork( const IOObj& inioobj, const IOObj& outioobj,
				const char* linenm, const char* attrnm )
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

    SEGY::TxtHeader::info2d = is2d;
    transffld_->scfmtfld->updateIOObj( const_cast<IOObj*>(&outioobj), true );
    PtrMan<SeisTrcWriter> wrr = new SeisTrcWriter( &outioobj );
    SeisStdImporterReader* rdr = new SeisStdImporterReader( inioobj, "SEG-Y" );
    rdr->removeNull( transffld_->removeNull() );
    rdr->setResampler( transffld_->getResampler() );
    rdr->setScaler( transffld_->scfmtfld->getScaler() );
    Seis::SelData* sd = transffld_->getSelData();
    if ( is2d )
    {
	if ( linenm && *linenm )
	    sd->lineKey().setLineName( linenm );
	if ( !isps )
	    sd->lineKey().setAttrName( attrnm );
	wrr->setSelData( sd->clone() );
    }
    rdr->setSelData( sd );

    PtrMan<SeisImporter> imp = new SeisImporter( rdr, *wrr, setup_.geom_ );
    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( imp->name() );
	nm += " ("; nm += linenm; nm += ")";
	imp->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = dlg.execute( *imp );
    if ( imp && imp->nrSkipped() > 0 )
	uiMSG().warning( BufferString("During import, ",
				      imp->nrSkipped(),
				      " traces were rejected") );
    imp.erase(); wrr.erase(); // closes output cube
    if ( rv && !is2d && ioobjinfo )
	rv = ioobjinfo->provideUserInfo();

    SEGY::TxtHeader::info2d = false;
    return rv;
}
