#ifndef uislider_h
#define uislider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.h,v 1.9 2003-11-07 12:21:54 bert Exp $
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
				 bool weditfld=false,int fact=1,bool log=false);

    const char*		text() const;
    int 		getIntValue() const;
    double 		getValue() const;

    void		setText(const char*);
    void		setValue(int);
    void		setValue(double);
    void		setTickMarks(bool yn=true);

    double		minValue() const;
    double		maxValue() const;
    void		setMinValue(double);
    void		setMaxValue(double);
    int			tickStep() const;
    void		setTickStep(int);

    void		setScaleFactor(int fact) 	{ factor = fact; }
    void		setLogScale(bool yn=true) 	{ logscale = yn; }
    bool		isLogScale()			{ return logscale; }
    int			getScaleFactor()		{ return factor; }

    void		processInput();

    Notifier<uiSlider>	valueChanged;
    Notifier<uiSlider>	sliderMoved;

private:

    mutable BufferString result;
    int			factor;
    bool		logscale;

    uiLineEdit*		editfld;

    uiSliderBody*	body_;
    uiSliderBody&	mkbody(uiParent*, const char*);

    void		editValue(CallBacker*);
    void		sliderMove(CallBacker*);

    double		getRealValue(int) const;
    int			getSliderValue(double) const;

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

    uiSlider*		slider;
    uiLabel*    	lbl;

};


#endif
