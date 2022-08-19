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

i_TextEditMessenger( QTextEdit* sndr, uiTextEditBase* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(textChanged()), this, SLOT(textChanged()) );
    connect( sndr, SIGNAL(copyAvailable(bool)),this,SLOT(copyAvailable(bool)));
}

private:

    uiTextEditBase* receiver_;
    QTextEdit*	sender_;

private slots:

void textChanged()
{ receiver_->textChanged.trigger( *receiver_ ); }

void copyAvailable( bool yn )
{ receiver_->copyAvailable.trigger( yn ); }

};

class i_ScrollBarMessenger : public QObject
{
    Q_OBJECT;
public:

    i_ScrollBarMessenger( QAbstractSlider* sndr, uiTextEditBase* receiver )
	: sender_(sndr)
	, receiver_(receiver)
    {
	connect(sender_,SIGNAL(sliderPressed()), this, SLOT(sliderPressed()) );
	connect(sender_,SIGNAL(sliderReleased()), this, SLOT(sliderReleased()));
    }

private:

    uiTextEditBase*	receiver_;
    QAbstractSlider*	sender_;

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
    friend class	uiTextBrowserBody;

protected:
			i_BrowserMessenger( QTextBrowser* sndr,
					    uiTextBrowser* receiver )
			    : i_TextEditMessenger(sndr,receiver)
			    , bsender_( sndr )
			    , breceiver_( receiver )
			{
			    connect( sndr, SIGNAL(backwardAvailable(bool)),
				     this,   SLOT(backwardAvailable(bool)) );
			    connect(sndr,SIGNAL(forwardAvailable(bool)),
				     this, SLOT(forwardAvailable(bool)));
			    connect(sndr,SIGNAL(highlighted(const QString&)),
				     this, SLOT(highlighted(const QString&)));
			    connect(sndr,SIGNAL(anchorClicked(const QUrl&)),
				    this,SLOT(anchorClicked(const QUrl&)));
			}

    virtual		~i_BrowserMessenger() {}

private:

    uiTextBrowser*	breceiver_;
    QTextBrowser*	bsender_;

private slots:

    void	backwardAvailable( bool yn )
		{
		    breceiver_->cangobackw_ = yn;
		    breceiver_->goneForwardOrBack.trigger( *breceiver_ );
		}

    void	forwardAvailable( bool yn )
		{
		    breceiver_->cangoforw_ = yn;
		    breceiver_->goneForwardOrBack.trigger( *breceiver_ );
		}

    void	highlighted( const QString& lnk )
		{
		    breceiver_->lastlink_ = lnk.toLatin1().data();
		    breceiver_->linkHighlighted.trigger( *breceiver_ );
		}

    void	anchorClicked( const QUrl& lnk )
		{
		    breceiver_->lastlink_ = lnk.toString().toLatin1().data();
		    breceiver_->linkClicked.trigger( *breceiver_ );
		}
};

QT_END_NAMESPACE
