#ifndef uidial_h
#define uidial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
 RCS:           $Id: uidial.h,v 1.5 2012-08-03 13:00:51 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigroup.h"
#include "uiobj.h"

class uiDialBody;
template <class T> class StepInterval;

class uiLabel;
class uiLineEdit;

mClass(uiBase) uiDial : public uiObject
{
public:

                        uiDial(uiParent*,const char* nm="Dial");
			~uiDial();

    enum		Orientation { Horizontal, Vertical };

    void		setValue(int);
    int 		getValue() const;

    void		setMinValue(int);
    int			minValue() const;
    void		setMaxValue(int);
    int			maxValue() const;
    void		setStep(int);
    int			step() const;

    void		setInterval(const StepInterval<int>&);
    void		getInterval(StepInterval<int>&) const;

    void		setOrientation(Orientation);
    uiDial::Orientation getOrientation() const;

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
mClass(uiBase) uiDialExtra : public uiGroup
{
public:

    mClass(uiBase) Setup
    {
	public:
	    		Setup(const char* l=0)
			    : lbl_(l)
			    , withedit_(false)
			    , isvertical_(false)
			    , dialsize_(60)
			{}

		mDefSetupMemb(bool,withedit)
		mDefSetupMemb(bool,isvertical)
		mDefSetupMemb(int,dialsize)
		mDefSetupMemb(BufferString,lbl)
    };

			uiDialExtra(uiParent*,const Setup&, const char* nm);

    uiDial*		dial()		{ return dial_; }
    uiLabel*   		label()   	{ return lbl_; }

    void                processInput();
    float               editValue() const;
                        //!<The val in the ed field, which may be outside range

protected:

    uiDial*		dial_;
    uiLabel*            lbl_;
    uiLineEdit*         editfld_;
    
    void                init(const Setup&,const char*);
    void                editRetPress(CallBacker*);
    void                sliderMove(CallBacker*);
};

#endif

