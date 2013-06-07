/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          start of 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiiosel.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uifiledlg.h"
#include "iopar.h"
#include "file.h"
#include "keystrs.h"

IOPar& uiIOFileSelect::ixtablehistory()
{ return *new IOPar("IXTable selection history"); }
IOPar& uiIOFileSelect::devicehistory()
{ return *new IOPar("Device selection history"); }
IOPar& uiIOFileSelect::tmpstoragehistory()
{ return *new IOPar("Temporay storage selection history"); }


uiIOSelect::uiIOSelect( uiParent* p, const Setup& su, const CallBack& butcb )
	: uiGroup(p)
	, doselcb_(butcb)
	, selectionDone(this)
	, optionalChecked(this)
	, specialitems(*new IOPar)
    	, keepmytxt_(su.keepmytxt_)
    	, lbl_(0)
    	, optbox_(0)
{
    if ( su.withclear_ ) addSpecialItem( "" );

    uiObject* alobj = 0;
#define mComboName BufferString("Select ",su.seltxt_.buf())
    if ( su.optional_ )
    {
	alobj = inp_ = new uiComboBox( this, mComboName );
	optbox_ = new uiCheckBox( this, su.seltxt_ );
	optbox_->activated.notify( mCB(this,uiIOSelect,optCheck) );
	optbox_->attach( leftOf, inp_ );
    }
    else
    {
	if ( su.seltxt_.isEmpty() )
	{
	    inp_ = new uiComboBox( this, mComboName );
	    alobj = inp_;
	}
	else
	{
	    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, su.seltxt_,
							    mComboName );
	    inp_ = lcb->box(); lbl_ = lcb->label(); alobj = lcb->attachObj();
	    lbl_->setAlignment( Alignment::Right );
	}
    }
    inp_->setReadOnly( false );
    inp_->selectionChanged.notify( mCB(this,uiIOSelect,selDone) );

    selbut_ = new uiPushButton( this, su.buttontxt_, false );
    selbut_->setName( BufferString( su.buttontxt_, " ", su.seltxt_ ) );
    selbut_->activated.notify( mCB(this,uiIOSelect,doSel) );
    selbut_->attach( rightOf, alobj );

    setHAlignObj( alobj );
    setHCenterObj( alobj );
    preFinalise().notify( mCB(this,uiIOSelect,doFinalise) );
    postFinalise().notify( mCB(this,uiIOSelect,optCheck) );
}


uiIOSelect::~uiIOSelect()
{
    delete &specialitems;
    deepErase( entries_ );
}


void uiIOSelect::stretchHor( bool yn )
{
    inp_->setHSzPol( uiObject::MedMax );
}


void uiIOSelect::doFinalise( CallBacker* cb )
{
    updateFromEntries();
}


void uiIOSelect::updateFromEntries()
{
    int curitnr = inp_->size() ? inp_->currentItem() : -1;
    BufferString curusrnm;
    if ( curitnr >= 0 )
	curusrnm = inp_->textOfItem( curitnr );

    if ( keepmytxt_ )
	curusrnm = inp_->text();

    inp_->setEmpty();

    for ( int idx=0; idx<specialitems.size(); idx++ )
	inp_->addItem( specialitems.getValue(idx) );

    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	const char* usrnm = userNameFromKey( *entries_[idx] );
	if ( usrnm )
	    inp_->addItem( usrnm );
	else
	{
	    delete entries_[idx];
	    entries_.remove( idx );
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
    if ( specialitems.find(key) ) return true;

    for ( int idx=0; idx<entries_.size(); idx++ )
	if ( *entries_[idx] == key ) return true;
    return false;
}


void uiIOSelect::updateHistory( IOPar& iopar ) const
{
    // Find the last key already present
    int lastidx = 0;
    for ( ; ; lastidx++ )
    {
	if ( !iopar.find( IOPar::compKey(sKey::IOSelection,lastidx+1)) )
	    break;
    }

    // Add the entries
    const int nrentries = entries_.size();
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const char* key = entries_.get( idx ).buf();
	if ( iopar.findKeyFor(key) ) continue;

	lastidx++;
	iopar.set( IOPar::compKey(sKey::IOSelection,lastidx), key );
    }
}


