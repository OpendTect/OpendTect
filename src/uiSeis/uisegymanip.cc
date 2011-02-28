/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegymanip.cc,v 1.1 2011-02-28 12:22:59 cvsbert Exp $";

#include "uisegymanip.h"

#include "uitextedit.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uifileinput.h"
#include "uicompoundparsel.h"
#include "uimsg.h"

#include "segyhdr.h"
#include "segyhdrdef.h"
#include "segyfiledef.h"
#include "strmprov.h"
#include <iostream>


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
    BufferString ret( "ns=", hdr_.nrSamples(), " ; fmt=" );
    ret.add( SEGY::FilePars::nameOfFmt( hdr_.format(), false ) );
    return ret;
}

void doDlg( CallBacker* )
{
    uiMSG().error( "TODO" );
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
    , errlbl_(0)
{
    if ( !openFile() )
	{ errlbl_ = new uiLabel( this, errmsg_ ); return; }

    txthdrfld_ = new uiTextEdit( this, "Text Header Editor" );
    txthdrfld_->setDefaultWidth( 80 );
    txthdrfld_->setDefaultHeight( 5 );
    txthdrfld_->setPrefHeightInChar( 5 );
    BufferString txt; txthdr_.getText( txt );
    txthdrfld_->setText( txt );
    uiLabel* lbl = new uiLabel( this, "Text Header" );
    lbl->attach( centeredLeftOf, txthdrfld_ );

    binhdrfld_ = new uiSEGYBinHdrEd( this, binhdr_ );
    binhdrfld_->attach( alignedBelow, txthdrfld_ );
}


uiSEGYFileManip::~uiSEGYFileManip()
{
    delete &txthdr_;
    delete &binhdr_;
}


bool uiSEGYFileManip::openFile()
{
    sd_ = StreamProvider(fname_).makeIStream();
    if ( !sd_.usable() )
	return false;

    strm().read( (char*)txthdr_.txt_, SegyTxtHeaderLength );
    if ( strm().gcount() < SegyTxtHeaderLength )
	{ errmsg_ = "Input file is too small to be a SEG-Y file:\n"
	            "Can't even read the text header"; return false; }
    strm().read( (char*)binhdr_.buf(), SegyBinHeaderLength );
    if ( strm().gcount() < SegyBinHeaderLength )
	{ errmsg_ = "Input file is too small to be a SEG-Y file:\n"
	    	    "Cannot read full binary header"; return false; }

    return true;
}


void uiSEGYFileManip::fileSel( CallBacker* )
{
}


bool uiSEGYFileManip::acceptOK( CallBacker* )
{
    if ( errlbl_ )
	return true;

    return false;
}
