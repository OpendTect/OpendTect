/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegymanip.cc,v 1.4 2011-03-02 16:11:04 cvsbert Exp $";

#include "uisegymanip.h"

#include "uitextedit.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uifileinput.h"
#include "uicompoundparsel.h"
#include "uisplitter.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uimsg.h"
#include "uisegydef.h"

#include "segyhdr.h"
#include "segyhdrdef.h"
#include "segyhdrcalc.h"
#include "segyfiledef.h"
#include "strmprov.h"
#include "filepath.h"
#include <iostream>

#define mErrRet(s) { uiMSG().error(s); return false; }

class uiSEGYBinHdrEdDlg : public uiDialog
{
public:

uiSEGYBinHdrEdDlg( uiParent* p, SEGY::BinHeader& h )
    : uiDialog(p,Setup("SEG-Y Binary Header",mNoDlgTitle,mTODOHelpID))
    , hdr_(h)
    , def_(SEGY::BinHeader::hdrDef())
    , orgns_(h.nrSamples())
    , orgfmt_(h.format())
    , havechg_(false)
{
    const int nrrows = def_.size();
    tbl_ = new uiTable( this, uiTable::Setup(nrrows,2),
	    		      "Bin header table" );
    tbl_->setColumnLabel( 0, "Byte" );
    tbl_->setColumnToolTip( 0, "Byte location in binary header" );
    tbl_->setColumnReadOnly( 0, true );
    tbl_->setColumnLabel( 1, "Value" );
    tbl_->setColumnToolTip( 1, "Value (initially from file)" );

    for ( int irow=0; irow<nrrows; irow++ )
    {
	const SEGY::HdrEntry& he = *def_[irow];
	tbl_->setRowLabel( irow, he.name() );
	tbl_->setRowToolTip( irow, he.description() );
	tbl_->setValue( RowCol(irow,0), he.bytepos_ );
	tbl_->setValue( RowCol(irow,1),
			he.getValue(hdr_.buf(),hdr_.isSwapped()) );
    }

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
    BufferString ret( hdr_.isSwapped() ? "[Swapped] " : "" );
    ret.add( "ns=" ).add( hdr_.nrSamples() )
       .add( " ; fmt=" )
       .add( SEGY::FilePars::nameOfFmt( hdr_.format(), false ) );
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
				  mTODOHelpID) )
    , fname_(fnm)
    , txthdr_(*new SEGY::TxtHeader)
    , binhdr_(*new SEGY::BinHeader)
    , calcset_(*new SEGY::HdrCalcSet(SEGY::TrcHeader::hdrDef()))
    , errlbl_(0)
{
    if ( !openFile() )
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
    fillAvtrcHdrFld( 0 );

    uiToolButton* addbut = new uiToolButton( grp, uiToolButton::RightArrow,
					    "Add to calculated list", addcb );
    addbut->attach( centeredRightOf, llb );
    trchdrfld_ = new uiListBox( grp, "Defined calculations" );
    trchdrfld_->attach( rightTo, llb );
    trchdrfld_->attach( ensureRightOf, addbut );
    trchdrfld_->selectionChanged.notify( mCB(this,uiSEGYFileManip,selChg) );
    trchdrfld_->doubleClicked.notify( edcb );
    trchdrfld_->setHSzPol( uiObject::Medium );

    edbut_ = new uiToolButton( grp, "edit.png", "Edit calculation", edcb );
    edbut_->attach( rightOf, trchdrfld_ );
    rmbut_ = new uiToolButton( grp, "trashcan.png",
		    "Remove calculation", mCB(this,uiSEGYFileManip,rmReq) );
    rmbut_->attach( alignedBelow, edbut_ );
    uiToolButton* openbut = new uiToolButton( grp, "openset.png",
		    "Open stored calculation set",
		    mCB(this,uiSEGYFileManip,openReq) );
    openbut->attach( alignedBelow, rmbut_ );
    savebut_ = new uiToolButton( grp, "save.png",
		    "Save calculation set",
		    mCB(this,uiSEGYFileManip,saveReq) );
    savebut_->attach( alignedBelow, openbut );

    const int nrrows = def.size();
    thtbl_ = new uiTable( grp, uiTable::Setup(nrrows,2),
	    		      "Trace header table" );
    thtbl_->setColumnLabel( 0, "Byte" );
    thtbl_->setColumnToolTip( 0, "Byte location in binary header" );
    thtbl_->setColumnReadOnly( 0, true );
    thtbl_->setColumnLabel( 1, "Value" );
    thtbl_->setColumnToolTip( 1, "Resulting value" );
    thtbl_->setColumnReadOnly( 1, true );

    for ( int irow=0; irow<nrrows; irow++ )
    {
	const SEGY::HdrEntry& he = *def[irow];
	thtbl_->setRowLabel( irow, he.name() );
	thtbl_->setRowToolTip( irow, he.description() );
    }
    thtbl_->attach( ensureRightOf, edbut_ );

    grp->setHAlignObj( llb );
    return grp;
}


