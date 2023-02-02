/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimnemonicsel.h"

#include "uilabel.h"
#include "uilistbox.h"


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
		mNoDlgTitle,mTODOHelpKey) )
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
