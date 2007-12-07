
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadiosel.cc,v 1.4 2007-12-07 15:15:51 cvsbert Exp $";

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
#include "keystrs.h"



uiMadIOSelDlg::uiMadIOSelDlg( uiParent* p, IOPar& iop, bool isinp )
	: uiDialog(p, Setup(BufferString("Processing ",isinp?"input":"output"),
		    	    BufferString("Specify the ",isinp?"input":"output",
					 " to the processing flow"),"0.0.0") )
	, ctio3d_(*mMkCtxtIOObj(SeisTrc))
    	, ctio2d_(*mMkCtxtIOObj(SeisTrc))
	, ctiops3d_(*mMkCtxtIOObj(SeisPS))
	, ctiops2d_(*mMkCtxtIOObj(SeisPS))
	, seis3dfld_(0), seis2dfld_(0), seisps3dfld_(0), seisps2dfld_(0)
	, subsel3dfld_(0), subsel2dfld_(0)
        , subselmadfld_(0), subselmadlbl_(0)
    	, idx3d_(-1), idx2d_(-1)
	, iop_(iop)
{
    const bool have2d = SI().has2D();
    const bool have3d = SI().has3D();
    ctio3d_.ctxt.forread = ctio2d_.ctxt.forread
      = ctiops3d_.ctxt.forread = ctiops2d_.ctxt.forread = isinp;
    BufferStringSet seistypes;
#   define mAdd(s,idx) { seistypes.add( s ); idx = seistypes.size() - 1; }
    if ( have3d )
    {
	mAdd( Seis::nameOf(Seis::Vol), idx3d_ );
	mAdd( Seis::nameOf(Seis::VolPS), idxps3d_ );
    }
    if ( have2d )
    {
	mAdd( Seis::nameOf(Seis::Line), idx2d_ );
	mAdd( Seis::nameOf(Seis::LinePS), idxps2d_ );
    }
    mAdd( ODMad::sKeyMadagascar, idxmad_ );
    mAdd( sKey::None, idxnone_ );

    typfld_ = new uiGenInput( this, "Input", StringListInpSpec(seistypes) );
    typfld_->valuechanged.notify( mCB(this,uiMadIOSelDlg,typSel) );
    if ( have3d )
    {
	seis3dfld_ = new uiSeisSel( this, ctio3d_, uiSeisSel::Setup(Seis::Vol));
	seis3dfld_->attach( alignedBelow, typfld_ );
	seis3dfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	seisps3dfld_ = new uiSeisSel( this, ctiops3d_,
				      uiSeisSel::Setup(Seis::VolPS));
	seisps3dfld_->attach( alignedBelow, typfld_ );
	seisps3dfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	if ( isinp )
	{
	    subsel3dfld_ = new uiSeis3DSubSel( this, Seis::SelSetup(Seis::Vol)
					.allowtable(true).allowpoly(true) );
	    subsel3dfld_->attach( alignedBelow, seis3dfld_ );
	}
    }
    if ( have2d )
    {
	seis2dfld_ = new uiSeisSel( this, ctio2d_,uiSeisSel::Setup(Seis::Line));
	seis2dfld_->attach( alignedBelow, typfld_ );
	seis2dfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	seisps2dfld_ = new uiSeisSel( this, ctio2d_,
					uiSeisSel::Setup(Seis::LinePS) );
	seisps2dfld_->attach( alignedBelow, typfld_ );
	seisps2dfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	if ( isinp )
	{
	    subsel2dfld_ = new uiSeis2DSubSel( this,Seis::SelSetup(Seis::Line));
	    subsel2dfld_->attach( alignedBelow, seis2dfld_ );
	}
    }

    madfld_ = new uiFileInput( this, isinp ? "Input file" : "Output file",
	    			uiFileInput::Setup() );
    madfld_->attach( alignedBelow, typfld_ );
    if ( isinp )
    {
	subselmadfld_ = new uiGenInput( this, "sfheaderwindow parameters" );
	subselmadfld_->attach( alignedBelow, madfld_ );
	subselmadlbl_ = new uiLabel( this, "[Empty=All]" );
	subselmadlbl_->attach( rightOf, subselmadfld_ );
    }

    finaliseDone.notify( mCB(this,uiMadIOSelDlg,initWin) );
}


uiMadIOSelDlg::~uiMadIOSelDlg()
{
    delete ctio3d_.ioobj; delete &ctio3d_;
    delete ctio2d_.ioobj; delete &ctio2d_;
    delete ctiops3d_.ioobj; delete &ctiops3d_;
    delete ctiops2d_.ioobj; delete &ctiops2d_;
}


