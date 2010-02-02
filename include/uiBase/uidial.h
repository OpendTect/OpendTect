#ifndef uidial_h
#define uidial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
 RCS:           $Id: uidial.h,v 1.3 2010-02-02 23:36:34 cvskarthika Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiDialBody;
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
	void		setStartAtTop(bool);
	bool		hasStartAtTop() const;

    Notifier<uiDial>	valueChanged;
    Notifier<uiDial>	sliderMoved;
    Notifier<uiDial>	sliderPressed;
    Notifier<uiDial>	sliderReleased;
    
private:

    uiDialBody*		body_;
    uiDialBody&		mkbody(uiParent*,const char*);

	bool			startAtTop;
					// true - numbering starts at the top
};

#endif
