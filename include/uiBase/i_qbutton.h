#ifndef i_qbutton_H
#define i_qbutton_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: i_qbutton.h,v 1.1 2000-11-27 10:19:26 bert Exp $
________________________________________________________________________

-*/

#include <qobject.h>
#include <qbutton.h>

#include <uibutton.h>
#include <i_qobjwrap.h>

//! Help class, because templates can not use signals/slots
/*!
    Relays QT button signals to the notifyHandler of a uiButton object.
*/
class i_ButMessenger : public QObject 
{ 
    Q_OBJECT
    friend class                uiButton;
public:
				i_ButMessenger( QButton*  sender,
                                       	  	uiButton* receiver )
                                : _receiver( receiver )
				{
				    connect( sender, SIGNAL( pressed() ), 
					     this,   SLOT( pressed() ) );
				    connect( sender, SIGNAL( released() ), 
					     this,   SLOT( released() ) );
				    connect( sender, SIGNAL(toggled(bool)), 
					     this,   SLOT(toggled(bool)) );
				    connect( sender, SIGNAL(stateChanged(int)), 
					     this,   SLOT(stateChanged(int)) );
				}

private:

    uiButton* _receiver;

private slots:

    void pressed() 		
		{ return _receiver->notifyHandler( uiButton::pressed ); }
    void released()		
		{ return _receiver->notifyHandler( uiButton::released); }
    void toggled( bool ) 	
		{ return _receiver->notifyHandler( uiButton::toggled ); }
    void stateChanged( int ) 	
		{ return _receiver->notifyHandler( uiButton::stateChanged ); }

};


//! Wrapper around QButtons. 
/*!
    Extends each Qbutton class <T> with a i_ButMessenger, which connects itself 
    to the signals transmitted from Qt buttons.
    Each signal is relayed to the notifyHandler of a uiButton client object.
*/
template <class T>
class i_QButtWrapper : public i_QObjWrapper<T>
{
public:
                        i_QButtWrapper( uiButton& client,
                                        uiObject* parnt=0, const char* name=0 )
                        : i_QObjWrapper<T>( client, parnt, name )
                        , _messenger ( this, &client )
                        , idInGroup(0) {}
                        i_QButtWrapper( uiButton& client,
                                       const char* name, uiButtonGroup* parnt )
                        : i_QObjWrapper<T>( client, name, parnt )
                        , _messenger ( this, &client )
                        , idInGroup(0) {}

protected:

    i_ButMessenger      _messenger;
    int			idInGroup;

};

#endif
