#ifndef uithumbwheel_h
#define uithumbwheel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2002
 RCS:           $Id: uithumbwheel.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class uiThumbWheelBody;

class uiThumbWheel : public uiObject
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

    int			minValue() const;
    int			maxValue() const;
    void		setMinValue( int );
    void		setMaxValue( int );
    int			step() const;
    void		setStep(int);

    float		lastMoveVal()		{ return lastmv; }

    Notifier<uiThumbWheel> wheelPressed;
    Notifier<uiThumbWheel> wheelMoved;
    Notifier<uiThumbWheel> wheelReleased;

protected:

    float               lastmv;

private:

    mutable BufferString result;

    uiThumbWheelBody*	body_;
    uiThumbWheelBody&	mkbody(uiParent*, const char*,bool);

};

#endif
