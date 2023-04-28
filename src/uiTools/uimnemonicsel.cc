/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimnemonicsel.h"

#include "uibuttongroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "separstr.h"
#include "survinfo.h"


uiMnemonicsSel::uiMnemonicsSel( uiParent* p, const Setup& set )
    : uiLabeledComboBox(p,set.lbltxt_,"Mnemonic")
    , setup_(set)
    , mns_(set.mnsel_)
{
    init();
}


uiMnemonicsSel::uiMnemonicsSel( uiParent* p, Mnemonic::StdType typ )
    : uiLabeledComboBox(p,Setup::defLabel(),"Mnemonic")
    , mns_(typ)
{
    init();
}


uiMnemonicsSel::uiMnemonicsSel( uiParent* p, const MnemonicSelection* mns )
    : uiLabeledComboBox(p,Setup::defLabel(),"Mnemonic")
    , mns_(nullptr)
{
    if ( mns )
	mns_ = *mns;

    init();
}


uiMnemonicsSel::~uiMnemonicsSel()
{
}


uiMnemonicsSel* uiMnemonicsSel::clone() const
{
    Setup set( setup_ );
    set.mnsel( mns_ );
    auto* ret = new uiMnemonicsSel( const_cast<uiParent*>( parent() ), setup_ );
    if ( !altnms_.isEmpty() )
	ret->setNames( altnms_ );

    return ret;
}


void uiMnemonicsSel::init()
{
    setFromSelection();
}


void uiMnemonicsSel::setFromSelection()
{
    BufferStringSet mnsnames;
    for ( const auto* mn : mns_ )
	mnsnames.add( mn->name() );

    mnsnames.sort();
    cb_->addItems( mnsnames );
    if ( !mnsnames.isEmpty() )
	cb_->setCurrentItem( 0 );
}


const Mnemonic* uiMnemonicsSel::mnemonic() const
{
    const BufferString curnm( cb_->text() );
    if ( altnms_.isEmpty() )
	return mns_.getByName( curnm, false );

    const int idx = altnms_.indexOf( curnm );
    return mns_.validIdx( idx ) ? mns_.get( idx ) : nullptr;
}


Mnemonic::StdType uiMnemonicsSel::propType() const
{
    const Mnemonic* mn = mnemonic();
    return mn ? mn->stdType() : Mnemonic::Other;
}


void uiMnemonicsSel::setNames( const BufferStringSet& nms )
{
    if ( !nms.isEmpty() && nms.size() != cb_->size() )
	return;

    const int curitmidx = cb_->currentItem();

    altnms_ = nms;
    if ( altnms_.isEmpty() )
	setFromSelection();
    else
    {
	cb_->setEmpty();
	cb_->addItems( altnms_ );
	cb_->setCurrentItem( 0 );
    }

    if ( curitmidx >= 0 && curitmidx < cb_->size() )
	cb_->setCurrentItem( curitmidx );
}


void uiMnemonicsSel::setMnemonic( const Mnemonic& mn )
{
    if ( !mns_.isPresent(&mn) )
	return;

    const Mnemonic* curmn = mnemonic();
    if ( curmn && curmn == &mn )
	return;

    cb_->setCurrentItem( mn.name().buf() );
}


// -- uiMulitMnemonicSel --

uiMultiMnemonicsSel::uiMultiMnemonicsSel( uiParent* p,
					  MnemonicSelection& mns,
					  const MnemonicSelection* mnsel )
    : uiDialog( p, uiDialog::Setup(tr("Multi-Mnemonic Selection"),
		mNoDlgTitle,mODHelpKey(mMultiMnemonicsSelHelpID)) )
    , mns_(mns)
{
    mnemlist_ = new uiListBox( this, "mnemonics", OD::ChooseZeroOrMore );
    BufferStringSet mnemnms;
    if ( mnsel )
    {
	for ( const auto* mn : *mnsel )
	    mnemnms.addIfNew( mn->name() );
    }
    else
	MNC().getNames( mnemnms );

    mnemnms.sort();
    mnemlist_->addItems( mnemnms );
    int maxsize = mnemlist_->size();
    if ( maxsize > 15 )
	maxsize = 15;

    mnemlist_->setNrLines( maxsize );
    mnemlist_->setHSzPol( uiObject::Wide );
}


uiMultiMnemonicsSel::~uiMultiMnemonicsSel()
{}


