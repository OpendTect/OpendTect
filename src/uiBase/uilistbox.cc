/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.cc,v 1.59 2004-11-09 13:13:58 nanne Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"
#include "uifont.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "bufstringset.h"
#include "color.h"
#include "pixmap.h"

#include "i_qlistbox.h"



class uiListBoxBody : public uiObjBodyImpl<uiListBox,QListBox>
{

public:

                        uiListBoxBody(uiListBox& handle, 
				  uiParent* parnt=0, 
				  const char* nm="uiListBoxBody",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

    virtual 		~uiListBoxBody()		{ delete &messenger_; }

    void 		setLines( int prefNrLines, bool adaptVStretch )
			{ 
			    if( prefNrLines >= 0 ) prefnrlines=prefNrLines;

			    if( adaptVStretch )
			    {
				int hs = stretch(true,true);
				setStretch( hs, (nrTxtLines()== 1) ? 0 : 2 );
			    }
			}

    virtual uiSize	minimumsize() const; //!< \reimp
    virtual int 	nrTxtLines() const
			    { return prefnrlines ? prefnrlines : 7; }

    int 		fieldWdt;
    int 		prefnrlines;

private:

    i_listMessenger&    messenger_;

};



uiListBoxBody::uiListBoxBody( uiListBox& handle, uiParent* parnt, 
			const char* nm, bool isMultiSelect,
			int preferredNrLines, int preferredFieldWidth )
	: uiObjBodyImpl<uiListBox,QListBox>( handle, parnt, nm )
	, messenger_ (*new i_listMessenger(this, &handle))
	, fieldWdt(preferredFieldWidth)
	, prefnrlines(preferredNrLines)
{
    if( isMultiSelect ) setSelectionMode( QListBox::Extended );

    setStretch( 2, (nrTxtLines()== 1) ? 0 : 2 );

    setHSzPol( uiObject::medium);
}


uiListBox::uiListBox( uiParent* p, const char* nm, bool ms, int nl, int pfw)
    : uiObject( p, nm, mkbody(p,nm,ms,nl,pfw) )
    , selectionChanged( this )
    , doubleClicked( this )
    , rightButtonClicked( this )
    , lastClicked_( -1 )
{}


uiListBox::uiListBox( uiParent* p, const BufferStringSet& uids,
		      const char* txt, bool ms, int nl, int pfw )
    : uiObject( p, txt, mkbody(p,txt,ms,nl,pfw))
    , selectionChanged( this )
    , doubleClicked( this )
    , rightButtonClicked( this )
    , lastClicked_( -1 )
{
    addItems( uids );
}

uiListBoxBody& uiListBox::mkbody( uiParent* p, const char* nm, bool ms,
				  int nl, int pfw)
{
    body_ = new uiListBoxBody(*this,p,nm,ms,nl,pfw);
    return *body_;
}


uiListBox::~uiListBox()
{
}

void uiListBox::setLines( int prefNrLines, bool adaptVStretch )
    { body_->setLines(prefNrLines,adaptVStretch); }


void uiListBox::setNotSelectable()
{
    body_->setSelectionMode( QListBox::NoSelection );
}


void uiListBox::setMultiSelect( bool yn )
{
    body_->setSelectionMode( yn ? QListBox::Extended : QListBox::Single );
}


int uiListBox::size() const
    { return body_->count(); }


bool uiListBox::isSelected ( int idx ) const
{
    if ( idx < 0 || idx >= body_->count() ) return false;
    return body_->isSelected( idx );
}


int uiListBox::nrSelected() const
{
    int res = 0;
    for ( int idx=0; idx<size(); idx++ )
	{ if ( isSelected(idx) ) res++; }
    return res;
}


void uiListBox::setSelected( int idx, bool yn )
{
    if ( idx >= 0 && idx < body_->count() )
	body_->setSelected( idx, yn );
}


void uiListBox::selectAll( bool yn )
{
    if ( body_->selectionMode() != QListBox::Extended ) return;

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
	body_->setSelected( idx, yn );
}


void uiListBox::createQString( QString& qs, const char* str, bool embed )
{
    if ( !str ) str = "";
    BufferString bs( str );
    if ( embed ) { bs = "["; bs += str; bs += "]"; }
    qs = bs.buf();
}


void uiListBox::createQPixmap( QPixmap& qpm, int index )
{
    if ( !size() ) return;
    const int height = body_->itemHeight( 0 );
    qpm.resize( height-2, height-2 );
}


void uiListBox::addItem( const char* text, bool embed ) 
{
    insertItem( text, -1, embed );
}


void uiListBox::addItems( const char** textList ) 
{
    int curidx = currentItem();
    const char* pt_cur = *textList;
    while ( pt_cur )
        addItem( pt_cur++ );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::addItems( const BufferStringSet& strs )
{
    int curidx = currentItem();
    for ( int idx=0; idx<strs.size(); idx++ )
	body_->insertItem( QString( strs.get(idx) ), -1 );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::insertItem( const char* text, int index, bool embed )
{
    QString qs;
    createQString( qs, text, embed );
    body_->insertItem( qs, index );
}


void uiListBox::insertItem( const char* text, const Color& col, int index )
{
    insertItem( text, index, false );

    if ( index < 0 ) index = size()-1;
    setColor( col, index );
}


void uiListBox::insertItem( const char* text, const ioPixmap& pm, int index )
{
    QString qs;
    createQString( qs, text, false );
    body_->insertItem( *pm.Pixmap(), qs, index );
}


void uiListBox::setColor( const Color& col, int index )
{
    if ( index >= size() ) return;

    QPixmap qpm;
    createQPixmap( qpm, index );
    qpm.fill( QColor(col.r(),col.g(),col.b()) );

    QString qs = body_->text( index );
    body_->changeItem( qpm, qs, index );
}


ioPixmap* uiListBox::pixmap( int index )
{
    const QPixmap* qpm = body_->pixmap(index);
    return qpm ? new ioPixmap( *qpm ) : 0;
}


void uiListBox::empty()
{
    body_->QListBox::clear();
}


void uiListBox::clear()
{
    body_->clearSelection();
}


void uiListBox::sort( bool asc )
{
    body_->sort( asc );
}


void uiListBox::removeItem( int idx )
{
    body_->removeItem( idx );
}


bool uiListBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString itmtxt( body_->text(idx) );
	char* ptr = itmtxt.buf();
	skipLeadingBlanks( ptr );
	if ( !strcmp(txt,ptr) ) return true;
    }
    return false;
}


const char* uiListBox::textOfItem( int idx, bool disembed ) const
{
    if ( idx < 0 || idx >= body_->count() )
	return "";

    rettxt = (const char*)body_->text(idx);
    if ( !disembed || rettxt[0] != '[' )
	return rettxt;

    const int sz = rettxt.size();
    if ( rettxt[sz-1] != ']' )
	return rettxt;

    rettxt[sz-1] = '\0';
    return ((const char*)rettxt) + 1;
}


bool uiListBox::isEmbedded( int idx ) const
{
    rettxt = (const char*)body_->text(idx);
    return rettxt[0] == '[' && rettxt[rettxt.size()-1] == ']';
}


int uiListBox::currentItem() const
{
    return body_->currentItem();
}


void uiListBox::setCurrentItem( const char* txt )
{
    if ( !txt ) return;

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	const char* ptr = body_->text(idx);
	skipLeadingBlanks(ptr);
	if ( !strcmp(ptr,txt) )
	    { setCurrentItem( idx ); return; }
    }
}

