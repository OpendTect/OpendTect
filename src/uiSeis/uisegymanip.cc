/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uisegymanip.cc,v 1.25 2012-08-01 14:44:27 cvshelene Exp $";

#include "uisegymanip.h"
#include "uisegytrchdrvalplot.h"

#include "uitextedit.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uicompoundparsel.h"
#include "uisplitter.h"
#include "uispinbox.h"
#include "uilineedit.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "uisegydef.h"
#include "uiselsimple.h"

#include "segyhdrdef.h"
#include "segyhdrcalc.h"
#include "segyfiledef.h"
#include "strmprov.h"
#include "strmoper.h"
#include "filepath.h"
#include "executor.h"
#include "oddirs.h"

static const od_int64 cFileHeaderSize = SegyTxtHeaderLength+SegyBinHeaderLength;
#define mErrRet(s) { uiMSG().error(s); return false; }


class uiSEGYBinHdrEdDlg : public uiDialog
{
public:

uiSEGYBinHdrEdDlg( uiParent* p, SEGY::BinHeader& h )
    : uiDialog(p,Setup("SEG-Y Binary Header",mNoDlgTitle,"103.0.20"))
    , hdr_(h)
    , def_(SEGY::BinHeader::hdrDef())
    , orgns_(h.nrSamples())
    , orgfmt_(h.format())
    , havechg_(false)
{
    const int nrrows = def_.size();
    tbl_ = new uiTable( this, uiTable::Setup(nrrows,3).manualresize(true),
	    		      "Bin header table" );
    tbl_->setColumnLabel( 0, "Byte" );
    tbl_->setColumnToolTip( 0, "Byte location in binary header" );
    tbl_->setColumnReadOnly( 0, true );
    tbl_->setColumnLabel( 1, "Value" );
    tbl_->setColumnToolTip( 1, "Value (initially from file)" );
    tbl_->setColumnLabel( 2, "Description" );
    tbl_->setColumnReadOnly( 2, true );

    for ( int irow=0; irow<nrrows; irow++ )
    {
	const SEGY::HdrEntry& he = *def_[irow];
	tbl_->setRowLabel( irow, he.name() );
	tbl_->setRowToolTip( irow, he.description() );
	tbl_->setValue( RowCol(irow,0), he.bytepos_+1 );
	tbl_->setValue( RowCol(irow,1),
			he.getValue(hdr_.buf(),hdr_.isSwapped()) );
	tbl_->setText( RowCol(irow,2), he.description() );
    }
    tbl_->setColumnResizeMode( uiTable::Interactive );
    tbl_->resizeColumnToContents( 2 );

#define mSetUsedRow(nm) \
    tbl_->setLabelBGColor( SEGY::BinHeader::Entry##nm(), Color::Peach(), true )
    mSetUsedRow(Dt);
    mSetUsedRow(Ns);
    mSetUsedRow(Fmt);
    mSetUsedRow(MFeet);
    mSetUsedRow(RevCode);

    tbl_->valueChanged.notify( mCB(this,uiSEGYBinHdrEdDlg,valChg) );
}

void valChg( CallBacker* )
{
    havechg_ = true;
}

bool acceptOK( CallBacker* )
{
    if ( !havechg_ )
	return true;

    const int ns = tbl_->getIntValue( RowCol(SEGY::BinHeader::EntryNs(),1) );
    if ( ns < 1 )
	mErrRet("The 'hns' entry (number of samples) must be > 0")

    const short fmt = (short)tbl_->getIntValue(
	    			RowCol(SEGY::BinHeader::EntryFmt(),1) );
    if ( !SEGY::BinHeader::isValidFormat(fmt) )
	mErrRet("The 'format' entry must be 1,3,5 or 8 (see row tooltip).\n")

    const int orgfmtbts = SEGY::BinHeader::formatBytes(orgfmt_);
    const int fmtbts = SEGY::BinHeader::formatBytes(fmt);
    if ( orgns_ * orgfmtbts != ns * fmtbts )
    {
	BufferString msg( "You have changed the number of bytes per trace:\n" );
	msg	.add( "\nWas: format=" ).add( orgfmt_ )
		.add( " hns=" ).add( orgns_ )
		.add( " => " ).add( orgns_ * orgfmtbts ).add( " b/trc" )
		.add( "\nNow: format=" ).add( fmt )
		.add( " hns=" ).add( ns )
		.add( " => " ).add( ns * fmtbts ).add( " b/trc" );
	msg += "\n\nContinue?";
	if ( !uiMSG().askGoOn(msg,true) )
	    return false;
    }

    const int nrrows = def_.size();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	const SEGY::HdrEntry& he = *def_[irow];
	const int val = tbl_->getIntValue( RowCol(irow,1) );
	he.putValue( hdr_.buf(), val );
    }

    return true;
}

