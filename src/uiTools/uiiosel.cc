/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          start of 2001
________________________________________________________________________

-*/

#include "uiiosel.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uifiledlg.h"
#include "iopar.h"
#include "file.h"
#include "keystrs.h"


uiIOSelect::uiIOSelect( uiParent* p, const Setup& su, const CallBack& butcb )
	: uiGroup(p)
	, doselcb_(butcb)
	, selectionDone(this)
	, optionalChecked(this)
	, keepmytxt_(su.keepmytxt_)
	, haveempty_(su.withclear_)
	, optbox_(0)
	, selbut_(0)
	, lbl_(0)
{
    inp_ = new uiComboBox( this, toString(su.seltxt_) );
    inp_->setEditable( true );
    inp_->setHSzPol( su.szpol_ );
    inp_->selectionChanged.notify( mCB(this,uiIOSelect,selDone) );
    setHAlignObj( inp_ );
    inp_->setSensitive( su.optionsselectable_ );

    if ( su.optional_ )
    {
	optbox_ = new uiCheckBox( this, su.seltxt_ );
	optbox_->activated.notify( mCB(this,uiIOSelect,optCheck) );
    }
    else if ( !su.compact_ )
    {
	lbl_ = new uiLabel( this, su.seltxt_ );
	lbl_->setAlignment( OD::Alignment::Right );
	lbl_->setSensitive( su.optionsselectable_ );
    }

    const CallBack selcb( mCB(this,uiIOSelect,doSel) );
    if ( !su.compact_ )
    {
	if ( su.buttontxt_ == uiStrings::sSelect() )
	    selbut_ = uiButton::getStd( this, OD::Select, selcb, false );
	else if ( !su.buttontxt_.isEmpty() )
	    selbut_ = new uiPushButton( this, su.buttontxt_, selcb, false );

	selbut_->setSensitive( su.optionsselectable_ );
    }
    if ( selbut_ )
    {
	BufferString butnm( toString(su.buttontxt_), " " );
	butnm += toString( su.seltxt_ );
	selbut_->setName( butnm.buf() );
	selbut_->attach( rightTo, inp_ );
    }

    setHAlignObj( inp_ );
    setHCenterObj( inp_ );
    preFinalise().notify( mCB(this,uiIOSelect,doFinalise) );
    postFinalise().notify( mCB(this,uiIOSelect,optCheck) );
}


uiIOSelect::~uiIOSelect()
{
}


void uiIOSelect::setHSzPol( uiObject::SzPolicy pol )
{
    inp_->setHSzPol( pol );
}


void uiIOSelect::addButton( uiButton* but, bool ins )
{
    if ( but )
	(ins ? insbuts_ : extbuts_) += but;
}


void uiIOSelect::doFinalise( CallBacker* cb )
{
    uiObject* rightmost = inp_;
    if ( selbut_ )
    {
	selbut_->attach( rightOf, inp_ );
	rightmost = selbut_;
    }
    for ( int idx=0; idx<extbuts_.size(); idx++ )
    {
	extbuts_[idx]->attach( rightOf, rightmost );
	rightmost = extbuts_[idx];
    }

    uiObject* leftmost = inp_;
    for ( int idx=0; idx<insbuts_.size(); idx++ )
    {
	insbuts_[idx]->attach( leftOf, leftmost );
	leftmost = insbuts_[idx];
    }
    if ( lbl_ )
    {
	lbl_->attach( leftOf, leftmost );
	leftmost = lbl_;
    }
    if ( optbox_ )
	optbox_->attach( leftOf, leftmost );
}


uiObject* uiIOSelect::endObj( bool left )
{
    if ( !left )
    {
	if ( !extbuts_.isEmpty() )
	    return extbuts_.last();
	return selbut_ ? (uiObject*)selbut_ : (uiObject*)inp_;
    }

    if ( optbox_ )
	return optbox_;
    else if ( lbl_ )
	return lbl_;
    else if ( !insbuts_.isEmpty() )
	return insbuts_[ insbuts_.size()-1 ];

    return inp_;
}


void uiIOSelect::setEntries( const BufferStringSet& keys,
			     const BufferStringSet& names )
{
    entries_ = keys;
    inp_->setEmpty();
    inp_->addItems( names );
}


void uiIOSelect::updateFromEntries()
{
    int curitnr = inp_->size() ? inp_->currentItem() : -1;
    BufferString curusrnm;
    if ( curitnr >= 0 )
	curusrnm = inp_->itemText( curitnr );

    if ( keepmytxt_ )
	curusrnm = inp_->text();

    inp_->setEmpty();
    if ( haveempty_ )
	inp_->addItem( uiString::empty() );

    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	const char* usrnm = userNameFromKey( entries_.get(idx) );
	if ( usrnm )
	    inp_->addItem( toUiString(usrnm) );
	else
	{
	    entries_.removeSingle( idx );
	    idx--;
	}
    }

    if ( curitnr >= 0 && inp_->size() )
	inp_->setCurrentItem( curusrnm );

    if ( keepmytxt_ )
	inp_->setText( curusrnm );
}


bool uiIOSelect::haveEntry( const char* key ) const
{
    if ( !key || !*key )
	return haveempty_;

    for ( int idx=0; idx<entries_.size(); idx++ )
	if ( entries_.get(idx) == key ) return true;

    return false;
}


