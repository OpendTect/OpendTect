/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilabel.h"

#include "bufstringset.h"
#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uistring.h"

#include "perthreadrepos.h"

#include "q_uiimpl.h"

#include <QLabel>

mUseQtnamespace

class uiLabelBody : public uiObjBodyImpl<uiLabel,QLabel>
{
public:

uiLabelBody( uiLabel& hndle, uiParent* parnt, const uiString& txt )
    : uiObjBodyImpl<uiLabel,QLabel>(hndle,parnt,txt.getOriginalString())
{}

int nrTxtLines() const override
{
    int nrl = 1;
    BufferString str = text();
    const char* txt = str.buf();
    while ( txt && *txt )
	{ if ( *txt == '\n' ) nrl++; txt++; }

    return nrl;
}

};


uiLabel::uiLabel( uiParent* p, const uiString& txt )
    : uiObject(p,txt.getOriginalString(),mkbody(p,txt))
    , isrequired_(false)
{
    init( txt, 0 );
}


uiLabel::uiLabel( uiParent* p, const uiString& txt, uiGroup* grp )
    : uiObject(p,txt.getOriginalString(),mkbody(p,txt))
    , isrequired_(false)
{
    init( txt, grp ? grp->attachObj() : 0 );
}


uiLabel::uiLabel( uiParent* p, const uiString& txt, uiObject* buddy )
    : uiObject(p,txt.getOriginalString(),mkbody(p,txt))
    , isrequired_(false)
{
    init( txt, buddy );
}


uiLabel::~uiLabel()
{}


void uiLabel::init( const uiString& txt, uiObject* buddy )
{
//Overcome QMacStyles setting of fonts, which is not inline with
//our layout.
#ifdef __mac__
    setFont( uiFontList::getInst().get(FontData::Control) );
#endif

    setText( txt );
    setTextSelectable( true );

    const QString qstr = toQString(txt);
    const int nrnewlines = qstr.count( "\n" );
    if ( nrnewlines>0 )
	setPrefHeightInChar( nrnewlines+1 );

    if ( buddy )
    {
	body_->setBuddy( buddy->body()->qwidget() );
	buddy->attach( rightOf, this );
    }

    setStretch( 0, 0 );
}


uiLabelBody& uiLabel::mkbody( uiParent* p, const uiString& txt )
{
    body_= new uiLabelBody( *this, p, txt );
    return *body_;
}


static void addRequiredChar( QString& qstr )
{
    qstr.insert( 0, "<font color='red'><sup>*</sup></font>" );
}


void uiLabel::updateWidth()
{
    BufferStringSet strs; strs.unCat( text_.getFullString().buf() );
    if ( strs.size() != 1 )
	return;

    int lblwidth = body_->fontWidthFor( text_ ) + 1;
    if ( isrequired_ )
	lblwidth++;

    if ( !body_ || !body_->itemInited() )
    {
	const int prefwidth = prefHNrPics();
	setPrefWidth( mMAX(lblwidth,prefwidth) );
    }
}


void uiLabel::setText( const uiString& txt )
{
    text_ = txt;
    QString qstr = toQString(text_);
    if ( isrequired_ ) addRequiredChar( qstr );
    body_->setText( qstr );
    updateWidth();
    setName( text_.getOriginalString() );
}


void uiLabel::makeRequired( bool yn )
{
    isrequired_ = yn;
    QString qstr = toQString(text_);
    if ( qstr.isEmpty() ) return;

    if ( isrequired_ ) addRequiredChar( qstr );
    body_->setText( qstr );
    updateWidth();
}


void uiLabel::translateText()
{
    uiObject::translateText();
    QString qstr = toQString(text_);
    if ( isrequired_ ) addRequiredChar( qstr );
    body_->setText( qstr );
    updateWidth();
}


const uiString& uiLabel::text() const
{
    return text_;
}


void uiLabel::setTextSelectable( bool yn )
{
    body_->setTextInteractionFlags( yn ? Qt::TextSelectableByMouse :
					 Qt::NoTextInteraction );
}


void uiLabel::setPixmap( const uiPixmap& pixmap )
{
    if ( !pixmap.qpixmap() ) return;

    const uiFont& ft =
	uiFontList::getInst().get( FontData::key(FontData::Control) );
    const QPixmap pm = pixmap.qpixmap()->scaledToHeight( ft.height() );
    body_->setPixmap( pm );
    body_->setAlignment( Qt::AlignCenter );
}


void uiLabel::setIcon( const char* iconnm )
{
    setPixmap( uiPixmap(iconnm) );
}


void uiLabel::setAlignment( Alignment::HPos hal )
{
    Alignment al( hal, Alignment::VCenter );
    body_->setAlignment( (Qt::AlignmentFlag)al.uiValue() );
}


Alignment::HPos uiLabel::alignment() const
{
    Alignment al;
    al.setUiValue( (int)body_->alignment() );
    return al.hPos();
}
