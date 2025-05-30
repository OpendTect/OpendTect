/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseislinesel.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "posinfo2d.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seisioobjinfo.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "transl.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjinserter.h"
#include "uilistbox.h"
#include "uilistbox.h"
#include "uilistboxchoiceio.h"
#include "uilistboxfilter.h"
#include "uimsg.h"
#include "uiselsurvranges.h"


// uiSeis2DLineChoose
uiSeis2DLineChoose::uiSeis2DLineChoose( uiParent* p, OD::ChoiceMode cm )
    : uiGroup(p,"Line chooser")
{
    SeisIOObjInfo::getLinesWithData( lnms_, geomids_ );
    Survey::sortByLinename( geomids_, &lnms_ );
    init( cm );

    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( SeisTrc2D );
    if ( ctio )
    {
	const BufferStringSet nms;
	uiButton* but = uiIOObjInserter::createInsertButton( this, *ctio,
							     inserters_, nms );
	for ( auto* inserter : inserters_ )
	    mAttachCB(inserter->objInserterd, uiSeis2DLineChoose::objInserted );

	but->attach( centeredLeftOf, listfld_ );
    }
}


uiSeis2DLineChoose::uiSeis2DLineChoose( uiParent* p, OD::ChoiceMode cm,
		const BufferStringSet& lnms, const TypeSet<Pos::GeomID>& gids )
    : uiGroup(p,"Line chooser")
    , geomids_(gids)
{
    Survey::sortByLinename( geomids_, &lnms_ );
    init( cm );
}


uiSeis2DLineChoose::uiSeis2DLineChoose( uiParent* p,
					const TypeSet<Pos::GeomID>& gids,
					OD::ChoiceMode cm )
    : uiGroup(p,"Line chooser")
    , geomids_(gids)
{
    Survey::sortByLinename( geomids_, &lnms_ );
    init( cm );
}


void uiSeis2DLineChoose::objInserted( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const MultiID&, dsid, cb );
    PtrMan<IOObj> dsobj = IOM().get( dsid );
    if ( !dsobj )
	return;

    Seis2DDataSet ds( *dsobj );
    for ( int idx=0; idx<ds.nrLines(); idx++ )
    {
	const Pos::GeomID geomid = ds.geomID( idx );
	if ( geomids_.isPresent(geomid) )
	    continue;

	geomids_ += geomid;
	lnms_.add( ds.lineName(idx) );
    }

    filtfld_->setItems( lnms_ );
}


void uiSeis2DLineChoose::init( OD::ChoiceMode cm )
{
    listfld_ = new uiListBox( this, "Lines", cm );
    listfld_->setHSzPol( uiObject::MedVar );
    filtfld_ = new uiListBoxFilter( *listfld_ );
    filtfld_->setItems( lnms_ );
    listfld_->resizeToContents();
    if ( isMultiChoice(cm) )
    {
	lbchoiceio_ = new uiListBoxChoiceIO( *listfld_, "Geometry" );
	mAttachCB( lbchoiceio_->readDone,
		   uiSeis2DLineChoose::readChoiceDone );
	mAttachCB( lbchoiceio_->storeRequested,
		   uiSeis2DLineChoose::writeChoiceReq );
    }

    setHAlignObj( listfld_ );
}


uiSeis2DLineChoose::~uiSeis2DLineChoose()
{
    detachAllNotifiers();
    delete lbchoiceio_;
    deepErase( inserters_ );
}


void uiSeis2DLineChoose::getChosen( TypeSet<Pos::GeomID>& chids ) const
{
    chids.setEmpty();
    TypeSet<int> chidxs; filtfld_->getChosen( chidxs );
    for ( int idx=0; idx<chidxs.size(); idx++ )
	chids += geomids_[ chidxs[idx] ];
}


void uiSeis2DLineChoose::getChosen( BufferStringSet& nms ) const
{
    nms.setEmpty();
    TypeSet<int> chidxs; filtfld_->getChosen( chidxs );
    for ( int idx=0; idx<chidxs.size(); idx++ )
	nms.add( lnms_.get(chidxs[idx]) );
}


