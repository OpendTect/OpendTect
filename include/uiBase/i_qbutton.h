#ifndef i_qbutton_H
#define i_qbutton_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: i_qbutton.h,v 1.6 2001-06-07 21:22:49 windev Exp $
________________________________________________________________________

-*/

#include <qobject.h>
#include <qbutton.h>

#include <uibutton.h>
#include <i_qobjwrap.h>

#ifdef __msvc__
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>
#include <qbuttongroup.h>
#endif

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
                                , _sender( sender )
				{}

    void 			do_connect()
				{
				    connect( _sender, SIGNAL( clicked() ), 
					     this,   SLOT( clicked() ) );
				    connect( _sender, SIGNAL( pressed() ), 
					     this,   SLOT( pressed() ) );
				    connect( _sender, SIGNAL( released() ), 
					     this,   SLOT( released() ) );
				    connect( _sender, SIGNAL(toggled(bool)), 
					     this,   SLOT(toggled(bool)) );
				    connect( _sender,SIGNAL(stateChanged(int)), 
					     this,   SLOT(stateChanged(int)) );
				}

private:

    uiButton*			_receiver;
    QButton*			_sender;

public slots:

    void toggled( bool ) 	
		{ _receiver->notifyHandler( uiButton::toggled ); }
    void stateChanged( int ) 	
		{ _receiver->notifyHandler( uiButton::stateChanged ); }
    void clicked() 		
		{ _receiver->notifyHandler( uiButton::clicked ); }
    void pressed() 		
		{ _receiver->notifyHandler( uiButton::pressed ); }
    void released()		
		{ _receiver->notifyHandler( uiButton::released); }

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
                                        uiParent* parnt=0, const char* name=0 )
			    : i_QObjWrapper<T>( client, parnt, name )
			    , _messenger ( this, &client )
			    , idInGroup(0) 
			    { _messenger.do_connect(); }

                        i_QButtWrapper( uiButton& client,
                                       const char* name, uiButtonGroup* parnt )
			    : i_QObjWrapper<T>( client, name, parnt )
			    , _messenger ( this, &client )
			    , idInGroup(0)
			    { _messenger.do_connect(); }

protected:

    i_ButMessenger      _messenger;
    int			idInGroup;

};

#endif
