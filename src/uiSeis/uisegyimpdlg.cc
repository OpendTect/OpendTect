/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.cc,v 1.4 2008-10-01 10:51:39 cvsbert Exp $
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
#include "uigeninput.h"
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


uiSEGYImpDlg::Setup::Setup( Seis::GeomType gt )
    : uiDialog::Setup("SEG-Y Import",0,"103.1.5")
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
    , ctio_(*uiSeisSel::mkCtxtIOObj(su.geom_))
    , readParsReq(this)
    , writeParsReq(this)
{
    ctio_.ctxt.forread = false;

    uiGroup* optsgrp = 0;
    if ( setup_.rev_ != uiSEGYRead::Rev1 )
    {
	optsgrp = new uiGroup( this, "Opts group" );
	uiSEGYFileOpts::Setup osu( setup_.geom_, uiSEGYRead::Import,
				    setup_.rev_ == uiSEGYRead::WeakRev1 );
	optsfld_ = new uiSEGYFileOpts( optsgrp, osu, &iop );
	optsfld_->readParsReq.notify( mCB(this,uiSEGYImpDlg,readParsCB) );

	savesetupfld_ = new uiGenInput( optsgrp, "On OK, save setup as" );
	savesetupfld_->attach( alignedBelow, optsfld_ );
	optsgrp->setHAlignObj( savesetupfld_ );
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

    return doWork( *inioobj, outioobj, lnm, attrnm );
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
