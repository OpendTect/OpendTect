/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.cc,v 1.26 2001-12-19 14:56:09 arend Exp $
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


void uiListBox::addItem( const char* text ) 
{ 
    body_->insertItem( QString( text ) , -1 );
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


const char* uiListBox::textOfItem( int idx ) const
{
    if ( idx < 0 || idx >= body_->count() ) return "";
    rettxt = (const char*)body_->text(idx);
    return (const char*)rettxt;
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


void uiLabeledListBox::mkRest( const char* txt, uiLabeledListBox::LblPos pos )
{
    setHAlignObj( lb );

    ObjectSet<BufferString> txts;
    BufferString s( txt );
    char* ptr = s.buf();
    while ( 1 )
    {
	txts += new BufferString( ptr );
	ptr = strchr( ptr, '\n' );
	if ( !ptr ) break;

	*ptr++ = '\0';
    }
    if ( txts.size() < 1 ) return;

    bool last1st = pos > RightTop && pos < BelowLeft;
    ptr = last1st ? txts[txts.size()-1]->buf() : txts[0]->buf();

    labl = new uiLabel( this, ptr );
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
    uiLabel* last = labl;
    while ( nrleft )
    {
	uiLabel* cur = new uiLabel( this, (last1st
			? txts[nrleft-1] : txts[txts.size()-nrleft])->buf() );
	cur->attach( lblct, last );
	last = cur;
    }

    deepErase( txts );
}
