/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Nov 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseislinesel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiselsimple.h"
#include "uiselsurvranges.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "transl.h"



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
    geomid_ = S2DPOS().getGeomID( lsnm_, lnm_ );
}


void uiSeis2DLineSel::set( const char* lsnm, const char* lnm )
{
    lsnm_ = IOObj::isKey(lsnm) ? IOM().nameOf( MultiID(lsnm) ) : lsnm;
    lnm_ = lnm;
    updateSummary();
}


void uiSeis2DLineSel::set( const PosInfo::GeomID& geomid )
{
    geomid_ = geomid;
    lsnm_ = S2DPOS().getLineSet( geomid.lsid_ );
    lnm_ = S2DPOS().getLineName( geomid.lineid_ );
    updateSummary();
}


const PosInfo::GeomID& uiSeis2DLineSel::getGeomID() const
{ return geomid_; }


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
	postFinalise().notify( mCB(this,uiSeis2DLineNameSel,fillAll) );
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
    fld_->setEmpty();
    if ( !forread_ ) fld_->addItem( "" );
    addLineNames( ky );
}



uiSeis2DMultiLineSelDlg::uiSeis2DMultiLineSelDlg( uiParent* p, CtxtIOObj& c,
       					const uiSeis2DMultiLineSel::Setup& su )
    : uiDialog( p, uiDialog::Setup("Select 2D Lines",mNoDlgTitle,"103.1.13") )
    , setup_(su)
    , ctio_(c)
    , linesetfld_(0)
    , zrgfld_(0)
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Line names", true );
    lnmsfld_ = llb->box();
    lnmsfld_->selectionChanged.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,lineSel) );

    if ( setup_.withlinesetsel_ )
    {
	uiSeisSel::Setup sssu(Seis::Line);
	sssu.selattr(setup_.withattr_).filldef(setup_.filldef_);
	if ( setup_.withattr_ && !setup_.allattribs_ )
	    sssu.selattr( true ).wantSteering(setup_.steering_);

	linesetfld_ = new uiSeisSel( this, ctio_, sssu );
	linesetfld_->selectionDone.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,lineSetSel) );
	llb->attach( alignedBelow, linesetfld_ );
    }

    trcrgfld_ = new uiSelNrRange( this, StepInterval<int>(),
	   			  setup_.withstep_, "Trace" );
    trcrgfld_->rangeChanged.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,trcRgChanged) );
    trcrgfld_->attach( alignedBelow, llb );

    if ( setup_.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, su.withstep_,
			BufferString("Z Range",SI().getZUnitString()) );
	zrgfld_->setRangeLimits( SI().zRange(false) );
	zrgfld_->attach( alignedBelow, trcrgfld_ );
    }

    postFinalise().notify( mCB(this,uiSeis2DMultiLineSelDlg,finalised) );
}


void uiSeis2DMultiLineSelDlg::finalised( CallBacker* )
{
    if ( !linesetfld_ ) return;

    const IOObj* lsetobj = linesetfld_->ioobj( true );
    if ( !lsetobj )
	linesetfld_->doSel( 0 );
    else
	lineSetSel( 0 );
}


void uiSeis2DMultiLineSelDlg::setLineSet( const MultiID& key, const char* attr )
{
    if ( linesetfld_ )
    {
	linesetfld_->setInput( key );
	if ( attr && *attr )
	    linesetfld_->setAttrNm( attr );
    }

    lineSetSel( 0 );
}


void uiSeis2DMultiLineSelDlg::setAll( bool yn )
{ lnmsfld_->selectAll( yn ); }

void uiSeis2DMultiLineSelDlg::setSelection( const BufferStringSet& sellines,
       				const TypeSet<StepInterval<int> >* rgs	)
{ 
    if ( rgs && rgs->size() != sellines.size() )
	return;

    lnmsfld_->clearSelection();
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const int selidx = lnmsfld_->indexOf( sellines.get(idx) );
	if ( selidx < 0 )
	    continue;

	lnmsfld_->setSelected( selidx );
	if ( rgs )
	    trcrgs_[selidx] = (*rgs)[idx];
    }

    lineSel(0);
}