void uiMadIOSelDlg::initWin( CallBacker* cb )
{
    const char* res = iop_.find( sKey::Type );
    const bool isinp = isInp();
    if ( !res || !strcmp(res,ODMad::sKeyMadagascar) )
    {
	BufferString txt;
	if ( iop_.get(sKey::FileName,txt) )
	    madfld_->setFileName( txt );
	if ( isinp && iop_.get(sKey::IOSelection,txt) )
	    subselmadfld_->setText( txt );
    }
    else
    {
	const Seis::GeomType gt = Seis::geomTypeOf( res );
	typfld_->setText( res );
	seisSel( gt )->usePar( iop_ );

	if ( isinp )
	    seisSubSel( gt )->usePar( iop_ );
    }

    typSel(cb);
}


void uiMadIOSelDlg::typSel( CallBacker* )
{
    const int choice = typfld_->getIntValue();

    if ( seis3dfld_ ) seis3dfld_->display( choice == idx3d_ );
    if ( seisps3dfld_ ) seisps3dfld_->display( choice == idxps3d_ );
    if ( seis2dfld_ ) seis2dfld_->display( choice == idx2d_ );
    if ( seisps2dfld_ ) seisps2dfld_->display( choice == idxps2d_ );
    madfld_->display( choice == idxmad_ );
    if ( !isInp() ) return;

    if ( subsel3dfld_ )
	subsel3dfld_->display( choice == idx3d_ || choice == idxps3d_ );
    if ( subsel2dfld_ )
	subsel2dfld_->display( choice == idx2d_ || choice == idxps2d_ );

    subselmadfld_->display( choice == idxmad_ );
    subselmadlbl_->display( choice == idxmad_ );
}


void uiMadIOSelDlg::selChg( CallBacker* )
{
    if ( isMad() || !isInp() ) return;

    const Seis::GeomType gt = geomType();
    CtxtIOObj& ctio = ctxtIOObj( gt );
    uiSeisSubSel* subsel = seisSubSel( gt );
    if ( !ctio.ioobj )
	subsel->clear();
    else
	subsel->setInput( *ctio.ioobj );
}


#define mErrRet(s) \
{ \
    uiMSG().error( "Please select the ", isinp ? "input " : "output ", s ); \
    return false; \
}

bool uiMadIOSelDlg::getInp()
{
    const bool isinp = isInp();
    if ( isMad() )
    {
	const BufferString fnm( madfld_->fileName() );
	if ( fnm.isEmpty() || (isinp && !File_exists(fnm)) )
	    mErrRet("file")
    }
    else
    {
	const Seis::GeomType gt = geomType();
	if ( !seisSel(gt)->commitInput(!isinp) )
	{
	    mErrRet(Seis::isPS(gt) ? "data store" : "seismics")
	    if ( !isinp && !Seis::is2D(gt) && ctio3d_.ioobj->implExists(false)
	       && !uiMSG().askGoOn("Output cube exists. Overwrite?") )
		return false;
	}
    }
    return true;
}


Seis::GeomType uiMadIOSelDlg::geomType() const
{
    const int choice = typfld_->getIntValue();
    return choice == idx3d_ ?	Seis::Vol
	: (choice == idx2d_ ?	Seis::Line
	: (choice == idxps2d_ ?	Seis::LinePS
	:			Seis::VolPS) );
}


CtxtIOObj& uiMadIOSelDlg::ctxtIOObj( Seis::GeomType gt )
{
    return gt == Seis::Vol ?	ctio3d_
	: (gt == Seis::Line ?	ctio2d_
	: (gt == Seis::LinePS ?	ctiops2d_
	:			ctiops3d_ ) );
}


bool uiMadIOSelDlg::isMad() const
{
    return typfld_->getIntValue() == idxmad_;
}


uiSeisSel* uiMadIOSelDlg::seisSel( Seis::GeomType gt )
{
    return gt == Seis::Vol ?	seis3dfld_
	: (gt == Seis::Line ?	seis2dfld_
	: (gt == Seis::LinePS ?	seisps2dfld_
	:			seisps3dfld_ ) );
}


uiSeisSubSel* uiMadIOSelDlg::seisSubSel( Seis::GeomType gt )
{
    if ( Seis::is2D(gt) )
	return subsel2dfld_;
    return subsel3dfld_;
}


bool uiMadIOSelDlg::acceptOK( CallBacker* )
{
    if ( !getInp() )
	return false;

    iop_.set( sKey::Type, typfld_->text() );
    const bool isinp = isInp();
    if ( isMad() )
    {
	iop_.set( sKey::Selection, ODMad::sKeyMadagascar );
	iop_.set( sKey::FileName, madfld_->fileName() );
	if ( isinp )
	    iop_.set( sKey::IOSelection, subselmadfld_->text() );
    }
    else
    {
	const Seis::GeomType gt = geomType();
	iop_.clear();
	iop_.set( sKey::Selection, Seis::nameOf(gt) );
	const CtxtIOObj& ctio = ctxtIOObj( gt );
	iop_.set( "ID", ctio.ioobj->key() );
	if ( isinp )
	{
	    if ( Seis::is2D(gt) )
		subsel2dfld_->fillPar( iop_ );
	    else
		subsel3dfld_->fillPar( iop_ );
	}
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
