#ifndef uithumbwheel_h
#define uithumbwheel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2002
 RCS:           $Id: uithumbwheel.h,v 1.2 2002-02-13 10:42:31 arend Exp $
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
