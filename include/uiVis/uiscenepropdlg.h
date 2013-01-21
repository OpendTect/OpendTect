#ifndef uiscenepropdlg_h
#define uiscenepropdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          February 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "color.h"

class uiCheckBox;
class uiColorInput;
class ui3DViewer;
class uiSliderExtra;
class uiGenInputDlg;

namespace visSurvey { class Scene; }

/*! Dialog for scene properties. */

mExpClass(uiVis) uiScenePropertyDlg : public uiDialog
{
public:
			uiScenePropertyDlg(uiParent*,const 
					ObjectSet<ui3DViewer>&,int);
			~uiScenePropertyDlg();

protected:
    void		updateScene(visSurvey::Scene*);
    void		updateCB(CallBacker*);
    void		setOffsetCB(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    const ObjectSet<ui3DViewer>&viewers_;
    int				curvwridx_;
    visSurvey::Scene*		scene_;
    bool			hadsurveybox_;
    bool			hadannot_;
    bool			hadannotscale_;
    bool			hadannotgrid_;
    bool			hadanimation_;
    Color			oldbgcolor_;
    float			oldmarkersize_;
    Color			oldmarkercolor_;
    Color			annotcolor_;
    float			oldfactor_;
    float			oldunits_;
    static bool			savestatus_;

    uiCheckBox*			annotfld_;
    uiCheckBox*			annotscalefld_;
    uiCheckBox*			annotgridfld_;
    uiCheckBox*			survboxfld_;
    uiColorInput*		bgcolfld_;
    uiSliderExtra*		markersizefld_;
    uiColorInput*		markercolfld_;
    uiColorInput*		annotcolfld_;
    uiGenInputDlg*		separationdlg_;
    uiCheckBox*			animationfld_;
};

#endif

