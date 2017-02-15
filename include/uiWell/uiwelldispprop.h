#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2008
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigroup.h"
#include "welldisp.h"
#include "uistrings.h"

class uiCheckBox;
class uiColorInput;
class uiColSeqUseMode;
class uiComboBox;
class uiColSeqSel;
class uiGenInput;
class uiLabeledComboBox;
class uiLabeledSpinBox;
class uiSpinBox;
class uiListBox;
class uiCheckList;
class uiSlider;
class uiWellLogDispProperties;

namespace Well { class LogDisplayParSet; class LogSet; }

/*!
\brief Well display properties.
*/

mExpClass(uiWell) uiWellDispProperties : public uiGroup
{ mODTextTranslationClass(uiWellDispProperties)
public:

    mExpClass(uiWell) Setup
    {
    public:
	    Setup( const uiString& sztxt=uiString::emptyString(),
		   const uiString& coltxt=uiString::emptyString() )
		: mysztxt_(!sztxt.isEmpty() ? sztxt : tr("Line thickness"))
		, mycoltxt_(!coltxt.isEmpty() ? coltxt :
			     uiStrings::phrJoinStrings(uiStrings::sLine(),
			     uiStrings::sColor().toLower()) )
		, onlyfor2ddisplay_(false) {}
	    mDefSetupMemb(uiString,mysztxt)
	    mDefSetupMemb(uiString,mycoltxt)
	    mDefSetupMemb(bool,onlyfor2ddisplay)
    };

			uiWellDispProperties(uiParent*,const Setup&,
					Well::BasicDispProps&);
    Well::BasicDispProps& props()	{ return *props_; }

    void		putToScreen();
    void		getFromScreen();


    Notifier<uiWellDispProperties>	propChanged;
    uiWellLogDispProperties*		curwelllogproperty_;

protected:

    virtual void	doPutToScreen()			{}
    virtual void	doGetFromScreen()		{}

    Well::BasicDispProps* props_;

    void		propChg(CallBacker*);
    uiColorInput*	colfld_;
    uiLabeledSpinBox*	szfld_;
    Setup		setup_;
};


/*!
\brief Well Track display properties.
*/

mExpClass(uiWell) uiWellTrackDispProperties : public uiWellDispProperties
{ mODTextTranslationClass(uiWellTrackDispProperties)
public:
			uiWellTrackDispProperties(uiParent*,const Setup&,
					Well::TrackDispProps&);

    Well::TrackDispProps& trackprops()
			{ return static_cast<Well::TrackDispProps&>(*props_); }

    void		resetProps(Well::TrackDispProps&);

protected:

    virtual void	doPutToScreen();
    virtual void	doGetFromScreen();

    uiCheckBox*		dispabovefld_;
    uiCheckBox*		dispbelowfld_;
    uiLabeledSpinBox*	nmsizefld_;
    uiComboBox*		nmstylefld_;
};


/*!\brief Well Markers display properties.  */

mExpClass(uiWell) uiWellMarkersDispProperties : public uiWellDispProperties
{ mODTextTranslationClass(uiWellMarkersDispProperties)
public:
			uiWellMarkersDispProperties(uiParent*,const Setup&,
					Well::MarkerDispProps&,
					const BufferStringSet& allmarkernms);

    Well::MarkerDispProps& mrkprops()
			{ return static_cast<Well::MarkerDispProps&>(*props_); }

    void		setAllMarkerNames(const BufferStringSet&);
    void		resetProps(Well::MarkerDispProps&);

protected:

    virtual void	doPutToScreen();
    virtual void	doGetFromScreen();
    void		markerFldsChged(CallBacker*);
    void		setMarkerNmColSel(CallBacker*);
    void		getSelNames();
    void		setSelNames();
    uiLabeledComboBox*	shapefld_;
    uiCheckBox*		singlecolfld_;
    uiLabeledSpinBox*	nmsizefld_;
    uiComboBox*		nmstylefld_;
    uiCheckBox*		samecolasmarkerfld_;
    uiColorInput*	nmcolfld_;
    uiLabeledSpinBox*	cylinderheightfld_;
    uiListBox*		displaymarkersfld_;
};


/*!
\brief Well Log display properties.
*/

mExpClass(uiWell) uiWellLogDispProperties : public uiWellDispProperties
{ mODTextTranslationClass(uiWellLogDispProperties)
public:
			uiWellLogDispProperties(uiParent*,const Setup&,
					Well::LogDispProps&,
					const Well::LogSet* wl);

    Well::LogDispProps&	logprops()
			{ return static_cast<Well::LogDispProps&>(*props_); }

    void		resetProps(Well::LogDispProps&);
    void		setLogSet(const Well::LogSet*);

protected:

    void		doPutToScreen();
    void		doGetFromScreen();
    void                isFilledSel(CallBacker*);
    void		setSeismicSel();
    void		setTubeSel();
    void		setWellLogSel();
    void		isStyleChanged(CallBacker*);
    void		choiceSel(CallBacker*);
    void		setRangeFields(Interval<float>&);
    void		setFillRangeFields(Interval<float>&);
    void		updateRange(CallBacker*);
    void		updateFillRange(CallBacker*);
    void		calcRange(const char*, Interval<float>&);
    void		setFldSensitive(bool);
    void		logSel(CallBacker*);
    void		selNone();
    void		setFieldVals();
    void		disableLogDisplays();
    void		setStyleSensitive(bool);

    uiCheckList*	stylefld_;
    uiGenInput*		clipratefld_;
    uiGenInput*		rangefld_;
    uiGenInput*		colorrangefld_;
    uiGenInput*		cliprangefld_;
    uiSpinBox*		ovlapfld_;
    uiSpinBox*		repeatfld_;
    uiLabeledSpinBox*	lblo_;
    uiLabeledSpinBox*	lblr_;
    uiSlider*		logwidthslider_;
    uiLabeledSpinBox*	lvlofdetailfld_;
    uiLabeledComboBox*	logsfld_;
    uiLabeledComboBox*	filllogsfld_;
    uiLabeledComboBox*	logfilltypefld_;
    uiCheckBox*		logarithmfld_;
    uiCheckBox*		revertlogfld_;
    uiCheckBox*		singlfillcolfld_;
    uiColSeqSel*	colseqselfld_;
    uiColSeqUseMode*	sequsefld_;
    uiColorInput*	seiscolorfld_;
    uiColorInput*	fillcolorfld_;

    Interval<float>	valuerange_;
    Interval<float>	fillvaluerange_;
    const Well::LogSet*	wls_;

};
