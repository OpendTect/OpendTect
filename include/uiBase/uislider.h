#ifndef uislider_h
#define uislider_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.h,v 1.2 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class uiSliderBody;

class uiSlider : public uiObject
{
public:

                        uiSlider(uiParent*, const char* nm="Line Edit");

    const char*		text() const;
    int 		getIntValue() const;
    double 		getValue() const;

    void		setText( const char* );
    void		setValue( int );
    void		setValue( double );
    void		setTickMarks( bool yn=true);

    Notifier<uiSlider>	valueChanged;
    Notifier<uiSlider>	sliderMoved;

private:

    mutable BufferString result;

    uiSliderBody*	body_;
    uiSliderBody&	mkbody(uiParent*, const char*);

};

#endif