bool uiMultiMnemonicsSel::acceptOK( CallBacker* )
{
    BufferStringSet selmnems;
    mnemlist_->getChosen( selmnems );
    for ( const auto* mnnm : selmnems )
    {
	const Mnemonic* mn = MNC().getByName( *mnnm, false );
	mns_.add( mn );
    }

    return true;
}


// uiMnSelFlds

static const int sDefNrRows = 5;
static const int sStdTypeCol = 0;
static const int sTemplateNmCol = 1;
static const int sMnemonicNmCol = 2;

class uiMnSelFlds : public NamedCallBacker
{
public:

uiMnSelFlds( uiTable& tbl, int rowidx, const Mnemonic* mn )
    : NamedCallBacker(mn ? mn->name().str() : nullptr)
    , tbl_(tbl)
{
    lastusedmns_.setNullAllowed();
    const BufferStringSet alltypnms( Mnemonic::StdTypeNames() );
    BufferStringSet typnms;
    for ( const auto* typnm : alltypnms )
    {
	const Mnemonic::StdType typ =
				Mnemonic::parseEnumStdType( typnm->buf() );
	const bool isall = typ == Mnemonic::Other;
	MnemonicSelection mnsel( typ );
	if ( mnsel.isEmpty() )
	    continue;

	if ( isall )
	    mnsel = MnemonicSelection( nullptr );

	for ( int idx=mnsel.size()-1; idx>=0; idx-- )
	{
	    if ( !mnsel.get(idx)->isTemplate() )
		mnsel.removeSingle( idx );
	}

	typnms.add( typnm->buf() );
	mnsels_.add( new MnemonicSelection(mnsel) );
	lastusedmns_.add( nullptr );
    }

    typfld_ = new uiComboBox( nullptr, typnms, "Property Type" );
    typfld_->setHSzPol( uiObject::SmallVar );
    const Mnemonic::StdType typ = mn ? mn->stdType() : Mnemonic::Imp;
    const int firstitm = typnms.indexOf( Mnemonic::toString(typ) );
    typfld_->setCurrentItem( firstitm );
    tbl_.setCellObject( RowCol(rowidx,sStdTypeCol), typfld_ );
    mAttachCB( typfld_->selectionChanged, uiMnSelFlds::typeChgCB );

    const MnemonicSelection& mnsel = *mnsels_.get( firstitm );
    const uiMnemonicsSel::Setup uimnsu( &mnsel, uiString::empty() );
    const Mnemonic* defmn = mn ? mn->getOrigin() : mnsel.getByName("EI",false);
    mnselfld_ = new uiMnemonicsSel( nullptr, uimnsu );
    if ( defmn )
	mnselfld_->setMnemonic( *defmn );

    lastusedmns_.replace( firstitm, mnselfld_->mnemonic() );
    mAttachCB( mnselfld_->box()->selectionChanged, uiMnSelFlds::mnChgCB );
    tbl_.setCellObject( RowCol(rowidx,sTemplateNmCol), mnselfld_->box() );

    if ( !name().isEmpty() )
    {
	NotifyStopper ns( tbl_.valueChanged );
	tbl_.setText( RowCol(rowidx,sMnemonicNmCol), name().str() );
    }

    tbl_.hideRow( rowidx, false );
    tbl_.resizeColumnToContents( sStdTypeCol );
    tbl_.resizeColumnToContents( sTemplateNmCol );
}


~uiMnSelFlds()
{
    detachAllNotifiers();
    deepErase( mnsels_ );
}


void setFrom( const Mnemonic& mn )
{
    if ( mn.isTemplate() )
	return;

    const Mnemonic::StdType typ = Mnemonic::parseEnumStdType( typfld_->text() );
    if ( mn.stdType() != typ )
    {
	typfld_->setCurrentItem( Mnemonic::toString(mn.stdType()) );
	typeChgCB( nullptr );
    }

    const int curidx = typfld_->currentItem();
    const MnemonicSelection& mnsel = *mnsels_.get( curidx );
    if ( mnselfld_ && mnsel.size() > 1 && mn.getOrigin() &&
	 mn.getOrigin() != mnselfld_->mnemonic() )
    {
	mnselfld_->setMnemonic( *mn.getOrigin() );
	mnChgCB( nullptr );
    }

    setName( mn.name() );
    const RowCol typrc = tbl_.getCell( typfld_ );
    NotifyStopper ns( tbl_.valueChanged );
    tbl_.setText( RowCol(typrc.row(),sMnemonicNmCol), mn.name().str() );
}


Mnemonic* getMnemonic( Repos::Source src ) const
{
    return name().isEmpty() ? nullptr
	    : Mnemonic::getFromTemplate( getTemplateMn(), name().str(), src );
}


private:

void typeChgCB( CallBacker* )
{
    const int curidx = typfld_->currentItem();
    const RowCol typrc = tbl_.getCell( typfld_ );
    const RowCol rc( typrc.row(), typrc.col()+1 );
    if ( mnselfld_ )
    {
	mDetachCB( mnselfld_->box()->selectionChanged, uiMnSelFlds::mnChgCB );
	tbl_.clearCellObject( rc );
	mnselfld_ = nullptr;
    }

    const bool isother = curidx == typfld_->size()-1;
    const MnemonicSelection& mnsel = *mnsels_.get( curidx );
    tbl_.setCellReadOnly( rc, isother || mnsel.size() < 2 );
    if ( isother )
	return;

    const uiMnemonicsSel::Setup uimnsu( &mnsel, uiString::empty() );
    mnselfld_ = new uiMnemonicsSel( nullptr, uimnsu );
    if ( lastusedmns_.get(curidx) )
	mnselfld_->setMnemonic( *lastusedmns_.get(curidx) );
    else
	lastusedmns_.replace( curidx, mnselfld_->mnemonic() );

    mAttachCB( mnselfld_->box()->selectionChanged, uiMnSelFlds::mnChgCB );
    tbl_.setCellObject( rc, mnselfld_->box() );
}


void mnChgCB( CallBacker* )
{
    const int curidx = typfld_->currentItem();
    lastusedmns_.replace( curidx, mnselfld_->mnemonic() );
}


const Mnemonic& getTemplateMn() const
{
    const Mnemonic* templatemn = mnselfld_ ? mnselfld_->mnemonic() : nullptr;
    return templatemn ? *templatemn : Mnemonic::undef();
}

