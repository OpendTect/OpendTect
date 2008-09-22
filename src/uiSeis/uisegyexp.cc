/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Sep 2001
 RCS:		$Id: uisegyexp.cc,v 1.1 2008-09-22 15:10:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyexp.h"
#include "uisegydef.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "segyhdr.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "executor.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "seistrctr.h"


uiSEGYExp::uiSEGYExp( uiParent* p, Seis::GeomType gt )
	: uiDialog(p,uiDialog::Setup("SEG-Y I/O","Export to SEG-Y","103.0.7"))
	, ctio_(*mMkCtxtIOObj(SeisTrc))
    	, geom_(gt)
{
    seissel_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(geom_) );
    seissel_->selectiondone.notify( mCB(this,uiSEGYExp,inpSel) );

    transffld_ = new uiSeisTransfer( this, uiSeisTransfer::Setup(geom_)
				    .withnullfill(true)
				    .fornewentry(false) );
    transffld_->attach( alignedBelow, seissel_ );

    fpfld_ = new uiSEGYFilePars( this, false );
    fpfld_->attach( alignedBelow, transffld_ );

    fsfld_ = new uiSEGYFileSpec( this, false );
    fsfld_->attach( alignedBelow, fpfld_ );
}


uiSEGYExp::~uiSEGYExp()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiSEGYExp::inpSel( CallBacker* )
{
    if ( ctio_.ioobj )
	transffld_->updateFrom( *ctio_.ioobj );
}


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSEGYExp::acceptOK( CallBacker* )
{
    if ( !seissel_->commitInput(false) )
    {
	uiMSG().error( "Please select the data to export" );
	return false;
    }

    const IOObj* inioobj = ctio_.ioobj;
    PtrMan<IOObj> outioobj = fsfld_->getSpec().getIOObj( true );
    fpfld_->fillPar( outioobj->pars() );
    const bool is2d = Seis::is2D( geom_ );
    outioobj->pars().setYN( SeisTrcTranslator::sKeyIs2D, is2d );

    const char* attrnm = seissel_->attrNm();
    const char* lnm = is2d && transffld_->selFld2D()
			   && transffld_->selFld2D()->isSingLine()
		    ? transffld_->selFld2D()->selectedLine() : 0;
    return doWork( *inioobj, *outioobj, lnm, attrnm );
}


bool uiSEGYExp::doWork( const IOObj& inioobj, const IOObj& outioobj,
				const char* linenm, const char* attrnm )
{
    const bool is2d = Seis::is2D( geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
    if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	return false;

    SEGY::TxtHeader::info2d = is2d;
    PtrMan<Executor> exec = transffld_->getTrcProc( inioobj, outioobj,
					"Export seismic data", "Putting traces",
					attrnm, linenm );
    if ( !exec )
	return false;

    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( exec->name() );
	nm += " ("; nm += linenm; nm += ")";
	exec->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = dlg.execute( *exec );
    exec.erase();

    SEGY::TxtHeader::info2d = false;
    return rv;
}
