#ifndef uispinbox_h
#define uispinbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.h,v 1.9 2005-01-25 13:30:26 nanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uigroup.h"
#include "ranges.h"

class uiSpinBoxBody;
class uiLabel;


class uiSpinBox : public uiObject
{
friend class		uiSpinBoxBody;

public:
                        uiSpinBox(uiParent*, int nrdecimals=0,
				  const char* nm="SpinBox");
			~uiSpinBox();

    void		setNrDecimals(int);

    void		setValue(int);
    void		setValue(float);
    int			getValue() const;
    float		getFValue() const;

    void		setInterval(int start,int stop,int step=1)
			{ setInterval(StepInterval<int>(start,stop,step)); }
    void		setInterval(const StepInterval<int>&);
    StepInterval<int>	getInterval() const;

    void		setInterval(float start,float stop,float step=1)
			{ setInterval(StepInterval<float>(start,stop,step)); }
    void		setInterval(const StepInterval<float>&);
    StepInterval<float> getFInterval() const;

    void		setMinValue(int);
    void		setMinValue(float);
    int			minValue() const;
    float		minFValue() const;

    void		setMaxValue(int);
    void		setMaxValue(float);
    int			maxValue() const;
    float		maxFValue() const;

    void		setStep(float,bool dosnap_=false);
    void		setStep(int,bool dosnap_=false);
    			/*!< if dosnap_ is true, value in spinbox will be 
    				snapped to a value equal to N*step */
    float		fstep() const;
    int			step() const;

    void		setSuffix(const char*);
    const char*		suffix() const;

    void		doSnap(bool yn)			{ dosnap = yn; }

    Notifier<uiSpinBox>	valueChanged;

private:

    int			factor;

    uiSpinBoxBody*	body_;
    uiSpinBoxBody&	mkbody(uiParent*, const char*);

    bool		dosnap; /*!< If true, value in spinbox will be snapped 
				  to a value equal to N*step. */
    void		snapToStep(CallBacker*);
};


class uiLabeledSpinBox : public uiGroup
{
public:
                	uiLabeledSpinBox(uiParent*,const char* txt,
					 int nrdecimals=0,
					 const char* nm="Labeled Spinbox");

    uiSpinBox*  	box()			{ return sb; }
    uiLabel*    	label()			{ return lbl; }

protected:

    uiSpinBox*		sb;
    uiLabel*    	lbl;
};

#endif
