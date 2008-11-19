
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.cc,v 1.10 2008-11-19 10:13:59 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiseislinesel.h"

#include "uiseissel.h"
#include "uilistbox.h"
#include "uispinbox.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "transl.h"

uiLineSel::uiLineSel( uiParent* p, BufferStringSet& sellines, 
	              CtxtIOObj* lsctio, BoolTypeSet& linechksum )
    : uiDialog( p, uiDialog::Setup("Select 2D LineSet/LineName",
				   mNoDlgTitle,mNoHelpID) )
    , lsctio_(lsctio)
    , nroflines_(0)
    , sellines_(sellines)		   
    , linechksum_(linechksum)					   
{
    linesetfld_ = new uiSeisSel( this, *lsctio_,
	    			 uiSeisSel::Setup(Seis::Line).selattr(false) );
    linesetfld_->selectiondone.notify( mCB(this,uiLineSel,lineSetSel) );

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Line names", false );
    llb->attach( alignedBelow, linesetfld_ );
    lnmsfld_ = llb->box();

    lsb_ = new uiLabeledSpinBox( this, "Trace range", 0, "Trc Start" );
    trc0fld_ = lsb_->box();
    trc0fld_->valueChanged.notify( mCB(this,uiLineSel,trc0Changed) );
    trc1fld_ = new uiSpinBox( this, 0, "Trc Stop" );
    trc1fld_->attach( rightTo, lsb_ );
    trc1fld_->valueChanged.notify( mCB(this,uiLineSel,trc1Changed) );
    
    lsb_->attach( alignedBelow, llb );

    lineSetSel( 0 );
    for ( int lineidx = 0; lineidx<linechksum.size(); lineidx++)
	lnmsfld_->setItemChecked( lineidx, linechksum[lineidx] );
}   


BufferString uiLineSel::getSummary() const
{
    BufferString ret;
    if ( !linesetfld_ || !lsctio_->ioobj ) return ret;

    ret = lsctio_->ioobj->name();
    const int nrsel = sellines_.size();
    if ( nroflines_==1 )
	ret += " (1 line)";
    else
    {
	ret += " (";
	if ( nroflines_ == nrsel ) ret += "all";
	else { ret += nrsel; ret += "/"; ret += nroflines_; }
	ret += " lines)";
    }

    return ret;

}


void uiLineSel::lineSetSel( CallBacker* )
{
    if ( !lsctio_->ioobj ) return;

    SeisIOObjInfo oinf( lsctio_->ioobj );
    BufferStringSet lnms;
    oinf.getLineNames( lnms );   

    lnmsfld_->empty();
    lnmsfld_->addItems( lnms );

    lsb_->display( !lnms.isEmpty() );
    trc1fld_->display( !lnms.isEmpty() );
    
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	lnmsfld_->setItemCheckable( idx, true );

	StepInterval<int> trcrg;
	StepInterval<float> zrg;

	BufferStringSet attrbnms;
	oinf.getAttribNamesForLine( lnms.get(idx).buf(), attrbnms );
	LineKey lk( lnms.get(idx).buf(),
		    (attrbnms.size() > 0 ? attrbnms.get(0) : 0) );
	oinf.getRanges( lk, trcrg, zrg );

	Interval<int> trcitval( trcrg.start, trcrg.stop );

	linetrcrgs_ += trcitval;
	linetrcflrgs_ += trcitval;
    }

    if ( lnms.size() && lsb_->box()->isDisplayed() && 
	    		trc1fld_->isDisplayed() )
    {
	trc0fld_->setInterval( linetrcrgs_[0].start, linetrcrgs_[0].stop );
	trc0fld_->setValue( linetrcrgs_[0].start );
	trc1fld_->setInterval( linetrcrgs_[0].start, linetrcrgs_[0].stop );
	trc1fld_->setValue( linetrcrgs_[0].stop );
    }

    lnmsfld_->selectionChanged.notify( mCB(this,uiLineSel,lineSelTrcRange) );
}


void uiLineSel::lineSelTrcRange( CallBacker* )
{
    if ( linetrcrgs_.isEmpty() )
	return;

    int curselno = lnmsfld_->currentItem(); 
    
    trc0fld_->setInterval( linetrcrgs_[curselno].start, 
	    		   linetrcrgs_[curselno].stop );
    trc0fld_->setValue( linetrcrgs_[curselno].start );

    trc1fld_->setInterval( linetrcrgs_[curselno].start,
	   		   linetrcrgs_[curselno].stop );
    trc1fld_->setValue( linetrcrgs_[curselno].stop );
}


void uiLineSel::trc0Changed( CallBacker* )
{
    linetrcflrgs_[ lnmsfld_->currentItem() ].start = trc0fld_->getValue();
}


void uiLineSel::trc1Changed( CallBacker* )
{
    linetrcflrgs_[ lnmsfld_->currentItem() ].stop = trc1fld_->getValue();
}


bool uiLineSel::acceptOK( CallBacker* )
{
    sellines_.erase();
    nroflines_ = lnmsfld_->size();
    lnmsfld_->getCheckedItems( sellines_ );
    
    linechksum_.erase();
    for ( int sellineidx = 0; sellineidx<nroflines_; sellineidx++ )
	linechksum_ += lnmsfld_->isItemChecked( sellineidx );
    
    return true;
}


uiSelection2DParSel::uiSelection2DParSel( uiParent* p )
    : uiCompoundParSel(p,"LineSet/LineName","Select")
    , lsctio_(mMkCtxtIOObj(SeisTrc))
    , linesel_(0)
{ butPush.notify( mCB(this,uiSelection2DParSel,doDlg) ); }


uiSelection2DParSel::~uiSelection2DParSel()
{
    if ( lsctio_ ) delete lsctio_->ioobj;
    delete lsctio_;
}


BufferString uiSelection2DParSel::getSummary() const
{
    if ( !linesel_ )
	return BufferString();
    return linesel_->getSummary();
}


void uiSelection2DParSel::doDlg( CallBacker* )
{
    linesel_ = new uiLineSel( this, sellines_, lsctio_, linechksum_ );
    linesel_->go();
}


IOObj* uiSelection2DParSel::getIOObj()
{ return lsctio_->ioobj; }
