/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uiiosel.cc,v 1.2 2001-04-27 16:48:16 bert Exp $
________________________________________________________________________

-*/

#include "uiiosel.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uifiledlg.h"
#include "uidset.h"
#include "iopar.h"


uiIOSelect::uiIOSelect( uiObject* p, const CallBack& butcb, const char* txt,
			bool seled, bool withclear )
	: uiGroup(p)
	, inpsels_(*new UserIDSet(txt))
	, withclear_(withclear)
	, doselcb_(butcb)
{
    if ( withclear ) inpsels_.add( "" );
    inp_ = new uiLabeledComboBox( this, inpsels_, seled );
    inp_->box()->notify( mCB(this,uiIOSelect,selDone) );
    inp_->box()->setPrefWidthInChar( 20 );
    selbut_ = new uiPushButton( this, "Select ..." );
    selbut_->notify( mCB(this,uiIOSelect,doSel) );
    selbut_->attach( rightOf, inp_ );

    setHAlignObj( inp_ );
    setHCentreObj( inp_ );
}


uiIOSelect::~uiIOSelect()
{
    inpsels_.deepErase();
    delete &inpsels_;
}


void uiIOSelect::fillPar( IOPar& iopar ) const
{
    int curidx = getItem();
    iopar.set( "Current", curidx );

    inpsels_.deepErase();
    const int sz = nrItems();
    for ( int idx=0; idx<sz; idx++ )
    {
	const char* res = idx == curidx ? getInput() : getItemText(idx);
	if ( *res ) inpsels_.add( res );
    }
    use( inpsels_, iopar, "Text" );
}


void uiIOSelect::usePar( const IOPar& iopar )
{
    UserIDSet newinps;
    use( iopar, newinps, "Text" );
    setItems( newinps );
    newinps.deepErase();
    setCurrentFromIOPar( iopar );
}


void uiIOSelect::setCurrentFromIOPar( const IOPar& iopar )
{
    if ( nrItems() )
    {
        int curidx = 0;
        iopar.get( "Current", curidx );
        setCurrentItem( curidx );
    }
}


const char* uiIOSelect::getInput() const
{
    return inp_->box()->getText();
}


void uiIOSelect::setInput( const char* txt )
{
    if ( !inp_->box()->isPresent(txt) )
	inp_->box()->addItem( txt );
    inp_->box()->setCurrentItem( txt );
}


int uiIOSelect::nrItems() const
{
    return inp_->box()->size();
}


int uiIOSelect::getItem() const
{
    return inp_->box()->currentItem();
}


const char* uiIOSelect::getItemText( int idx ) const
{
    return inp_->box()->textOfItem( idx );
}


void uiIOSelect::setCurrentItem( int idx )
{
    inp_->box()->setCurrentItem( idx );
}


void uiIOSelect::setItems( const UserIDSet& newitems )
{
    inpsels_.deepErase();
    inp_->box()->clear();

    inpsels_.copyFrom( Ptr(newitems) );
    if ( withclear_ )
    {
	int idx = inpsels_.indexOf( "" );
	if ( idx < 0 )
	{
	    UserIDObject* empty = new UserIDObject( "" );
	    inpsels_ += empty;
	    inpsels_.moveAfter( empty, 0 );
	}
    }
    inp_->box()->addItems( inpsels_ );
}


void uiIOSelect::doSel( CallBacker* c )
{
    BufferString oldinp = getInput();
    doselcb_.doCall( this );
    if ( oldinp != getInput() )
	seldonecb_.doCall( this );
}


void uiIOSelect::selDone( CallBacker* )
{
    seldonecb_.doCall( this );
}


uiIOFileSelect::uiIOFileSelect( uiObject* p, const char* txt, bool frrd,
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
	setInput( fd.fileName() );
    
}
