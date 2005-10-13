#ifndef uimpeman_h
#define uimpeman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.h,v 1.25 2005-10-13 21:22:38 cvskris Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "uitoolbar.h"
#include "menuhandler.h"

namespace EM { class EMManager; };
namespace MPE { class EMSeedPicker; };
namespace visSurvey { class MPEDisplay; class MPEClickCatcher;}
namespace Geometry  { class Element; };
namespace visSurvey { class PickSetDisplay; }


class BufferStringSet;
class uiColorBarDialog;
class uiComboBox;
class uiSpinBox;
class uiPushButton;
class uiSlider;
class uiVisPartServer;


/*! \brief Dialog for tracking properties
*/
class uiMPEMan : public uiToolBar
{
public:		
				uiMPEMan(uiParent*, uiVisPartServer*);
				~uiMPEMan();

    void			deleteVisObjects();
    void			updateAttribNames();
    void			initFromDisplay();

    void			turnSeedPickingOn(bool);
    void			setSensitive(bool);

protected:
    visSurvey::MPEDisplay*	getDisplay(int sceneid,bool create=false);
    
    uiColorBarDialog*		colbardlg;

    MenuItem			seedmnuitem;
    MenuItem			createmnuitem;
    uiVisPartServer*		visserv;
    CubeSampling		oldactivevol;
    bool			didtriggervolchange;
    MPE::EMSeedPicker*		seedpicker;

    visSurvey::MPEClickCatcher*	clickcatcher;

    uiComboBox*			attribfld;
    uiSlider*			transfld;

    uiSpinBox*			nrstepsbox;

    void			cubeDeselCB(CallBacker*);
    void			cubeSelectCB(CallBacker*);
    void			showCubeCB(CallBacker*);

    void			attribSel(CallBacker*);
    bool			blockattribsel;

    void			transpChg(CallBacker*);

    void			undoPush(CallBacker*);
    void			redoPush(CallBacker*);
    void			updateButtonSensitivity(CallBacker* = 0);
    void			updateSelectedAttrib();
    void			trackBackward(CallBacker*);
    void			trackForward(CallBacker*);
    void			trackInVolume(CallBacker*);

    void			seedModeCB(CallBacker*);
    void			setColorbarCB(CallBacker*);
    void                        onColTabClosing(CallBacker*);
    void			movePlaneCB(CallBacker*);
    void			extendModeCB(CallBacker*);
    void			retrackModeCB(CallBacker*);
    void			eraseModeCB(CallBacker*);
    void			mouseEraseModeCB(CallBacker*);
    void			setTrackButton();
    void			showTracker(bool);
    
    void			setHistoryLevel(int);

    void			seedClick(CallBacker*);
    int				seedclickobject;

    int				seedidx;
    int				clrtabidx;
    int				moveplaneidx, extendidx, retrackidx, eraseidx;
    int				showcubeidx, displayidx;
    int				mouseeraseridx;
    int				undoidx, redoidx;
    int				trackforwardidx, trackbackwardidx;
    int				trackinvolidx;
    bool			trackerwasonbeforemouseerase;

    bool			init;

    static const char*		sKeyNoAttrib() { return "No attribute"; }
};

#endif