void uiIOSelect::getHistory( const IOPar& iopar )
{
    checkState();
    BufferStringSet bss;
    for ( int idx=1; ; idx++ )
    {
	BufferString bs;
	if ( iopar.get( IOPar::compKey(sKey::IOSelection,idx), bs ) )
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
	updateFromEntries();
	if ( !haveold )
	    selDone(0);
    }
}


void uiIOSelect::addSpecialItem( const char* key, const char* value )
{
    if ( !value ) value = key;
    specialitems.set( key, value );
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

    const int nrspec = specialitems.size();
    const int curit = getCurrentItem();
    if ( curit < 0 ) return "";
    if ( curit < nrspec ) return specialitems.getKey(curit);

    return entries_.isEmpty() ? "" : entries_.get(curit-nrspec).buf();
}


void uiIOSelect::checkState() const
{
    if ( inp_->size() != nrItems() )
	const_cast<uiIOSelect*>(this)->updateFromEntries();
}


void uiIOSelect::setInput( const char* key )
{
    checkState();

    if ( specialitems.find(key) )
    {
	inp_->setCurrentItem( specialitems.find(key) );
	return;
    }

    const char* usrnm = userNameFromKey( key );
    if ( !usrnm ) return;

    const int nrspec = specialitems.size();
    const int nrentries = entries_.size();
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const int boxidx = idx + nrspec;
	if ( entries_.get(idx) == key )
	{
	    inp_->setItemText( boxidx, usrnm );
	    inp_->setCurrentItem( boxidx );
	    return;
	}
    }

    entries_.addIfNew( key );
    if ( !inp_->isPresent(usrnm) )
	inp_->addItem( usrnm );
    inp_->setCurrentItem( usrnm );
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


int uiIOSelect::nrItems() const
{
    return specialitems.size() + entries_.size();
}


const char* uiIOSelect::getItem( int idx ) const
{
    const int nrspec = specialitems.size();
    const int nrentries = entries_.size();
    return idx < nrspec
	 ? (idx < 0 ? "" : specialitems.getValue(idx))
	 : (idx < nrentries + nrspec ? entries_.get(idx-nrspec).buf() : "");
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


void uiIOSelect::setEmpty( bool withclear )
{
    inp_->setEmpty();
    entries_.erase();
    if ( withclear ) addSpecialItem( "" );
}


void uiIOSelect::setReadOnly( bool yn )
{
    inp_->setReadOnly( yn );
}


const char* uiIOSelect::labelText() const
{
    return lbl_ ? lbl_->text() : (optbox_ ? optbox_->text() : "");
}


void uiIOSelect::setLabelText( const char* s )
{
    if ( lbl_ )
    {
	lbl_->setPrefWidthInChar( strlen(s)+1 );
	return lbl_->setText( s );
    }
    else if ( optbox_ )
	optbox_->setText( s );
}


void uiIOSelect::setLabelSelectable( bool yn )
{
    if ( lbl_ )
	lbl_->setTextSelectable( yn );
}


uiIOFileSelect::uiIOFileSelect( uiParent* p, const char* txt, bool frrd,
				const char* inp, bool wclr )
	: uiIOSelect(p,uiIOSelect::Setup(txt).withclear(wclr),
			mCB(this,uiIOFileSelect,doFileSel))
	, forread(frrd)
	, seldir(false)
{
    if ( inp && *inp ) setInput( inp );
}


void uiIOFileSelect::doFileSel( CallBacker* c )
{
    BufferString caption( "Select " );
    caption += labelText();
    uiFileDialog fd( this, forread, getInput(),
		     filter.isEmpty() ? 0 : (const char*)filter, caption );
    if ( seldir )
	fd.setMode( uiFileDialog::DirectoryOnly );

    if ( fd.go() )
    {
	setInput( fd.fileName() );
	selDone( 0 );
    }
}


bool uiIOFileSelect::fillPar( IOPar& iopar ) const
{
    const char* res = getInput();
    iopar.set( "File name", res );
    return res && *res && File::exists(res);
}


void uiIOFileSelect::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "File name" );
    if ( res ) setInput( res );
}
