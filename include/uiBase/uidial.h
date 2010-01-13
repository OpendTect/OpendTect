#ifndef uidial_h
#define uidial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uidial.h,v 1.1 2010-01-13 08:12:40 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uiobj.h"

class LinScaler;
class uiDialBody;
class uiLabel;
class uiLineEdit;
template <class T> class StepInterval;

mClass uiDial : public uiObject
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

    Notifier<uiDial>	valueChanged;
    Notifier<uiDial>	sliderMoved;
    Notifier<uiDial>	sliderPressed;
    Notifier<uiDial>	sliderReleased;
    
private:

    uiDialBody*		body_;
    uiDialBody&		mkbody(uiParent*,const char*);
};

#endif