    SEGY::BinHeader&	hdr_;
    const SEGY::HdrDef&	def_;
    int			orgns_;
    short		orgfmt_;
    bool		havechg_;

    uiTable*		tbl_;

};


class uiSEGYBinHdrEd : public uiCompoundParSel
{
public:

uiSEGYBinHdrEd( uiParent* p, SEGY::BinHeader& h )
    : uiCompoundParSel(p,"Binary header","&Change")
    , hdr_(h)
{
    txtfld_->setElemSzPol( uiObject::WideVar );
    butPush.notify( mCB(this,uiSEGYBinHdrEd,doDlg) );
}

BufferString getSummary() const
{
    BufferString ret;
    if ( hdr_.isSwapped() )
	ret.add( "[Swapped] " );
    if ( hdr_.isRev1() )
	ret.add( "[Rev 1] " );
    if ( hdr_.isInFeet() )
	ret.add( "[feet] " );
    ret.add( hdr_.nrSamples() ).add( " samples (" )
	.add( hdr_.bytesPerSample() ).add( "-byte), interval=" )
	.add( hdr_.sampleRate(true) );
    return ret;
}

void doDlg( CallBacker* )
{
    uiSEGYBinHdrEdDlg dlg( this, hdr_ );
    if ( dlg.go() )
	updateSummary();
}

    SEGY::BinHeader&	hdr_;

};


uiSEGYFileManip::uiSEGYFileManip( uiParent* p, const char* fnm )
    : uiDialog(p,uiDialog::Setup("Manipulate SEG-Y File",
				  BufferString("Manipulate '",fnm,"'"),
				  "103.0.19") )
    , fname_(fnm)
    , txthdr_(*new SEGY::TxtHeader)
    , binhdr_(*new SEGY::BinHeader)
    , calcset_(*new SEGY::HdrCalcSet(SEGY::TrcHeader::hdrDef()))
    , errlbl_(0)
{
    if ( !openInpFile() )
	{ errlbl_ = new uiLabel( this, errmsg_ ); return; }

    uiGroup* filehdrgrp = new uiGroup( this, "File header group" );
    txthdrfld_ = new uiTextEdit( filehdrgrp, "Text Header Editor" );
    txthdrfld_->setDefaultWidth( 80 );
    txthdrfld_->setDefaultHeight( 5 );
    txthdrfld_->setPrefHeightInChar( 5 );
    BufferString txt; txthdr_.getText( txt );
    txthdrfld_->setText( txt );
    uiLabel* lbl = new uiLabel( filehdrgrp, "Text Header" );
    lbl->attach( centeredLeftOf, txthdrfld_ );

    binhdrfld_ = new uiSEGYBinHdrEd( filehdrgrp, binhdr_ );
    binhdrfld_->attach( alignedBelow, txthdrfld_ );

    uiGroup* trchdrgrp = mkTrcGroup();
    uiSplitter* spl = new uiSplitter( this, "Splitter", false );
    spl->addGroup( filehdrgrp );
    spl->addGroup( trchdrgrp );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, spl );

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( false );
    fnmfld_ = new uiFileInput( this, "Output file", fisu );
    FilePath inpfp( fname_ );
    fnmfld_->setDefaultSelectionDir( inpfp.pathOnly() );
    fnmfld_->attach( ensureBelow, sep );
    fnmfld_->attach( hCentered );

    postFinalise().notify( mCB(this,uiSEGYFileManip,initWin) );
}


