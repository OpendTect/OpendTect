#ifndef i_qlineedit_h
#define i_qlineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qlineedit.h,v 1.3 2003-11-07 12:21:53 bert Exp $
________________________________________________________________________

-*/

#include <uilineedit.h>

#include <qobject.h>
#include <qwidget.h>
#include <qlineedit.h> 

class QString;

//! Helper class for uilineedit to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_lineEditMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiLineEditBody;

protected:
			i_lineEditMessenger( QLineEdit*  sender,
					     uiLineEdit* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( sender, SIGNAL( returnPressed()),
				     this,   SLOT( returnPressed()) );
			    connect(sender,SIGNAL(textChanged(const QString&)),
				     this, SLOT(textChanged(const QString& )));
			}

    virtual		~i_lineEditMessenger() {}
   
private:

    uiLineEdit* 	_receiver;
    QLineEdit*  	_sender;

private slots:

    void 		returnPressed() 
			{ _receiver->returnPressed.trigger(*_receiver); }
    void 		textChanged(const QString&)
			{ _receiver->textChanged.trigger(*_receiver); }

};

#endif