void uiSeis2DLineChoose::setChosen( const TypeSet<Pos::GeomID>& gids )
{
    const OD::ChoiceMode cm = listfld_->choiceMode();

    chooseAll( false );
    for ( const auto& gid : gids )
    {
	if ( !geomids_.isPresent(gid) )
	    continue;

	const int idxof = geomids_.indexOf( gid );
	listfld_->setChosen( idxof, true );
    }

    if ( cm == OD::ChooseOnlyOne && gids.size() > 1 )
	{ pErrMsg("Incompatible line choice (more than one chosen)"); }
    else if ( cm == OD::ChooseAtLeastOne && gids.isEmpty() &&
	      !listfld_->isEmpty() )
    {
	pErrMsg("Incompatible line choice (need at least one)");
	listfld_->setChosen( 0, true );
    }

    listfld_->setChoiceMode( cm );
}


void uiSeis2DLineChoose::setChosen( const BufferStringSet& nms )
{
    const OD::ChoiceMode cm = listfld_->choiceMode();

    chooseAll( false );
    for ( const auto* lnm : nms )
    {
	if ( !lnms_.isPresent(lnm->buf()) )
	    continue;

	const int idxof = lnms_.indexOf( lnm->buf() );
	listfld_->setChosen( idxof, true );
    }

    if ( cm == OD::ChooseOnlyOne && nms.size() > 1 )
	{ pErrMsg("Incompatible line choice (more than one chosen)"); }
    else if ( cm == OD::ChooseAtLeastOne && nms.isEmpty() &&
	      !listfld_->isEmpty() )
    {
	pErrMsg("Incompatible line choice (need at least one)");
	listfld_->setChosen( 0, true );
    }

    listfld_->setChoiceMode( cm );
}


void uiSeis2DLineChoose::chooseAll( bool yn )
{
    if ( yn )
	filtfld_->setFilter( nullptr );

    listfld_->setAllowNoneChosen( yn );
    listfld_->chooseAll( yn );
}


void uiSeis2DLineChoose::readChoiceDone( CallBacker* )
{
    TypeSet<Pos::GeomID> gids;
    for ( int idx=0; idx<lbchoiceio_->chosenKeys().size(); idx++ )
    {
	const MultiID mid = lbchoiceio_->chosenKeys().get(idx);
	gids += Pos::GeomID( mid.objectID() );
    }

    setChosen( gids );
}


void uiSeis2DLineChoose::writeChoiceReq( CallBacker* )
{
    MultiID mid = IOObjContext::getStdDirData(IOObjContext::Geom)->id_;

    lbchoiceio_->keys().setEmpty();
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	const int idxof = lnms_.indexOf( listfld_->textOfItem(idx) );
	if ( idxof < 0 )
	    { pErrMsg("Huh"); continue; }

	mid.setObjectID( geomids_[idxof].asInt() );
	lbchoiceio_->keys().add( mid );
    }
}



// uiSeis2DLineSel

uiSeis2DLineSel::uiSeis2DLineSel( uiParent* p, bool multisel )
    : uiCompoundParSel(p,multisel
	? uiStrings::phrSelect(mJoinUiStrs(s2D(),sLine(2).toLower()))
	: uiStrings::phrSelect(mJoinUiStrs(s2D(),sLine(1).toLower())))
    , ismultisel_(multisel)
    , selectionChanged(this)
{
    txtfld_->setElemSzPol( uiObject::Wide );
    mAttachCB( butPush, uiSeis2DLineSel::selPush );
    BufferStringSet lnms; TypeSet<Pos::GeomID> geomids;
    SeisIOObjInfo::getLinesWithData( lnms, geomids );
    ConstArrPtrMan<int> idxs = lnms.getSortIndexes( false );
    if ( !idxs )
    {
	lnms_ = lnms;
	geomids_ = geomids;
    }
    else
    {
	const int sz = lnms.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    lnms_.add( lnms[ idxs[idx] ]->buf() );
	    geomids_.add( geomids[ idxs[idx] ] );
	}
    }

    if ( lnms_.size()==1 )
    {
	setAll( true );
	updateSummary();
    }
}


