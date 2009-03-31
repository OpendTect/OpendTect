/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseislinesel.cc,v 1.20 2009-03-31 06:58:02 cvsnanne Exp $";

#include "uiseislinesel.h"

#include "uiseissel.h"
#include "uilistbox.h"
#include "uiselsurvranges.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "transl.h"

uiSeis2DLineSubSel::uiSeis2DLineSubSel( uiParent* p, CtxtIOObj& lsctio )
    : uiDialog( p, uiDialog::Setup("Select 2D LineSet/LineName",
				   mNoDlgTitle,mTODOHelpID) )
    , lsctio_(lsctio)
{
    linesetfld_ = new uiSeisSel( this, lsctio_,
	    			 uiSeisSel::Setup(Seis::Line).selattr(false) );
    linesetfld_->selectiondone.notify( mCB(this,uiSeis2DLineSubSel,lineSetSel));

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Line names", false );
    llb->attach( alignedBelow, linesetfld_ );
    lnmsfld_ = llb->box();
    lnmsfld_->setItemsCheckable( true );
    lnmsfld_->selectionChanged.notify( mCB(this,uiSeis2DLineSubSel,lineSel) );

    trcrgfld_ = new uiSelNrRange( this, StepInterval<int>(),
	   			  false, "Trace" );
    trcrgfld_->rangeChanged.notify( mCB(this,uiSeis2DLineSubSel,trcChanged) );
    trcrgfld_->attach( alignedBelow, llb );

    lineSetSel( 0 );

    finaliseDone.notify( mCB(this,uiSeis2DLineSubSel,finalised) );
}


void uiSeis2DLineSubSel::finalised( CallBacker* )
{
    if ( !lsctio_.ioobj )
	linesetfld_->doSel( 0 );
}


void uiSeis2DLineSubSel::setSelLines( const BufferStringSet& sellines )
{ 
    sellines_ = sellines; 
    lnmsfld_->setCheckedItems( sellines_ );
}


BufferString uiSeis2DLineSubSel::getSummary() const
{
    BufferString ret;
    if ( !linesetfld_ || !lsctio_.ioobj ) return ret;

    ret = lsctio_.ioobj->name();
    const int nrsel = sellines_.size();
    const int nroflines = lnmsfld_->size();
    if ( nroflines==1 )
	ret += " (1 line)";
    else
    {
	ret += " (";
	if ( nroflines == nrsel ) ret += "all";
	else { ret += nrsel; ret += "/"; ret += nroflines; }
	ret += " lines)";
    }

    return ret;
}


void uiSeis2DLineSubSel::lineSetSel( CallBacker* )
{
    if ( !linesetfld_->commitInput() || !lsctio_.ioobj )
	return;

    SeisIOObjInfo oinf( lsctio_.ioobj );
    BufferStringSet lnms;
    oinf.getLineNames( lnms );   
    lnmsfld_->empty();
    lnmsfld_->addItems( lnms );
    lnmsfld_->setItemsChecked( true );
    maxtrcrgs_.erase();
    trcrgs_.erase();

    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	BufferStringSet attrbnms;
	oinf.getAttribNamesForLine( lnms.get(idx).buf(), attrbnms );
	StepInterval<int> globtrcrg( mUdf(int), -mUdf(int), 1 );
	for ( int attridx=0; attridx<attrbnms.size(); attridx++ )
	{
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    LineKey lk( lnms.get(idx).buf(), attrbnms.get(attridx) );
	    oinf.getRanges( lk, trcrg, zrg );
	    globtrcrg.include( trcrg, false );
	}

	maxtrcrgs_ += globtrcrg;
	trcrgs_ += globtrcrg;
    }

    lineSel(0);
}


Interval<int> uiSeis2DLineSubSel::getTrcRange( const char* nm ) const
{
    const int idx = lnmsfld_->indexOf( nm );
    return idx<0 ? Interval<int>(0,0) : (Interval<int>)trcrgs_[idx];
}


void uiSeis2DLineSubSel::lineSel( CallBacker* )
{
    NotifyStopper ns( trcrgfld_->rangeChanged );
    const int curitm = lnmsfld_->currentItem(); 
    if ( trcrgs_.isEmpty() || curitm<0 )
	return;

    trcrgfld_->setLimitRange( maxtrcrgs_[curitm] );
    trcrgfld_->setRange( trcrgs_[curitm] );
}


void uiSeis2DLineSubSel::trcChanged( CallBacker* )
{
    const int curitm = lnmsfld_->currentItem();
    if ( curitm<0 ) return;

    trcrgs_[curitm].start = trcrgfld_->getRange().start;
    trcrgs_[curitm].stop = trcrgfld_->getRange().stop;
}


bool uiSeis2DLineSubSel::acceptOK( CallBacker* )
{
    sellines_.erase();
    lnmsfld_->getCheckedItems( sellines_ );
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
    linesel_ = new uiSeis2DLineSubSel( this, *lsctio_ );
    linesel_->setSelLines( sellines_ );
    linesel_->go();
    sellines_ = linesel_->getSelLines();
}


void uiSelection2DParSel::fillPar( IOPar& par )
{
    par.set( "LineSet.ID", getIOObj()->key() );

    BufferString mergekey;
    IOPar lspar;
    for ( int idx=0; idx<sellines_.size(); idx++ )
    {
	IOPar linepar;
	linepar.set( sKey::Name, sellines_[idx]->buf() );
	Interval<int> trcrg = linesel_->getTrcRange( sellines_.get(idx) );
	linepar.set( sKey::TrcRange, trcrg );
	mergekey = idx;
	lspar.mergeComp( linepar, mergekey );
    }

    par.mergeComp( lspar, sKey::LineKey );
}


IOObj* uiSelection2DParSel::getIOObj()
{ return lsctio_->ioobj; }
