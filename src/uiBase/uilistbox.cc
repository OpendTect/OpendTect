/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.cc,v 1.29 2002-01-01 23:05:15 bert Exp $
________________________________________________________________________

-*/

#include <uilistbox.h>
#include <uifont.h>
#include <uidobjset.h>
#include <uilabel.h>
#include <uiobjbody.h>

#include <i_qlistbox.h>

#include <qsize.h> 



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

    void 		setLines( int prefNrLines )
			{ 
			    if(prefNrLines >= 0) nLines=prefNrLines;
//			    setStretch( 1, isSingleLine() ? 0 : 2 );
			    setStretch( 1, isSingleLine() ? 0 : 1 );
			}

    virtual uiSize	minimumSize() const; //!< \reimp
    virtual bool	isSingleLine() const		 { return nLines==1; }

    QSize 		sizeHint() const;

protected:

    int 		fieldWdt;
    int 		nLines;

private:

    i_listMessenger&    messenger_;

};



uiListBoxBody::uiListBoxBody( uiListBox& handle, uiParent* parnt, 
			const char* nm, bool isMultiSelect,
			int preferredNrLines, int preferredFieldWidth )
	: uiObjBodyImpl<uiListBox,QListBox>( handle, parnt, nm )
	, messenger_ (*new i_listMessenger(this, &handle))
	, fieldWdt(preferredFieldWidth)
	, nLines(preferredNrLines)
{
    if( isMultiSelect ) setSelectionMode( QListBox::Extended );
    setStretch( 1, isSingleLine() ? 0 : 2 );
}

/* TODO: over-ride uiObjectBody::prefHNrPics() 
and uiObjectBody::prefVNrPics() instead of sizeHint.

Do something like:

- determine preferred char-xxx, depending on items in list
- set pref_char_xxx (uiObjectBody)
- set pref_xxx to 0
- return uiObjectBody::preferredXxxx()

*/
//! over-rides QWidget::sizeHint()
QSize uiListBoxBody::sizeHint() const
{

    // initialize to requested size or reasonable default size
    // reasonable sizes are 3 <= nrlines <= 7 , 20 <= nrchars <= 40.
    const int sz = count();
    int nrchars = fieldWdt ? fieldWdt : 20;
    int nrlines = nLines ? nLines : sz > 7 ? 7 : (sz < 3 ? 3 : sz);

    // if biggest string is over 20 chars, grow box to max 40 chars.
    const int fontavgpixwidth = fontWdt();
    const int maxwdth = 40 * fontavgpixwidth;
    int pixwidth = nrchars * fontavgpixwidth;
    if ( !fieldWdt )
    {
	QListBoxItem* itm = item( 0 );
	for ( int idx=0; itm; itm = item(++idx) )
	{
	    const int pixw = fontWdtFor( itm->text() ) + 2 * fontavgpixwidth;
	    if ( pixw > pixwidth )
		pixwidth = pixw > maxwdth ? maxwdth : pixw;
	}
    }

    const int extrasz = 2 * frameWidth();
    const int pixheight = fontHgt() * nrlines;
    return QSize ( pixwidth+extrasz, pixheight+extrasz );
}


uiListBox::uiListBox( uiParent* p, const char* nm, bool ms, int nl, int pfw)
    : uiObject( p, nm, mkbody(p,nm,ms,nl,pfw) )
    , selectionChanged( this )
    , doubleClicked( this )
    , rightButtonClicked( this )
    , lastClicked_( -1 )
{}


uiListBox::uiListBox( uiParent* p, const PtrUserIDObjectSet& uids,
		      bool ms, int nl, int pfw )
    : uiObject( p, (const char*)uids->name(), 
		mkbody(p,(const char*)uids->name(), ms,nl,pfw))
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


void uiListBox::setLines( int prefNrLines )
    { body_->setLines(prefNrLines); }


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


void uiListBox::setSelected( int idx, bool yn )
{
    if ( idx >= 0 && idx < body_->count() )
	body_->setSelected( idx, yn );
}


void uiListBox::selAll( bool yn )
{
    if ( body_->selectionMode() != QListBox::Extended ) return;

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
	body_->setSelected( idx, yn );
}


void uiListBox::addItem( const char* text, bool embed ) 
{
    if ( !embed )
	body_->insertItem( QString(text?text:""), -1 );
    else
    {
	BufferString s( "[" ); s += text; s += "]";
	body_->insertItem( QString(s), -1 );
    }
}


void uiListBox::addItems( const char** textList ) 
{
    const char* pt_cur = *textList;
    while ( pt_cur )
        addItem( pt_cur++ );
}


void uiListBox::addItems( const PtrUserIDObjectSet& uids )
{
    int curidx = currentItem();
    if ( uids.currentIndex() >= 0 ) curidx = size() + uids.currentIndex();
    for ( int idx=0; idx<uids.size(); idx++ )
	body_->insertItem( QString( uids[idx]->name() ), -1 );
    setCurrentItem( curidx );
}


void uiListBox::empty()
{
    body_->QListBox::clear();
}


bool uiListBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	if ( body_->text(idx) == txt ) return true;
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
    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( body_->text(idx) == txt )
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


uiSize uiListBoxBody::minimumSize() const
{ 
#if 0 
    const uiFont* mFont = const_cast<uiListBoxBody*>(this)->uiObjHandle().font();
    if( !mFont ) { pErrMsg("uiObjHandle has no font!"); return uiSize(); }

    int totHeight = mFont->height() * nLines;
    int totWidth  = mFont->maxWidth() * fieldWdt;
#else
    int totHeight = fontHgt() * nLines;
    int totWidth  = fontWdt( true ) * fieldWdt;
#endif
    return uiSize ( totWidth , totHeight );
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const char* txt, bool multisel,
				    uiLabeledListBox::LblPos pos )
	: uiGroup(p,"Labeled listbox")
{
    lb = new uiListBox( this, txt, multisel );
    mkRest( txt, pos );
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const PtrUserIDObjectSet& s,
				    bool multisel, uiLabeledListBox::LblPos pos)
	: uiGroup(p,"Labeled listbox")
{
    lb = new uiListBox( this, s, multisel );
    mkRest( s->name(), pos );
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

    ObjectSet<BufferString> txts;
    BufferString s( txt );
    char* ptr = s.buf();
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