void uiListBox::setCurrentItem( int idx )
{
    if ( idx >= 0 && idx < body_->count() )
    {
	body_->setCurrentItem( idx );
	if ( body_->selectionMode() != QListBox::Extended )
	    setSelected( idx );
    }
}


void uiListBox::setItemText( int idx, const char* txt )
{
    body_->changeItem( QString(txt), idx );
}


void uiListBox::getSelectedItems( BufferStringSet& items )
{
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isSelected(idx) ) items.add( textOfItem(idx) );
}


void uiListBox::getSelectedItems( TypeSet<int>& items )
{
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isSelected(idx) ) items += idx;
}


void uiListBox::setFieldWidth( int fw )
{
    body_->fieldWdt = fw;
}


int uiListBox::optimumFieldWidth( int minwdth, int maxwdth ) const
{
    const int sz = size();
    int len = minwdth;
    for ( int idx=0; idx<sz; idx++ )
    {
	int itlen = strlen( textOfItem(idx) );
	if ( itlen >= maxwdth )
	    { len = maxwdth; break; }
	else if ( itlen > len )
	    len = itlen;
    }
    return len + 1;
}


uiSize uiListBoxBody::minimumsize() const
{ 
    int totHeight = fontHgt() * prefnrlines;
    int totWidth  = fontWdt( true ) * fieldWdt;

    return uiSize ( totWidth , totHeight, true );
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const char* txt, bool multisel,
				    uiLabeledListBox::LblPos pos )
	: uiGroup(p,"Labeled listbox")
{
    lb = new uiListBox( this, txt, multisel );
    mkRest( txt, pos );
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const BufferStringSet& s,
				    const char* txt,
				    bool multisel, uiLabeledListBox::LblPos pos)
	: uiGroup(p,"Labeled listbox")
{
    lb = new uiListBox( this, s, txt, multisel );
    mkRest( txt, pos );
}