uiSeis2DLineSel::~uiSeis2DLineSel()
{
    detachAllNotifiers();
}


const char* uiSeis2DLineSel::lineName() const
{ return selidxs_.isEmpty() ? "" : lnms_.get( selidxs_[0] ).buf(); }

int uiSeis2DLineSel::nrSelected() const
{ return selidxs_.size(); }

Pos::GeomID uiSeis2DLineSel::geomID() const
{
    return selidxs_.isEmpty() ? Pos::GeomID::udf() : geomids_[selidxs_[0]];
}


void uiSeis2DLineSel::getSelGeomIDs( TypeSet<Pos::GeomID>& selids ) const
{
    selids.erase();
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	selids += geomids_[selidxs_[idx]];
}


void uiSeis2DLineSel::getSelLineNames( BufferStringSet& selnms ) const
{
    selnms.setEmpty();
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	selnms.add( lnms_.get(selidxs_[idx]) );
}


void uiSeis2DLineSel::setSelLineNames( const BufferStringSet& selnms )
{
    selidxs_.erase();
    for ( int idx=0; idx<selnms.size(); idx++ )
    {
	const int index = lnms_.indexOf( selnms.get(idx) );
	if ( index >= 0 )
	    selidxs_ += index;
    }

    updateSummary();
}


void uiSeis2DLineSel::setSelGeomIDs( const TypeSet<Pos::GeomID>& selgeomids )
{
    selidxs_.erase();
    for ( int idx=0; idx<selgeomids.size(); idx++ )
    {
	const int index = geomids_.indexOf( selgeomids[idx] );
	if ( index >= 0 )
	    selidxs_ += index;
    }

    updateSummary();
}


void uiSeis2DLineSel::setAll( bool yn )
{
    selidxs_.erase();
    if ( !yn )
	return;

    for ( int idx=0; idx<geomids_.size(); idx++ )
	selidxs_ += idx;
}


bool uiSeis2DLineSel::isAll() const
{
    return selidxs_.size() == geomids_.size();
}


void uiSeis2DLineSel::setInput( const BufferStringSet& lnms )
{
    clearAll();
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const Pos::GeomID geomid = Survey::GM().getGeomID( lnms.get(idx) );
	if ( !geomid.is2D() )
	    continue;

	geomids_ += geomid;
	lnms_.add( lnms.get(idx) );
    }

    setAll( true );
    updateSummary();
    selectionChanged.trigger();
}


void uiSeis2DLineSel::setInput( const TypeSet<Pos::GeomID>& geomids )
{
    clearAll();
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const char* linenm = Survey::GM().getName( geomids[idx] );
	if ( !linenm )
	    continue;

	geomids_ += geomids[idx];
	lnms_.add( linenm );
    }

    setAll( true );
    updateSummary();
    selectionChanged.trigger();
}


void uiSeis2DLineSel::setInput( const MultiID& datasetid )
{
    const SeisIOObjInfo oi( datasetid );
    if ( !oi.isOK() )
	return;

    BufferStringSet lnms;
    oi.getLineNames( lnms );
    setInput( lnms );
}


BufferString uiSeis2DLineSel::getSummary() const
{
    BufferString ret( "No lines selected" );
    if ( selidxs_.isEmpty() )
	return ret;

    ret = lnms_.get( selidxs_[0] );
    if ( !ismultisel_ || selidxs_.size()==1 )
	return ret;

    if ( selidxs_.size() == lnms_.size() )
	ret = "All";
    else
	ret = selidxs_.size();

    ret += " lines";
    return ret;
}


bool uiSeis2DLineSel::inputOK( bool doerr ) const
{
    if ( selidxs_.isEmpty() )
    {
	if ( doerr )
	    uiMSG().error( tr("Please select the line") );
	return false;
    }

    return true;
}


void uiSeis2DLineSel::clearAll()
{
    lnms_.setEmpty();
    geomids_.setEmpty();
    clearSelection();
}


void uiSeis2DLineSel::clearSelection()
{
    selidxs_.erase();
    updateSummary();
}

