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
#include "uilistboxfilter.h"
#include "uilistboxchoiceio.h"
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
#include "seis2dlineio.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "transl.h"
#include "od_helpids.h"


static void getGMList( BufferStringSet& lnms, TypeSet<Pos::GeomID>& gids )
{
    Survey::GM().getList( lnms, gids, true );
    for ( int idx=gids.size()-1; idx>=0; idx-- )
    {
	if ( !SeisIOObjInfo::hasData(gids[idx]) )
	{
	    lnms.removeSingle( idx );
	    gids.removeSingle( idx );
	}
    }
}


uiSeis2DLineChoose::uiSeis2DLineChoose( uiParent* p, OD::ChoiceMode cm )
    : uiGroup(p,"Line chooser")
    , lbchoiceio_(0)
{
    getGMList( lnms_, geomids_ );
    init( cm );
}


uiSeis2DLineChoose::uiSeis2DLineChoose( uiParent* p, OD::ChoiceMode cm,
		const BufferStringSet& lnms, const TypeSet<Pos::GeomID>& gids )
    : uiGroup(p,"Line chooser")
    , lnms_(lnms)
    , geomids_(gids)
    , lbchoiceio_(0)
{
    init( cm );
}


void uiSeis2DLineChoose::init( OD::ChoiceMode cm )
{
    listfld_ = new uiListBox( this, "Lines", cm );
    filtfld_ = new uiListBoxFilter( *listfld_ );
    filtfld_->setItems( lnms_ );
    if ( isMultiChoice(cm) )
    {
	lbchoiceio_ = new uiListBoxChoiceIO( *listfld_, "2D Lines" );
	lbchoiceio_->readDone.notify(
				mCB(this,uiSeis2DLineChoose,readChoiceDone) );
	lbchoiceio_->storeRequested.notify(
				mCB(this,uiSeis2DLineChoose,writeChoiceReq) );
    }
}


uiSeis2DLineChoose::~uiSeis2DLineChoose()
{
    delete lbchoiceio_;
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
    filtfld_->setFilter( 0 );
    listfld_->chooseAll( false );
    for ( int idx=0; idx<gids.size(); idx++ )
    {
	const int idxof = geomids_.indexOf( gids[idx] );
	if ( idxof >= 0 )
	    listfld_->setChosen( idxof, true );
    }
}


void uiSeis2DLineChoose::setChosen( const BufferStringSet& nms )
{
    filtfld_->setFilter( 0 );
    listfld_->chooseAll( false );
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	const int idxof = lnms_.indexOf( nms.get(idx) );
	if ( idxof >= 0 )
	    listfld_->setChosen( idxof, true );
    }
}


void uiSeis2DLineChoose::chooseAll( bool yn )
{
    if ( yn )
	filtfld_->setFilter( 0 );
    listfld_->chooseAll( yn );
}


void uiSeis2DLineChoose::readChoiceDone( CallBacker* )
{
    TypeSet<Pos::GeomID> gids;
    for ( int idx=0; idx<lbchoiceio_->chosenKeys().size(); idx++ )
	gids += (Pos::GeomID)toInt( lbchoiceio_->chosenKeys().get(idx).buf() );
    setChosen( gids );
}


void uiSeis2DLineChoose::writeChoiceReq( CallBacker* )
{
    lbchoiceio_->keys().setEmpty();
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	const int idxof = lnms_.indexOf( listfld_->textOfItem(idx) );
	if ( idxof < 0 )
	    { pErrMsg("Huh"); continue; }
	lbchoiceio_->keys().add( toString(geomids_[idxof]) );
    }
}


uiSeis2DLineSel::uiSeis2DLineSel( uiParent* p, bool multisel )
    : uiCompoundParSel(p,multisel?"Select 2D lines":"Select 2D line")
    , ismultisel_(multisel)
    , selectionChanged(this)
{
    txtfld_->setElemSzPol( uiObject::Wide );
    butPush.notify( mCB(this,uiSeis2DLineSel,selPush) );
    getGMList( lnms_, geomids_ );
}


const char* uiSeis2DLineSel::lineName() const
{ return selidxs_.isEmpty() ? "" : lnms_.get( selidxs_[0] ).buf(); }

int uiSeis2DLineSel::nrSelected() const
{ return selidxs_.size(); }

