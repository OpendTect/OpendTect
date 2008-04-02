
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadiosel.cc,v 1.12 2008-04-02 11:43:57 cvsraman Exp $";

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
		    BufferString("Specify the ",isinp?"input to":"output of",
					 " the processing flow"),"0.0.0") )
	, ctio3d_(*mMkCtxtIOObj(SeisTrc))
    	, ctio2d_(*mMkCtxtIOObj(SeisTrc))
	, ctiops3d_(*mMkCtxtIOObj(SeisPS3D))
	, ctiops2d_(*mMkCtxtIOObj(SeisPS2D))
	, seis3dfld_(0), seis2dfld_(0), seisps3dfld_(0), seisps2dfld_(0)
	, subsel3dfld_(0), subsel2dfld_(0), subsel2dpsfld_(0)
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

    typfld_ = new uiGenInput( this, isinp ? "Input" : "Output",
	    		      StringListInpSpec(seistypes) );
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
					.onlyrange(false) );
	    subsel3dfld_->attach( alignedBelow, seis3dfld_ );
	}
    }
    if ( have2d )
    {
	seis2dfld_ = new uiSeisSel( this, ctio2d_,uiSeisSel::Setup(Seis::Line));
	seis2dfld_->attach( alignedBelow, typfld_ );
	seis2dfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	seisps2dfld_ = new uiSeisSel( this, ctiops2d_,
					uiSeisSel::Setup(Seis::LinePS) );
	seisps2dfld_->attach( alignedBelow, typfld_ );
	seisps2dfld_->selectiondone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	if ( isinp )
	{
	    subsel2dfld_ = new uiSeis2DSubSel( this,
		    				Seis::SelSetup(Seis::Line));
	    subsel2dfld_->attach( alignedBelow, seis2dfld_ );
	    subsel2dpsfld_ = new uiSeis2DSubSel( this,
		    				Seis::SelSetup(Seis::LinePS));
	    subsel2dpsfld_->attach( alignedBelow, seis2dfld_ );
	}
    }

    uiFileInput::Setup setup;
    setup.defseldir( ODMad::FileSpec::defPath() );
    setup.forread( isinp );
    madfld_ = new uiFileInput( this, "Data file", setup );
    madfld_->attach( alignedBelow, typfld_ );

    finaliseDone.notify( mCB(this,uiMadIOSelDlg,initWin) );
}


uiMadIOSelDlg::~uiMadIOSelDlg()
{
    delete ctio3d_.ioobj; delete &ctio3d_;
    delete ctio2d_.ioobj; delete &ctio2d_;
    delete ctiops3d_.ioobj; delete &ctiops3d_;
    delete ctiops2d_.ioobj; delete &ctiops2d_;
}


bool uiMadIOSelDlg::isNone() const
{
    return typfld_->getIntValue() == idxnone_;
}


bool uiMadIOSelDlg::isMad() const
{
    return typfld_->getIntValue() == idxmad_;
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
	return Seis::isPS(gt) ? subsel2dpsfld_ : subsel2dfld_;
    return subsel3dfld_;
}


void uiMadIOSelDlg::initWin( CallBacker* cb )
{
    usePar( iop_ );
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
	subsel2dfld_->display( choice == idx2d_ );
    if ( subsel2dpsfld_ )
	subsel2dpsfld_->display( choice == idxps2d_ );
}


void uiMadIOSelDlg::selChg( CallBacker* )
{
    if ( isMad() || isNone() || !isInp() ) return;

    const Seis::GeomType gt = geomType();
    CtxtIOObj& ctio = ctxtIOObj( gt );
    uiSeisSubSel* subsel = seisSubSel( gt );
    if ( !ctio.ioobj )
	subsel->clear();
    else
	subsel->setInput( *ctio.ioobj );
}


void uiMadIOSelDlg::usePar( const IOPar& iop )
{
    ODMad::ProcFlow::IOType iot = ODMad::ProcFlow::ioType( iop );
    const Seis::GeomType gt = (Seis::GeomType)iot;

    if ( iot == ODMad::ProcFlow::None )
	typfld_->setValue( 0 );
    else
	typfld_->setText( iot == ODMad::ProcFlow::Madagascar
			? ODMad::sKeyMadagascar
			: Seis::nameOf(gt) );
    typSel( this );
    if ( iot == ODMad::ProcFlow::None ) return;

    if ( iot == ODMad::ProcFlow::Madagascar )
    {
	BufferString txt;
	if ( iop.get(sKey::FileName,txt) )
	    madfld_->setFileName( txt );
	return;
    }

    seisSel( gt )->usePar( iop );
    selChg( this );
    if ( isInp() )
    {
	PtrMan<IOPar> subpar = iop.subselect( sKey::Selection );
	if ( subpar ) seisSubSel( gt )->usePar( *subpar );
    }
}


void uiMadIOSelDlg::fillPar( IOPar& iop )
{
    iop.clear();
    ODMad::ProcFlow::setIOType( iop, ioType() );
    const bool isinp = isInp();
    if ( isMad() )
	iop.set( sKey::FileName, madfld_->fileName() );
    else if ( !isNone() )
    {
	const Seis::GeomType gt = geomType();
	seisSel(gt)->fillPar( iop );
	if ( isinp )
	{
	    IOPar subpar;
	    if ( Seis::is2D(gt) )
		(Seis::isPS(gt)?subsel2dpsfld_:subsel2dfld_)->fillPar( subpar );
	    else
		subsel3dfld_->fillPar( subpar );

	    if ( subpar.size() )
		iop.mergeComp( subpar, sKey::Selection );
	}
    }
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
    else if ( !isNone() )
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


bool uiMadIOSelDlg::acceptOK( CallBacker* )
{
    if ( !getInp() )
	return false;

    fillPar( iop_ );
    return true;
}


uiMadIOSel::uiMadIOSel( uiParent* p, bool isinp )
	: uiCompoundParSel(p,isinp ? "INPUT" : "OUTPUT")
	, iop_(BufferString("Madagascar ",isinp?"input":"output"," selection"))
	, isinp_(isinp)
{
    butPush.notify( mCB(this,uiMadIOSel,doDlg) );
}


void uiMadIOSel::usePar( const IOPar& iop )
{
    iop_ = iop;
    updateSummary();
}


void uiMadIOSel::doDlg( CallBacker* )
{
    uiMadIOSelDlg dlg( this, iop_, isinp_ );
    dlg.go();
}


BufferString uiMadIOSel::getSummary() const
{
    BufferString ret( "-" );

    ODMad::ProcFlow::IOType iot = ODMad::ProcFlow::ioType( iop_ );
    if ( iot == ODMad::ProcFlow::Madagascar )
	ret = iop_.find( sKey::FileName );
    else if ( iot != ODMad::ProcFlow::None )
	ret = IOM().nameOf( iop_.find("ID") );

    return ret;
}