   uiTable&	tbl_;
   uiComboBox*	typfld_;
   uiMnemonicsSel*	mnselfld_;
   ObjectSet<MnemonicSelection> mnsels_;
   MnemonicSelection	lastusedmns_;

};


// uiCustomMnemonicsSel

class uiCustomMnemonicsSel : public uiDialog
{ mODTextTranslationClass(uiCustomMnemonicsSel)
public:

uiCustomMnemonicsSel( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Project specific mnemonics"),
				 mNoDlgTitle,mTODOHelpKey))
    , originalcustommns_(nullptr)
{
    setOkText( uiStrings::sSave() );

    for ( int imn=originalcustommns_.size()-1; imn>=0; imn-- )
	if ( originalcustommns_.get(imn)->isTemplate() )
	    originalcustommns_.removeSingle( imn );

    uiTable::Setup tblsu( sDefNrRows, sMnemonicNmCol+1 );
    tblsu.rowgrow( true ).fillcol( true ).selmode( uiTable::Multi );

    tbl_ = new uiTable( this, tblsu, "Custom Mnemonics Table" );
    tbl_->setPrefWidth( 600 );
    tbl_->setSelectionBehavior( uiTable::SelectRows );
    tbl_->setLeftHeaderHidden( true );
    tbl_->setColumnReadOnly( sStdTypeCol, true );
    tbl_->setColumnReadOnly( sTemplateNmCol, true );
    tbl_->setColumnStretchable( sStdTypeCol, false );
    tbl_->setColumnStretchable( sTemplateNmCol, false );
    tbl_->setColumnLabel( sStdTypeCol, uiStrings::sProperty() );
    tbl_->setColumnLabel( sTemplateNmCol,
			  tr("Existing %1").arg(uiStrings::sMnemonic()) );
    tbl_->setColumnLabel( sMnemonicNmCol,
			  tr("Extra %1").arg(uiStrings::sMnemonic()) );
    tbl_->showGrid( false );
    tbl_->setTableReadOnly( true );
    mAttachCB( tbl_->valueChanged, uiCustomMnemonicsSel::cellEditCB );
    mAttachCB( tbl_->rowInserted, uiCustomMnemonicsSel::addRowTblRowCB );
    mAttachCB( tbl_->rowDeleted, uiCustomMnemonicsSel::removeTblRowCB );
    mAttachCB( tbl_->selectionDeleted, uiCustomMnemonicsSel::removeTblSelCB );

    bgrp_ = new uiButtonGroup( this, "Operations buttons", OD::Vertical );
    new uiToolButton( bgrp_, "add", uiStrings::phrAdd(uiStrings::sRow()),
		      mCB(this,uiCustomMnemonicsSel,addRowButCB) );
    new uiToolButton( bgrp_, "remove", uiStrings::sRemoveSelected(),
		      mCB(this,uiCustomMnemonicsSel,removeButCB) );
    new uiToolButton( bgrp_, "save", uiStrings::sSave(),
		      mCB(this,uiCustomMnemonicsSel,saveButCB) );
    bgrp_->attach( rightOf, tbl_ );

    mAttachCB( postFinalize(), uiCustomMnemonicsSel::initDlg );
}


