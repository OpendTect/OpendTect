#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uigroup.h"
#include "ranges.h"

class uiSpinBoxBody;
class uiLabel;


mExpClass(uiBase) uiSpinBox : public uiObject
{ mODTextTranslationClass(uiSpinBox)
friend class		uiSpinBoxBody;

public:
			uiSpinBox(uiParent*,int nrdecimals=0,
				  const char* nm="SpinBox");
			~uiSpinBox();

    void		setReadOnly(bool yn);
    bool		isReadOnly() const;

    void		setNrDecimals(int);
    void		setAlpha(bool);
    bool		isAlpha() const;

    void		setSpecialValueText(const char*); // First entry

    void		setValue(int);
    void		setValue(od_int64);
    void		setValue(float);
    void		setValue(double);
    void		setValue(const char*);

    int			getIntValue() const;
    od_int64		getInt64Value() const;
    double		getDValue() const;
    float		getFValue() const;
    bool		getBoolValue() const;
    const char*		text() const;

    void		setInterval( int start, int stop, int s=1 )
			{ setInterval( StepInterval<int>(start,stop,s) ); }
    void		setInterval( const Interval<int>& i, int s=1 )
			{ setInterval( StepInterval<int>(i.start,i.stop,s) ); }
    void		setInterval(const StepInterval<int>&);
    StepInterval<int>	getInterval() const;

    void		setInterval( float start, float stop, float s=1 )
			{ setInterval(StepInterval<float>(start,stop,s)); }
    void		setInterval(const StepInterval<float>&);
    StepInterval<float> getFInterval() const;

    void		setInterval( double start, double stop, double s=1 )
			{ setInterval(StepInterval<double>(start,stop,s)); }
    void		setInterval(const StepInterval<double>&);

    void		setMinValue(int);
    void		setMinValue(float);
    void		setMinValue( double d )	{ setMinValue( (float)d ); }
    int			minValue() const;
    float		minFValue() const;

    void		setMaxValue(int);
    void		setMaxValue(float);
    void		setMaxValue(double);
    int			maxValue() const;
    float		maxFValue() const;

    void		setStep(int,bool snap_cur_value=false);
    void		setStep(float,bool snap_cur_value=false);
    void		setStep(double d,bool snap_cur_value=false );
    float		fstep() const;
    int			step() const;

    void		stepBy( int nrsteps );

    uiString		prefix() const;
    void		setPrefix(const uiString&);
    uiString		suffix() const;
    void		setSuffix(const uiString&);
    void		doSnap( bool yn )		{ dosnap_ = yn; }

    void		setKeyboardTracking(bool);
    bool		keyboardTracking() const;
    void		setFocusChangeTrigger(bool);
    bool		focusChangeTrigger() const;

    bool		handleLongTabletPress() override;
    void		popupVirtualKeyboard(int globalx=-1,int globaly=-1);

    Notifier<uiSpinBox>	valueChanged;
    Notifier<uiSpinBox>	valueChanging;

    void		notifyHandler(bool editingfinished);

    void		translateText() override;
private:

    float		oldvalue_;

    uiString		prefix_;
    uiString		suffix_;
    uiSpinBoxBody*	body_;
    uiSpinBoxBody&	mkbody(uiParent*, const char*);

    bool		dosnap_; /*!< If true, value in spinbox will be snapped
				  to a value equal to N*step. */
    bool		focuschgtrigger_;
    void		snapToStep(CallBacker*);

public:

    mDeprecated		("Use getIntValue")
    int			getValue() const	{ return getIntValue(); }

    mDeprecated		("Use getFValue")
    float		getfValue() const	{ return getFValue(); }

    mDeprecated		("Use getDValue")
    double		getdValue() const	{ return getDValue(); }
};


mExpClass(uiBase) uiLabeledSpinBox : public uiGroup
{ mODTextTranslationClass(uiLabeledSpinBox)
public:
			uiLabeledSpinBox(uiParent*,const uiString&,
					 int nrdecimals=0,const char* nm=0);

    uiSpinBox*		box()			{ return sb_; }
    const uiSpinBox*	box() const		{ return sb_; }
    uiLabel*		label()			{ return lbl_; }

protected:

    uiSpinBox*		sb_;
    uiLabel*		lbl_;

};