void uiSeis2DMultiLineSelDlg::setZRange( const StepInterval<float>& rg )
{ if ( zrgfld_ ) zrgfld_->setRange( rg ); }

void uiSeis2DMultiLineSelDlg::lineSetSel( CallBacker* )
{
    const IOObj* lsetobj = linesetfld_ ? linesetfld_->ioobj() : ctio_.ioobj;
    if ( !lsetobj )
	return;

    SeisIOObjInfo oinf( lsetobj );
    BufferStringSet lnms;
    oinf.getLineNames( lnms );   
    lnmsfld_->setEmpty();
    maxtrcrgs_.erase();
    trcrgs_.erase();
    CubeSampling cs;
    if ( oinf.getRanges(cs) )
	setZRange( cs.zrg );

    BufferString selattrnm = linesetfld_ ? linesetfld_->attrNm() : "";

    StepInterval<float> maxzrg( mUdf(float), -mUdf(float), 1 );;
    
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const char* lnm = lnms.get(idx).buf();
	BufferStringSet attrbnms;
	oinf.getAttribNamesForLine( lnm, attrbnms );
	StepInterval<int> globtrcrg( 0, 0, 1 );
	int startidx=0; int maxattridx = attrbnms.size() - 1;
	const int defidx = attrbnms.indexOf( LineKey::sKeyDefAttrib() );
	if ( defidx>=0 && !setup_.withattr_ )
	    startidx = maxattridx = defidx;

	int maxnrtrcs = 0;
	for ( int attridx=startidx; attridx<=maxattridx; attridx++ )
	{
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    const char* attrnm = attrbnms.get( attridx );
	    if ( setup_.withattr_ && selattrnm != attrnm )
		continue;

	    LineKey lk( lnm, attrnm );
	    oinf.getRanges( lk, trcrg, zrg );
	    maxzrg.step = zrg.step;
	    maxzrg.include( zrg, false );
	    if ( trcrg.nrSteps() > maxnrtrcs )
	    {
		globtrcrg = trcrg;
		maxnrtrcs = trcrg.nrSteps();
	    }
	}

	if ( !maxnrtrcs )
	    continue;
	
	lnmsfld_->addItem( lnm );
	maxtrcrgs_ += globtrcrg;
	trcrgs_ += globtrcrg;
    }

    setZRange( maxzrg );
    lnmsfld_->selectAll();
    lineSel(0);
}


IOObj* uiSeis2DMultiLineSelDlg::getIOObj()
{ return linesetfld_ ? linesetfld_->getIOObj() : ctio_.ioobj->clone(); }

const char* uiSeis2DMultiLineSelDlg::getAttribName() const
{ return linesetfld_ ? linesetfld_->attrNm() : 0; }

void uiSeis2DMultiLineSelDlg::getSelLines( BufferStringSet& sellines ) const
{
    deepErase( sellines );
    lnmsfld_->getSelectedItems( sellines );
}

void uiSeis2DMultiLineSelDlg::getTrcRgs(TypeSet<StepInterval<int> >& rgs) const
{
    rgs.erase();
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( !lnmsfld_->isSelected(idx) )
	    continue;

	rgs += trcrgs_[idx];
    }
}

bool uiSeis2DMultiLineSelDlg::isAll() const
{
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
	if ( !lnmsfld_->isSelected(idx) || trcrgs_[idx] != maxtrcrgs_[idx] )
	    return false;

    return true;
}


void uiSeis2DMultiLineSelDlg::getZRange( StepInterval<float>& zrg ) const
{
    if ( zrgfld_ )
	zrg = zrgfld_->getRange();
}