~uiCustomMnemonicsSel()
{
    detachAllNotifiers();
    deepErase( selflds_ );
}


private:

void initDlg( CallBacker* )
{
    tbl_->setColumnWidthInChar( sStdTypeCol, 20 );
    tbl_->setColumnWidthInChar( sTemplateNmCol, 24 );
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
	tbl_->hideRow( irow, true );

    if ( !originalcustommns_.isEmpty() )
	doRead();
}


void addRowButCB( CallBacker* )
{
    const int nrentries = selflds_.size();
    if ( tbl_->isTableReadOnly() )
	tbl_->setTableReadOnly( false );

    if ( nrentries >= tbl_->nrRows() )
    {
	const int newrow = tbl_->nrRows();
	tbl_->insertRows( newrow, 1 );
	tbl_->hideRow( newrow, false );
    }

    auto* selfld = new uiMnSelFlds( *tbl_, nrentries, nullptr );
    selflds_.add( selfld );
}


void addRowTblRowCB( CallBacker* )
{
    if ( tbl_->isTableReadOnly() )
	tbl_->setTableReadOnly( false );

    const TypeSet<int>& notifrows = tbl_->getNotifRCs();
    for ( const auto& newrow : notifrows )
    {
	auto* selfld = new uiMnSelFlds( *tbl_, newrow, nullptr );
	if ( selflds_.validIdx(newrow) )
	    selflds_.insertAt( selfld, newrow );
	else
	    selflds_.add( selfld );
    }
}


void removeButCB( CallBacker* )
{
     TypeSet<int> selectedrows;
     tbl_->getSelectedRows( selectedrows );
     if ( selectedrows.isEmpty() )
	 return;

     tbl_->removeRows( selectedrows );
     removeEntries( selectedrows );
}


void removeTblRowCB( CallBacker* )
{
    const TypeSet<int>& selectedrows = tbl_->getNotifRCs();
    if ( selectedrows.isEmpty() )
	return;

    removeEntries( selectedrows );
}


void removeTblSelCB( CallBacker* cb )
{
    removeTblRowCB( cb );
}


void removeEntries( const TypeSet<int>& rows )
{
    for ( int irow=rows.size()-1; irow>=0; irow-- )
    {
	if ( selflds_.validIdx(rows[irow]) )
	    delete selflds_.removeSingle( rows[irow] );
    }

    const int nrrows = tbl_->nrRows();
    if ( nrrows < sDefNrRows )
    {
	tbl_->insertRows( nrrows, sDefNrRows-nrrows );
	for ( int irow=nrrows; irow<tbl_->nrRows(); irow++ )
	    tbl_->hideRow( irow, true );

	tbl_->removeAllSelections();
    }
}