#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeis2DLineSel::selPush( CallBacker* )
{
    if ( lnms_.isEmpty() )
	mErrRet( tr("No 2D lines available") )

    const TypeSet<int> curselidxs = selidxs_;
    uiDialog dlg( this, uiDialog::Setup(tr("Line selection"),mNoHelpKey) );
    auto* lchfld = new uiSeis2DLineChoose( &dlg, geomids_,
		    ismultisel_ ? OD::ChooseAtLeastOne : OD::ChooseOnlyOne );
    TypeSet<Pos::GeomID> chosenids;
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	chosenids += geomids_[ selidxs_[idx] ];
    lchfld->setChosen( chosenids );

    if ( dlg.go() != uiDialog::Accepted )
	return;

    selidxs_.erase();
    lchfld->getChosen( chosenids );
    for ( int idx=0; idx<chosenids.size(); idx++ )
	selidxs_ += geomids_.indexOf( chosenids[idx] );

    if ( curselidxs != selidxs_ )
	selectionChanged.trigger();
}


void uiSeis2DLineSel::setSelLine( const char* lnm )
{
    selidxs_.erase();
    const int idx = lnms_.indexOf( lnm );
    if ( idx >= 0 )
	selidxs_ += idx;

    updateSummary();
}


void uiSeis2DLineSel::setSelLine( const Pos::GeomID geomid )
{
    selidxs_.erase();
    const int idx = geomids_.indexOf( geomid );
    if ( idx >= 0 )
	selidxs_ += idx;

    updateSummary();
}



// uiSeis2DLineNameSel

uiSeis2DLineNameSel::uiSeis2DLineNameSel( uiParent* p, bool forread )
    : uiGroup(p,"2D line name sel")
    , forread_(forread)
    , nameChanged(this)
{
    auto* lcb = new uiLabeledComboBox( this, tr("Line name") );
    fld_ = lcb->box();
    fld_->setReadOnly( forread_ );
    fld_->setHSzPol( uiObject::Wide );
    setHAlignObj( lcb );
    if ( !forread_ )
	fillWithAll();

    mAttachCB( fld_->selectionChanged, uiSeis2DLineNameSel::selChg );
}


uiSeis2DLineNameSel::~uiSeis2DLineNameSel()
{
    detachAllNotifiers();
}


void uiSeis2DLineNameSel::fillWithAll()
{
    fld_->setEmpty();
    if ( !forread_ )
	fld_->addItem( uiStrings::sEmptyString() );

    BufferStringSet lnms;
    TypeSet<Pos::GeomID> geomids;
    Survey::GM().getList( lnms, geomids, true );
    lnms.sort();
    fld_->addItems( lnms );
    if ( fld_->size() )
	fld_->setCurrentItem( 0 );
}


void uiSeis2DLineNameSel::addLineNames( const MultiID& ky )
{
    const SeisIOObjInfo oi( ky );
    if ( !oi.isOK() || !oi.is2D() )
	return;

    BufferStringSet lnms; oi.getLineNames( lnms );
    lnms.sort();
    nameChanged.disable();
    fld_->addItems( lnms );
    nameChanged.enable();
}


const char* uiSeis2DLineNameSel::getInput() const
{
    return fld_->text();
}


