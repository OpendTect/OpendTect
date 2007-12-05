
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadiosel.cc,v 1.3 2007-12-05 11:55:49 cvsbert Exp $";

#include "uimadiosel.h"
#include "madio.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "seispsioprov.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "survinfo.h"
#include "filegen.h"



uiMadIOSelDlg::uiMadIOSelDlg( uiParent* p, IOPar& iop, bool isinp )
	: uiDialog(p, Setup(BufferString("Processing ",isinp?"input":"output"),
		    	    BufferString("Specify the ",isinp?"input":"output",
					 " to the processing flow"),"0.0.0") )
	, ctio3d_(*mMkCtxtIOObj(SeisTrc))
    	, ctio2d_(*mMkCtxtIOObj(SeisTrc))
	, ctiops_(*mMkCtxtIOObj(SeisPS))
	, seis3dfld_(0), seis2dfld_(0), seispsfld_(0)
	, subsel3dfld_(0), subsel2dfld_(0), subselpsfld_(0)
        , subselmadfld_(0), subselmadlbl_(0)
    	, idx3d_(-1), idx2d_(-1)
	, iop_(iop)
{
    ctio2d_.ctxt.forread = ctio3d_.ctxt.forread = ctiops_.ctxt.forread = isinp;
    BufferStringSet seistypes;
#   define mAdd(s,idx) { seistypes.add( s ); idx = seistypes.size() - 1; }
    if ( SI().has3D() ) mAdd( Seis::nameOf(Seis::Vol), idx3d_ );
    if ( SI().has2D() ) mAdd( Seis::nameOf(Seis::Line), idx2d_ );
    mAdd( Seis::nameOf(Seis::VolPS), idxps_ );
    mAdd( ODMad::sKeyMadagascar, idxmad_ );
    mAdd( sKey::None, idxnone_ );

    typfld_ = new uiGenInput( this, "Input", StringListInpSpec(seistypes) );
    typfld_->valuechanged.notify( mCB(this,uiMadIOSelDlg,typSel) );
    if ( SI().has3D() )
    {
	seis3dfld_ = new uiSeisSel( this, ctio3d_, uiSeisSel::Setup(Seis::Vol));
	seis3dfld_->attach( alignedBelow, typfld_ );
	seis3dfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	if ( isinp )
	{
	    subsel3dfld_ = new uiSeis3DSubSel( this, Seis::SelSetup(Seis::Vol)
					.allowtable(true).allowpoly(true) );
	    subsel3dfld_->attach( alignedBelow, seis3dfld_ );
	}
    }
    if ( SI().has2D() )
    {
	seis2dfld_ = new uiSeisSel( this, ctio2d_,uiSeisSel::Setup(Seis::Line));
	seis2dfld_->attach( alignedBelow, typfld_ );
	seis2dfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	if ( isinp )
	{
	    subsel2dfld_ = new uiSeis2DSubSel( this,Seis::SelSetup(Seis::Line));
	    subsel2dfld_->attach( alignedBelow, seis2dfld_ );
	}
    }

    seispsfld_ = new uiIOObjSel( this, ctiops_ );
    seispsfld_->attach( alignedBelow, typfld_ );
    seispsfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
    subselpsfld_ = SI().has3D() ? (uiSeisSubSel*)subsel3dfld_
				: (uiSeisSubSel*)subsel2dfld_;

    madfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup() );
    madfld_->attach( alignedBelow, typfld_ );
    if ( isinp )
    {
	subselmadfld_ = new uiGenInput( this, "sfheaderwindow parameters" );
	subselmadfld_->attach( alignedBelow, madfld_ );
	subselmadlbl_ = new uiLabel( this, "[Empty=All]" );
	subselmadlbl_->attach( rightOf, subselmadfld_ );
    }

    finaliseDone.notify( mCB(this,uiMadIOSelDlg,typSel) );
}


uiMadIOSelDlg::~uiMadIOSelDlg()
{
    delete ctio3d_.ioobj; delete &ctio3d_;
    delete ctio2d_.ioobj; delete &ctio2d_;
    delete ctiops_.ioobj; delete &ctiops_;
}