void uiSeis2DMultiLineSelDlg::lineSel( CallBacker* )
{
    const bool multisel = lnmsfld_->nrSelected() > 1;
    trcrgfld_->setSensitive( !multisel );
    if ( multisel ) return;

    NotifyStopper ns( trcrgfld_->rangeChanged );
    const int curitm = lnmsfld_->currentItem();
    if ( trcrgs_.isEmpty() || curitm<0 )
	return;

    trcrgfld_->setLimitRange( maxtrcrgs_[curitm] );
    trcrgfld_->setRange( trcrgs_[curitm] );
}


void uiSeis2DMultiLineSelDlg::trcRgChanged( CallBacker* )
{
    const int curitm = lnmsfld_->currentItem();
    if ( curitm<0 ) return;

    trcrgs_[curitm].start = trcrgfld_->getRange().start;
    trcrgs_[curitm].stop = trcrgfld_->getRange().stop;
}


bool uiSeis2DMultiLineSelDlg::acceptOK( CallBacker* )
{
    if ( lnmsfld_->nrSelected() == 1 )
	trcRgChanged( 0 );
    return true;
}


uiSeis2DMultiLineSel::uiSeis2DMultiLineSel( uiParent* p, const Setup& setup )
    : uiCompoundParSel(p,"LineSet/LineName","Select")
    , setup_(*new uiSeis2DMultiLineSel::Setup(setup))
    , ctio_(*mMkCtxtIOObj(SeisTrc))
    , isall_(true)
{
    if ( !setup.lbltxt_.isEmpty() ) txtfld_->setTitleText( setup.lbltxt_ );
    butPush.notify( mCB(this,uiSeis2DMultiLineSel,doDlg) );
    ctio_.ctxt.deftransl = "2D";
    if ( setup_.filldef_ )
	ctio_.fillDefault();

    updateFromLineset();
}


uiSeis2DMultiLineSel::~uiSeis2DMultiLineSel()
{
    deepErase( sellines_ );
    delete &setup_;
    delete ctio_.ioobj;
    delete &ctio_;
}


BufferString uiSeis2DMultiLineSel::getSummary() const
{
    BufferString ret;
    if ( !ctio_.ioobj || !ctio_.ioobj->implExists(true) )
	return ret;

    ret = ctio_.ioobj->name();
    const int nrsel = sellines_.size();
    if ( nrsel==1 )
	ret += " (1 line)";
    else
    {
	ret += " (";
	if ( isall_ )
	    ret += "all";
	else
	    ret += nrsel;

	ret += " lines)";
    }

    return ret;
}


void uiSeis2DMultiLineSel::doDlg( CallBacker* )
{
    uiSeis2DMultiLineSelDlg dlg( this, ctio_, setup_ );
    if ( ctio_.ioobj )
    {
	dlg.setLineSet( ctio_.ioobj->key(), attrnm_.buf() );

	if ( isall_ )
	    dlg.setAll( true );
	else
	    dlg.setSelection( sellines_, &trcrgs_ );

	dlg.setZRange( zrg_ );
    }

    if ( !dlg.go() )
	return;

    if ( setup_.withlinesetsel_ )
    {
	IOObj* newobj = dlg.getIOObj();
	if ( newobj != ctio_.ioobj )
	{
	    delete ctio_.ioobj;
	    ctio_.ioobj = newobj;
	}

	attrnm_ = dlg.getAttribName();
    }

    isall_ = dlg.isAll();
    dlg.getZRange( zrg_ );
    dlg.getSelLines( sellines_ );
    dlg.getTrcRgs( trcrgs_ );
    updateSummary();
}


bool uiSeis2DMultiLineSel::fillPar( IOPar& par ) const
{
    if ( !ctio_.ioobj || !ctio_.ioobj->implExists(true) )
	return false;

    par.set( "LineSet.ID", ctio_.ioobj->key() );
    if ( !attrnm_.isEmpty() )
	par.set( sKey::Attribute(), attrnm_ );

    if ( setup_.withz_ )
	par.set( sKey::ZRange(), zrg_ );

    BufferString mergekey;
    IOPar lspar;
    for ( int idx=0; idx<sellines_.size(); idx++ )
    {
	IOPar linepar;
	linepar.set( sKey::Name(), sellines_[idx]->buf() );
	linepar.set( sKey::TrcRange(), trcrgs_[idx] );
	mergekey = idx;
	lspar.mergeComp( linepar, mergekey );
    }

    par.mergeComp( lspar, "Line" );
    return true;
}


