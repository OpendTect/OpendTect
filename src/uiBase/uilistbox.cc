/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.cc,v 1.20 2001-09-17 21:25:01 bert Exp $
________________________________________________________________________

-*/

#include <uilistbox.h>
#include <uifont.h>
#include <uidobjset.h>
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
    setStretch( 1, isSingleLine() ? 0 : 1 );
}


/* TODO: over-ride uiObjectBody::preferredWidth() 
and uiObjectBody::preferredHeight() instead of sizeHint.

Do something like:

- determine preferred char-xxx, depending on items in list
- set pref_char_xxx (uiObjectBody)
- set pref_xxx to 0
- return uiObjectBody::preferredXxxx()

*/
//! over-rides QWidget::sizeHint()
QSize uiListBoxBody::sizeHint() const
{
    const uiFont* mFont = const_cast<uiListBoxBody*>(this)->uiObjHandle().font();
    if( !mFont ) { pErrMsg("uiObjHandle has no font!"); return QSize(); }

    // initialize to requested size or reasonable default size
    // reasonable sizes are 3 <= nrlines <= 7 , 20 <= nrchars <= 40.
    const int sz = count();
    int nrchars = fieldWdt ? fieldWdt : 20;
    int nrlines = nLines ? nLines : sz > 7 ? 7 : (sz < 3 ? 3 : sz);

    // if biggest string is over 20 chars, grow box to max 40 chars.
    const int fontavgpixwidth = mFont->avgWidth();
    const int maxwdth = 40 * fontavgpixwidth;
    int pixwidth = nrchars * fontavgpixwidth;
    if ( !fieldWdt )
    {
	QListBoxItem* itm = item( 0 );
	for ( int idx=0; itm; itm = item(++idx) )
	{
	    const int pixw = mFont->width( itm->text() ) + 2 * fontavgpixwidth;
	    if ( pixw > pixwidth )
		pixwidth = pixw > maxwdth ? maxwdth : pixw;
	}
    }

    const int extrasz = 2 * frameWidth();
    const int pixheight = mFont->height() * nrlines;
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
    const uiFont* mFont = const_cast<uiListBoxBody*>(this)->uiObjHandle().font();
    if( !mFont ) { pErrMsg("uiObjHandle has no font!"); return uiSize(); }

    int totHeight = mFont->height() * nLines;
    int totWidth  = mFont->maxWidth() * fieldWdt;

    return uiSize ( totWidth , totHeight );
}
