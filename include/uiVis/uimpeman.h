#ifndef uimpeman_h
#define uimpeman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.h,v 1.3 2005-03-09 16:44:44 cvsnanne Exp $
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

    void			seedModeCB(CallBacker*);
    void			extendModeCB(CallBacker*);
    void			retrackModeCB(CallBacker*);
    void			eraseModeCB(CallBacker*);
    void			mouseEraseModeCB(CallBacker*);
    void			trackModeChg(CallBacker*);
    void			setTrackButton(bool,bool,bool);
    void			showTracker(bool yn,int trackmode);

    int				seedidx;
    int				extendidx, retrackidx, eraseidx;
    int				showcubeidx, displayidx;
    int				mouseeraseridx;
    int				undoidx, redoidx;
    int				trackforwardidx, trackbackwardidx;
    bool			trackerwasonbeforemouseerase;

    BufferString		destrackertype;
};

#endif
