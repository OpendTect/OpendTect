/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uiiosel.cc,v 1.9 2001-05-30 16:11:33 bert Exp $
________________________________________________________________________

-*/

#include "uiiosel.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uifiledlg.h"
#include "uidset.h"
#include "iopar.h"


uiIOSelect::uiIOSelect( uiParent* p, const CallBack& butcb, const char* txt,
			bool seled, bool withclear )
	: uiGroup(p)
	, withclear_(withclear)
	, doselcb_(butcb)
	, selectiondone(this)
{
    if ( withclear ) entries_ += new BufferString;
    inp_ = new uiLabeledComboBox( this, txt, "uiIOSelect", seled );
    inp_->box()->selectionchanged.notify( mCB(this,uiIOSelect,selDone) );
    inp_->box()->setPrefWidthInChar( 20 );
    selbut_ = new uiPushButton( this, "Select ..." );
    selbut_->notify( mCB(this,uiIOSelect,doSel) );
    selbut_->attach( rightOf, inp_ );

    setHAlignObj( inp_ );
    setHCentreObj( inp_ );
    updateFromEntries();
}


uiIOSelect::~uiIOSelect()
{
    deepErase( entries_ );
}


void uiIOSelect::updateFromEntries()
{
    int curitnr = inp_->box()->size() ? inp_->box()->currentItem() : -1;
    BufferString curusrnm;
    if ( curitnr >= 0 )
	curusrnm = inp_->box()->textOfItem( curitnr );

    inp_->box()->empty();
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
    for ( int idx=0; idx<entries_.size(); idx++ )
	if ( *entries_[idx] == key ) return true;
    return false;
}


void uiIOSelect::fillPar( IOPar& iopar ) const
{
    int startidx = withclear_ ? 1 : 0;
    int lastidx = 0;
    for ( ; ; lastidx++ )
    {
	if ( !iopar.find( IOPar::compKey("I/O Selection",lastidx+1)) )
	    break;
    }

    const int sz = nrItems();
    for ( int idx=startidx; idx<sz; idx++ )
    {
	const char* key = *entries_[idx];
	if ( iopar.findKeyFor(key) ) continue;

	lastidx++;
	iopar.set( IOPar::compKey("I/O Selection",lastidx), key );
    }
}


void uiIOSelect::usePar( const IOPar& iopar )
{
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
	updateFromEntries();
}


const char* uiIOSelect::getInput() const
{
    return inp_->box()->getText();
}


const char* uiIOSelect::getKey() const
{
    return entries_.size() ? (const char*)(*entries_[getCurrentItem()]) : "";
}


void uiIOSelect::setInput( const char* key )
{
    const char* usrnm = userNameFromKey( key );
    if ( !key ) key = "";
    if ( !usrnm && (!withclear_ || *key ) )
	return;

    if ( !usrnm ) usrnm = "";

    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( *entries_[idx] == key )
	{
	    inp_->box()->setItemText( idx, usrnm );
	    inp_->box()->setCurrentItem( idx );
	    return;
	}
    }

    entries_ += new BufferString( key );
    inp_->box()->addItem( usrnm );
    inp_->box()->setCurrentItem( entries_.size() - 1 );
}


int uiIOSelect::getCurrentItem() const
{
    return inp_->box()->currentItem();
}


void uiIOSelect::setCurrentItem( int idx )
{
    inp_->box()->setCurrentItem( idx );
}


void uiIOSelect::doSel( CallBacker* )
{
    doselcb_.doCall( this );
}


void uiIOSelect::selDone( CallBacker* )
{
    objSel();
    selectiondone.trigger();
}


uiIOFileSelect::uiIOFileSelect( uiParent* p, const char* txt, bool frrd,
				const char* inp, bool wclr )
	: uiIOSelect(p,mCB(this,uiIOFileSelect,doFileSel),txt,!frrd,wclr)
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