void uiLabeledListBox::setLabelText( const char* txt, int nr )
{
    if ( nr >= lbls.size() ) return;
    lbls[nr]->setText( txt );
}


const char* uiLabeledListBox::labelText( int nr ) const
{
    if ( nr >= lbls.size() ) return "";
    return lbls[nr]->text();
}


void uiLabeledListBox::mkRest( const char* txt, uiLabeledListBox::LblPos pos )
{
    setHAlignObj( lb );

    BufferStringSet txts;
    BufferString s( txt );
    char* ptr = s.buf();
    if( !ptr || !*ptr ) return;
    while ( 1 )
    {
	char* nlptr = strchr( ptr, '\n' );
	if ( nlptr ) *nlptr = '\0';
	txts += new BufferString( ptr );
	if ( !nlptr ) break;

	ptr = nlptr + 1;
    }
    if ( txts.size() < 1 ) return;

    bool last1st = pos > RightTop && pos < BelowLeft;
    ptr = last1st ? txts[txts.size()-1]->buf() : txts[0]->buf();

    uiLabel* labl = new uiLabel( this, ptr );
    lbls += labl;
    constraintType lblct = alignedBelow;
    switch ( pos )
    {
    case LeftTop:
	lb->attach( rightOf, labl );		lblct = rightAlignedBelow;
    break;
    case RightTop:
	labl->attach( rightOf, lb );		lblct = alignedBelow;
    break;
    case AboveLeft:
	lb->attach( alignedBelow, labl );	lblct = alignedAbove;
    break;
    case AboveMid:
	lb->attach( centeredBelow, labl );	lblct = centeredAbove;
    break;
    case AboveRight:
	lb->attach( rightAlignedBelow, labl );	lblct = rightAlignedAbove;
    break;
    case BelowLeft:
	labl->attach( alignedBelow, lb );	lblct = alignedBelow;
    break;
    case BelowMid:
	labl->attach( centeredBelow, lb );	lblct = centeredBelow;
    break;
    case BelowRight:
	labl->attach( rightAlignedBelow, lb );	lblct = rightAlignedBelow;
    break;
    }

    int nrleft = txts.size() - 1;
    while ( nrleft )
    {
	uiLabel* cur = new uiLabel( this, (last1st
			? txts[nrleft-1] : txts[txts.size()-nrleft])->buf() );
	cur->attach( lblct, labl );
	lbls += cur;
	labl = cur;
	nrleft--;
    }

    deepErase( txts );
}
