/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
________________________________________________________________________

-*/

#include "uisegymanip.h"
#include "uisegytrchdrvalplot.h"

#include "uitextedit.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uifilesel.h"
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
#include "uistrings.h"

#include "segyhdrdef.h"
#include "segyhdrcalc.h"
#include "segyfiledef.h"
#include "filepath.h"
#include "executor.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "od_helpids.h"

static const od_int64 cFileHeaderSize = SegyTxtHeaderLength+SegyBinHeaderLength;
#define mErrRet(s) { uiMSG().error(s); return false; }


class uiSEGYBinHdrEdDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYBinHdrEdDlg);
public:

    mUseType( SEGY,	BinHeader );
    mUseType( SEGY,	HdrEntry );

uiSEGYBinHdrEdDlg( uiParent* p, BinHeader& h )
    : uiDialog(p,Setup(tr("SEG-Y Binary Header"),mNoDlgTitle,
                        mODHelpKey(mSEGYBinHdrEdDlgHelpID) ))
    , hdr_(h)
    , def_(BinHeader::hdrDef())
    , orgns_(h.nrSamples())
    , orgfmt_(h.format())
    , havechg_(false)
{
    const int nrrows = def_.size();
    tbl_ = new uiTable( this, uiTable::Setup(nrrows,3).manualresize(true),
			      "Bin header table" );
    tbl_->setColumnLabel( 0, uiSEGYFileManip::sByte() );
    tbl_->setColumnToolTip( 0, tr("Byte location in binary header") );
    tbl_->setColumnReadOnly( 0, true );
    tbl_->setColumnLabel( 1, uiStrings::sValue() );
    tbl_->setColumnToolTip( 1, tr("Value (initially from file)") );
    tbl_->setColumnLabel( 2, uiStrings::sDescription() );
    tbl_->setColumnReadOnly( 2, true );

    for ( int irow=0; irow<nrrows; irow++ )
    {
	const HdrEntry& he = *def_[irow];
	tbl_->setRowLabel( irow, toUiString(he.name()) );
	tbl_->setRowToolTip( irow, mToUiStringTodo(he.description()) );
	tbl_->setValue( RowCol(irow,0), he.bytepos_+1 );
	tbl_->setValue( RowCol(irow,1),
			he.getValue(hdr_.buf(),hdr_.isSwapped()) );
	tbl_->setText( RowCol(irow,2), he.description() );
    }
    tbl_->setColumnResizeMode( uiTable::Interactive );
    tbl_->resizeColumnToContents( 2 );
    tbl_->setStretch( 2, 2 );

#define mSetUsedRow(nm) \
    tbl_->setLabelBGColor( BinHeader::Entry##nm(), Color::Peach(), true )
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

bool acceptOK()
{
    if ( !havechg_ )
	return true;

    const int ns = tbl_->getIntValue( RowCol(BinHeader::EntryNs(),1) );
    if ( ns < 1 )
	mErrRet(tr("The 'hns' entry (number of samples) must be > 0"))

    const short fmt = (short)tbl_->getIntValue(
				RowCol(BinHeader::EntryFmt(),1) );
    if ( !BinHeader::isValidFormat(fmt) )
	mErrRet(tr("The 'format' entry must be 1,3,5 "
		   "or 8 (see row tooltip).\n"))

    const int orgfmtbts = BinHeader::formatBytes(orgfmt_);
    const int fmtbts = BinHeader::formatBytes(fmt);
    if ( orgns_ * orgfmtbts != ns * fmtbts )
    {
	uiString msg = tr("You have changed the number of bytes per trace:\n"
			  "\nWas: format= %1 hns= %2 => %3 b/trc"
			  "\nNow: format= %4 hns= %5 => %6 b/trc\n\nContinue?")
		     .arg(orgfmt_).arg(orgns_).arg(orgns_ * orgfmtbts)
		     .arg(fmt).arg(ns).arg(ns * fmtbts);
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

    BinHeader&		hdr_;
    const SEGY::HdrDef&	def_;
    int			orgns_;
    short		orgfmt_;
    bool		havechg_;

    uiTable*		tbl_;

};


class uiSEGYBinHdrEd : public uiCompoundParSel
{ mODTextTranslationClass(uiSEGYBinHdrEd);
public:

    mUseType( SEGY,	BinHeader );

uiSEGYBinHdrEd( uiParent* p, BinHeader& h )
    : uiCompoundParSel(p,tr("Binary header"),uiStrings::sChange())
    , hdr_(h)
{
    txtfld_->setElemSzPol( uiObject::WideVar );
    butPush.notify( mCB(this,uiSEGYBinHdrEd,doDlg) );
}

uiString getSummary() const
{
    uiPhrase ret;
    if ( hdr_.isSwapped() )
	ret.appendPhrase( tr("Swapped").optional() );
    if ( hdr_.isRev0() )
	ret.appendPhrase( tr("Rev 0","SEG-Y revision").optional(),
							    uiString::NoSep );
    if ( hdr_.isInFeet() )
	ret.appendPhrase( uiStrings::sFeet(false).toLower().optional(),
				    uiString::NoSep  );
    //have a space
    ret.appendPhrase( tr("%1 samples( %2 - byte), (interval = %3)")
	.arg(hdr_.nrSamples()).arg(hdr_.bytesPerSample()))
	.arg(hdr_.sampleRate(true));
    return ret;
}

void doDlg( CallBacker* )
{
    uiSEGYBinHdrEdDlg dlg( this, hdr_ );
    if ( dlg.go() )
	updateSummary();
}

    BinHeader&	hdr_;

};


uiSEGYFileManip::uiSEGYFileManip( uiParent* p, const char* fnm )
    : uiDialog(p,uiDialog::Setup(tr("Manipulate SEG-Y File"),
				  tr("Manipulate '%1'").arg(fnm),
				  mODHelpKey(mSEGYFileManipHelpID) ) )
    , fname_(fnm)
    , txthdr_(*new TxtHeader)
    , binhdr_(*new BinHeader)
    , calcset_(*new HdrCalcSet(SEGY::TrcHeader::hdrDef()))
    , strm_(0)
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
    uiLabel* lbl = new uiLabel( filehdrgrp, tr("Text Header") );
    lbl->attach( centeredLeftOf, txthdrfld_ );

    binhdrfld_ = new uiSEGYBinHdrEd( filehdrgrp, binhdr_ );
    binhdrfld_->attach( alignedBelow, txthdrfld_ );

    uiGroup* trchdrgrp = mkTrcGroup();
    uiSplitter* spl = new uiSplitter( this, "Splitter", OD::Horizontal );
    spl->addGroup( filehdrgrp );
    spl->addGroup( trchdrgrp );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, spl );

    uiGroup* outgrp = new uiGroup( this, "Output group" );
    selmultifld_ = new uiGenInput( outgrp, tr("Apply to"),
			BoolInpSpec( true, tr("This file"),
			    tr("A selection of files similar to this one") ) );
    selmultifld_->valuechanged.notify( mCB(this,uiSEGYFileManip,destSelCB) );

    uiFileSel::Setup fssu( OD::GeneralContent );
    File::Path inpfp( fname_ );
    fssu.objtype( uiStrings::sSEGY() )
	.formats( uiSEGYFileSpec::fileFmts() )
	.initialselectiondir( inpfp.pathOnly() );
    uiFileSel::Setup singfssu( fssu ); singfssu.setForWrite();
    outfnmfld_ = new uiFileSel( outgrp, uiStrings::sOutputFile(), singfssu );
    outfnmfld_->attach( alignedBelow, selmultifld_ );
    uiFileSel::Setup multfssu( fssu );
    inpfnmsfld_ = new uiFileSel( outgrp, uiStrings::sInputFile(mPlural), fssu );
    inpfnmsfld_->setSelectionMode( OD::SelectMultiFile );
    inpfnmsfld_->attach( alignedBelow, selmultifld_ );
    postfixfld_ = new uiGenInput( outgrp, tr("Postfix for file basename"),
				  StringInpSpec("edited") );
    postfixfld_->attach( alignedBelow, inpfnmsfld_ );
    dotxthdbox_ = new uiCheckBox( outgrp, tr("Replace text header") );
    dotxthdbox_->attach( rightOf, postfixfld_ );
    dobinhdbox_ = new uiCheckBox( outgrp, tr("Replace binary header") );
    dobinhdbox_->attach( rightOf, dotxthdbox_ );

    outgrp->attach( ensureBelow, sep );
    outgrp->attach( hCentered );

    postFinalise().notify( mCB(this,uiSEGYFileManip,initWin) );
}


uiSEGYFileManip::~uiSEGYFileManip()
{
    delete &txthdr_;
    delete &binhdr_;
    delete &calcset_;
    delete strm_;
}


uiGroup* uiSEGYFileManip::mkTrcGroup()
{
    uiGroup* leftgrp = new uiGroup( this, "Main Trace header group" );
    const CallBack addcb( mCB(this,uiSEGYFileManip,addReq) );
    const CallBack edcb( mCB(this,uiSEGYFileManip,edReq) );

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Trace headers") );
    avtrchdrsfld_ = new uiListBox( leftgrp, su );
    avtrchdrsfld_->setHSzPol( uiObject::Small );
    const SEGY::HdrDef&	def = calcset_.hdrDef();
    for ( int idx=0; idx<def.size(); idx++ )
	trchdrdefined_ += false;
    avtrchdrsfld_->doubleClicked.notify( addcb );
    fillAvTrcHdrFld( 0 );

    uiToolButton* addbut = new uiToolButton( leftgrp, uiToolButton::RightArrow,
					    tr("Add to calculated list"),
					    addcb );
    addbut->attach( centeredRightOf, avtrchdrsfld_ );
    trchdrfld_ = new uiListBox( leftgrp, "Defined calculations" );
    trchdrfld_->attach( rightTo, avtrchdrsfld_ );
    trchdrfld_->attach( ensureRightOf, addbut );
    trchdrfld_->selectionChanged.notify( mCB(this,uiSEGYFileManip,selChg) );
    trchdrfld_->doubleClicked.notify( edcb );
    trchdrfld_->setHSzPol( uiObject::Medium );

    edbut_ = new uiToolButton( leftgrp, "edit", tr("Edit calculation"), edcb );
    edbut_->attach( rightOf, trchdrfld_ );
    rmbut_ = new uiToolButton( leftgrp, "remove", tr("Remove calculation"),
		    mCB(this,uiSEGYFileManip,rmReq) );
    rmbut_->attach( alignedBelow, edbut_ );
    uiToolButton* openbut = new uiToolButton( leftgrp, "open",
		    tr("Open stored calculation set"),
		    mCB(this,uiSEGYFileManip,openReq) );
    openbut->attach( alignedBelow, rmbut_ );
    savebut_ = new uiToolButton( leftgrp, "save", tr("Save calculation set"),
		    mCB(this,uiSEGYFileManip,saveReq) );
    savebut_->attach( alignedBelow, openbut );

    uiGroup* rightgrp = new uiGroup( this, "Trace header right group" );
    const int nrrows = def.size();
    uiTable::Setup tsu( nrrows, 2 ); tsu.selmode( uiTable::SingleRow );
    thtbl_ = new uiTable( rightgrp, tsu, "Trace header table" );
    thtbl_->setColumnLabel( 0, sByte() );
    thtbl_->setColumnToolTip( 0, tr("Byte location in trace header") );
    thtbl_->setColumnReadOnly( 0, true );
    thtbl_->setColumnLabel( 1, uiStrings::sValue() );
    thtbl_->setColumnToolTip( 1, tr("Resulting value") );
    thtbl_->setColumnReadOnly( 1, true );
    for ( int irow=0; irow<nrrows; irow++ )
    {
	const SEGY::HdrEntry& he = *def[irow];
	thtbl_->setRowLabel( irow, toUiString(he.name()) );
	thtbl_->setRowToolTip( irow, mToUiStringTodo(he.description()) );
	thtbl_->setValue( RowCol(irow,0), he.bytepos_ + 1 );
    }
    thtbl_->setStretch( 1, 1 );
    thtbl_->selectionChanged.notify( mCB(this,uiSEGYFileManip,rowClck) );

    plotbut_ = new uiToolButton( rightgrp, "distmap",
		    tr("Plot the values of the selected header entries"),
		    mCB(this,uiSEGYFileManip,plotReq) );
    plotbut_->attach( alignedBelow, thtbl_ );
    plotbut_->setSensitive( false );
    plotallbox_ = new uiCheckBox( rightgrp, uiStrings::sAll() );
    plotallbox_->attach( rightOf, plotbut_ );
    plotallbox_->setHSzPol( uiObject::Small );
    plotallbox_->setChecked( true );

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( rightgrp, uiStrings::sTrc() );
    trcnrfld_ = lsb->box();
    lsb->attach( rightAlignedBelow, thtbl_ );
    trcnrfld_->setHSzPol( uiObject::Small );
    trcnrfld_->setMinValue( 1 );
    trcnrfld_->setMaxValue( mUdf(int) );
    trcnrfld_->setValue( 1 );
    trcnrfld_->valueChanging.notify( mCB(this,uiSEGYFileManip,trcNrChg) );

    uiGroup* grp = new uiGroup( this, "Trace header group" );
    uiSplitter* spl = new uiSplitter( grp, "Vert splitter", OD::Vertical );
    spl->addGroup( leftgrp );
    spl->addGroup( rightgrp );
    return grp;
}


bool uiSEGYFileManip::openInpFile()
{
    strm_ = new od_istream( fname_ );
    if ( !strm_ || !strm_->isOK() )
	{ errmsg_ = uiStrings::phrCannotOpenInpFile(); return false; }

    if ( !strm().getBin( txthdr_.txt_, SegyTxtHeaderLength ) )
    {
	errmsg_ = tr("Input file is too small to be a SEG-Y file:\n"
	          "Cannot fully read the text header"); return false;
    }
    char buf[SegyBinHeaderLength];
    if ( !strm().getBin( buf, SegyBinHeaderLength ) )
    {
	errmsg_ = tr("Input file is too small to be a SEG-Y file:\n"
		  "Cannot read full binary header"); return false;
    }

    txthdr_.setAscii();
    binhdr_.setInput( buf );
    binhdr_.guessIsSwapped();

    filesize_ = strm_->endPosition();
    databytespertrace_ = binhdr_.nrSamples() * binhdr_.bytesPerSample();
    return true;
}


void uiSEGYFileManip::fillAvTrcHdrFld( int selidx )
{
    avtrchdrsfld_->setEmpty();
    for ( int idx=0; idx<calcset_.hdrDef().size(); idx++ )
    {
	if ( !trchdrdefined_[idx] )
	    avtrchdrsfld_->addItem(toUiString(calcset_.hdrDef()[idx]->name()));
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
	trchdrfld_->addItem( toUiString("%1 = %2").arg(hc.he_.name())
						  .arg(hc.def_ ) );
    }

    if ( selidx < 0 ) selidx = 0;
    if ( selidx >= calcset_.size() )
	selidx = calcset_.size() - 1;
    if ( selidx >= 0 )
	trchdrfld_->setCurrentItem( selidx );
}


void uiSEGYFileManip::updTrcVals()
{
    OD::memCopy( curhdrbuf_, inphdrbuf_, SegyTrcHeaderLength );
    calcset_.reSetSeqNr( trcnrfld_->getIntValue() );
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
    destSelCB( 0 );
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
{ mODTextTranslationClass(uiSEGYFileManipHdrCalcEd);
public:

uiSEGYFileManipHdrCalcEd( uiParent* p, SEGY::HdrCalc& hc, SEGY::HdrCalcSet& cs )
    : uiDialog(p,Setup(tr("Header Calculation"),cs.indexOf(hc.he_.name()) < 0
		     ? tr("Add header calculation")
		     : tr("Edit header calculation"),
                          mODHelpKey(mSEGYFileManipHdrCalcEdHelpID) ) )
    , hc_(hc)
    , calcset_(cs)
{
    const CallBack cb( mCB(this,uiSEGYFileManipHdrCalcEd,insTxt) );
    uiListBox::Setup su( OD::ChooseOnlyOne, uiStrings::sAvailable(),
			 uiListBox::AboveMid );
    hdrfld_ = new uiListBox( this, su );
    hdrfld_->addItem( toUiString(calcset_.trcIdxEntry().name()) );
    for ( int idx=0; idx<calcset_.hdrDef().size(); idx++ )
	hdrfld_->addItem( toUiString(calcset_.hdrDef()[idx]->name()) );
    hdrfld_->setCurrentItem( hc.he_.name() );
    hdrfld_->doubleClicked.notify( cb );

    uiToolButton* addbut = new uiToolButton( this, uiToolButton::RightArrow,
					     uiStrings::phrInsert(
					     tr("in formula")), cb );
    addbut->attach( centeredRightOf, hdrfld_ );
    formfld_ = new uiLineEdit( this, "Formula" );
    formfld_->setText( hc_.def_ );
    formfld_->attach( rightOf, addbut );
    formfld_->setHSzPol( uiObject::WideVar );
    formfld_->returnPressed.notify(mCB(this,uiSEGYFileManipHdrCalcEd,
								acceptOKCB));
    uiLabel* lbl = new uiLabel(this, tr("Formula for '%1'")
				   .arg(hc_.he_.name()));
    lbl->attach( centeredBelow, formfld_ );
}

void insTxt( CallBacker* )
{
    const int selidx = hdrfld_->currentItem();
    if ( selidx < 0 ) return;
    const int curpos = formfld_->cursorPosition();
    BufferString toins( curpos > 0 ? " " : "" );
    toins.add( hdrfld_->itemText(selidx) ).add( " " );
    formfld_->insert( toins );
}


bool acceptOK()
{
    const char* txt = formfld_->text();
    if ( !txt || !*txt )
	mErrRet(uiStrings::phrEnter(tr("a formula")))

    const SEGY::HdrEntry& he = hc_.he_;
    const int hidx = calcset_.indexOf( he.name() );
    bool setok = false; uiString emsg;
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
    const char* nm = avtrchdrsfld_->itemText( selidx );
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
	{ uiMSG().error(tr("No manipulation sets defined yet")); return; }

    uiSelectFromList::Setup sflsu( uiStrings::phrSelect(
					      tr("header manipulation")), nms );
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
    uiGetObjectName::Setup su( tr("Store manipulation set"), nms );
    uiGetObjectName dlg( this, su );
    if ( !dlg.go() ) return;

    calcset_.setName( dlg.text() );
    if ( !calcset_.storeInSettings() )
	uiMSG().error(uiStrings::phrCannotWrite(
		 tr("to the user settings file:\n%1")
		 .arg(GetSettingsFileName(HdrCalcSet::sKeySettsFile()))));
}


od_int64 uiSEGYFileManip::traceBytes() const
{
    return SegyTrcHeaderLength + binhdr_.nrSamples() * binhdr_.bytesPerSample();
}


void uiSEGYFileManip::trcNrChg( CallBacker* )
{
    const od_int64 tbyts = traceBytes();
    od_stream::Pos offs = cFileHeaderSize + (trcnrfld_->getIntValue()-1)*tbyts;
    if ( offs >= filesize_ )
    {
	const od_int64 nrtrcsinfile = (filesize_-cFileHeaderSize) / tbyts;
	if ( nrtrcsinfile < 1 )
	    uiMSG().error( tr("File contains no complete traces") );
	else
	{
	    offs = cFileHeaderSize + (nrtrcsinfile-1) * tbyts;
	    NotifyStopper ns( trcnrfld_->valueChanging );
	    trcnrfld_->setValue( (int)nrtrcsinfile );
	}
    }

    strm().setReadPosition( offs );
    strm().getBin( inphdrbuf_, SegyTrcHeaderLength );

    updTrcVals();
}


void uiSEGYFileManip::rowClck( CallBacker* cb )
{
    rowSel( thtbl_->currentRow() );
}


void uiSEGYFileManip::destSelCB( CallBacker* cb )
{
    const bool wantmulti = !selmultifld_->getBoolValue();
    outfnmfld_->display( !wantmulti );
    inpfnmsfld_->display( wantmulti );
    postfixfld_->display( wantmulti );
    dotxthdbox_->display( wantmulti );
    dobinhdbox_->display( wantmulti );
}


class uiSEGYFileManipDataExtracter : public Executor
{ mODTextTranslationClass(uiSEGYFileManipDataExtracter);
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
    fm_.strm().setReadPosition( cFileHeaderSize );
    trcrg_.start = 1;
    totalnr_ = (fm_.filesize_-cFileHeaderSize) / fm_.traceBytes();
    trcrg_.stop = (int)totalnr_;
    if ( !plotall )
    {
	DataInpSpec* spec = new IntInpIntervalSpec( trcrg_ );
	uiGenInputDlg dlg(p, tr("Specify range"), tr("Trace range to plot"),
									spec);
	if ( !dlg.go() )
	    { totalnr_ = -1; return; }
	trcrg_ = dlg.getFld(0)->getIInterval();
	trcrg_.sort();
	trcrg_.limitTo( Interval<int>(1,mCast(int,totalnr_)) );
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

uiString message() const	{ return tr("Collecting data"); }
uiString nrDoneText() const	{ return tr("Traces scanned"); }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totalnr_; }

int nextStep()
{
    if ( totalnr_ < 0 )
	return Finished();

    fm_.strm().setReadPosition( cFileHeaderSize + nrdone_ * fm_.traceBytes() );
    if ( !fm_.strm().getBin(buf_,SegyTrcHeaderLength) )
	return Finished();

    fm_.calcset_.apply( buf_, fm_.binhdr_.isSwapped() );

    for ( int idx=0; idx<sel_.size(); idx++ )
	*data_[idx] += mCast(float,hdef_[sel_[idx]]->getValue(buf_,needswap_));

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
    uiTaskRunner taskrunner( this );
    TaskRunner::execute( &taskrunner, de );
    if ( de.data_[0]->size() < 2 )
	return;

    uiMainWin::Setup su( tr("Header value plot") );
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


bool uiSEGYFileManip::acceptOK()
{
    if ( errlbl_ )
	return true;

    if ( binhdr_.nrSamples() < 1 )
	mErrRet( tr("Binary header's number of samples must be > 0") )
    const bool wantmulti = !selmultifld_->getBoolValue();
    const BufferString orgfname( fname_ );
    BufferStringSet fnms; BufferString postfix;
    dotxthd_ = dobinhd_ = true;
    if ( !wantmulti )
    {
	fnms.add( outfnmfld_->fileName() );
	if ( fnms.first()->isEmpty() )
	    mErrRet( tr("Please specify an output filename") )
    }
    else
    {
	postfix = postfixfld_->text();
	postfix.clean();
	if ( postfix.isEmpty() )
	    mErrRet( tr("Please specify a postfix") )
	inpfnmsfld_->getFileNames( fnms );
	if ( fnms.isEmpty() )
	    mErrRet( tr("No files to apply the editing to") )
	dotxthd_ = dotxthdbox_->isChecked();
	dobinhd_ = dobinhdbox_->isChecked();
    }

    bool retval = true;
    for ( auto fnm : fnms )
    {
	BufferString outfnm( *fnm );
	if ( wantmulti )
	{
	    fname_ = *fnm;
	    File::Path fp( *fnm );
	    const BufferString ext = fp.extension();
	    fp.setExtension( nullptr );
	    fp.setFileName( BufferString(fp.fileName(),"_",postfix) );
	    fp.setExtension( ext );
	    outfnm = fp.fullPath();
	}
	if ( !handleFile(outfnm) )
	{
	    if ( !wantmulti )
		{ retval = false; break; }
	    else if ( fnm == fnms.last()
		|| !uiMSG().askContinue(tr("Try handling next file?") ) )
		{ retval = false; break; }
	}
    }

    if ( wantmulti )
	fname_ = orgfname;
    return retval;
}


bool uiSEGYFileManip::handleFile( const char* outpfnm )
{
    File::Path inpfp( fname_ );
    File::Path outfp( outpfnm );
    if ( !outfp.isAbsolute() )
	outfp.setPath( inpfp.pathOnly() );
    if ( inpfp == outfp )
	mErrRet(tr("Input and output file cannot be the same") )

    od_ostream outstrm( outfp.fullPath() );
    if ( !outstrm.isOK() )
	{ mErrRet(uiStrings::phrCannotOpenOutpFile()) }
    od_istream instrm( inpfp.fullPath() );
    if ( !instrm.isOK() )
	{ mErrRet(uiStrings::phrCannotOpenInpFile()) }

    txthdr_.setText( txthdrfld_->text() );
    calcset_.reSetSeqNr( 1 );

    TxtHeader* thptr = dotxthd_ ? &txthdr_ : nullptr;
    BinHeader* bhptr = dobinhd_ ? &binhdr_ : nullptr;
    Executor* exec = calcset_.getApplier( instrm, outstrm, thptr, bhptr );
    uiTaskRunner uitr( this );
    const bool rv = uitr.execute( *exec );
    delete exec;

    if ( rv )
	fname_ = outstrm.fileName();
    return rv;
}
