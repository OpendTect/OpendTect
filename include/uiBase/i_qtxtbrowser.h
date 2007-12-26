#ifndef i_qtxtbrowser_h
#define i_qtxtbrowser_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/03/2002
 RCS:           $Id: i_qtxtbrowser.h,v 1.6 2007-12-26 07:09:43 cvsnanne Exp $
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
			i_BrowserMessenger( QTextBrowser* sender,
					    uiTextBrowser* receiver )
			: sender_( sender )
			, receiver_( receiver )
			{ 
			    connect( sender, SIGNAL(backwardAvailable(bool)),
				     this,   SLOT(backwardAvailable(bool)) );
			    connect(sender,SIGNAL(forwardAvailable(bool)),
				     this, SLOT(forwardAvailable(bool)));
			    connect(sender,SIGNAL(highlighted(const QString&)),
				     this, SLOT(highlighted(const QString&)));
			    connect(sender,SIGNAL(anchorClicked(const QUrl&)),
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
			    receiver_->lastlink_ = lnk.path().toAscii().data();
			    receiver_->linkClicked.trigger(*receiver_);
			}
};

#endif
