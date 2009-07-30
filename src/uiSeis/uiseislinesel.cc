/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Nov 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseislinesel.cc,v 1.26 2009-07-30 09:47:24 cvsraman Exp $";

#include "uiseislinesel.h"

#include "uiseissel.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uiselsurvranges.h"
#include "uiselsimple.h"
#include "uimsg.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "transl.h"
#include "ioman.h"
#include "iodir.h"



uiSeis2DLineSel::uiSeis2DLineSel( uiParent* p, const char* lsnm )
    : uiCompoundParSel(p,"Line name")
    , fixedlsname_(lsnm && *lsnm)
    , lsnm_(lsnm)
{
    butPush.notify( mCB(this,uiSeis2DLineSel,selPush) );
}


BufferString uiSeis2DLineSel::getSummary() const
{
    BufferString ret( lnm_ );
    if ( !lnm_.isEmpty() )
	{ ret += " ["; ret += lsnm_; ret += "]"; }
    return ret;
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeis2DLineSel::selPush( CallBacker* )
{
    BufferString newlsnm( lsnm_ );
    if ( !fixedlsname_ )
    {
	BufferStringSet lsnms;
	SeisIOObjInfo::get2DLineInfo( lsnms );
	if ( lsnms.isEmpty() )
	    mErrRet("No line sets available.\nPlease import 2D data first")

	if ( lsnms.size() == 1 )
	    newlsnm = lsnm_ = lsnms.get( 0 );
	else
	{
	    uiSelectFromList::Setup su( "Select Line Set", lsnms );
	    su.current_ = lsnm_.isEmpty() ? 0 : lsnms.indexOf( lsnm_ );
	    if ( su.current_ < 0 ) su.current_ = 0;
	    uiSelectFromList dlg( this, su );
	    if ( !dlg.go() || dlg.selection() < 0 )
		return;
	    newlsnm = lsnms.get( dlg.selection() );
	}
    }

    BufferStringSet lnms;
    SeisIOObjInfo ioinf( newlsnm );
    if ( !ioinf.isOK() )
	mErrRet("Invalid line set selected")
    if ( ioinf.isPS() || !ioinf.is2D() )
	mErrRet("Selected Line Set duplicates name with other object")

    ioinf.getLineNames( lnms );
    uiSelectFromList::Setup su( "Select 2D line", lnms );
    su.current_ = lnm_.isEmpty() ? 0 : lnms.indexOf( lnm_ );
    if ( su.current_ < 0 ) su.current_ = 0;
    uiSelectFromList dlg( this, su );
    if ( !dlg.go() || dlg.selection() < 0 )
	return;

    lsnm_ = newlsnm;
    lnm_ = lnms.get( dlg.selection() );
}


void uiSeis2DLineSel::set( const char* lsnm, const char* lnm )
{
    lsnm_ = IOObj::isKey(lsnm) ? IOM().nameOf( MultiID(lsnm) ) : lsnm;
    lnm_ = lnm;
    updateSummary();
}


MultiID uiSeis2DLineSel::lineSetID() const
{
    TypeSet<MultiID> mids; BufferStringSet lsnms;
    SeisIOObjInfo::get2DLineInfo( lsnms, &mids );
    for ( int idx=0; idx<lsnms.size(); idx++ )
    {
	const MultiID& key = mids[idx];
	if ( lsnms.get(idx) == lsnm_ )
	    return key;
    }
    return MultiID("");
}


uiSeis2DLineNameSel::uiSeis2DLineNameSel( uiParent* p, bool forread )
    : uiGroup(p,"2D line name sel")
    , forread_(forread)
    , nameChanged(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Line name" );
    fld_ = lcb->box();
    fld_->setReadOnly( forread_ );
    if ( !forread_ ) fld_->addItem( "" );
    setHAlignObj( lcb );
    if ( !forread_ )
	finaliseDone.notify( mCB(this,uiSeis2DLineNameSel,fillAll) );
    fld_->selectionChanged.notify( mCB(this,uiSeis2DLineNameSel,selChg) );
}


void uiSeis2DLineNameSel::fillAll( CallBacker* )
{
    if ( lsid_.isEmpty() )
	fillWithAll();
}


void uiSeis2DLineNameSel::fillWithAll()
{
    IOM().to( mIOObjContext(SeisTrc).getSelKey() );
    const IODir& iodir = *IOM().dirPtr();
    const ObjectSet<IOObj>& objs = iodir.getObjs();
    for ( int idx=0; idx<objs.size(); idx++ )
	addLineNames( objs[idx]->key() );
    if ( fld_->size() )
	fld_->setCurrentItem( 0 );
}


void uiSeis2DLineNameSel::addLineNames( const MultiID& ky )
{
    const SeisIOObjInfo oi( ky );
    if ( !oi.isOK() || !oi.is2D() ) return;

    BufferStringSet lnms; oi.getLineNames( lnms );
    nameChanged.disable();
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const char* lnm = lnms.get( idx );
	if ( !fld_->isPresent(lnm) )
	    fld_->addItem( lnm );
    }
    nameChanged.enable();
}


const char* uiSeis2DLineNameSel::getInput() const
{
    return fld_->text();
}


int uiSeis2DLineNameSel::getLineIndex() const
{
    return fld_->currentItem();
}


void uiSeis2DLineNameSel::setInput( const char* nm )
{
    if ( fld_->isPresent(nm) )
	fld_->setCurrentItem( nm );

    if ( !forread_ )
    {
	nameChanged.disable();
	fld_->setCurrentItem( 0 );
	nameChanged.enable();
	fld_->setText( nm );
	nameChanged.trigger();
    }
}


void uiSeis2DLineNameSel::setLineSet( const MultiID& ky )
{
    lsid_ = ky;
    fld_->empty();
    if ( !forread_ ) fld_->addItem( "" );
    addLineNames( ky );
}



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
	StepInterval<int> globtrcrg( 0, 0, 1 );
	int startidx=0; int maxattridx = attrbnms.size() - 1;
	const int defidx = attrbnms.indexOf( LineKey::sKeyDefAttrib() );
	if ( defidx>=0 ) { startidx=maxattridx=defidx; }

	int maxnrtrcs = 0;
	for ( int attridx=startidx; attridx<=maxattridx; attridx++ )
	{
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    LineKey lk( lnms.get(idx).buf(), attrbnms.get(attridx) );
	    oinf.getRanges( lk, trcrg, zrg );
	    if ( trcrg.nrSteps() > maxnrtrcs )
	    {
		globtrcrg = trcrg;
		maxnrtrcs = trcrg.nrSteps();
	    }
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
