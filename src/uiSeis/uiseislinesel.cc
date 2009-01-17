
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseislinesel.cc,v 1.15 2009-01-17 07:29:36 cvsumesh Exp $";

#include "uiseislinesel.h"

#include "uiseissel.h"
#include "uilistbox.h"
//#include "uispinbox.h"
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
				   mNoDlgTitle,mNoHelpID) )
    , lsctio_(lsctio)
{
    linesetfld_ = new uiSeisSel( this, lsctio_,
	    			 uiSeisSel::Setup(Seis::Line).selattr(false) );
    linesetfld_->selectiondone.notify( mCB(this,uiSeis2DLineSubSel,lineSetSel));

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Line names", false );
    llb->attach( alignedBelow, linesetfld_ );
    lnmsfld_ = llb->box();
    lnmsfld_->selectionChanged.notify( mCB(this,uiSeis2DLineSubSel,lineSel) );

//  TODO: Replace by uiSelNrRange
//    lsb_ = new uiLabeledSpinBox( this, "Trace range", 0, "Trc Start" );
//    trc0fld_ = lsb_->box();
//    trc0fld_->valueChanged.notify( mCB(this,uiSeis2DLineSubSel,trc0Changed) );
//    trc1fld_ = new uiSpinBox( this, 0, "Trc Stop" );
//    trc1fld_->attach( rightTo, lsb_ );
//    trc1fld_->valueChanged.notify( mCB(this,uiSeis2DLineSubSel,trc1Changed) );
    
//    lsb_->attach( alignedBelow, llb );
    trcrgfld_ = new uiSelNrRange( this, StepInterval<int>(),
	   			  false, "Trace" );
    trcrgfld_->valueChanged.notify( mCB(this,uiSeis2DLineSubSel,trcChanged) );
    trcrgfld_->attach( alignedBelow, llb );

    lineSetSel( 0 );
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
    if ( !lsctio_.ioobj ) return;

    SeisIOObjInfo oinf( lsctio_.ioobj );
    BufferStringSet lnms;
    oinf.getLineNames( lnms );   

    lnmsfld_->empty();
    lnmsfld_->addItems( lnms );

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

    if ( lnms.size() )
    {
	StepInterval<int> interval( linetrcrgs_[0].start,
				    linetrcrgs_[0].stop, 1 );
	trcrgfld_->setValInterval( interval );
	trcrgfld_->setRange( interval );
    }
}


const Interval<int> uiSeis2DLineSubSel::getLineTrcRange( int idx ) const
{ return linetrcflrgs_[idx]; }


void uiSeis2DLineSubSel::lineSel( CallBacker* )
{
    if ( linetrcrgs_.isEmpty() )
	return;

    int curselno = lnmsfld_->currentItem(); 

    StepInterval<int> interval( linetrcrgs_[curselno].start,
	    			linetrcrgs_[curselno].stop, 1 );

    trcrgfld_->setValInterval( interval );
    trcrgfld_->setRange( interval );
}


void uiSeis2DLineSubSel::trcChanged( CallBacker* )
{
    linetrcflrgs_[ lnmsfld_->currentItem() ].start = 
				trcrgfld_->getRange().start;
    linetrcflrgs_[ lnmsfld_->currentItem() ].stop = 
				trcrgfld_->getRange().stop;
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
    SeisIOObjInfo oinf( lsctio_->ioobj );
    BufferStringSet lnms;
    oinf.getLineNames( lnms );

    par.set( "LineSet.ID", getIOObj()->key() );

    IOPar linespar;
    BufferString key;
    for ( int idx=0; idx<sellines_.size(); idx++ )
    {
	key = idx;

	IOPar lntrcpar;
	lntrcpar.set( sKey::Name, sellines_[idx]->buf() );
	
	Interval<int> trcitval = linesel_->getLineTrcRange(
				          lnms.indexOf(sellines_[idx]->buf()) );
	lntrcpar.set( sKey::TrcRange, trcitval );
	linespar.mergeComp( lntrcpar, key );
    }

    par.mergeComp( linespar, sKey::LineKey );
}


IOObj* uiSelection2DParSel::getIOObj()
{ return lsctio_->ioobj; }
