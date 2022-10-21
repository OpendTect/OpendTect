#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uigroup.h"
#include "uiobj.h"
#include "uistring.h"
#include "odcommonenums.h"

class LinScaler;

class uiLabel;
class uiLineEdit;
class uiSliderBody;
class uiSpinBox;


mExpClass(uiBase) uiSliderObj : public uiObject
{
public:
			uiSliderObj(uiParent*,const char* nm);
			~uiSliderObj();

    uiSliderBody&	body()		{ return *body_; }

private:
    uiSliderBody*	body_;
    uiSliderBody&	mkbody(uiParent*,const char*);
};


mExpClass(uiBase) uiSlider : public uiGroup
{
public:
    mExpClass(uiBase) Setup
    {
    public:
			Setup(const uiString& l=uiString::emptyString())
			    : lbl_(l)
			    , withedit_(false)
			    , nrdec_(0)
			    , logscale_(false)
			    , isvertical_(false)
			    , sldrsize_(200)
			    , isinverted_(false)
			    {}
			~Setup()
			{}

	mDefSetupMemb(bool,withedit)
	mDefSetupMemb(bool,logscale)
	mDefSetupMemb(bool,isvertical)
	mDefSetupMemb(int,nrdec)
	mDefSetupMemb(int,sldrsize)
	mDefSetupMemb(bool,isinverted)
	mDefSetupMemb(uiString,lbl)
    };

			uiSlider(uiParent*,const Setup&,const char* nm=0);
			~uiSlider();

    enum		TickPosition { NoMarks=0, Above=1, Left=Above, Below=2,
				      Right=Below, Both=3 };

    void		processInput();
    void		setToolTip(const uiString&);
    void		setText(const char*);
    const char*		text() const;

    void		setValue(int);
    void		setValue(float);
    int			getIntValue() const;
    float		getFValue() const;
    float		editValue() const;

    void		setMinValue(float);
    float		minValue() const;
    void		setMaxValue(float);
    float		maxValue() const;
    void		setStep(float);
    void		setScale(float fact,float constant);
    float		step() const;

    void		setInterval(const StepInterval<int>&);
    void		setInterval(int start,int stop,int step=1);
    void		setInterval(const StepInterval<float>&);
    void		setInterval(float start,float stop,float step);
    void		getInterval(StepInterval<float>&) const;
    void		setLinearScale(double,double);

    void		setTickMarks(TickPosition);
    TickPosition	tickMarks() const;
    void		setTickStep(int);
    int			tickStep() const;
    void		setOrientation(OD::Orientation);
    OD::Orientation	getOrientation() const;
    void		setPageStep(int);
    int			pageStep() const;

    void		setInverted(bool);
    bool		isInverted() const;
    void		setInvertedControls(bool);
    bool		hasInvertedControls() const;

    bool		isLogScale()			{ return logscale_; }
    void		setNrDecimalsEditFld(int);

    Notifier<uiSlider>	valueChanged;
    Notifier<uiSlider>	sliderMoved;
    Notifier<uiSlider>	sliderPressed;
    Notifier<uiSlider>	sliderReleased;

    float		getLinearFraction() const;
    void		setLinearFraction(float fraction);

    const uiLabel*	label() const			{ return lbl_; }
    uiLabel*		label()				{ return lbl_; }

    uiSliderObj*	slider()			{ return slider_; }

private:

    uiSliderObj*	slider_;
    uiLabel*		lbl_;
    uiLineEdit*		editfld_;

    mutable BufferString result_;
    LinScaler*		scaler_;
    bool		logscale_;

    void		init(const Setup&,const char*);

    void		sliderMove(CallBacker*);
    void		editRetPress(CallBacker*);

    float		userValue(int) const;
    int			sliderValue(float) const;

public:
    mDeprecated		("Use getFValue")
    float		getValue() const	{ return getFValue(); }
};
