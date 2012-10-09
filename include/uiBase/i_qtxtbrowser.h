#ifndef i_qtxtbrowser_h
#define i_qtxtbrowser_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          13/03/2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitextedit.h"

#include <QObject>
#include <QTextBrowser> 
#include <QWidget>


class QString;

//! Helper class for uiTextBrowser to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

class i_BrowserMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiTextBrowserBody;

protected:
			i_BrowserMessenger( QTextBrowser* sndr,
					    uiTextBrowser* receiver )
			: sender_( sndr )
			, receiver_( receiver )
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

    uiTextBrowser* 	receiver_;
    QTextBrowser* 	sender_;

private slots:

    void 		backwardAvailable( bool yn )
			{ 
			    receiver_->cangobackw_ = yn;
			    receiver_->goneForwardOrBack.trigger(*receiver_); 
			}

    void 		forwardAvailable( bool yn )
			{ 
			    receiver_->cangoforw_ = yn;
			    receiver_->goneForwardOrBack.trigger(*receiver_); 
			}

    void 		highlighted( const QString& lnk )
			{ 
			    receiver_->lastlink_ = lnk.toAscii().data();
			    receiver_->linkHighlighted.trigger(*receiver_); 
			}

    void		anchorClicked( const QUrl& lnk )
			{
			    receiver_->lastlink_ = lnk.toString().toAscii().data();
			    receiver_->linkClicked.trigger(*receiver_);
			}
};

#endif
