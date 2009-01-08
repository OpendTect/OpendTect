#ifndef uithumbwheel_h
#define uithumbwheel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2002
 RCS:           $Id: uithumbwheel.h,v 1.6 2009-01-08 10:32:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class uiThumbWheelBody;
class QEvent;

mClass uiThumbWheel : public uiObject
{
friend class		i_ThumbWheelMessenger;
public:

                        uiThumbWheel( uiParent*, const char* nm="uiThumbWheel",
				      bool hor=true);

    const char*		text() const;
    int 		getIntValue() const;
    float 		getValue() const;

    void		setText( const char* );
    void		setValue( int );
    void		setValue( float );

    float		lastMoveVal()		{ return lastmv; }

    Notifier<uiThumbWheel> wheelPressed;
    Notifier<uiThumbWheel> wheelMoved;
    Notifier<uiThumbWheel> wheelReleased;

    			//! Force activation in GUI thread
    void		activate(float angle);

    Notifier<uiThumbWheel> activatedone;

protected:

    bool		handleEvent(const QEvent*);
    float		activateangle_;
    float               lastmv;

private:

    mutable BufferString result;

    uiThumbWheelBody*	body_;
    uiThumbWheelBody&	mkbody(uiParent*, const char*,bool);

};

#endif