uiSEGYFileManip::~uiSEGYFileManip()
{
    delete &txthdr_;
    delete &binhdr_;
    delete &calcset_;
}


uiGroup* uiSEGYFileManip::mkTrcGroup()
{
    uiGroup* grp = new uiGroup( this, "Trace header group" );
    const CallBack addcb( mCB(this,uiSEGYFileManip,addReq) );
    const CallBack edcb( mCB(this,uiSEGYFileManip,edReq) );

    uiLabeledListBox* llb = new uiLabeledListBox( grp, "Trace headers", false,
	    				uiLabeledListBox::LeftMid );
    avtrchdrsfld_ = llb->box();
    avtrchdrsfld_->setHSzPol( uiObject::Small );
    const SEGY::HdrDef&	def = calcset_.hdrDef();
    for ( int idx=0; idx<def.size(); idx++ )
	trchdrdefined_ += false;
    avtrchdrsfld_->doubleClicked.notify( addcb );
    fillAvTrcHdrFld( 0 );

    uiToolButton* addbut = new uiToolButton( grp, uiToolButton::RightArrow,
					    "Add to calculated list", addcb );
    addbut->attach( centeredRightOf, llb );
    trchdrfld_ = new uiListBox( grp, "Defined calculations" );
    trchdrfld_->attach( rightTo, llb );
    trchdrfld_->attach( ensureRightOf, addbut );
    trchdrfld_->selectionChanged.notify( mCB(this,uiSEGYFileManip,selChg) );
    trchdrfld_->doubleClicked.notify( edcb );
    trchdrfld_->setHSzPol( uiObject::Medium );

    edbut_ = new uiToolButton( grp, "edit", "Edit calculation", edcb );
    edbut_->attach( rightOf, trchdrfld_ );
    rmbut_ = new uiToolButton( grp, "trashcan", "Remove calculation",
		    mCB(this,uiSEGYFileManip,rmReq) );
    rmbut_->attach( alignedBelow, edbut_ );
    uiToolButton* openbut = new uiToolButton( grp, "openset",
		    "Open stored calculation set",
		    mCB(this,uiSEGYFileManip,openReq) );
    openbut->attach( alignedBelow, rmbut_ );
    savebut_ = new uiToolButton( grp, "save", "Save calculation set",
		    mCB(this,uiSEGYFileManip,saveReq) );
    savebut_->attach( alignedBelow, openbut );

    const int nrrows = def.size();
    uiTable::Setup tsu( nrrows, 2 ); tsu.selmode( uiTable::SingleRow );
    thtbl_ = new uiTable( grp, tsu, "Trace header table" );
    thtbl_->setColumnLabel( 0, "Byte" );
    thtbl_->setColumnToolTip( 0, "Byte location in trace header" );
    thtbl_->setColumnReadOnly( 0, true );
    thtbl_->setColumnLabel( 1, "Value" );
    thtbl_->setColumnToolTip( 1, "Resulting value" );
    thtbl_->setColumnReadOnly( 1, true );
    for ( int irow=0; irow<nrrows; irow++ )
    {
	const SEGY::HdrEntry& he = *def[irow];
	thtbl_->setRowLabel( irow, he.name() );
	thtbl_->setRowToolTip( irow, he.description() );
	thtbl_->setValue( RowCol(irow,0), he.bytepos_ + 1 );
    }
    thtbl_->attach( ensureRightOf, edbut_ );
    thtbl_->setStretch( 0, 1 );
    thtbl_->selectionChanged.notify( mCB(this,uiSEGYFileManip,rowClck) );

    plotbut_ = new uiToolButton( grp, "distmap",
		    "Plot the values of the selected header entries",
		    mCB(this,uiSEGYFileManip,plotReq) );
    plotbut_->attach( alignedBelow, thtbl_ );
    plotbut_->setSensitive( false );
    plotallbox_ = new uiCheckBox( grp, "All" );
    plotallbox_->attach( rightOf, plotbut_ );
    plotallbox_->setHSzPol( uiObject::Small );
    plotallbox_->setChecked( true );

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( grp, "Trc" );
    trcnrfld_ = lsb->box();
    lsb->attach( rightAlignedBelow, thtbl_ );
    trcnrfld_->setHSzPol( uiObject::Small );
    trcnrfld_->setMinValue( 1 );
    trcnrfld_->setMaxValue( mUdf(int) );
    trcnrfld_->setValue( 1 );
    trcnrfld_->valueChanging.notify( mCB(this,uiSEGYFileManip,trcNrChg) );

    grp->setHAlignObj( llb );
    return grp;
}


