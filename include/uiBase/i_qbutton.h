#ifndef i_qbutton_h
#define i_qbutton_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: i_qbutton.h,v 1.10 2006-04-28 15:23:20 cvsnanne Exp $
________________________________________________________________________

-*/

#include <qobject.h>
#include <uibutton.h>

#ifdef USEQT4
# define mButton QAbstractButton
# include <qabstractbutton.h>
#else
# define mButton QButton
# include <qbutton.h>
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
				i_ButMessenger( mButton*  sender,
                                       	  	uiButtonBody* receiver )
                                : _receiver( receiver )
                                , _sender( sender )
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

    uiButtonBody*		_receiver;
    mButton*			_sender;

public slots:

    void toggled( bool ) 	
		{ _receiver->notifyHandler( uiButtonBody::toggled ); }
    void stateChanged( int ) 	
		{ _receiver->notifyHandler( uiButtonBody::stateChanged ); }
    void clicked() 		
		{ _receiver->notifyHandler( uiButtonBody::clicked ); }
    void pressed() 		
		{ _receiver->notifyHandler( uiButtonBody::pressed ); }
    void released()		
		{ _receiver->notifyHandler( uiButtonBody::released); }

};

#endif