Pos::GeomID uiSeis2DLineNameSel::getInputGeomID() const
{
    return Survey::GM().getGeomID( getInput() );
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


void uiSeis2DLineNameSel::setDataSet( const MultiID& ky )
{
    dsid_ = ky;
    fld_->setEmpty();
    if ( !forread_ )
	fld_->addItem( uiString::empty() );

    addLineNames( ky );
    nameChanged.trigger();
}



// uiSeis2DMultiLineSel

class uiSeis2DMultiLineSelDlg : public uiDialog
{ mODTextTranslationClass(uiSeis2DMultiLineSelDlg)
public:
			uiSeis2DMultiLineSelDlg(uiParent*,
				const TypeSet<Pos::GeomID>& geomids,
				const BufferStringSet& lnms,
				const TypeSet<StepInterval<int> >& maxtrcrgs,
				const TypeSet<StepInterval<float> >& maxzrgs,
				const uiSeis2DMultiLineSel::Setup&,
				OD::ChoiceMode cm=OD::ChooseAtLeastOne);
			~uiSeis2DMultiLineSelDlg();

    BufferString	getSummary() const;

    bool		isAll() const;
    void		getSelLines(TypeSet<int>&) const;
    void		getTrcRgs(TypeSet<StepInterval<int> >&) const;
    void		getZRgs(TypeSet<StepInterval<float> >&)const;

    void		setAll(bool);
    void		setSelection(const TypeSet<int>&,
				const TypeSet<StepInterval<int> >&,
				const TypeSet<StepInterval<float> >&);
    void		setZRgs(const StepInterval<float>&);

private:

    uiListBox*		lnmsfld_;
    uiSelNrRange*	trcrgfld_;
    uiSelZRange*	zrgfld_ = nullptr;

    TypeSet<StepInterval<int> >	trcrgs_;
    TypeSet<StepInterval<float> > zrgs_;

    const TypeSet<StepInterval<int> >&	maxtrcrgs_;
    const TypeSet<StepInterval<float> >& maxzrgs_;

    void		initDlgCB(CallBacker*);
    void		lineSel(CallBacker*);
    void		trcRgChanged(CallBacker*);
    void		zRgChanged(CallBacker*);
    void		applyZRgToAllCB(CallBacker*);

    bool		acceptOK(CallBacker*) override;
};


uiSeis2DMultiLineSelDlg::uiSeis2DMultiLineSelDlg( uiParent* p,
				const TypeSet<Pos::GeomID>& geomids,
				const BufferStringSet& lnms,
				const TypeSet<StepInterval<int> >& maxtrcrgs,
				const TypeSet<StepInterval<float> >& maxzrgs,
				const uiSeis2DMultiLineSel::Setup& selsu,
				OD::ChoiceMode cm )
    : uiDialog(p,Setup(tr("Select 2D Lines"),
		       mODHelpKey(mSeis2DMultiLineSelDlgHelpID)))
    , trcrgs_(maxtrcrgs)
    , zrgs_(maxzrgs)
    , maxtrcrgs_(maxtrcrgs)
    , maxzrgs_(maxzrgs)
{
    uiListBox::Setup su( cm, tr("Select Lines") );
    lnmsfld_ = new uiListBox( this, su, "linenames" );
    lnmsfld_->addItems( lnms );
    lnmsfld_->resizeToContents();
    mAttachCB( lnmsfld_->selectionChanged, uiSeis2DMultiLineSelDlg::lineSel );

    trcrgfld_ = new uiSelNrRange( this, selsu.withstep_, uiStrings::sTrace() );
    mAttachCB( trcrgfld_->rangeChanged, uiSeis2DMultiLineSelDlg::trcRgChanged );
    trcrgfld_->attach( alignedBelow, lnmsfld_ );

    if ( selsu.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, selsu.withstep_,
				   selsu.zdomkey_.buf(), selsu.zunitstr_.buf());
	mAttachCB( zrgfld_->rangeChanged, uiSeis2DMultiLineSelDlg::zRgChanged );
	zrgfld_->attach( alignedBelow, trcrgfld_ );

	auto* allbut = new uiPushButton( this, tr("Apply to all lines"),
		mCB(this,uiSeis2DMultiLineSelDlg,applyZRgToAllCB), true );
	allbut->attach( rightOf, zrgfld_ );
    }

    mAttachCB( postFinalize(), uiSeis2DMultiLineSelDlg::initDlgCB );
}


uiSeis2DMultiLineSelDlg::~uiSeis2DMultiLineSelDlg()
{
    detachAllNotifiers();
}


void uiSeis2DMultiLineSelDlg::initDlgCB( CallBacker* )
{
    lineSel( nullptr );
}


void uiSeis2DMultiLineSelDlg::setAll( bool yn )
{
    lnmsfld_->chooseAll( yn );
    trcrgs_ = maxtrcrgs_;
    if ( zrgfld_ )
	zrgs_ = maxzrgs_;
}