void uiMadIOSelDlg::typSel( CallBacker* )
{
    const int choice = typfld_->getIntValue();
    if ( seis3dfld_ ) seis3dfld_->display( choice == idx3d_ );
    if ( seis2dfld_ ) seis2dfld_->display( choice == idx2d_ );
    if ( seispsfld_ ) seispsfld_->display( choice == idxps_ );
    madfld_->display( choice == idxmad_ );
    if ( !isInp() ) return;

    if ( subsel3dfld_ ) subsel3dfld_->display( choice == idx3d_ );
    if ( subsel2dfld_ ) subsel2dfld_->display( choice == idx2d_ );
    subselpsfld_->compoundParSel()->display( choice == idxps_ );
    subselmadfld_->display( choice == idxmad_ );
    subselmadlbl_->display( choice == idxmad_ );
}


void uiMadIOSelDlg::selChg( CallBacker* cb )
{
    if ( cb == madfld_ || !isInp() ) return;

    const bool isps = cb == seispsfld_;
    const bool is2d = cb == seis2dfld_;
    uiSeisSubSel* subsel = subsel2dfld_;
    if ( !is2d ) subsel = isps ? subselpsfld_ : subsel3dfld_;
    CtxtIOObj& ctio = is2d ? ctio2d_ : (isps ? ctiops_ : ctio3d_);

    if ( !ctio.ioobj )
	subsel->clear();
    else
	subsel->setInput( *ctio.ioobj );
}


#define mErrRet(s) \
{ \
    uiMSG().error( "Please select the ", isinp?"input ":"output ", s ); \
    return false; \
}

bool uiMadIOSelDlg::getInp()
{
    const int choice = typfld_->getIntValue();
    const bool isinp = isInp();
    if ( choice == idx3d_ || choice == idx2d_ )
    {
	uiSeisSel* ss = choice == idx3d_ ? seis3dfld_ : seis2dfld_;
	if ( !ss->commitInput(!isinp) )
	    mErrRet("seismics")
	if ( !isinp && choice == idx3d_ && ctio3d_.ioobj->implExists(false)
	   && !uiMSG().askGoOn("Output cube exists. Overwrite?") )
	    return false;
    }
    else if ( choice == idxps_ )
    {
	if ( !seispsfld_->commitInput(!isinp) )
	    mErrRet("data store")
    }
    else if ( choice == idxmad_ )
    {
	const BufferString fnm( madfld_->fileName() );
	if ( fnm.isEmpty() || (isinp && !File_exists(fnm)) )
	    mErrRet("file")
    }
    return true;
}


bool uiMadIOSelDlg::acceptOK( CallBacker* )
{
    if ( !getInp() )
	return false;

    iop_.set( sKey::Type, typfld_->text() );
    const int choice = typfld_->getIntValue();
    const bool isinp = isInp();
    if ( choice == idx3d_ || choice == idx2d_ )
    {
	const bool is3d = choice == idx3d_;
	iop_.clear();
	iop_.set( sKey::Selection, is3d ?
		  Seis::nameOf(Seis::Vol) : Seis::nameOf(Seis::Line) );
	const CtxtIOObj& ctio = is3d ? ctio3d_ : ctio2d_;
	iop_.set( "ID", ctio.ioobj->key() );
	if ( isinp )
	{
	    if ( is3d )	subsel3dfld_->fillPar( iop_ );
	    else	subsel2dfld_->fillPar( iop_ );
	}
    }
    else if ( choice == idxps_ )
    {
	iop_.set( sKey::Selection, Seis::nameOf(Seis::VolPS) );
	iop_.set( "ID", ctiops_.ioobj->key() );
	if ( isinp )
	    subselpsfld_->fillPar( iop_ );
    }
    else if ( choice == idxmad_ )
    {
	iop_.set( sKey::Selection, ODMad::sKeyMadagascar );
	if ( isinp )
	    iop_.set( sKey::IOSelection, subselmadfld_->text() );
    }

    return true;
}


uiMadIOSel::uiMadIOSel( uiParent* p, bool isinp )
	: uiCompoundParSel(p,isinp ? "INPUT" : "OUTPUT")
	, iop_(BufferString("Madagascar ",isinp?"input":"output"," selection"))
	, isinp_(isinp)
{
    butPush.notify( mCB(this,uiMadIOSel,doDlg) );
}


void uiMadIOSel::doDlg( CallBacker* )
{
    uiMadIOSelDlg dlg( this, iop_, isinp_ );
    dlg.go();
}


BufferString uiMadIOSel::getSummary() const
{
    BufferString ret;
    //TODO better
    const char* res = iop_.find( sKey::Selection );
    if ( !res || !*res ) return ret;

    ret = res; res = iop_.find( "ID" );
    if ( *ret.buf() != 'M' && *ret.buf() != 'N' )
	{ ret += " "; ret += IOM().nameOf( res ); }

    return ret;
}