Pos::GeomID uiSeis2DLineSel::geomID() const
{
    return selidxs_.isEmpty() ? Survey::GeometryManager::cUndefGeomID()
			      : geomids_[selidxs_[0]];
}


void uiSeis2DLineSel::getSelGeomIDs( TypeSet<Pos::GeomID>& selids ) const
{
    selids.erase();
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	selids += geomids_[selidxs_[idx]];
}


void uiSeis2DLineSel::getSelLineNames( BufferStringSet& selnms ) const
{
    deepErase( selnms );
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
	Pos::GeomID geomid = Survey::GM().getGeomID( lnms.get(idx) );
	if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	    continue;

	geomids_ += geomid;
	lnms_.add( lnms.get(idx) );
	selidxs_ += selidxs_.size();
    }

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
	selidxs_ += selidxs_.size();
    }

    updateSummary();
    selectionChanged.trigger();
}


void uiSeis2DLineSel::setInput( const MultiID& datasetid )
{
    const SeisIOObjInfo oi( datasetid );
    BufferStringSet lnms; oi.getLineNames( lnms );
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
    deepErase( lnms_ );
    geomids_.erase();
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

    uiDialog dlg( this,
		  uiDialog::Setup("Line selection",mNoDlgTitle,mNoHelpKey) );
    uiSeis2DLineChoose* lchfld = new uiSeis2DLineChoose( &dlg,
		    ismultisel_ ? OD::ChooseAtLeastOne : OD::ChooseOnlyOne,
		    lnms_, geomids_ );
    TypeSet<Pos::GeomID> chosenids;
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	chosenids += geomids_[ selidxs_[idx] ];
    lchfld->setChosen( chosenids );

    if ( !dlg.go() )
	return;

    selidxs_.erase();
    lchfld->getChosen( chosenids );
    for ( int idx=0; idx<chosenids.size(); idx++ )
	selidxs_ += geomids_.indexOf( chosenids[idx] );

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
    if ( dsid_.isEmpty() )
	fillWithAll();
}


void uiSeis2DLineNameSel::fillWithAll()
{
    BufferStringSet lnms;
    TypeSet<Pos::GeomID> geomids;
    getGMList( lnms, geomids );
    fld_->addItems( lnms );
    if ( fld_->size() )
	fld_->setCurrentItem( 0 );
}