void cellEditCB( CallBacker* )
{
    const RowCol& rc = tbl_->notifiedCell();
    if ( rc.col() != sMnemonicNmCol )
	return;

    const int curidx = rc.row();
    uiMnSelFlds* selfld = selflds_.validIdx(curidx) ? selflds_.get( curidx )
						    : nullptr;
    BufferStringSet newnms;
    for ( const auto* othselfld : selflds_ )
    {
	if ( othselfld == selfld )
	    continue;

	newnms.add( othselfld->name().buf() );
    }

    const BufferString mnnm( tbl_->text(rc) );
    if ( MNC().getByName(mnnm.buf(),false) || newnms.isPresent(mnnm.buf()) )
    {
	NotifyStopper ns( tbl_->valueChanged );
	if ( selfld && !selfld->name().isEmpty() )
	    tbl_->setText( rc, selfld->name().buf() );
	else
	    tbl_->clearCell( rc );

	if ( !dontshowmgr_ )
	{
	    dontshowmgr_ = uiMSG().error(
				tr("Mnemonic name '%1' already exists.\n")
						.arg( mnnm.buf() ),
					    tr("Please specify another name."),
					    uiString::empty(), true );
	}
    }
    else if ( selfld )
	selfld->setName( mnnm.buf() );
}


void saveButCB( CallBacker* )
{
    ManagedObjectSet<Mnemonic> custommns;
    getEntries( custommns, false );
    commitEntries( custommns, false );
    doSave( custommns, false );
}


void doRead()
{
    tbl_->setTableReadOnly( false );
    const int nrentries = originalcustommns_.size();
    if ( nrentries >= tbl_->nrRows() )
    {
	const int newrow = tbl_->nrRows();
	tbl_->insertRows( newrow, nrentries-tbl_->nrRows() );
	for ( int irow=newrow; irow<tbl_->nrRows(); irow++ )
	    tbl_->hideRow( irow, false );
    }

    for ( int irow=0; irow<originalcustommns_.size(); irow++ )
    {
	auto* selfld = new uiMnSelFlds( *tbl_, irow,
					originalcustommns_.get(irow) );
	selflds_.add( selfld );
    }
}


void getEntries( ObjectSet<Mnemonic>& mns, bool forrestore ) const
{
    if ( forrestore )
    {
	for ( const auto* mn : originalcustommns_ )
	    mns.add( new Mnemonic(*mn) );
    }
    else
    {
	for ( const auto* selfld : selflds_ )
	    mns.add( selfld->getMnemonic( Repos::Survey ) );
    }
}


bool commitEntries( ObjectSet<Mnemonic>& mns, bool forrestore )
{
    MnemonicSet& eMNC = mNonConst( MNC() );
    for ( int idx=eMNC.size()-1; idx>=0; idx-- )
    {
	if ( !eMNC.get(idx)->isTemplate() )
	    eMNC.removeSingleWithCache( idx );
    }
    if ( forrestore )
	originalcustommns_.setEmpty();

    for ( const auto* mn : mns )
    {
	const int sz = eMNC.size();
	eMNC.add( new Mnemonic(*mn) );
	if ( eMNC.size() <= sz )
	    { pErrMsg("Failed to add a custom mnemonic"); }
	else
	    originalcustommns_.add( eMNC.last() );
    }

    return true;
}


bool doSave( ObjectSet<Mnemonic>& custommns, bool restore )
{
    IOPar& sidefs = SI().getPars();
    if ( originalcustommns_.isEmpty() && custommns.isEmpty() &&
	 !sidefs.hasKey(IOPar::compKey(sKey::Mnemonics(),0)) )
	return true;

    sidefs.removeSubSelection( sKey::Mnemonics() );
    IOPar iop;
    for ( int imn=0; imn<custommns.size(); imn++ )
    {
	const Mnemonic& mn = *custommns.get( imn );
	FileMultiString fms;
	fms.add( mn.name().str() ).add( mn.getOrigin()->name().str() );
	iop.set( toString(imn), fms.str()  );
    }
    sidefs.mergeComp( iop, sKey::Mnemonics() );

    SI().savePars();
    return true;
}


bool rejectOK( CallBacker* )
{
    ManagedObjectSet<Mnemonic> custommns;
    getEntries( custommns, true );
    return commitEntries( custommns, true ) && doSave( custommns, true );
}


bool acceptOK( CallBacker* )
{
    ManagedObjectSet<Mnemonic> custommns;
    getEntries( custommns, false );
    return commitEntries( custommns, false ) && doSave( custommns, false );
}

    ObjectSet<uiMnSelFlds>	selflds_;
    uiTable*			tbl_;
    uiButtonGroup*		bgrp_;

    bool			dontshowmgr_ = false;
    MnemonicSelection		originalcustommns_;

};


bool doCustomMnemonicEditDlg( uiParent* p )
{
    uiCustomMnemonicsSel dlg( p );
    const bool ret = dlg.go();
    return ret;
}
