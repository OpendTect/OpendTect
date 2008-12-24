#ifndef uiwelldispprop_h
#define uiwelldispprop_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Dec 2008
 RCS:           $Id: uiwelldispprop.h,v 1.9 2008-12-24 15:58:12 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uicombobox.h"
#include "ranges.h"
#include "uigroup.h"
#include "welldisp.h"
#include "multiid.h"
#include "sets.h"

class MultiID;
class uiSpinBox;
class uiCheckBox;
class uiColorInput;
class uiGenInput;

namespace Well
{
    class LogDisplayParSet;
    class Data;
    class LogSet;
}

class uiWellDispProperties : public uiGroup
{
public:

    class Setup
    {
    public:
			Setup( const char* sztxt=0, const char* coltxt=0 )
			    : mysztxt_(sztxt ? sztxt : "Line thickness")
			    , mycoltxt_(coltxt ? coltxt : "Line color")	{}

	mDefSetupMemb(BufferString,mysztxt)
	mDefSetupMemb(BufferString,mycoltxt)
    };

    			uiWellDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::BasicProps&);

    Well::DisplayProperties::BasicProps& props()	{ return props_; }

    void		putToScreen();
    void		getFromScreen();

    Notifier<uiWellDispProperties>	propChanged;

protected:

    virtual void	doPutToScreen()			{}
    virtual void	doGetFromScreen()		{}

    Well::DisplayProperties::BasicProps&	props_;

    void		propChg(CallBacker*);
    uiColorInput*	colfld_;
    uiSpinBox*		szfld_;

};


class uiWellTrackDispProperties : public uiWellDispProperties
{
public:
    			uiWellTrackDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Track&);

    Well::DisplayProperties::Track&	trackprops()
	{ return static_cast<Well::DisplayProperties::Track&>(props_); }

protected:

    virtual void	doPutToScreen();
    virtual void	doGetFromScreen();

    uiCheckBox*		dispabovefld_;
    uiCheckBox*		dispbelowfld_;
};


class uiWellMarkersDispProperties : public uiWellDispProperties
{
public:
    			uiWellMarkersDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Markers&);

    Well::DisplayProperties::Markers&	mrkprops()
	{ return static_cast<Well::DisplayProperties::Markers&>(props_); }

protected:

    virtual void	doPutToScreen();
    virtual void	doGetFromScreen();

    uiGenInput*		circfld_;

};

class uiWellLogDispProperties : public uiWellDispProperties
{
public:
    			uiWellLogDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Log&,
					Well::LogSet* wl);

    Well::DisplayProperties::Log&	logprops()
	{ return static_cast<Well::DisplayProperties::Log&>(props_); }


protected:

    void        	doPutToScreen();
    void        	doGetFromScreen();
    void                isFilledSel(CallBacker*);
    void 		isRepeatSel(CallBacker*);
    void 		isSeismicSel(CallBacker*);
    void 		isStyleChanged(CallBacker*);
    void 		recoverProp();
    void 		choiceSel(CallBacker*);
    void 		setRangeFields(Interval<float>&);
    void 		setFillRangeFields(Interval<float>&);
    void 		updateRange(CallBacker*);
    void 		updateFillRange(CallBacker*);
    void 		calcRange(const char*, Interval<float>&);
    void  		setFldSensitive(bool);
    void 		logSel(CallBacker*);
    void 		selNone();
    void 		setFieldVals( bool );


    uiGenInput*		stylefld_;
    uiGenInput*		repeatfld_;
    uiGenInput*         clipratefld_;
    uiGenInput*         rangefld_;
    uiGenInput*         cliprangefld_;
    uiGenInput*         ovlapfld_;
    uiLabeledComboBox*  logsfld_;
    uiLabeledComboBox*  filllogsfld_;
    uiCheckBox*         logfillfld_;
    uiCheckBox*         singlfillcolfld_;
    uiComboBox*         coltablistfld_;
    uiColorInput*	seiscolorfld_;

    Interval<float>     valuerange_;
    Interval<float>     fillvaluerange_;
    Well::LogSet*  wl_;

};

#endif
