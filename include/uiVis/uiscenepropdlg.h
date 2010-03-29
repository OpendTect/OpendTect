#ifndef uiscenedlg_h
#define uiscenedlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          February 2006
 RCS:           $Id: uiscenepropdlg.h,v 1.5 2010-03-29 22:10:48 cvskarthika Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "color.h"

class uiCheckBox;
class uiColorInput;
class uiVisPartServer;
class uiSoViewer;
class uiSliderExtra;

namespace visSurvey { class Scene; class Seis2DDisplay; }

/*! Dialog for scene properties. */

mClass uiScenePropertyDlg : public uiDialog
{
public:
			uiScenePropertyDlg(uiParent*,visSurvey::Scene*,
					   uiSoViewer*,
					   const ObjectSet<uiSoViewer>&,
					   uiVisPartServer* );

protected:
    void		updateScene(visSurvey::Scene*);
    void		updateSeis2DDisplay();
    void		updateCB(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    visSurvey::Scene*		scene_;
    uiSoViewer*			viewer_;
    const ObjectSet<uiSoViewer>&viewers_;
    uiVisPartServer*		visserv_;
    bool			hadsurveybox_;
    bool			hadannot_;
    bool			hadannotscale_;
    Color			oldbgcolor_;
    float			oldmarkersize_;
    Color			oldmarkercolor_;
    Color			annotcolor_;
    static bool			savestatus;

    uiCheckBox*			annotfld_;
    uiCheckBox*			annotscalefld_;
    uiCheckBox*			survboxfld_;
    uiColorInput*		bgcolfld_;
    uiSliderExtra*		markersizefld_;
    uiColorInput*		markercolfld_;
    uiColorInput*		annotcolfld_;
};

#endif