void uiSeis2DLineNameSel::addLineNames( const MultiID& ky )
{
    const SeisIOObjInfo oi( ky );
    if ( !oi.isOK() || !oi.is2D() ) return;

    BufferStringSet lnms; oi.getLineNames( lnms );
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


void uiSeis2DLineNameSel::setDataSet( const MultiID& ky )
{
    dsid_ = ky;
    fld_->setEmpty();
    if ( !forread_ ) fld_->addItem( uiStrings::sEmptyString() );
    addLineNames( ky );
}


class uiSeis2DMultiLineSelDlg : public uiDialog
{
public:
				uiSeis2DMultiLineSelDlg(uiParent*,
					const TypeSet<Pos::GeomID>& geomids,
					const BufferStringSet& lnms,
					bool withz,bool withstep);
				~uiSeis2DMultiLineSelDlg()	{}

    BufferString		getSummary() const;

    void			getSelLines(TypeSet<int>&) const;
    bool			isAll() const;
    void			getZRgs(TypeSet<StepInterval<float> >&)const;
    void			getTrcRgs(TypeSet<StepInterval<int> >&) const;

    void			setSelection(const TypeSet<int>&,
					const TypeSet<StepInterval<int> >&,
					const TypeSet<StepInterval<float> >&);
    void			setAll(bool);
    void			setZRgs(const StepInterval<float>&);

protected:

    uiListBox*			lnmsfld_;
    uiSelNrRange*		trcrgfld_;
    uiSelZRange*		zrgfld_;

    TypeSet<StepInterval<int> >	trcrgs_;
    TypeSet<StepInterval<float> > zrgs_;

    TypeSet<StepInterval<int> >	maxtrcrgs_;
    TypeSet<StepInterval<float> > maxzrgs_;

    void			finalised(CallBacker*);
    void			lineSel(CallBacker*);
    void			trcRgChanged(CallBacker*);
    void			zRgChanged(CallBacker*);

    virtual bool		acceptOK(CallBacker*);
};


uiSeis2DMultiLineSelDlg::uiSeis2DMultiLineSelDlg( uiParent* p,
					const TypeSet<Pos::GeomID>& geomids,
					const BufferStringSet& lnms,
					bool withz, bool withstep )
    : uiDialog( p, uiDialog::Setup("Select 2D Lines",mNoDlgTitle,
                                    mODHelpKey(mSeis2DMultiLineSelDlgHelpID) ) )
    , zrgfld_(0)
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, lnms, "Select Lines",
						  OD::ChooseAtLeastOne );
    lnmsfld_ = llb->box();
    lnmsfld_->selectionChanged.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,lineSel) );

    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			 Survey::GM().getGeometry(geomids[idx]) );
	StepInterval<int> trcrg( 0, 0, 1 );
	StepInterval<float> zrg = SI().zRange( true );
	if ( geom2d )
	{
	    trcrg = geom2d->data().trcNrRange();
	    zrg = geom2d->data().zRange();
	}

	maxtrcrgs_ += trcrg; trcrgs_ += trcrg;
	maxzrgs_ += zrg; zrgs_ += zrg;
    }

    trcrgfld_ = new uiSelNrRange( this, StepInterval<int>(), withstep,
				  "Trace" );
    trcrgfld_->rangeChanged.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,trcRgChanged) );
    trcrgfld_->attach( alignedBelow, llb );

    if ( withz )
    {
	zrgfld_ = new uiSelZRange( this, withstep,
			BufferString("Z range",SI().getZUnitString()) );
	zrgfld_->setRangeLimits( SI().zRange(false) );
	zrgfld_->rangeChanged.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,zRgChanged) );
	zrgfld_->attach( alignedBelow, trcrgfld_ );
    }

    postFinalise().notify( mCB(this,uiSeis2DMultiLineSelDlg,finalised) );
}


void uiSeis2DMultiLineSelDlg::finalised( CallBacker* )
{
    lineSel(0);
}


void uiSeis2DMultiLineSelDlg::setAll( bool yn )
{ lnmsfld_->chooseAll( yn ); }

void uiSeis2DMultiLineSelDlg::setSelection( const TypeSet<int>& selidxs,
				const TypeSet<StepInterval<int> >& trcrgs,
				const TypeSet<StepInterval<float> >& zrgs )
{
    if ( trcrgs.size() != selidxs.size() )
	return;

    const bool usezrgs = zrgs.size() == selidxs.size();
    lnmsfld_->chooseAll( false );
    if ( !selidxs.isEmpty() )
	lnmsfld_->setCurrentItem( selidxs[0] );

    for ( int idx=0; idx<selidxs.size(); idx++ )
    {
	lnmsfld_->setChosen( selidxs[idx] );
	trcrgs_[selidxs[idx]] = trcrgs[idx];
	if ( usezrgs )
	    zrgs_[selidxs[idx]] = zrgs[idx];
    }

    lineSel(0);
}


void uiSeis2DMultiLineSelDlg::getSelLines( TypeSet<int>& selidxs ) const
{
    selidxs.erase();
    lnmsfld_->getChosen( selidxs );
}


void uiSeis2DMultiLineSelDlg::getTrcRgs(TypeSet<StepInterval<int> >& rgs) const
{
    rgs.erase();
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( !lnmsfld_->isChosen(idx) )
	    continue;

	rgs += trcrgs_[idx];
    }
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
    zrgs.erase();
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( !lnmsfld_->isChosen(idx) )
	    continue;

	zrgs += zrgs_[idx];
    }
}


void uiSeis2DMultiLineSelDlg::lineSel( CallBacker* )
{
    const bool multisel = lnmsfld_->nrChosen() > 1;
    trcrgfld_->setSensitive( !multisel );
    if ( zrgfld_ ) zrgfld_->setSensitive( !multisel );
    if ( multisel ) return;

    NotifyStopper ns( trcrgfld_->rangeChanged );
    const int curitm = lnmsfld_->currentItem();
    if ( !trcrgs_.validIdx(curitm) )
	return;

    trcrgfld_->setLimitRange( maxtrcrgs_[curitm] );
    trcrgfld_->setRange( trcrgs_[curitm] );
    if ( !zrgfld_ || !zrgs_.validIdx(curitm) ) return;

    zrgfld_->setRangeLimits( maxzrgs_[curitm] );
    zrgfld_->setRange( zrgs_[curitm] );
}