bool uiSEGYFileManip::openInpFile()
{
    sd_ = StreamProvider(fname_).makeIStream();
    if ( !sd_.usable() )
	{ errmsg_ = "Cannot open input file"; return false; }

    if ( !StrmOper::readBlock( strm(), txthdr_.txt_, SegyTxtHeaderLength ) )
	{ errmsg_ = "Input file is too small to be a SEG-Y file:\n"
	            "Cannot fully read the text header"; return false; }
    char buf[SegyBinHeaderLength];
    if ( !StrmOper::readBlock( strm(), buf, SegyBinHeaderLength ) )
	{ errmsg_ = "Input file is too small to be a SEG-Y file:\n"
	    	    "Cannot read full binary header"; return false; }

    txthdr_.setAscii();
    binhdr_.setInput( buf );
    binhdr_.guessIsSwapped();

    StrmOper::seek( strm(), 0, std::ios::end );
    filesize_ = StrmOper::tell( strm() );

    return true;
}


void uiSEGYFileManip::fillAvTrcHdrFld( int selidx )
{
    avtrchdrsfld_->setEmpty();
    for ( int idx=0; idx<calcset_.hdrDef().size(); idx++ )
    {
	if ( !trchdrdefined_[idx] )
	    avtrchdrsfld_->addItem( calcset_.hdrDef()[idx]->name() );
    }

    if ( selidx < 0 ) selidx = 0;
    if ( selidx >= avtrchdrsfld_->size() )
	selidx = avtrchdrsfld_->size()-1;
    if ( selidx < 0 ) return;

    for ( int idx=selidx; idx<calcset_.hdrDef().size(); idx++ )
    {
	if ( !trchdrdefined_[idx] )
	{
	    avtrchdrsfld_->setCurrentItem( calcset_.hdrDef()[idx]->name() );
	    break;
	}
    }
}


void uiSEGYFileManip::fillDefCalcs( int selidx )
{
    trchdrfld_->setEmpty();
    for ( int idx=0; idx<calcset_.size(); idx++ )
    {
	const SEGY::HdrCalc& hc = *calcset_[idx];
	trchdrfld_->addItem( BufferString( hc.he_.name(), " = ", hc.def_ ) );
    }

    if ( selidx < 0 ) selidx = 0;
    if ( selidx >= calcset_.size() )
	selidx = calcset_.size() - 1;
    if ( selidx >= 0 )
	trchdrfld_->setCurrentItem( selidx );
}


void uiSEGYFileManip::updTrcVals()
{
    memcpy( curhdrbuf_, inphdrbuf_, SegyTrcHeaderLength );
    calcset_.reSetSeqNr( trcnrfld_->getValue() );
    calcset_.apply( curhdrbuf_, binhdr_.isSwapped() );
    for ( int idx=0; idx<calcset_.hdrDef().size(); idx++ )
    {
	const SEGY::HdrEntry& he = *calcset_.hdrDef()[idx];
	thtbl_->setValue( RowCol(idx,1), he.getValue(curhdrbuf_,false) );
    }
}


void uiSEGYFileManip::initWin( CallBacker* )
{
    selChg( 0 );
    trcNrChg( 0 );
}


void uiSEGYFileManip::selChg( CallBacker* )
{
    const bool havesel = !trchdrfld_->isEmpty()
			&& trchdrfld_->currentItem() >= 0;
    edbut_->setSensitive( havesel );
    savebut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel );
}

