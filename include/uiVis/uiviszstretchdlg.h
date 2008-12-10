#ifndef uiviszstretchdlg_h
#define uiviszstretchdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiviszstretchdlg.h,v 1.1 2008-12-10 18:05:30 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiCheckBox;
class uiLabeledComboBox;
class uiSliderExtra;

class uiZStretchDlg : public uiDialog
{
public:
			uiZStretchDlg(uiParent*);

    bool		valueChanged() const	{ return valchgd; }

    CallBack		vwallcb; //!< If not set -> no button
    CallBack		homecb; //!< If not set -> no button

protected:

    uiLabeledComboBox*	scenefld;
    uiSliderExtra*	sliderfld;
    uiCheckBox*		savefld;
    uiButton*		vwallbut;

    TypeSet<int>	sceneids;
    float		initslval;
    bool		valchgd;

    void		setZStretch(float);
    float		getCurrentZStretch() const;
    void		updateSliderValues();

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		sliderMove(CallBacker*);
    void		butPush(CallBacker*);
    void		sceneSel(CallBacker*);
};

#endif
