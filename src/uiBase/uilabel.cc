/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          7/9/2000
________________________________________________________________________

-*/


#include "uilabel.h"

#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uistring.h"

#include "q_uiimpl.h"

#include <QLabel>

mUseQtnamespace

class uiLabelBody : public uiObjBodyImpl<uiLabel,QLabel>
{
public:

uiLabelBody( uiLabel& hndle, uiParent* parnt, const uiString& txt )
    : uiObjBodyImpl<uiLabel,QLabel>(hndle,parnt,txt.getOriginalString())
{}

virtual int nrTxtLines() const
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
{
    init( txt, buddy );
}


void uiLabel::init( const uiString& txt, uiObject* buddy )
{
    isrequired_ = false;
    horalign_ = OD::Alignment::HCenter;

#ifdef __mac__
    // Overcome QMacStyles setting of fonts, which is not in line with
    // our layout
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


void uiLabel::setText( const uiString& txt )
{
    if ( text_ == txt )
	return;

    const bool wasempty = text_.isEmpty();
    text_ = txt;

    QString qstr = toQString( text_ );
    if ( isrequired_ )
	addRequiredChar( qstr );

    if ( !wasempty )
    {
	// body_->adjustSize();
        setAlignment( horalign_ );
    }
    body_->setText( qstr );
    setName( text_.getOriginalString() );

}


void uiLabel::makeRequired( bool yn )
{
    isrequired_ = yn;
    QString qstr = toQString( text_ );
    if ( qstr.isEmpty() )
	return;

    if ( isrequired_ )
	addRequiredChar( qstr );
    body_->setText( qstr );
}


void uiLabel::translateText()
{
    uiObject::translateText();
    QString qstr = toQString( text_ );
    if ( isrequired_ ) addRequiredChar( qstr );
    body_->setText( qstr );
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
    if ( !pixmap.qpixmap() )
	return;

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


void uiLabel::setAlignment( OD::Alignment::HPos hal )
{
    horalign_ = hal;
    OD::Alignment al( horalign_, OD::Alignment::VCenter );
    body_->setAlignment( (Qt::AlignmentFlag)al.uiValue() );
}


OD::Alignment::HPos uiLabel::alignment() const
{
    return horalign_;
}
