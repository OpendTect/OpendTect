#ifndef uimpeman_h
#define uimpeman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.h,v 1.7 2005-06-09 03:32:33 cvsduntao Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"

namespace EM { class EMManager; };
namespace visSurvey { class MPEDisplay; class SeedEditor;}
namespace Geometry  { class Element; };


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

    void			seedPropertyChangeCB(CallBacker*);
    void			createSeedMenuCB(CallBacker*);
    void			handleSeedMenuCB(CallBacker*);
    int				seedmnuid;
    uiVisPartServer*		visserv;

    visSurvey::SeedEditor*	seededitor;

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
};

#endif