void uiSeis2DMultiLineSelDlg::setSelection( const TypeSet<int>& selidxs,
				const TypeSet<StepInterval<int> >& trcrgs,
				const TypeSet<StepInterval<float> >& zrgs )
{
    lnmsfld_->setChosen( selidxs );
    if ( !selidxs.isEmpty() )
	lnmsfld_->setCurrentItem( selidxs[0] );

    trcrgs_ = trcrgs;
    zrgs_ = zrgs;
    lineSel( nullptr );
}


void uiSeis2DMultiLineSelDlg::getSelLines( TypeSet<int>& selidxs ) const
{
    selidxs.erase();
    lnmsfld_->getChosen( selidxs );
}


void uiSeis2DMultiLineSelDlg::getTrcRgs(TypeSet<StepInterval<int> >& rgs) const
{
    rgs = trcrgs_;
}


bool uiSeis2DMultiLineSelDlg::isAll() const
{
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
	if ( !lnmsfld_->isChosen(idx) || trcrgs_[idx] != maxtrcrgs_[idx] )
	    return false;

    return true;
}


void uiSeis2DMultiLineSelDlg::getZRgs(
				TypeSet<StepInterval<float> >& zrgs ) const
{
    zrgs = zrgs_;
}


void uiSeis2DMultiLineSelDlg::lineSel( CallBacker* )
{
    const int curitm = lnmsfld_->currentItem();
    if ( !trcrgs_.validIdx(curitm) )
	return;

    NotifyStopper ns( trcrgfld_->rangeChanged );
    trcrgfld_->setLimitRange( maxtrcrgs_[curitm] );
    trcrgfld_->setRange( trcrgs_[curitm] );
    if ( !zrgfld_ || !zrgs_.validIdx(curitm) )
	return;

    NotifyStopper ns2( zrgfld_->rangeChanged );
    zrgfld_->setRangeLimits( maxzrgs_[curitm] );
    zrgfld_->setRange( zrgs_[curitm] );
}


void uiSeis2DMultiLineSelDlg::trcRgChanged( CallBacker* )
{
    const int curitm = lnmsfld_->currentItem();
    if ( curitm<0 )
	return;

    trcrgs_[curitm] = trcrgfld_->getRange();
}


void uiSeis2DMultiLineSelDlg::zRgChanged( CallBacker* )
{
    if ( !zrgfld_ )
	return;

    const int curitm = lnmsfld_->currentItem();
    if ( curitm<0 )
	return;

    zrgs_[curitm] = zrgfld_->getRange();
}


void uiSeis2DMultiLineSelDlg::applyZRgToAllCB( CallBacker* )
{
    const StepInterval<float> zrg = zrgfld_->getRange();
    for ( int idx=0; idx<zrgs_.size(); idx++ )
    {
	zrgs_[idx] = maxzrgs_[idx];
	zrgs_[idx].limitTo( zrg );
    }
}


bool uiSeis2DMultiLineSelDlg::acceptOK( CallBacker* )
{
    if ( lnmsfld_->nrChosen() == 1 )
    {
	trcRgChanged( 0 );
	zRgChanged( 0 );
    }

    return true;
}



// uiSeis2DMultiLineSel::Setup

uiSeis2DMultiLineSel::Setup::Setup( const uiString& text, bool forread,
				    bool isall, bool withz, bool withstep )
    : seltxt_(text)
    , forread_(forread)
    , isall_(isall)
    , withz_(withz)
    , withstep_(withstep)
{
}


uiSeis2DMultiLineSel::Setup::~Setup()
{
}



// uiSeis2DMultiLineSel

uiSeis2DMultiLineSel::uiSeis2DMultiLineSel( uiParent* p, const Setup& su )
    : uiSeis2DLineSel(p,su.forread_)
    , setup_(su)
    , isall_(su.isall_)
{
    if ( !setup_.seltxt_.isEmpty() )
	setSelText( setup_.seltxt_ );

    initRanges();
}


uiSeis2DMultiLineSel::uiSeis2DMultiLineSel( uiParent* p, const uiString& text,
					    bool withz, bool withstep )
    : uiSeis2DLineSel(p,true)
    , setup_(text,true,false,withz,withstep)
    , isall_(false)
{
    if ( !setup_.seltxt_.isEmpty() )
	txtfld_->setTitleText( setup_.seltxt_ );

    initRanges();
}


