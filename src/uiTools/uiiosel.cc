/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uiiosel.cc,v 1.7 2001-05-18 13:36:45 bert Exp $
________________________________________________________________________

-*/

#include "uiiosel.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uifiledlg.h"
#include "uidset.h"
#include "separstr.h"
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


void uiIOSelect::fillPar( IOPar& iopar ) const
{
    int startidx = withclear_ ? 1 : 0;
    iopar.removeWithKey( "Selection.[0-9]*" );
    int curidx = getCurrentItem();
    iopar.set( "Current", curidx - startidx + 1 );

    const int sz = nrItems();
    for ( int idx=startidx; idx<sz; idx++ )
    {
	BufferString buf;
	const char* key = *entries_[idx];
	const char* usrnm = idx == curidx ? getInput() : userNameFromKey( key );
	buf = usrnm; buf += "`"; buf += usrnm;
	iopar.set( IOPar::compKey("Selection",idx-startidx+1),
		   (const char*)buf );
    }
}


void uiIOSelect::usePar( const IOPar& iopar )
{
    deepErase( entries_ );
    if ( withclear_ ) entries_ += new BufferString;

    BufferString bs;
    for ( int idx=0; ; idx++ )
    {
	if ( !iopar.get( IOPar::compKey("Selection",idx), bs ) )
	    { if ( idx ) break; else continue; }

	FileMultiString fms( (const char*)bs );
	const char* key = fms[1];
	if ( !userNameFromKey(key) ) continue;

	entries_ += new BufferString( key );
    }
    updateFromEntries();

    if ( nrItems() )
    {
        int curidx = 0;
        iopar.get( "Current", curidx );
        setCurrentItem( curidx - withclear_ ? 0 : 1 );
    }
}


const char* uiIOSelect::getInput() const
{
    return inp_->box()->getText();
}


const char* uiIOSelect::getKey() const
{
    return *entries_[getCurrentItem()];
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