void uiSeis2DMultiLineSelDlg::trcRgChanged( CallBacker* )
{
    const int curitm = lnmsfld_->currentItem();
    if ( curitm<0 ) return;

    trcrgs_[curitm] = trcrgfld_->getRange();
}


void uiSeis2DMultiLineSelDlg::zRgChanged( CallBacker* )
{
    const int curitm = lnmsfld_->currentItem();
    if ( curitm<0 ) return;

    zrgs_[curitm] = zrgfld_->getRange();
}


bool uiSeis2DMultiLineSelDlg::acceptOK( CallBacker* )
{
    if ( lnmsfld_->nrChosen() == 1 )
	trcRgChanged( 0 );
    return true;
}


uiSeis2DMultiLineSel::uiSeis2DMultiLineSel( uiParent* p, const char* text,
					    bool withz, bool withstep )
    : uiSeis2DLineSel(p,true)
    , isall_(false),withz_(withz),withstep_(withstep)
{
    if ( text && *text ) txtfld_->setTitleText( text );
}


uiSeis2DMultiLineSel::~uiSeis2DMultiLineSel()
{
}


void uiSeis2DMultiLineSel::selPush( CallBacker* )
{
    uiSeis2DMultiLineSelDlg dlg( this, geomids_, lnms_, withz_, withstep_ );
    if ( isall_ )
	dlg.setAll( true );
    else
	dlg.setSelection( selidxs_, trcrgs_, zrgs_ );

    if ( !dlg.go() )
	return;

    isall_ = dlg.isAll();
    dlg.getSelLines( selidxs_ );
    dlg.getZRgs( zrgs_ );
    dlg.getTrcRgs( trcrgs_ );
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
	linepar.set( sKey::GeomID(), geomids_[selidxs_[idx]] );
	linepar.set( sKey::TrcRange(), trcrgs_[idx] );
	if ( withz_ )
	    linepar.set( sKey::ZRange(), zrgs_[idx] );

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
    trcrgs_.erase();
    zrgs_.erase();
    PtrMan<IOPar> lspar = par.subselect( "Line" );
    if ( !lspar ) return;

    TypeSet<Pos::GeomID> selgeomids;
    for ( int idx=0; idx<1024; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar )
	    break;

	Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
	if ( !linepar->get(sKey::GeomID(),geomid) )
	{
	    FixedString lnm = linepar->find( sKey::Name() );
	    geomid = Survey::GM().getGeomID( lsetname, lnm );
	}

	if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	    break;

	selgeomids += geomid;
	StepInterval<int> trcrg;
	if ( linepar->get(sKey::TrcRange(),trcrg) )
	    continue;

	trcrgs_ += trcrg;
	if ( !withz_ )
	    continue;

	StepInterval<float> zrg;
	if ( linepar->get(sKey::ZRange(),zrg) )
	    zrgs_ += zrg;
    }

    setSelGeomIDs( selgeomids );
    isall_ = uiSeis2DLineSel::isAll();
}


bool uiSeis2DMultiLineSel::isAll() const
{ return isall_; }

const TypeSet<StepInterval<float> >& uiSeis2DMultiLineSel::getZRanges() const
{ return zrgs_; }

const TypeSet<StepInterval<int> >&  uiSeis2DMultiLineSel::getTrcRanges() const
{ return trcrgs_; }

void uiSeis2DMultiLineSel::setSelLines( const BufferStringSet& sellines )
{ isall_ = false; uiSeis2DLineSel::setSelLineNames( sellines ); }

void uiSeis2DMultiLineSel::setAll( bool yn )
{ uiSeis2DLineSel::setAll( yn ); isall_ = yn; }

void uiSeis2DMultiLineSel::setZRanges(
				const TypeSet<StepInterval<float> >& zrgs )
{ zrgs_ = zrgs; }

void uiSeis2DMultiLineSel::setTrcRanges(
				const TypeSet<StepInterval<int> >& trcrgs )
{ trcrgs_ = trcrgs; }