bool uiSEGYFileManip::openFile()
{
    sd_ = StreamProvider(fname_).makeIStream();
    if ( !sd_.usable() )
	{ errmsg_ = "Cannot open input file"; return false; }

    strm().read( (char*)txthdr_.txt_, SegyTxtHeaderLength );
    if ( strm().gcount() < SegyTxtHeaderLength )
	{ errmsg_ = "Input file is too small to be a SEG-Y file:\n"
	            "Cannot fully read the text header"; return false; }
    char buf[SegyBinHeaderLength];
    strm().read( buf, SegyBinHeaderLength );
    if ( strm().gcount() < SegyBinHeaderLength )
	{ errmsg_ = "Input file is too small to be a SEG-Y file:\n"
	    	    "Cannot read full binary header"; return false; }

    binhdr_.setInput( buf );
    binhdr_.guessIsSwapped();
    return true;
}


void uiSEGYFileManip::fillAvtrcHdrFld( int selidx )
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
    if ( selidx >= 0 )
	avtrchdrsfld_->setCurrentItem( selidx );
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


void uiSEGYFileManip::selChg( CallBacker* )
{
    const bool havesel = !trchdrfld_->isEmpty()
			&& trchdrfld_->currentItem() >= 0;
    edbut_->setSensitive( havesel );
    savebut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel );
}


void uiSEGYFileManip::addReq( CallBacker* )
{
    const int selidx = avtrchdrsfld_->currentItem();
    if ( selidx < 0 ) return;
    const char* nm = avtrchdrsfld_->textOfItem( selidx );
    const SEGY::HdrDef&	def = SEGY::TrcHeader::hdrDef();
    const int hdridx = calcset_.hdrDef().indexOf( nm );
    if ( hdridx < 0 ) { pErrMsg("Huh" ); return; }

    SEGY::HdrCalc* hc = new SEGY::HdrCalc( *calcset_.hdrDef()[hdridx], "" );
    // TODO uiSEGYFileManipHdrCalcEd dlg( this, *hc, calcset_ );
    // if ( !dlg.go() )
	// delete hc;
    // else
    {
	calcset_.add( hc );
	fillDefCalcs( calcset_.size()-1 );
	trchdrdefined_[selidx] = true;
	fillAvtrcHdrFld( selidx );
    }
}


void uiSEGYFileManip::edReq( CallBacker* )
{
    const int selidx = trchdrfld_->currentItem();
    if ( selidx < 0 ) return;
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
    fillAvtrcHdrFld( avidx );
    avtrchdrsfld_->setCurrentItem( nm );
}


void uiSEGYFileManip::openReq( CallBacker* )
{
}


void uiSEGYFileManip::saveReq( CallBacker* )
{
}


bool uiSEGYFileManip::acceptOK( CallBacker* )
{
    if ( errlbl_ )
	return true;

    const BufferString fnm = fnmfld_->fileName();
    FilePath inpfp( fname_ );
    FilePath outfp( fnmfld_->fileName() );
    if ( outfp.isEmpty() )
	mErrRet("Please enter an output filename" )
    if ( !outfp.isAbsolute() )
	outfp.setPath( inpfp.pathOnly() );
    if ( inpfp == outfp )
	mErrRet("input and output file cannot be the same" )

    txthdr_.setText( txthdrfld_->text() );

    fname_ = fnm;
    uiMSG().warning( "Not impl: no output generated" );
    return true;
}
