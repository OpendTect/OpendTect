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
#include "odcommonenums.h"

class uiDialBody;

class uiLabel;
class uiLineEdit;

mExpClass(uiBase) uiDial : public uiObject
{
public:
			uiDial(uiParent*,const char* nm="Dial");
			~uiDial();
			mOD_DisableCopy(uiDial)

    void		setValue(int);
    int			getValue() const;

    void		setMinValue(int);
    int			minValue() const;
    void		setMaxValue(int);
    int			maxValue() const;
    void		setStep(int);
    int			step() const;

    void		setInterval(const StepInterval<int>&);
    void		getInterval(StepInterval<int>&) const;

    void		setOrientation(OD::Orientation);
    OD::Orientation	getOrientation() const;

    void		setInverted(bool);
    bool		isInverted() const;
    void		setInvertedControls(bool);
    bool		hasInvertedControls() const;
    void		setWrapping(bool);
    bool		hasWrapping() const;
    void		setStartAtTop(bool);
    bool		hasStartAtTop() const;

    Notifier<uiDial>	valueChanged;
    Notifier<uiDial>	sliderMoved;
    Notifier<uiDial>	sliderPressed;
    Notifier<uiDial>	sliderReleased;

private:

    uiDialBody*		body_;
    uiDialBody&		mkbody(uiParent*,const char*);

    bool		startAtTop_;
			// true - numbering starts at the top
};

/*! Dial with label */
mExpClass(uiBase) uiDialExtra : public uiGroup
{
public:

    mExpClass(uiBase) Setup
    {
	public:
			Setup(const uiString& l)
			    : lbl_(l)
			    , withedit_(false)
			    , isvertical_(false)
			    , dialsize_(60)
			{}
			~Setup()
			{}

		mDefSetupMemb(bool,withedit)
		mDefSetupMemb(bool,isvertical)
		mDefSetupMemb(int,dialsize)
		mDefSetupMemb(uiString,lbl)
    };

			uiDialExtra(uiParent*,const Setup&, const char* nm);
			~uiDialExtra();
			mOD_DisableCopy(uiDialExtra)

    uiDial*		dial()		{ return dial_; }
    uiLabel*		label()		{ return lbl_; }

    void		processInput();
    float		editValue() const;
			//!<The val in the ed field, which may be outside range

protected:

    uiDial*		dial_;
    uiLabel*		lbl_;
    uiLineEdit*		editfld_;

    void		init(const Setup&,const char*);
    void		editRetPress(CallBacker*);
    void		sliderMove(CallBacker*);
};
