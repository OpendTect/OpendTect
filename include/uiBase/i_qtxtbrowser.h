#ifndef i_qtxtbrowser_h
#define i_qtxtbrowser_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          13/03/2002
 RCS:           $Id: i_qtxtbrowser.h,v 1.1 2002-03-18 15:28:57 arend Exp $
________________________________________________________________________

-*/

#include <uitextedit.h>

#include <qobject.h>
#include <qwidget.h>
#include <qtextbrowser.h> 

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
			i_BrowserMessenger( QTextBrowser*  sender,
					    uiTextBrowser* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( sender, SIGNAL( backwardAvailable(bool)),
				     this,   SLOT( backwardAvailable(bool)) );
			    connect(sender,SIGNAL(forwardAvailable(bool)),
				     this, SLOT(forwardAvailable(bool)));
			    connect(sender,SIGNAL(highlighted(const QString&)),
				     this, SLOT(highlighted(const QString&)));
			    connect(sender,SIGNAL(linkClicked(const QString&)),
				     this, SLOT(linkClicked(const QString&)));
			}

    virtual		~i_BrowserMessenger() {}
   
private:

    uiTextBrowser* 	_receiver;
    QTextBrowser*  	_sender;

private slots:

    void 		backwardAvailable(bool yn)
			{ 
			    _receiver->cangobackw_=yn;
			    _receiver->goneforwardorback.trigger(*_receiver); 
			}

    void 		forwardAvailable(bool yn)
			{ 
			    _receiver->cangoforw_=yn;
			    _receiver->goneforwardorback.trigger(*_receiver); 
			}

    void 		highlighted(const QString& lnk)
			{ 
			    _receiver->lastlink_ = (const char*)lnk;
			    _receiver->linkhighlighted.trigger(*_receiver); 
			}

    void 		linkClicked(const QString& lnk)
			{ 
			    _receiver->lastlink_ = (const char*)lnk;
			    _receiver->linkclicked.trigger(*_receiver); 
			}

};

#endif