class uiSEGYFileManipHdrCalcEd : public uiDialog
{
public:

uiSEGYFileManipHdrCalcEd( uiParent* p, SEGY::HdrCalc& hc, SEGY::HdrCalcSet& cs )
    : uiDialog( p, Setup("Header Calculation",cs.indexOf(hc.he_.name()) < 0 ?
	    "Add header calculation":"Edit header calculation","103.0.21") )
    , hc_(hc)
    , calcset_(cs)
{
    const CallBack cb( mCB(this,uiSEGYFileManipHdrCalcEd,insTxt) );
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Available", false,
	    				uiLabeledListBox::AboveMid );
    hdrfld_ = llb->box();
    hdrfld_->addItem( calcset_.trcIdxEntry().name() );
    for ( int idx=0; idx<calcset_.hdrDef().size(); idx++ )
	hdrfld_->addItem( calcset_.hdrDef()[idx]->name() );
    hdrfld_->setCurrentItem( hc.he_.name() );
    hdrfld_->doubleClicked.notify( cb );

    uiToolButton* addbut = new uiToolButton( this, uiToolButton::RightArrow,
					     "Insert in formula", cb );
    addbut->attach( centeredRightOf, llb );
    formfld_ = new uiLineEdit( this, "Formula" );
    formfld_->setText( hc_.def_ );
    formfld_->attach( rightOf, addbut );
    formfld_->setHSzPol( uiObject::WideVar );
    formfld_->returnPressed.notify(mCB(this,uiSEGYFileManipHdrCalcEd,acceptOK));
    uiLabel* lbl = new uiLabel( this,
	    		BufferString("Formula for '",hc_.he_.name(),"'") );
    lbl->attach( centeredBelow, formfld_ );
}

void insTxt( CallBacker* )
{
    const int selidx = hdrfld_->currentItem();
    if ( selidx < 0 ) return;
    const int curpos = formfld_->cursorPosition();
    BufferString toins( curpos > 0 ? " " : "" );
    toins.add( hdrfld_->textOfItem(selidx) ).add( " " );
    formfld_->insert( toins );
}


bool acceptOK( CallBacker* )
{
    const char* txt = formfld_->text();
    if ( !txt || !*txt )
	mErrRet("Please enter a formula")

    const SEGY::HdrEntry& he = hc_.he_;
    const int hidx = calcset_.indexOf( he.name() );
    bool setok = false; BufferString emsg;
    if ( hidx >= 0 )
	setok = calcset_.set( hidx, txt, &emsg );
    else
	setok = calcset_.add( he, txt, &emsg );
    if ( !setok )
	mErrRet(emsg)

    return true;
}

    SEGY::HdrCalc&	hc_;
    SEGY::HdrCalcSet&	calcset_;

    uiListBox*		hdrfld_;
    uiLineEdit*		formfld_;

};


void uiSEGYFileManip::addReq( CallBacker* )
{
    const int selidx = avtrchdrsfld_->currentItem();
    if ( selidx < 0 ) return;
    const char* nm = avtrchdrsfld_->textOfItem( selidx );
    const SEGY::HdrDef&	def = calcset_.hdrDef();
    const int hdridx = def.indexOf( nm );
    if ( hdridx < 0 ) { pErrMsg("Huh" ); return; }

    SEGY::HdrCalc hc( *def[hdridx], "" );
    uiSEGYFileManipHdrCalcEd dlg( this, hc, calcset_ );
    if ( dlg.go() )
    {
	fillDefCalcs( calcset_.size()-1 );
	trchdrdefined_[hdridx] = true;
	fillAvTrcHdrFld( hdridx );
	updTrcVals();
    }
}


void uiSEGYFileManip::edReq( CallBacker* )
{
    const int selidx = trchdrfld_->currentItem();
    if ( selidx < 0 ) return;
    uiSEGYFileManipHdrCalcEd dlg( this, *calcset_[selidx], calcset_ );
    if ( dlg.go() )
    {
	fillDefCalcs( selidx );
	updTrcVals();
    }
}


void uiSEGYFileManip::rmReq( CallBacker* )
{
    const int selidx = trchdrfld_->currentItem();
    if ( selidx < 0 ) return;
    const char* nm = calcset_[selidx]->he_.name();

    calcset_.discard( selidx );
    fillDefCalcs( selidx );
    const int avidx = calcset_.hdrDef().indexOf( nm );
    trchdrdefined_[avidx] = false;
    fillAvTrcHdrFld( avidx );
    avtrchdrsfld_->setCurrentItem( nm );

    updTrcVals();
}