void uiSeis2DMultiLineSel::usePar( const IOPar& par )
{
    MultiID lsetkey;
    if ( !par.get("LineSet.ID",lsetkey) )
	return;

    delete ctio_.ioobj;
    ctio_.ioobj = IOM().get( lsetkey );
    par.get( sKey::Attribute(), attrnm_ );
    par.get( sKey::ZRange(), zrg_ );

    deepErase( sellines_ );
    trcrgs_.erase();
    PtrMan<IOPar> lspar = par.subselect( "Line" );
    if ( !lspar ) return;

    for ( int idx=0; idx<1024; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar )
	    break;

	FixedString lnm = linepar->find( sKey::Name() );
	StepInterval<int> trcrg;
	if ( !lnm || !linepar->get(sKey::TrcRange(),trcrg) )
	    continue;

	sellines_.add( lnm );
	trcrgs_ += trcrg;
    }

    updateSummary();
}


const IOObj* uiSeis2DMultiLineSel::ioObj() const
{ return ctio_.ioobj; }

IOObj* uiSeis2DMultiLineSel::getIOObj()
{ return ctio_.ioobj ? ctio_.ioobj->clone() : 0; }

BufferString uiSeis2DMultiLineSel::getAttribName() const
{ return attrnm_; }

const BufferStringSet& uiSeis2DMultiLineSel::getSelLines() const
{ return sellines_; }

bool uiSeis2DMultiLineSel::isAll() const
{ return isall_; }

const StepInterval<float>& uiSeis2DMultiLineSel::getZRange() const
{ return zrg_; }

const TypeSet<StepInterval<int> >&  uiSeis2DMultiLineSel::getTrcRgs() const
{ return trcrgs_; }

void uiSeis2DMultiLineSel::setLineSet( const MultiID& key, const char* attr )
{
    delete ctio_.ioobj;
    ctio_.ioobj = IOM().get( key );
    attrnm_ = attr;
    deepErase( sellines_ );
    trcrgs_.erase();
    isall_ = true;
    updateFromLineset();
    updateSummary();
}


void uiSeis2DMultiLineSel::updateFromLineset()
{
    if ( !ctio_.ioobj )
	return;

    SeisIOObjInfo oinf( ctio_.ioobj );
    BufferStringSet lnms;
    BufferStringSet attrnms;
    oinf.getAttribNames( attrnms );
    if ( attrnm_.isEmpty() )
	attrnm_ = LineKey::sKeyDefAttrib();

    if ( attrnms.indexOf(attrnm_) < 0 )
	attrnm_ = attrnms.get(0);

    oinf.getLineNamesWithAttrib( attrnm_.buf(), lnms );
    if ( !lnms.size() )
	return;

    CubeSampling cs;
    if ( oinf.getRanges(cs) )
	zrg_ = cs.zrg;

    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const char* lnm = lnms.get(idx).buf();
	StepInterval<int> trcrg( 0, 0, 1 );
	StepInterval<float> zrg;
	LineKey lk( lnm, attrnm_.buf() );
	oinf.getRanges( lk, trcrg, zrg );
	trcrgs_ += trcrg;
	sellines_.add( lnm );
    }
}


void uiSeis2DMultiLineSel::setSelLines( const BufferStringSet& sellines )
{ sellines_ = sellines; updateSummary(); }

void uiSeis2DMultiLineSel::setAll( bool yn )
{ isall_ = yn; updateSummary(); }

void uiSeis2DMultiLineSel::setZRange( const StepInterval<float>& zrg )
{ zrg_ = zrg; }

void uiSeis2DMultiLineSel::setTrcRgs( const TypeSet<StepInterval<int> >& rgs )
{ trcrgs_ = rgs; }

