#ifndef uislider_h
#define uislider_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.h,v 1.6 2002-04-16 12:50:04 nanne Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <uigroup.h>

class uiSliderBody;
class uiLabel;
class uiLineEdit;

class uiSlider : public uiObject
{
public:

                        uiSlider(uiParent*, const char* nm="Slider",
				 bool weditfld=false);

    const char*		text() const;
    int 		getIntValue() const;
    double 		getValue() const;

    void		setText( const char* );
    void		setValue( int );
    void		setValue( double );
    void		setTickMarks( bool yn=true);

    int			minValue() const;
    int			maxValue() const;
    void		setMinValue( int );
    void		setMaxValue( int );
    int			tickStep() const;
    void		setTickStep( int );

    uiLineEdit*		editfld;

    Notifier<uiSlider>	valueChanged;
    Notifier<uiSlider>	sliderMoved;

private:

    mutable BufferString result;

    uiSliderBody*	body_;
    uiSliderBody&	mkbody(uiParent*, const char*);

    void		editValue(CallBacker*);
    void		sliderMove(CallBacker*);

};


class uiLabeledSlider : public uiGroup
{
public:
                uiLabeledSlider( uiParent*,const char* txt,
                                 const char* nm="Labeled Slider",
				 bool weditfld=false);

    uiSlider*		sldr()			{ return slider; }
    uiLabel*		label()			{ return lbl; }

protected:

    uiSlider*	slider;
    uiLabel*    lbl;

};


#endif