void uiSEGYFileManip::openReq( CallBacker* )
{
    BufferStringSet nms; calcset_.getStoredNames( nms );
    if ( nms.isEmpty() )
	{ uiMSG().error("No manipulation sets defined yet"); return; }

    uiSelectFromList::Setup sflsu( "Select header manipulation", nms );
    uiSelectFromList dlg( this, sflsu );
    if ( !dlg.go() || dlg.selection() < 0 )
	return;

    trchdrfld_->setEmpty();
    calcset_.getFromSettings( nms.get(dlg.selection()) );

    fillDefCalcs( 0 );
    updTrcVals();
}


void uiSEGYFileManip::saveReq( CallBacker* )
{
    BufferStringSet nms; calcset_.getStoredNames( nms );
    uiGetObjectName::Setup su( "Store manipulation set", nms );
    uiGetObjectName dlg( this, su );
    if ( !dlg.go() ) return;

    calcset_.setName( dlg.text() );
    if ( !calcset_.storeInSettings() )
	uiMSG().error( "Could not write to the user settings file:\n",
		    GetSettingsFileName(SEGY::HdrCalcSet::sKeySettsFile()) );
}


od_int64 uiSEGYFileManip::traceBytes() const
{
    return SegyTrcHeaderLength + binhdr_.nrSamples() * binhdr_.bytesPerSample();
}


void uiSEGYFileManip::trcNrChg( CallBacker* )
{
    const od_int64 tbyts = traceBytes();
    od_int64 offs = cFileHeaderSize + (trcnrfld_->getValue()-1) * tbyts;
    if ( offs >= filesize_ )
    {
	const od_int64 nrtrcsinfile = (filesize_-cFileHeaderSize) / tbyts;
	if ( nrtrcsinfile < 1 )
	    uiMSG().error( "File contains no complete traces" );
	else
	{
	    offs = cFileHeaderSize + (nrtrcsinfile-1) * tbyts;
	    NotifyStopper ns( trcnrfld_->valueChanging );
	    trcnrfld_->setValue( (int)nrtrcsinfile );
	}
    }

    StrmOper::seek( strm(), offs );
    StrmOper::readBlock( strm(), inphdrbuf_, SegyTrcHeaderLength );

    updTrcVals();
}


void uiSEGYFileManip::rowClck( CallBacker* cb )
{
    rowSel( thtbl_->currentRow() );
}


class uiSEGYFileManipDataExtracter : public Executor
{
public:

uiSEGYFileManipDataExtracter( uiSEGYFileManip* p, const TypeSet<int>& sel,
       			      bool plotall )
    : Executor("Trace header scan")
    , fm_(*p)
    , sel_(sel)
    , nrdone_(0)
    , hdef_(SEGY::TrcHeader::hdrDef())
    , needswap_(p->binhdr_.isSwapped())
{
    StrmOper::seek( fm_.strm(), cFileHeaderSize );
    trcrg_.start = 1;
    totalnr_ = (fm_.filesize_-cFileHeaderSize) / fm_.traceBytes();
    trcrg_.stop = (int)totalnr_;
    if ( !plotall )
    {
	DataInpSpec* spec = new IntInpIntervalSpec( trcrg_ );
	uiGenInputDlg dlg( p, "Specify range", "Trace range to plot", spec );
	if ( !dlg.go() )
	    { totalnr_ = -1; return; }
	trcrg_ = dlg.getFld(0)->getIInterval();
	trcrg_.sort();
	trcrg_.limitTo( Interval<int>(1,totalnr_) );
    }
    totalnr_ = trcrg_.stop - trcrg_.start + 1;

    for ( int idx=0; idx<sel_.size(); idx++ )
	data_ += new TypeSet<float>;
    fm_.calcset_.reSetSeqNr( 1 );
}

~uiSEGYFileManipDataExtracter()
{
    deepErase( data_ );
}

const char* message() const	{ return "Collecting data"; }
const char* nrDoneText() const	{ return "Traces scanned"; }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totalnr_; }

