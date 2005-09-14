#ifndef uimpeman_h
#define uimpeman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.h,v 1.12 2005-09-14 08:26:50 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"
#include "menuhandler.h"

namespace EM { class EMManager; };
namespace visSurvey { class MPEDisplay; class MPEClickCatcher;}
namespace Geometry  { class Element; };
namespace visSurvey { class PickSetDisplay; }


class BufferStringSet;
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

    const char*			getDesTrackerType() const
    				{ return destrackertype; }

protected:
    visSurvey::MPEDisplay*	getDisplay(int sceneid,bool create=false);

    MenuItem			seedmnuitem;
    MenuItem			createmnuitem;
    uiVisPartServer*		visserv;

    visSurvey::MPEClickCatcher*	clickcatcher;

    uiComboBox*			attribfld;
    uiSlider*			transfld;

    uiSpinBox*			nrstepsbox;

    void			cubeDeselCB(CallBacker*);
    void			cubeSelectCB(CallBacker*);
    void			showCubeCB(CallBacker*);

    void			attribSel(CallBacker*);
    void			transpChg(CallBacker*);

    void			undoPush(CallBacker*);
    void			redoPush(CallBacker*);
    void			updateButtonSensitivity(CallBacker* = 0);
    void			trackBackward(CallBacker*);
    void			trackForward(CallBacker*);
    void			trackInVolume(CallBacker*);

    void			seedModeCB(CallBacker*);
    void			extendModeCB(CallBacker*);
    void			retrackModeCB(CallBacker*);
    void			eraseModeCB(CallBacker*);
    void			mouseEraseModeCB(CallBacker*);
    void			setTrackButton();
    void			showTracker(bool);

    void			seedClick(CallBacker*);
    int				seedclickobject;

    int				seedidx;
    int				extendidx, retrackidx, eraseidx;
    int				showcubeidx, displayidx;
    int				mouseeraseridx;
    int				undoidx, redoidx;
    int				trackforwardidx, trackbackwardidx;
    int				trackinvolidx;
    bool			trackerwasonbeforemouseerase;

    BufferString		destrackertype;
    bool			init;
    
private:
    bool			picknewseeds;
    				// indicator, whom seed picking is invoked by
				// false - by program, true - by user
};

#endif
