#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigroup.h"
#include "welldisp.h"
#include "uistrings.h"

class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiColorTableSel;
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

    virtual		~uiWellDispProperties();

    Well::DisplayProperties::BasicProps& props()	{ return *props_; }

    void		putToScreen();
    void		getFromScreen();


    Notifier<uiWellDispProperties>	propChanged;
    uiWellLogDispProperties*		curwelllogproperty_ = nullptr;

protected:
			uiWellDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::BasicProps&);

    virtual void	doPutToScreen()			{}
    virtual void	doGetFromScreen()		{}

    Well::DisplayProperties::BasicProps*	props_;

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
					Well::DisplayProperties::Track&);
			~uiWellTrackDispProperties();

    Well::DisplayProperties::Track&	trackprops()
	{ return static_cast<Well::DisplayProperties::Track&>(*props_); }

    void		resetProps(Well::DisplayProperties::Track&);

protected:

    void		doPutToScreen() override;
    void		doGetFromScreen() override;

    uiCheckBox*		dispabovefld_;
    uiCheckBox*		dispbelowfld_;
    uiLabeledSpinBox*	nmsizefld_;
    uiComboBox*		nmstylefld_;
    uiCheckBox*		nmsizedynamicfld_;
};


/*!
\brief Well Markers display properties.
*/

mExpClass(uiWell) uiWellMarkersDispProperties : public uiWellDispProperties
{ mODTextTranslationClass(uiWellMarkersDispProperties)
public:
			uiWellMarkersDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Markers&,
					const BufferStringSet& allmarkernms,
					bool is2d=false);
			~uiWellMarkersDispProperties();

    Well::DisplayProperties::Markers&	mrkprops()
	{ return static_cast<Well::DisplayProperties::Markers&>(*props_); }

    void		setAllMarkerNames(const BufferStringSet&);
    void		setAllMarkerNames(const BufferStringSet&,
					  const TypeSet<OD::Color>&);
    void		resetProps(Well::DisplayProperties::Markers&);

protected:

    void		doPutToScreen() override;
    void		doGetFromScreen() override;
    void		markerFldsChged(CallBacker*);
    void		setMarkerNmColSel(CallBacker*);
    void		getSelNames();
    void		setSelNames();
    void		selectFirstChosen();

    uiLabeledComboBox*	shapefld_;
    uiCheckBox*		singlecolfld_;
    uiLabeledSpinBox*	nmsizefld_;
    uiComboBox*		nmstylefld_;
    uiCheckBox*		samecolasmarkerfld_;
    uiColorInput*	nmcolfld_;
    uiLabeledSpinBox*	cylinderheightfld_;
    uiListBox*		displaymarkersfld_;
    bool		is2d_;
    uiCheckBox*		nmsizedynamicfld_;
};


/*!
\brief Well Log display properties.
*/

mExpClass(uiWell) uiWellLogDispProperties : public uiWellDispProperties
{ mODTextTranslationClass(uiWellLogDispProperties)
public:
			uiWellLogDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Log&,
					const Well::LogSet* wl);
			uiWellLogDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Log&,
					const Well::Data*);
			~uiWellLogDispProperties();

    Well::DisplayProperties::Log&	logprops()
	{ return static_cast<Well::DisplayProperties::Log&>(*props_); }

    void		resetProps(Well::DisplayProperties::Log&);
    void		setLogSet(const Well::LogSet*);

protected:

    void		doPutToScreen() override;
    void		doGetFromScreen() override;
    void		isFilledSel(CallBacker*);
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
    uiCheckBox*		flipcoltabfld_;
    uiColorTableSel*	coltablistfld_;
    uiColorInput*	seiscolorfld_;
    uiColorInput*	fillcolorfld_;

    Interval<float>	valuerange_;
    Interval<float>	fillvaluerange_;
    const Well::LogSet*  wl_ = nullptr;
    const Well::Data*	wd_ = nullptr;

};

