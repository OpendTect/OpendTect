#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitextedit.h"

#include <QAbstractSlider>
#include <QTextEdit>
#include <QTextBrowser>


//! Helper class for uiTextEdit to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_TextEditMessenger : public QObject
{
    Q_OBJECT
    friend class	uiTextEditBody;

protected:

i_TextEditMessenger( QTextEdit* sndr, uiTextEditBase* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QTextEdit::textChanged,
	     this, &i_TextEditMessenger::textChanged );
    connect( sndr, &QTextEdit::copyAvailable,
	     this, &i_TextEditMessenger::copyAvailable );
}


~i_TextEditMessenger()
{}

private:

    QTextEdit*		sender_;
    uiTextEditBase*	receiver_;

private slots:

void textChanged()
{
    receiver_->textChanged.trigger( *receiver_ );
}

void copyAvailable( bool yn )
{
    receiver_->copyAvailable.trigger( yn );
}

};

class i_ScrollBarMessenger : public QObject
{
    Q_OBJECT;
public:

i_ScrollBarMessenger( QAbstractSlider* sndr, uiTextEditBase* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sender_,&QAbstractSlider::sliderPressed,
	     this, &i_ScrollBarMessenger::sliderPressed );
    connect( sender_,&QAbstractSlider::sliderReleased,
	     this, &i_ScrollBarMessenger::sliderReleased);
}


~i_ScrollBarMessenger()
{}

private:

    QAbstractSlider*	sender_;
    uiTextEditBase*	receiver_;

private slots:

void sliderPressed()
{
    receiver_->sliderPressed.trigger( *receiver_ );
}

void sliderReleased()
{
    receiver_->sliderReleased.trigger( *receiver_ );
}

};


//! Helper class for uiTextBrowser to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

class i_BrowserMessenger : public i_TextEditMessenger
{
Q_OBJECT
friend class uiTextBrowserBody;

protected:
i_BrowserMessenger( QTextBrowser* sndr, uiTextBrowser* rec )
    : i_TextEditMessenger(sndr,rec)
    , bsender_(sndr)
    , breceiver_(rec)
{
    connect( sndr, &QTextBrowser::backwardAvailable,
	     this,   &i_BrowserMessenger::backwardAvailable );
    connect( sndr, &QTextBrowser::forwardAvailable,
	     this, &i_BrowserMessenger::forwardAvailable);
    connect( sndr, QOverload<const QUrl &>::of(&QTextBrowser::highlighted),
	     this,&i_BrowserMessenger::highlighted );
    connect(sndr,&QTextBrowser::anchorClicked,
	    this,&i_BrowserMessenger::anchorClicked);
}


~i_BrowserMessenger()
{}

private:

    QTextBrowser*	bsender_;
    uiTextBrowser*	breceiver_;

private slots:

void backwardAvailable( bool yn )
{
    breceiver_->cangobackw_ = yn;
    breceiver_->goneForwardOrBack.trigger( *breceiver_ );
}

void forwardAvailable( bool yn )
{
    breceiver_->cangoforw_ = yn;
    breceiver_->goneForwardOrBack.trigger( *breceiver_ );
}

void highlighted( const QUrl& url )
{
    breceiver_->lastlink_ = url.toString();
    breceiver_->linkHighlighted.trigger( *breceiver_ );
}

void anchorClicked( const QUrl& lnk )
{
    breceiver_->lastlink_ = lnk.toString().toLatin1().data();
    breceiver_->linkClicked.trigger( *breceiver_ );
}

};

QT_END_NAMESPACE
