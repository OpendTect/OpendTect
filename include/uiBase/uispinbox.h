#ifndef uispinbox_h
#define uispinbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.h,v 1.7 2004-02-02 15:21:37 nanne Exp $
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
                        uiSpinBox(uiParent*, const char* nm="SpinBox");
			~uiSpinBox();

    const char*		text() const;
    int 		getIntValue() const;
    double 		getValue() const;

    void		setText(const char*);
    void		setValue(int);
    void		setValue(double);

    void		setInterval(int start,int stop,int step=1)
			{ setInterval(StepInterval<int>(start,stop,step)); }
    void		setInterval(StepInterval<int>);
    StepInterval<int>	getInterval() const;

    int			minValue() const;
    int			maxValue() const;
    void		setMinValue(int);
    void		setMaxValue(int);
    int			step() const;
    void		setStep(int,bool dosnap_=false);
    			/*!< if dosnap_ is true, value in spinbox will be 
    				snapped to a value equal to N*step */

    void		doSnap(bool yn)			{ dosnap = yn; }

    Notifier<uiSpinBox>	valueChanged;

protected:

    virtual bool	useMappers()			{ return false; }
    virtual int		mapTextToValue(bool* ok)	{ return 0; }
    virtual const char*	mapValueToText(int)		{ return 0; }

private:

    mutable BufferString result;

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
					 const char* nm="Labeled Spinbox");

    uiSpinBox*  	box()			{ return sb; }
    uiLabel*    	label()			{ return lbl; }

protected:

    uiSpinBox*		sb;
    uiLabel*    	lbl;
};

#endif