int nextStep()
{
    if ( totalnr_ < 0 )
	return Finished();

    StrmOper::seek( fm_.strm(), cFileHeaderSize + nrdone_ * fm_.traceBytes() );
    if ( !StrmOper::readBlock(fm_.strm(),buf_,SegyTrcHeaderLength) )
	return Finished();

    fm_.calcset_.apply( buf_, fm_.binhdr_.isSwapped() );

    for ( int idx=0; idx<sel_.size(); idx++ )
	*data_[idx] += hdef_[ sel_[idx] ]->getValue( buf_, needswap_ );

    nrdone_++;
    return nrdone_ < totalnr_ ? MoreToDo() : Finished();
}

    uiSEGYFileManip&		fm_;
    const TypeSet<int>		sel_;
    ObjectSet< TypeSet<float> >	data_;
    unsigned char		buf_[SegyTrcHeaderLength];
    const SEGY::HdrDef&		hdef_;
    const bool			needswap_;
    Interval<int>		trcrg_;
    od_int64			nrdone_;
    od_int64			totalnr_;

};


void uiSEGYFileManip::plotReq( CallBacker* cb )
{
    const int nrrows = thtbl_->nrRows();
    TypeSet<int> selrows;
    for ( int irow=0; irow<nrrows; irow++ )
    {
	if ( thtbl_->isRowSelected(irow) )
	    selrows += irow;
    }
    if ( selrows.isEmpty() ) return;

    uiSEGYFileManipDataExtracter de( this, selrows, plotallbox_->isChecked() );
    if ( de.totalnr_ < 0 )
	return;
    uiTaskRunner tr( this );
    tr.execute( de );
    if ( de.data_[0]->size() < 2 )
	return;

    uiMainWin::Setup su( "Header value plot" );
    su.withmenubar( false ).deleteonclose( true );
    for ( int idx=0; idx<de.data_.size(); idx++ )
    {
	uiMainWin* mw = new uiMainWin( this, su );
	uiSEGYTrcHdrValPlot* vp = new uiSEGYTrcHdrValPlot( mw, true,
							   de.trcrg_.start );
	vp->setData( *calcset_.hdrDef()[ selrows[idx] ],
		     de.data_[idx]->arr(), de.data_[idx]->size() );
	mw->show();
    }
}


void uiSEGYFileManip::rowSel( int rownr )
{
    if ( rownr < 0 ) return;
    plotbut_->setSensitive( true );

    const char* nm = calcset_.hdrDef()[rownr]->name();
    int lidx = avtrchdrsfld_->indexOf( nm );
    if ( lidx >= 0 )
	avtrchdrsfld_->setCurrentItem( lidx );
    else
    {
	lidx = calcset_.indexOf( nm );
	if ( lidx < 0 ) { pErrMsg("Huh"); return; }
	trchdrfld_->setCurrentItem( lidx );
    }
}


bool uiSEGYFileManip::acceptOK( CallBacker* )
{
    if ( errlbl_ )
	return true;

    if ( binhdr_.nrSamples() < 1 )
	{ mErrRet("Binary header's number of samples must be > 0" ) }
    const BufferString fnm = fnmfld_->fileName();
    FilePath inpfp( fname_ );
    FilePath outfp( fnmfld_->fileName() );
    if ( outfp.isEmpty() )
	mErrRet("Please enter an output filename" )
    if ( !outfp.isAbsolute() )
	outfp.setPath( inpfp.pathOnly() );
    if ( inpfp == outfp )
	mErrRet("input and output file cannot be the same" )
    StreamData sdout( StreamProvider(outfp.fullPath()).makeOStream() );
    if ( !sdout.usable() )
	{ mErrRet("Cannot open output file" ) }

    txthdr_.setText( txthdrfld_->text() );
    calcset_.reSetSeqNr( 1 );

    const int bptrc = binhdr_.nrSamples() * binhdr_.bytesPerSample();
    Executor* exec = calcset_.getApplier( strm(), *sdout.ostrm, bptrc,
	   				  &binhdr_, &txthdr_ );
    uiTaskRunner tr( this );
    const bool rv = tr.execute( *exec );
    sdout.close(); delete exec;

    if ( rv )
	fname_ = fnm;
    return rv;
}