void uiIOSelect::updateHistory( IOPar& iopar ) const
{
    // Find the last key already present
    int lastidx = 0;
    for ( ; ; lastidx++ )
    {
	if ( !iopar.find( IOPar::compKey(sKey::IOSelection(),lastidx+1)) )
	    break;
    }

    // Add the entries
    const int nrentries = entries_.size();
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const char* key = entries_.get( idx ).buf();
	if ( iopar.findKeyFor(key) )
	    continue;

	lastidx++;
	iopar.set( IOPar::compKey(sKey::IOSelection(),lastidx), key );
    }
}


void uiIOSelect::getHistory( const IOPar& iopar )
{
    checkState();
    BufferStringSet bss;
    for ( int idx=1; ; idx++ )
    {
	BufferString bs;
	if ( iopar.get( IOPar::compKey(sKey::IOSelection(),idx), bs ) )
	    bss.add( bs );
	else
	    break;
    }
    addToHistory( bss );
}


void uiIOSelect::addToHistory( const char* key )
{
    BufferStringSet bss; bss.add( key );
    addToHistory( bss );
}


void uiIOSelect::addToHistory( const BufferStringSet& bss )
{
    const bool haveold = !inp_->isEmpty();
    bool havenew = false;
    for ( int idx=0; idx<bss.size(); idx++ )
    {
	const char* key = bss.get(idx).buf();
	if ( haveEntry(key) || !userNameFromKey(key) )
	    continue;
	havenew = true;
	entries_.add( key );
    }
    if ( havenew )
    {
	{
	    NotifyStopper ns( selectionDone );
	    updateFromEntries();
	}

	if ( !haveold )
	    selDone(0 );
    }
}


bool uiIOSelect::isEmpty() const
{
    const char* inp = getInput();
    return !inp || !*inp;
}



const char* uiIOSelect::getInput() const
{
    return inp_->text();
}


const char* uiIOSelect::getKey() const
{
    checkState();
    const_cast<uiIOSelect*>(this)->processInput();

    const int curit = getCurrentItem() - nrSpec();
    return curit < 0 || entries_.isEmpty()
	 ? "" : entries_.get(curit).buf();
}


void uiIOSelect::checkState() const
{
    if ( inp_->size() != nrItems() )
	const_cast<uiIOSelect*>(this)->updateFromEntries();
}


void uiIOSelect::setInput( const char* key )
{
    checkState();

    if ( !key || !*key )
    {
	if ( haveempty_ )
	    inp_->setCurrentItem( 0 );
	return;
    }

    const char* usrnm = userNameFromKey(key) ;
    if ( !usrnm ) return;

    const int nrentries = entries_.size();
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const int boxidx = idx + nrSpec();
	if ( entries_.get(idx) == key )
	{
	    inp_->setItemText( boxidx, toUiString(usrnm) );
	    inp_->setCurrentItem( boxidx );
	    return;
	}
    }

    entries_.addIfNew( key );
    if ( !inp_->isPresent(usrnm) )
	inp_->addItem( toUiString(usrnm) );
    inp_->setCurrentItem(usrnm);
    setInputText(usrnm);
}


void uiIOSelect::setInputText( const char* txt )
{
    inp_->setText( txt );
}


int uiIOSelect::getCurrentItem() const
{
    checkState();
    if ( inp_->size() == 0 )
	return -1;

    const char* curtxt = inp_->text();
    if ( !inp_->isPresent(curtxt) )
	return -1;

    return inp_->currentItem();
}


void uiIOSelect::setCurrentItem( int idx )
{
    checkState();
    if ( idx >= 0 ) inp_->setCurrentItem( idx );
}


int uiIOSelect::nrSpec() const
{
    return haveempty_ ? 1 : 0;
}


int uiIOSelect::nrItems() const
{
    return nrSpec() + entries_.size();
}


const char* uiIOSelect::getItem( int idx ) const
{
    const int nrspec = nrSpec();
    const int nrentries = entries_.size();
    return idx < nrspec || idx >= nrentries + nrspec
	 ? "" : entries_.get( idx - nrspec ).buf();
}


bool uiIOSelect::isChecked() const
{
    return !optbox_ || optbox_->isChecked();
}


void uiIOSelect::setChecked( bool yn )
{
    if ( optbox_ )
	optbox_->setChecked( yn );
}


void uiIOSelect::optCheck( CallBacker* )
{
    if ( !optbox_ ) return;

    const bool isch = isChecked();
    inp_->setSensitive( isch );
    if ( selbut_ )
	selbut_->setSensitive( isch );
    optionalChecked.trigger();
}


void uiIOSelect::doSel( CallBacker* )
{
    processInput();
    selok_ = false;
    doselcb_.doCall( this );
    if ( selok_ )
    {
	updateFromEntries();
	selectionDone.trigger();
    }
}


void uiIOSelect::selDone( CallBacker* )
{
    processInput();
    selectionDone.trigger();
}


void uiIOSelect::setEmpty()
{
    inp_->setEmpty();
    entries_.setEmpty();
}


void uiIOSelect::setReadOnly( bool yn )
{
    inp_->setReadOnly( yn );
}


const uiString& uiIOSelect::labelText() const
{

    return lbl_
	? lbl_->text()
	: (optbox_ ? optbox_->text() : uiString::empty() );
}


void uiIOSelect::setLabelText( const uiString& s )
{
    if ( lbl_ )
    {
	lbl_->setText( s );
	lbl_->setPrefWidthInChar( s.size() + 3 );
	return ;
    }
    else if ( optbox_ )
	optbox_->setText( s );
}


void uiIOSelect::setLabelSelectable( bool yn )
{
    if ( lbl_ )
	lbl_->setTextSelectable( yn );
}
