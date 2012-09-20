
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uimadiosel.h"
#include "madio.h"
#include "uibutton.h"
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
#include "file.h"
#include "keystrs.h"


static const char* sKeyScons = "Scons";

uiMadIOSelDlg::uiMadIOSelDlg( uiParent* p, IOPar& iop, bool isinp )
	: uiDialog(p, Setup(BufferString("Processing ",isinp?"input":"output"),
		    BufferString("Specify the ",isinp?"input to":"output of",
					 " the processing flow"),"103.5.1") )
	, ctio3d_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,isinp))
	, ctio2d_(*uiSeisSel::mkCtxtIOObj(Seis::Line,isinp))
	, ctiops3d_(*uiSeisSel::mkCtxtIOObj(Seis::VolPS,isinp))
	, ctiops2d_(*uiSeisSel::mkCtxtIOObj(Seis::LinePS,isinp))
	, seis3dfld_(0), seis2dfld_(0), seisps3dfld_(0), seisps2dfld_(0)
	, subsel3dfld_(0), subsel2dfld_(0), subsel2dpsfld_(0)
    	, idx3d_(-1), idx2d_(-1)
	, iop_(iop)
        , isinp_(isinp)
{
    const bool have2d = SI().has2D();
    const bool have3d = SI().has3D();
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
    mAdd( ODMad::sKeyMadagascar(), idxmad_ );
    idxsu_ = -1;
    if ( isinp )
	mAdd( "SU", idxsu_ );

    mAdd( sKey::None(), idxnone_ );

    typfld_ = new uiGenInput( this, isinp ? "Input" : "Output",
	    		      StringListInpSpec(seistypes) );
    typfld_->valuechanged.notify( mCB(this,uiMadIOSelDlg,typSel) );
    if ( have3d )
    {
	seis3dfld_ = new uiSeisSel( this, ctio3d_, uiSeisSel::Setup(Seis::Vol));
	seis3dfld_->attach( alignedBelow, typfld_ );
	seis3dfld_->selectionDone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	seisps3dfld_ = new uiSeisSel( this, ctiops3d_,
				      uiSeisSel::Setup(Seis::VolPS));
	seisps3dfld_->attach( alignedBelow, typfld_ );
	seisps3dfld_->selectionDone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	if ( isinp )
	{
	    subsel3dfld_ = new uiSeis3DSubSel( this, Seis::SelSetup(Seis::Vol)
					.onlyrange(true) );
	    subsel3dfld_->attach( alignedBelow, seis3dfld_ );
	}
    }
    if ( have2d )
    {
	seis2dfld_ = new uiSeisSel( this, ctio2d_,uiSeisSel::Setup(Seis::Line));
	seis2dfld_->attach( alignedBelow, typfld_ );
	seis2dfld_->selectionDone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	seisps2dfld_ = new uiSeisSel( this, ctiops2d_,
					uiSeisSel::Setup(Seis::LinePS) );
	seisps2dfld_->attach( alignedBelow, typfld_ );
	seisps2dfld_->selectionDone.notify( mCB(this,uiMadIOSelDlg,selChg) );
	subsel2dfld_ = new uiSeis2DSubSel( this, Seis::SelSetup(Seis::Line)
						 .fornewentry(!isinp));
	subsel2dfld_->attach( alignedBelow, seis2dfld_ );
	subsel2dpsfld_ = new uiSeis2DSubSel( this,Seis::SelSetup(Seis::LinePS)
						  .fornewentry(!isinp));
	subsel2dpsfld_->attach( alignedBelow, seis2dfld_ );
    }

    uiFileInput::Setup fisu;
    fisu.defseldir( ODMad::FileSpec::defPath() ).forread( isinp );
    madfld_ = new uiFileInput( this, "Select file", fisu );
    madfld_->attach( alignedBelow, typfld_ );

    sconsfld_ = new uiCheckBox( this, "SCons script",
	    			mCB(this,uiMadIOSelDlg,sconsCB) );
    sconsfld_->attach( rightTo, madfld_ );

    postFinalise().notify( mCB(this,uiMadIOSelDlg,initWin) );
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


bool uiMadIOSelDlg::isSU() const
{
    return typfld_->getIntValue() == idxsu_;
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
    const bool filesel = choice == idxmad_ || choice == idxsu_;
    madfld_->display( filesel );
    sconsfld_->display( choice == idxmad_ );
    if ( choice == idxsu_ )
	madfld_->setFilter( "*.su" );
    else if ( choice == idxmad_ )
	sconsCB(0);

    if ( subsel3dfld_ )
	subsel3dfld_->display( choice == idx3d_ || choice == idxps3d_ );
    if ( subsel2dfld_ )
	subsel2dfld_->display( choice == idx2d_ );
    if ( subsel2dpsfld_ )
	subsel2dpsfld_->display( choice == idxps2d_ );
}


void uiMadIOSelDlg::sconsCB( CallBacker* )
{
    if ( typfld_->getIntValue() != idxmad_ )
	return;

    madfld_->setFilter( sconsfld_->isChecked() ? "*" : "*.rsf" );
}


void uiMadIOSelDlg::selChg( CallBacker* )
{
    if ( isMad() || isNone() || !isinp_ ) return;

    const Seis::GeomType gt = geomType();
    if ( !seisSel(gt)->commitInput() ) return;

    CtxtIOObj& ctio = ctxtIOObj( gt );
    uiSeisSubSel* subsel = seisSubSel( gt );
    if ( !ctio.ioobj )
	subsel->clear();
    else if ( gt == Seis::Line )
    {
	mDynamicCastGet(uiSeis2DSubSel*,subsel2d,subsel);
	subsel2d->setInputWithAttrib( *ctio.ioobj, seisSel(gt)->attrNm() );
    }
    else
	subsel->setInput( *ctio.ioobj );
}


void uiMadIOSelDlg::usePar( const IOPar& iop )
{
    bool istypselected = false;
    if ( !iop.find(sKey::Type()) )
	typfld_->setValue( 0 );
    else
	istypselected = true;

    ODMad::ProcFlow::IOType iot = ODMad::ProcFlow::ioType( iop );
    const Seis::GeomType gt = (Seis::GeomType)iot;
    if ( istypselected )
	typfld_->setText( iot == ODMad::ProcFlow::Madagascar
			? ODMad::sKeyMadagascar()
			: iot == ODMad::ProcFlow::None ? sKey::None().str()
						       : Seis::nameOf(gt) );
    typSel( this );
    if ( iot == ODMad::ProcFlow::None ) return;

    if ( iot == ODMad::ProcFlow::Madagascar || iot == ODMad::ProcFlow::SU )
    {
	if ( iot == ODMad::ProcFlow::Madagascar )
	{
	    bool isscons = false;
	    iop.getYN( sKeyScons, isscons );
	    sconsfld_->setChecked( isscons );
	}

	BufferString txt;
	if ( iop.get(sKey::FileName(),txt) )
	    madfld_->setFileName( txt );
	return;
    }

    seisSel( gt )->usePar( iop );
    selChg( this );
    uiSeisSubSel* subsel = seisSubSel( gt );
    if ( subsel )
    {
	PtrMan<IOPar> subpar = iop.subselect( sKey::Subsel() );
	if ( subpar ) subsel->usePar( *subpar );
    }
}


bool uiMadIOSelDlg::fillPar( IOPar& iop )
{
    iop.setEmpty();
    ODMad::ProcFlow::setIOType( iop, ioType() );
    if ( isMad() )
    {
	iop.setYN( sKeyScons, sconsfld_->isChecked() );
	iop.set( sKey::FileName(), madfld_->fileName() );
    }
    else if ( isSU() )
	iop.set( sKey::FileName(), madfld_->fileName() );
    else if ( !isNone() )
    {
	const Seis::GeomType gt = geomType();
	seisSel(gt)->fillPar( iop );
	uiSeisSubSel* subsel = seisSubSel( gt );
	if ( subsel )
	{
	    IOPar subpar;
	    if ( !subsel->fillPar(subpar) )
		return false;

	    if ( subpar.size() )
		iop.mergeComp( subpar, sKey::Subsel() );
	}
    }

    return true;
}


#define mErrRet(s) \
{ \
    uiMSG().error( "Please select the ", isinp_ ? "input " : "output ", s ); \
    return false; \
}

bool uiMadIOSelDlg::getInp()
{
    if ( isMad() || isSU() )
    {
	const BufferString fnm( madfld_->fileName() );
	if ( fnm.isEmpty() || (isinp_ && !File::exists(fnm)) )
	    mErrRet("file")
    }
    else if ( !isNone() )
    {
	const Seis::GeomType gt = geomType();
	if ( !seisSel(gt)->commitInput() )
	{
	    mErrRet(Seis::isPS(gt) ? "data store" : "seismics")
	    if ( !isinp_ && !Seis::is2D(gt) && ctio3d_.ioobj->implExists(false)
	       && !uiMSG().askOverwrite("Output cube exists. Overwrite?") )
		return false;
	}
    }

    return true;
}


bool uiMadIOSelDlg::acceptOK( CallBacker* )
{
    if ( !getInp() )
	return false;

    return fillPar( iop_ );
}


uiMadIOSel::uiMadIOSel( uiParent* p, bool isinp )
	: uiCompoundParSel(p,isinp ? "INPUT" : "OUTPUT")
	, iop_(BufferString("Madagascar ",isinp?"input":"output"," selection"))
	, isinp_(isinp)
        , selectionMade(this)
{
    butPush.notify( mCB(this,uiMadIOSel,doDlg) );
}


void uiMadIOSel::usePar( const IOPar& iop )
{
    iop_ = iop;
    updateSummary();
}


void uiMadIOSel::useParIfNeeded( const IOPar& iop )
{
    if ( iop_.find(sKey::Type()) ) return;

    BufferString typ( iop.find(sKey::Type()) );
    if ( typ.isEmpty() || typ != Seis::nameOf(Seis::Line) ) return;

    BufferString idval( iop.find(sKey::ID()) );
    if ( !idval.isEmpty() && !iop_.find(sKey::ID()) )
	iop_.set( sKey::ID(), idval );

    iop_.set( sKey::Type(), typ );
    const char* lkey = IOPar::compKey( sKey::Subsel(), sKey::LineKey() );
    BufferString lnm( iop.find(lkey) );
    if ( !lnm.isEmpty() && !iop_.find(lkey) )
	iop_.set( lkey, lnm );

    updateSummary();
}


void uiMadIOSel::doDlg( CallBacker* )
{
    uiMadIOSelDlg dlg( this, iop_, isinp_ );
    if ( dlg.go() )
	selectionMade.trigger();
}


BufferString uiMadIOSel::getSummary() const
{
    BufferString ret( "-" );

    if ( !iop_.find(sKey::Type()) )
	return ret;

    ODMad::ProcFlow::IOType iot = ODMad::ProcFlow::ioType( iop_ );
    if ( iot == ODMad::ProcFlow::None )
	ret = sKey::None();
    else if ( iot == ODMad::ProcFlow::Madagascar )
	ret = iop_.find( sKey::FileName() );
    else
	ret = IOM().nameOf( iop_.find("ID") );

    return ret;
}
