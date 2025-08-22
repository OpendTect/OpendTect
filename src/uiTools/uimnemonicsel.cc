/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimnemonicsel.h"

#include "uibuttongroup.h"
#include "uicolor.h"
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
    : uiDialog(p,Setup(tr("Multi-Mnemonic Selection"),
		       mODHelpKey(mMultiMnemonicsSelHelpID)))
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
static const int sMnemonicColorCol = 3;

class uiMnSelFlds : public NamedCallBacker
{ mODTextTranslationClass(uiMnSelFlds);
public:

    enum class EditType   { StdType, Mnemonic, NameOnly, Color, None };

uiMnSelFlds( uiTable& tbl, int rowidx, const Mnemonic* mn )
    : NamedCallBacker(mn ? mn->name().str() : nullptr)
    , tbl_(tbl)
    , iscustom_(mn)
    , editstate_(mn ? EditType::None : EditType::StdType)
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

    const OD::Color color = mn ? mn->disp_.color_
			     : mnselfld_->mnemonic()->disp_.color_;
    NotifyStopper ns( tbl_.valueChanged );
    tbl_.setColor( RowCol(rowidx,sMnemonicColorCol), color );

    tbl_.hideRow( rowidx, false );
    tbl_.resizeColumnToContents( sStdTypeCol );
    tbl_.resizeColumnToContents( sTemplateNmCol );
}


~uiMnSelFlds()
{
    detachAllNotifiers();
    deepErase( mnsels_ );
}


bool isCustom() const
{
    return iscustom_;
}


Mnemonic* getMnemonic( Repos::Source src, uiRetVal& uirv ) const
{
    if ( name().isEmpty() )
    {
	uirv.add( tr("Custom Mnemonic name unpecified. Please provide a "
		     "unique and valid name for Custom Mnemonics") );
	return nullptr;
    }

    const Mnemonic& templatemn = getTemplateMn();
    const bool isudf = templatemn.isUdf();
    const bool istemplate = templatemn.isTemplate();
    if ( isudf || !istemplate )	
    {
	uirv.add( tr("Please choose a %1 source mnemonic")
		     .arg( isudf ? "valid" : "different" ) );
	return nullptr;
    }

    return Mnemonic::getFromTemplate( getTemplateMn(), name().str(), src );
}


const OD::Color getColor() const
{
    const int currrow = tbl_.getCell( typfld_ ).row();
    return tbl_.getColor( RowCol(currrow,sMnemonicColorCol) );
}


EditType editState() const
{
    return editstate_;
}


void colorChanged()
{
    const bool srcchanged = editstate_ == EditType::StdType ||
			    editstate_ == EditType::Mnemonic;
    if ( !srcchanged )
	editstate_ = EditType::Color;
}


void setName( const char* nm ) override
{
    NamedCallBacker::setName( nm );
    const bool srcchanged = editstate_ == EditType::StdType ||
			    editstate_ == EditType::Mnemonic;
    if ( !srcchanged )
	editstate_ = EditType::NameOnly;
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
    {
	editstate_ = EditType::StdType;
	return;
    }

    const uiMnemonicsSel::Setup uimnsu( &mnsel, uiString::empty() );
    mnselfld_ = new uiMnemonicsSel( nullptr, uimnsu );
    if ( lastusedmns_.get(curidx) )
	mnselfld_->setMnemonic( *lastusedmns_.get(curidx) );
    else
	lastusedmns_.replace( curidx, mnselfld_->mnemonic() );

    updateColor();
    mAttachCB( mnselfld_->box()->selectionChanged, uiMnSelFlds::mnChgCB );
    tbl_.setCellObject( rc, mnselfld_->box() );
    editstate_ = EditType::StdType;
}


void mnChgCB( CallBacker* )
{
    const int curidx = typfld_->currentItem();
    lastusedmns_.replace( curidx, mnselfld_->mnemonic() );
    updateColor();
    editstate_ = EditType::Mnemonic;
}


