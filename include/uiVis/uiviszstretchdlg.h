#ifndef uiviszstretchdlg_h
#define uiviszstretchdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

class uiCheckBox;
class uiLabeledComboBox;
class uiSliderExtra;

/*!Dialog to set the z-stretch of a scene. */

mExpClass(uiVis) uiZStretchDlg : public uiDialog
{
public:
			uiZStretchDlg(uiParent*);

    bool		valueChanged() const	{ return valchgd_; }

    CallBack		vwallcb; //!< If not set -> no button
    CallBack		homecb; //!< If not set -> no button

protected:

    uiLabeledComboBox*	scenefld_;
    uiSliderExtra*	sliderfld_;
    uiCheckBox*		savefld_;
    uiButton*		vwallbut_;

    TypeSet<int>	sceneids_;
    float		initslval_;
    bool		valchgd_;

    void		setZStretch(float,bool permanent);
    float		getCurrentZStretch() const;
    void		updateSliderValues();

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    void		sliderMove(CallBacker*);
    void		butPush(CallBacker*);
    void		sceneSel(CallBacker*);
};

#endif

