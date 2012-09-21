#ifndef uislider_h
#define uislider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigroup.h"
#include "uiobj.h"

class LinScaler;
class uiSliderBody;
class uiLabel;
class uiLineEdit;
template <class T> class StepInterval;

mClass(uiBase) uiSlider : public uiObject
{
public:

                        uiSlider(uiParent*,const char* nm="Slider",
				 int nrdec=0,bool log=false, bool vert=false);
			~uiSlider();

    enum 		TickPosition { NoMarks=0, Above=1, Left=Above, Below=2, 
				      Right=Below, Both=3 };
    enum		Orientation { Horizontal, Vertical };

    void		setText(const char*);
    const char*		text() const;

    void		setValue(float);
    int 		getIntValue() const;
    float 		getValue() const;

    void		setMinValue(float);
    float		minValue() const;
    void		setMaxValue(float);
    float		maxValue() const;
    void		setStep(float);
    void		setScale(float fact,float constant);
    float		step() const;
    void		setInterval(const StepInterval<float>&);
    void		getInterval(StepInterval<float>&) const;
    void		setLinearScale(double,double);

    void		setTickMarks(TickPosition);
    TickPosition	tickMarks() const;
    void		setTickStep(int);
    int			tickStep() const;
    void		setOrientation(Orientation);
    uiSlider::Orientation getOrientation() const;

    void		setInverted(bool);
    bool		isInverted() const;
    void		setInvertedControls(bool);
    bool		hasInvertedControls() const;

    bool		isLogScale()			{ return logscale; }

    Notifier<uiSlider>	valueChanged;
    Notifier<uiSlider>	sliderMoved;
    Notifier<uiSlider>	sliderPressed;
    Notifier<uiSlider>	sliderReleased;
    
    float		getLinearFraction() const;
    void		setLinearFraction(float fraction);

private:

    mutable BufferString result;
    LinScaler*		scaler_;
    bool		logscale;

    uiSliderBody*	body_;
    uiSliderBody&	mkbody(uiParent*,const char*);

    void		sliderMove(CallBacker*);

    float		userValue(int) const;
    int			sliderValue(float) const;
};

/*! Slider with label */

mClass(uiBase) uiSliderExtra : public uiGroup
{
public:

    mClass(uiBase) Setup
    {
    public:
			Setup(const char* l=0)
			    : lbl_(l)
			    , withedit_(false)
			    , nrdec_(0)
			    , logscale_(false)
			    , isvertical_(false)
			    , sldrsize_(200)
			    {}

	mDefSetupMemb(bool,withedit)
	mDefSetupMemb(bool,logscale)
	mDefSetupMemb(bool,isvertical)
	mDefSetupMemb(int,nrdec)
	mDefSetupMemb(int,sldrsize)
	mDefSetupMemb(BufferString,lbl)
    };

                	uiSliderExtra(uiParent*,const Setup&,
				      const char* nm=0);

    uiSlider*		sldr()			{ return slider; }
    uiLabel*		label()			{ return lbl; }

    void		processInput();
    float		editValue() const;
    			//!<The val in the ed field, which may be outside range

protected:

    uiSlider*		slider;
    uiLabel*    	lbl;
    uiLineEdit*		editfld;

    void		init(const Setup&,const char*);
    void		editRetPress(CallBacker*);
    void		sliderMove(CallBacker*);
};


#endif

