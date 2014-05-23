#ifndef uiwelldispprop_h
#define uiwelldispprop_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "bufstringset.h"
#include "multiid.h"
#include "ranges.h"
#include "sets.h"
#include "uigroup.h"
#include "uistring.h"
#include "welldisp.h"

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

namespace Well { class LogDisplayParSet; class LogSet; }

/*!
\brief Well display properties.
*/

mExpClass(uiWell) uiWellDispProperties : public uiGroup
{
public:

    mExpClass(uiWell) Setup
    {
    public:
	    Setup( const uiString& sztxt=uiString::emptyString(),
		   const uiString& coltxt=uiString::emptyString() )
		: mysztxt_(!sztxt.isEmpty() ? sztxt : "Line thickness")
		, mycoltxt_(!coltxt.isEmpty() ? coltxt : "Line color")	{}

	mDefSetupMemb(uiString,mysztxt)
	mDefSetupMemb(uiString,mycoltxt)
    };

			uiWellDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::BasicProps&);

    Well::DisplayProperties::BasicProps& props()	{ return *props_; }

    void		putToScreen();
    void		getFromScreen();


    Notifier<uiWellDispProperties>	propChanged;

protected:

    virtual void	doPutToScreen()			{}
    virtual void	doGetFromScreen()		{}

    Well::DisplayProperties::BasicProps*	props_;

    void		propChg(CallBacker*);
    uiColorInput*	colfld_;
    //uiSpinBox*		szfld_;
    uiLabeledSpinBox*	szfld_;

};


/*!
\brief Well Track display properties.
*/

mExpClass(uiWell) uiWellTrackDispProperties : public uiWellDispProperties
{
public:
			uiWellTrackDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Track&);

    Well::DisplayProperties::Track&	trackprops()
	{ return static_cast<Well::DisplayProperties::Track&>(*props_); }

    void		resetProps(Well::DisplayProperties::Track&);

protected:

    virtual void	doPutToScreen();
    virtual void	doGetFromScreen();

    uiCheckBox*		dispabovefld_;
    uiCheckBox*		dispbelowfld_;
    uiLabeledSpinBox*	nmsizefld_;
    uiComboBox*		nmstylefld_;
};


/*!
\brief Well Markers display properties.
*/

mExpClass(uiWell) uiWellMarkersDispProperties : public uiWellDispProperties
{
public:
			uiWellMarkersDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Markers&,
					const BufferStringSet& allmarkernms,
					bool is2d);

    Well::DisplayProperties::Markers&	mrkprops()
	{ return static_cast<Well::DisplayProperties::Markers&>(*props_); }

    void		setAllMarkerNames(const BufferStringSet&);
    void		resetProps(Well::DisplayProperties::Markers&);

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
    bool		is2d_;
};


/*!
\brief Well Log display properties.
*/

mExpClass(uiWell) uiWellLogDispProperties : public uiWellDispProperties
{
public:
			uiWellLogDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Log&,
					const Well::LogSet* wl);

    Well::DisplayProperties::Log&	logprops()
	{ return static_cast<Well::DisplayProperties::Log&>(*props_); }

    void		resetProps(Well::DisplayProperties::Log&);
    void		setLogSet(const Well::LogSet*);
    void		disableSeisStyle(bool);
    void		disableLogWidth(bool);

protected:

    void	doPutToScreen();
    void	doGetFromScreen();
    void                isFilledSel(CallBacker*);
    void		isRepeatSel(CallBacker*);
    void		setSeismicSel();
    void		setTubeSel();
    void		setWellLogSel();
    void		isStyleChanged(CallBacker*);
    void		recoverProp();
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
    uiGenInput*         clipratefld_;
    uiGenInput*         rangefld_;
    uiGenInput*         colorrangefld_;
    uiGenInput*         cliprangefld_;
    uiSpinBox*          ovlapfld_;
    uiSpinBox*		repeatfld_;
    uiLabeledSpinBox*   lblo_;
    uiLabeledSpinBox*   lblr_;
    uiLabeledSpinBox*   logwidthfld_;
    uiLabeledSpinBox*   lvlofdetailfld_;
    uiLabeledComboBox*  logsfld_;
    uiLabeledComboBox*  filllogsfld_;
    uiLabeledComboBox*  logfilltypefld_;
    uiCheckBox*         logarithmfld_;
    uiCheckBox*         revertlogfld_;
    uiCheckBox*         singlfillcolfld_;
    uiCheckBox*         flipcoltabfld_;
    uiColorTableSel*	coltablistfld_;
    uiColorInput*	seiscolorfld_;
    uiColorInput*	fillcolorfld_;

    Interval<float>     valuerange_;
    Interval<float>     fillvaluerange_;
    const Well::LogSet*  wl_;
};

#endif