void updateColor()
{
    const int currrow = tbl_.getCell( typfld_ ).row();
    const BufferString newname = tbl_.text( RowCol(currrow,sMnemonicNmCol) );
    if ( newname.isEmpty() && mnselfld_ )
    {
	tbl_.setColor( RowCol(currrow,sMnemonicColorCol),
		       mnselfld_->mnemonic()->disp_.color_ );
	colorChanged();
    }
}


const Mnemonic& getTemplateMn() const
{
    const Mnemonic* templatemn = mnselfld_ ? mnselfld_->mnemonic() : nullptr;
    return templatemn ? *templatemn : Mnemonic::undef();
}

   uiTable&			    tbl_;
   uiComboBox*			    typfld_;
   uiMnemonicsSel*		    mnselfld_;

   ObjectSet<MnemonicSelection>     mnsels_;
   MnemonicSelection		    lastusedmns_;
   EditType			    editstate_;

   const bool			    iscustom_;
};


// uiCustomMnemonicsSel

uiCustomMnemonicsSel::uiCustomMnemonicsSel( uiParent* p )
    : uiDialog(p,Setup(tr("Project specific mnemonics"),mTODOHelpKey))
    , originalcustommns_(nullptr)
{
    setOkText( uiStrings::sSave() );

    for ( int imn=originalcustommns_.size()-1; imn>=0; imn-- )
    {
	if ( originalcustommns_.get(imn)->isTemplate() )
	    originalcustommns_.removeSingle( imn );
    }

    createTable();

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


uiCustomMnemonicsSel::~uiCustomMnemonicsSel()
{
    detachAllNotifiers();
    deepErase( selflds_ );
}


void uiCustomMnemonicsSel::createTable()
{
    uiTable::Setup tblsu( sDefNrRows, sMnemonicColorCol+1 );
    tblsu.rowgrow( true ).fillcol( true ).selmode( uiTable::Multi );
    tbl_ = new uiTable( this, tblsu, "Custom Mnemonics Table" );
    tbl_->setPrefWidth( 600 );
    tbl_->setSelectionBehavior( uiTable::SelectRows );
    tbl_->setLeftHeaderHidden( true );
    tbl_->setColumnReadOnly( sStdTypeCol, true );
    tbl_->setColumnReadOnly( sTemplateNmCol, true );
    tbl_->setColumnReadOnly( sMnemonicColorCol, true );
    tbl_->setColumnStretchable( sStdTypeCol, false );
    tbl_->setColumnStretchable( sTemplateNmCol, false );
    tbl_->setColumnStretchable( sMnemonicColorCol, true );
    tbl_->setColumnLabel( sStdTypeCol, uiStrings::sProperty() );
    tbl_->setColumnLabel( sTemplateNmCol,
			  tr("Existing %1").arg(uiStrings::sMnemonic()) );
    tbl_->setColumnLabel( sMnemonicNmCol,
			  tr("Custom %1").arg(uiStrings::sMnemonic()) );
    tbl_->setColumnLabel( sMnemonicColorCol,
			  tr("Custom %1 %2").arg(uiStrings::sMnemonic())
					   .arg(uiStrings::sColor()) );
    tbl_->showGrid( false );
    tbl_->setTableReadOnly( true );
    mAttachCB( tbl_->valueChanged, uiCustomMnemonicsSel::cellEditCB );
    mAttachCB( tbl_->doubleClicked, uiCustomMnemonicsSel::changeColCB );
    mAttachCB( tbl_->rowInserted, uiCustomMnemonicsSel::addRowTblRowCB );
    mAttachCB( tbl_->rowDeleted, uiCustomMnemonicsSel::removeTblRowCB );
    mAttachCB( tbl_->selectionDeleted, uiCustomMnemonicsSel::removeTblSelCB );
}


void uiCustomMnemonicsSel::initDlg( CallBacker* )
{
    tbl_->setColumnWidthInChar( sStdTypeCol, 20 );
    tbl_->setColumnWidthInChar( sTemplateNmCol, 24 );
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
	tbl_->hideRow( irow, true );

    if ( !originalcustommns_.isEmpty() )
	doRead();
}


void uiCustomMnemonicsSel::addRowButCB( CallBacker* )
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


void uiCustomMnemonicsSel::addRowTblRowCB( CallBacker* )
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


void uiCustomMnemonicsSel::removeButCB( CallBacker* )
{
     TypeSet<int> selectedrows;
     tbl_->getSelectedRows( selectedrows );
     if ( selectedrows.isEmpty() )
	 return;

     tbl_->removeRows( selectedrows );
     removeEntries( selectedrows );
}


void uiCustomMnemonicsSel::removeTblRowCB( CallBacker* )
{
    const TypeSet<int>& selectedrows = tbl_->getNotifRCs();
    if ( selectedrows.isEmpty() )
	return;

    removeEntries( selectedrows );
}


void uiCustomMnemonicsSel::removeTblSelCB( CallBacker* cb )
{
    removeTblRowCB( cb );
}


void uiCustomMnemonicsSel::removeEntries( const TypeSet<int>& rows,
					  bool forreset )
{
    for ( int irow=rows.size()-1; irow>=0; irow-- )
    {
	const int idx = rows[irow];
	if ( selflds_.validIdx(idx) )
	{
	    const auto* selfld = selflds_[idx];
	    if ( selfld->isCustom() && !forreset )
		origentriesremoved_.addIfNew(
					originalcustommns_.get(rows[irow]) );

	    delete selflds_.removeSingle( rows[irow] );
	}
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


void uiCustomMnemonicsSel::changeColCB( CallBacker* )
{
    const RowCol rc = tbl_->notifiedCell();
    const int currrow = rc.row();
    if ( rc.col() != sMnemonicColorCol )
	return;

    const OD::Color oldcol = tbl_->getColor( rc );
    OD::Color newcol = oldcol;
    if ( selectColor(newcol, this, tr("Marker color")) )
    {
	if ( newcol != oldcol )
	{
	    tbl_->setColor( rc, newcol );
	    selflds_[rc.row()]->colorChanged();
	    if ( originalcustommns_.validIdx(currrow) )
		newcolors_ += new std::pair<int,const OD::Color>( currrow,
								  newcol );
	}
    }
}


void uiCustomMnemonicsSel::cellEditCB( CallBacker* )
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
    {
	selfld->setName( mnnm.buf() );
	if ( originalcustommns_.validIdx(curidx) )
	    newnames_ += new std::pair<int,const BufferString>(curidx,mnnm);
    }
}


void uiCustomMnemonicsSel::saveButCB( CallBacker* )
{
    ManagedObjectSet<Mnemonic> custommns;
    if ( !getEntries(custommns) )
	return;

    const bool srcchgd = !custommns.isEmpty() || !origentriesremoved_.isEmpty();
    const bool nochange = !srcchgd && newnames_.isEmpty()
				   && newcolors_.isEmpty();
    if ( nochange )
	return;

    if ( srcchgd )
	commitEntries( custommns );

    if ( !newnames_.isEmpty() )
	commitNameChanges();

    if ( !newcolors_.isEmpty() )
	commitColorChanges();

    doSave();
    resetTable();
    origentriesremoved_.setEmpty();
}


void uiCustomMnemonicsSel::resetTable()
{
    TypeSet<int> rows;
    for ( int row=0; row<tbl_->nrRows(); row++ )
	rows += row;

    tbl_->removeRows( rows );
    removeEntries( rows, true );
    doRead();
}


void uiCustomMnemonicsSel::doRead()
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


bool uiCustomMnemonicsSel::getEntries( ObjectSet<Mnemonic>& mns ) const
{
    uiRetVal uirv;
    for ( const auto* selfld : selflds_ )
    {
	const bool srcchanged
		    = selfld->editState() == uiMnSelFlds::EditType::StdType ||
		      selfld->editState() == uiMnSelFlds::EditType::Mnemonic;
	if ( srcchanged )
	{
	    Mnemonic* mn = selfld->getMnemonic( Repos::Survey, uirv );
	    if ( !mn )
	    {
		uiString msg = tr( "Error: Cannot create new custom "
				   "mnemonics." );
		msg.addNewLine().addNewLine().append(uirv);
		uiMSG().error( msg );
		return false;
	    }

	    const OD::Color newcol = selfld->getColor();
	    if ( newcol != mn->disp_.color_ )
		mn->disp_.color_ = newcol;

	    mns.add( mn );
	}
    }

    return true;
}


bool uiCustomMnemonicsSel::commitEntries( ObjectSet<Mnemonic>& mns )
{
    MnemonicSet& eMNC = getNonConst( MNC() );
    for ( int idx=eMNC.size()-1; idx>=0; idx-- )
    {
	const Mnemonic* mn = eMNC.get(idx);
	if ( mn->isTemplate() )
	    continue;

	bool isinset = false;
	for ( const auto* custommn : mns )
	{
	    if ( mn->name() == custommn->name() )
	    {
		isinset = true;
		break;
	    }
	}

	const bool doremove = isinset || origentriesremoved_.isPresent( mn );
	if ( doremove )
	{
	    const int origidx = originalcustommns_.indexOf( mn );
	    originalcustommns_.removeSingle( origidx );
	    eMNC.removeSingle( idx );
	}
    }

    for ( const auto* mn : mns )
    {
	const int sz = eMNC.size();
	eMNC.add( new Mnemonic(*mn) );
	if ( eMNC.size() <= sz )
	    pErrMsg("Failed to add a custom mnemonic");
	else
	    originalcustommns_.add( eMNC.last() );
    }

    return true;
}


void uiCustomMnemonicsSel::commitNameChanges()
{
    if ( newnames_.isEmpty() )
	return;

    for ( const auto* idxnmpair : newnames_ )
    {
	const int idx = idxnmpair->first;
	const BufferString& newnm = idxnmpair->second;
	if ( !originalcustommns_.validIdx(idx) || newnm.isEmpty() )
	    continue;

	const Mnemonic* mn = originalcustommns_.get( idx );
	const_cast<Mnemonic*>( mn )->setName( newnm );
    }
}


void uiCustomMnemonicsSel::commitColorChanges()
{
    if ( newcolors_.isEmpty() )
	return;

    for ( const auto* idxcolpair : newcolors_ )
    {
	const int idx = idxcolpair->first;
	const OD::Color newcol = idxcolpair->second;
	if ( !originalcustommns_.validIdx(idx) )
	    continue;

	const Mnemonic* mn = originalcustommns_.get( idx );
	const_cast<Mnemonic*>( mn )->disp_.color_ = newcol;
    }
}


bool uiCustomMnemonicsSel::doSave()
{
    IOPar& sidefs = SI().getPars();
    if ( originalcustommns_.isEmpty() &&
	 !sidefs.hasKey(IOPar::compKey(sKey::Mnemonics(),0)) )
	return true;

    sidefs.removeSubSelection( sKey::Mnemonics() );
    IOPar iop;
    for ( int imn=0; imn<originalcustommns_.size(); imn++ )
    {
	const Mnemonic& mn = *originalcustommns_.get( imn );
	FileMultiString fms;
	fms.add( mn.name().str() )
	   .add( mn.getOrigin()->name().str() )
	   .add( sCast(int,mn.disp_.color_.r()) )
	   .add( sCast(int,mn.disp_.color_.g()) )
	   .add( sCast(int,mn.disp_.color_.b()) );
	iop.set( toString(imn), fms.str()  );
    }

    sidefs.mergeComp( iop, sKey::Mnemonics() );
    SI().savePars();
    return true;
}


bool uiCustomMnemonicsSel::acceptOK( CallBacker* )
{
    ManagedObjectSet<Mnemonic> custommns;
    if ( !getEntries(custommns) )
	return false;

    const bool srcchgd = !custommns.isEmpty() || !origentriesremoved_.isEmpty();
    const bool nochange = !srcchgd && newnames_.isEmpty()
				   && newcolors_.isEmpty();
    if ( nochange )
	return true;

    if ( srcchgd )
	commitEntries( custommns );

    if ( !newnames_.isEmpty() )
	commitNameChanges();

    if ( !newcolors_.isEmpty() )
	commitColorChanges();

    doSave();
    origentriesremoved_.setEmpty();
    return true;
}
