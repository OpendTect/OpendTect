/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          start of 2001
 RCS:           $Id: uiiosel.cc,v 1.16 2001-07-20 16:17:05 bert Exp $
________________________________________________________________________

-*/

#include "uiiosel.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uifiledlg.h"
#include "uidset.h"
#include "iopar.h"
#include "filegen.h"

IOPar& uiIOFileSelect::ixtablehistory = *new IOPar("IXTable selection history");
IOPar& uiIOFileSelect::devicehistory = *new IOPar("Device selection history");


uiIOSelect::uiIOSelect( uiParent* p, const CallBack& butcb, const char* txt,
			bool withclear )
	: uiGroup(p)
	, doselcb_(butcb)
	, selectiondone(this)
	, specialitems(*new IOPar)
{
    if ( withclear ) addSpecialItem( "" );

    inp_ = new uiLabeledComboBox( this, txt, "uiIOSelect", true );
    inp_->box()->selectionchanged.notify( mCB(this,uiIOSelect,selDone) );
    inp_->box()->setPrefWidthInChar( 20 );
    selbut_ = new uiPushButton( this, "Select ..." );
    selbut_->notify( mCB(this,uiIOSelect,doSel) );
    selbut_->attach( rightOf, inp_ );

    setHAlignObj( inp_ );
    setHCentreObj( inp_ );
}


uiIOSelect::~uiIOSelect()
{
    delete &specialitems;
    deepErase( entries_ );
}


void uiIOSelect::finalise_()
{
    uiGroup::finalise_();
    updateFromEntries();
}


void uiIOSelect::updateFromEntries()
{
    int curitnr = inp_->box()->size() ? inp_->box()->currentItem() : -1;
    BufferString curusrnm;
    if ( curitnr >= 0 )
	curusrnm = inp_->box()->textOfItem( curitnr );

    inp_->box()->empty();

    for ( int idx=0; idx<specialitems.size(); idx++ )
	inp_->box()->addItem( specialitems.getValue(idx) );

    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	const char* usrnm = userNameFromKey( *entries_[idx] );
	if ( usrnm )
	    inp_->box()->addItem( usrnm );
	else
	{
	    delete entries_[idx];
	    entries_.remove( idx );
	    idx--;
	}
    }

    if ( curitnr >= 0 && inp_->box()->size() )
	inp_->box()->setCurrentItem( curusrnm );
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
	if ( !iopar.find( IOPar::compKey("I/O Selection",lastidx+1)) )
	    break;
    }

    // Add the entries
    const int nrentries = entries_.size();
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const char* key = *entries_[idx];
	if ( iopar.findKeyFor(key) ) continue;

	lastidx++;
	iopar.set( IOPar::compKey("I/O Selection",lastidx), key );
    }
}


void uiIOSelect::getHistory( const IOPar& iopar )
{
    checkState();
    bool haveold = inp_->box()->size();
    bool havenew = false; BufferString bs;
    for ( int idx=1; ; idx++ )
    {
	if ( !iopar.get( IOPar::compKey("I/O Selection",idx), bs ) )
	    break;

	const char* key = bs;
	if ( haveEntry(key) || !userNameFromKey(key) ) continue;

	havenew = true;
	entries_ += new BufferString( key );
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


const char* uiIOSelect::getInput() const
{
    return inp_->box()->getText();
}


const char* uiIOSelect::getKey() const
{
    checkState();
    const_cast<uiIOSelect*>(this)->processInput();

    const int nrspec = specialitems.size();
    const int curit = getCurrentItem();
    if ( curit < 0 ) return "";
    if ( curit < nrspec ) return specialitems.getKey(curit);

    return entries_.size() ? (const char*)(*entries_[curit-nrspec]) : "";
}


void uiIOSelect::checkState() const
{
    if ( inp_->box()->size() != specialitems.size() + entries_.size() )
	const_cast<uiIOSelect*>(this)->updateFromEntries();
}


void uiIOSelect::setInput( const char* key )
{
    checkState();

    if ( specialitems.find(key) )
    {
	inp_->box()->setCurrentItem( specialitems.find(key) );
	return;
    }

    const char* usrnm = userNameFromKey( key );
    if ( !usrnm ) return;

    const int nrspec = specialitems.size();
    const int nrentries = entries_.size();
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const int boxidx = idx + nrspec;
	if ( *entries_[idx] == key )
	{
	    inp_->box()->setItemText( boxidx, usrnm );
	    inp_->box()->setCurrentItem( boxidx );
	    return;
	}
    }

    entries_ += new BufferString( key );
    inp_->box()->addItem( usrnm );
    inp_->box()->setCurrentItem( nrspec + nrentries );
}


int uiIOSelect::getCurrentItem() const
{
    checkState();
    return inp_->box()->size() ? inp_->box()->currentItem() : -1;
}


void uiIOSelect::setCurrentItem( int idx )
{
    checkState();
    if ( idx >= 0 ) inp_->box()->setCurrentItem( idx );
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
	 : (idx < nrentries + nrspec ? (const char*)*entries_[idx-nrspec] : "");
}


void uiIOSelect::doSel( CallBacker* )
{
    processInput();
    doselcb_.doCall( this );
    updateFromEntries();
}


void uiIOSelect::selDone( CallBacker* )
{
    objSel();
    selectiondone.trigger();
}


uiIOFileSelect::uiIOFileSelect( uiParent* p, const char* txt, bool frrd,
				const char* inp, bool wclr )
	: uiIOSelect(p,mCB(this,uiIOFileSelect,doFileSel),txt,wclr)
	, forread(frrd)
{
    if ( inp && *inp ) setInput( inp );
}


void uiIOFileSelect::doFileSel( CallBacker* c )
{
    BufferString caption( "Select " );
    caption += inp_->label()->text();
    uiFileDialog fd( this, forread, getInput(),
		     filter == "" ? 0 : (const char*)filter, caption );
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
    return res && *res && File_exists(res);
}


void uiIOFileSelect::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "File name" );
    if ( res ) setInput( res );
}