uiSeis2DMultiLineSel::~uiSeis2DMultiLineSel()
{
}


void uiSeis2DMultiLineSel::clearAll()
{
    uiSeis2DLineSel::clearAll();
    trcrgs_.erase();
    zrgs_.erase();
    maxtrcrgs_.erase();
    maxzrgs_.erase();
}


void uiSeis2DMultiLineSel::setInput( const BufferStringSet& lnms )
{
    selectionChanged.disable();
    uiSeis2DLineSel::setInput( lnms );
    initRanges();
    selectionChanged.enable();
    selectionChanged.trigger();
}


void uiSeis2DMultiLineSel::setInput( const TypeSet<Pos::GeomID>& geomids )
{
    selectionChanged.disable();
    uiSeis2DLineSel::setInput( geomids );
    initRanges();
    selectionChanged.enable();
    selectionChanged.trigger();
}


void uiSeis2DMultiLineSel::setInput( const MultiID& datasetid )
{
    selectionChanged.disable();
    const SeisIOObjInfo oi( datasetid );
    if ( !oi.isOK() )
	return;

    BufferStringSet lnms;
    oi.getLineNames( lnms );
    uiSeis2DLineSel::setInput( lnms );
    initRanges( &datasetid );
    selectionChanged.enable();
    selectionChanged.trigger();
}


void uiSeis2DMultiLineSel::initRanges( const MultiID* datasetid )
{
    zrgs_.erase(); trcrgs_.erase();
    maxzrgs_.erase(); maxtrcrgs_.erase();

    PtrMan<IOObj> ioobj = datasetid ? IOM().get( *datasetid ) : nullptr;
    PtrMan<Seis2DDataSet> dataset = ioobj ?  new Seis2DDataSet( *ioobj )
					  : nullptr;

    for ( const auto& geomid : geomids_ )
    {
	StepInterval<int> trcrg(0,0,1);
	StepInterval<float> zrg = SI().zRange(false);
	if ( dataset && !dataset->isEmpty() )
	   dataset->getRanges( geomid, trcrg, zrg );
	else
	{
	    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			     Survey::GM().getGeometry( geomid ) );
	    if ( geom2d )
	    {
		trcrg = geom2d->data().trcNrRange();
		zrg = geom2d->data().zRange();
	    }
	}

	maxtrcrgs_ += trcrg; trcrgs_ += trcrg;
	maxzrgs_ += zrg; zrgs_ += zrg;
    }
}


void uiSeis2DMultiLineSel::selPush( CallBacker* )
{
    OD::ChoiceMode cm = ismultisel_ ? OD::ChooseAtLeastOne : OD::ChooseOnlyOne;
    uiSeis2DMultiLineSelDlg dlg( this, geomids_, lnms_, maxtrcrgs_, maxzrgs_,
				 setup_, cm );
    if ( isall_ )
	dlg.setAll( true );
    else
	dlg.setSelection( selidxs_, trcrgs_, zrgs_ );

    if ( dlg.go() != uiDialog::Accepted )
	return;

    isall_ = dlg.isAll();
    dlg.getSelLines( selidxs_ );
    dlg.getTrcRgs( trcrgs_ );
    dlg.getZRgs( zrgs_ );
    updateSummary();
    selectionChanged.trigger();
}


bool uiSeis2DMultiLineSel::fillPar( IOPar& par ) const
{
    BufferString mergekey;
    IOPar lspar;
    for ( int idx=0; idx<selidxs_.size(); idx++ )
    {
	IOPar linepar;
	const Pos::GeomID selgeomid = geomids_[selidxs_[idx]];
	linepar.set( sKey::GeomID(), selgeomid );
	linepar.set( sKey::TrcRange(), trcrgs_[selidxs_[idx]] );
	if ( setup_.withz_ )
	    linepar.set( sKey::ZRange(), zrgs_[selidxs_[idx]] );

	mergekey = idx;
	lspar.mergeComp( linepar, mergekey );
    }

    par.mergeComp( lspar, "Line" );
    return true;
}


