
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.cc,v 1.2 2008-11-14 11:31:28 cvsumesh Exp $
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
	              CtxtIOObj* lsctio )
    : uiDialog( p, uiDialog::Setup("Select 2D LineSet/LineName",
				   mNoDlgTitle,mNoHelpID) )
    , lsctio_(lsctio)
    , nroflines_(0)
    , sellines_(sellines)		   
{
    linesetfld_ = new uiSeisSel( this, *lsctio_,
	    			 uiSeisSel::Setup(Seis::Line).selattr(false) );
    linesetfld_->selectiondone.notify(
	    			mCB(this,uiLineSel,lineSetSel) );

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Line names", false );
    llb->attach( alignedBelow, linesetfld_ );
    lnmsfld_ = llb->box();
    lnmsfld_->setCheckedItems( sellines );

    lsb_ = new uiLabeledSpinBox( this, "Trace range", 0, "Trc Start" );
    trc0fld_ = lsb_->box();
    trc1fld_ = new uiSpinBox( this, 0, "Trc Stop" );
    trc1fld_->attach( rightTo, lsb_ );
    
    lsb_->attach( alignedBelow, llb );

    lineSetSel( 0 );
}   


uiLineSel::~uiLineSel()
{
    if ( lsctio_ ) delete lsctio_->ioobj;
    delete lsctio_;
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

    if ( lnms.size() == 0 )
    {
	lsb_->display( false );
	trc1fld_->display( false );
    }
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	lnmsfld_->setItemCheckable( idx, true );

	StepInterval<int> trcrg;
	StepInterval<float> zrg;

	BufferStringSet attrbnms;
	oinf.getAttribNamesForLine( lnms.get(idx).buf(), attrbnms );
	LineKey lk( lnmsfld_->getText(),
		    (attrbnms.size() > 0 ? attrbnms.get(0) : 0) );
	oinf.getRanges( lk, trcrg, zrg );
	linetrcrgs_ += trcrg;
    }

    if ( lnms.size() && lsb_->box()->isDisplayed() && 
	    		trc1fld_->isDisplayed() )
    {
	trc0fld_->setInterval( linetrcrgs_[0] );
	trc0fld_->setValue( linetrcrgs_[0].start );
	trc1fld_->setInterval( linetrcrgs_[0] );
	trc1fld_->setValue( linetrcrgs_[0].stop );
    }

    lnmsfld_->selectionChanged.notify( mCB(this,uiLineSel,trcRangeSel) );
}


void uiLineSel::trcRangeSel( CallBacker* )
{
    if ( linetrcrgs_.isEmpty() )
	return;

    int curselno = lnmsfld_->currentItem(); 
    if ( !lsb_->box()->isDisplayed() )
	lsb_->display( true );
    trc0fld_->setInterval( linetrcrgs_[curselno] );
    trc0fld_->setValue( linetrcrgs_[curselno].start );

    if ( !trc1fld_->isDisplayed() )
	trc1fld_->display( true );
    trc1fld_->setInterval( linetrcrgs_[curselno] );
    trc1fld_->setValue( linetrcrgs_[curselno].stop );
}


MultiID uiLineSel::getLineSetKey()
{ return getIOObj()->key(); }


bool uiLineSel::acceptOK( CallBacker* )
{
    sellines_.erase();
    nroflines_ = lnmsfld_->size();
    lnmsfld_->getCheckedItems( sellines_ );
    return true;
}


IOObj* uiLineSel::getIOObj()
{ return lsctio_->ioobj; }