void uiSeis2DMultiLineSel::usePar( const IOPar& par )
{
    isall_ = false;
    BufferString lsetname; // For backward compatibility.
    MultiID lsetkey;
    if ( par.get("LineSet.ID",lsetkey) )
    {
	PtrMan<IOObj> lsobj = IOM().get( lsetkey );
	if ( lsobj )
	    lsetname = lsobj->name();
    }

    clearSelection();
    PtrMan<IOPar> lspar = par.subselect( "Line" );
    if ( !lspar )
	return;

    TypeSet<Pos::GeomID> selgeomids;
    TypeSet<StepInterval<float> > selzrgs;
    TypeSet<StepInterval<int> > seltrcrgs;
    for ( int idx=0; idx<1024; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar )
	    break;

	Pos::GeomID geomid;
	if ( !linepar->get(sKey::GeomID(),geomid) )
	{
	    const BufferString lnm = linepar->find( sKey::Name() );
	    geomid = Survey::GM().getGeomID( lsetname, lnm );
	}

	if ( !geomid.is2D() )
	    break;

	selgeomids += geomid;
	StepInterval<int> trcrg;
	if ( !linepar->get(sKey::TrcRange(),trcrg) )
	    continue;

	seltrcrgs += trcrg;
	if ( !setup_.withz_ )
	    continue;

	StepInterval<float> zrg;
	if ( linepar->get(sKey::ZRange(),zrg) )
	    selzrgs += zrg;
    }

    setSelGeomIDs( selgeomids );
    if ( !seltrcrgs.isEmpty() )
	setTrcRanges( seltrcrgs );

    if ( !selzrgs.isEmpty() )
	setZRanges( selzrgs );

    isall_ = uiSeis2DLineSel::isAll();
}


bool uiSeis2DMultiLineSel::isAll() const
{
    return isall_;
}


void uiSeis2DMultiLineSel::getZRanges(
				TypeSet<StepInterval<float> >& zrgs ) const
{
    zrgs.erase();
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	zrgs += zrgs_[selidxs_[idx]];
}


void uiSeis2DMultiLineSel::getTrcRanges(
				TypeSet<StepInterval<int> >& trcrgs ) const
{
    trcrgs.erase();
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	trcrgs += trcrgs_[selidxs_[idx]];
}


StepInterval<float> uiSeis2DMultiLineSel::getZRange( Pos::GeomID geomid ) const
{
    const int idx = geomids_.indexOf( geomid );
    if ( zrgs_.validIdx(idx) )
	return zrgs_[idx];

    return zrgs_.isEmpty() ? StepInterval<float>::udf() : zrgs_[0];
}


StepInterval<int> uiSeis2DMultiLineSel::getTrcRange( Pos::GeomID geomid ) const
{
    StepInterval<int> trcrg = StepInterval<int>::udf();
    const int idx = geomids_.indexOf( geomid );
    if ( zrgs_.validIdx(idx) )
	trcrg = trcrgs_[idx];

    return trcrg;
}


void uiSeis2DMultiLineSel::setSelLines( const BufferStringSet& sellines )
{ isall_ = false; uiSeis2DLineSel::setSelLineNames( sellines ); }

void uiSeis2DMultiLineSel::setAll( bool yn )
{ uiSeis2DLineSel::setAll( yn ); isall_ = yn; }

void uiSeis2DMultiLineSel::setZRanges(
				const TypeSet<StepInterval<float> >& zrgs )
{
    if ( zrgs.size() != selidxs_.size() )
	return;

    for ( int idx=0; idx<selidxs_.size(); idx++ )
	zrgs_[selidxs_[idx]] = zrgs[idx];
}


void uiSeis2DMultiLineSel::setTrcRanges(
				const TypeSet<StepInterval<int> >& trcrgs )
{
    if ( trcrgs.size() != selidxs_.size() )
	return;

    for ( int idx=0; idx<selidxs_.size(); idx++ )
	trcrgs_[selidxs_[idx]] = trcrgs[idx];
}


void uiSeis2DMultiLineSel::setSingleLine( bool yn )
{
    ismultisel_ = !yn;
}
