#ifndef uimpeman_h
#define uimpeman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.h,v 1.1 2005-01-17 08:34:17 kristofer Exp $
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

    void			turnSeedPickingOn( bool yn );

    const char*			getDesTrackerType() const
    				{ return destrackertype; }

protected:
    void			seedPropertyChangeCB(CallBacker*);
    void			createSeedMenuCB(CallBacker*);
    void			handleSeedMenuCB(CallBacker*);
    int				seedmnuid;
    void			createMPEDisplay();
    uiVisPartServer*		partserver;

    visSurvey::MPEDisplay*	mpedisplay;
    visSurvey::SeedEditor*	seededitor;

    uiComboBox*			attribfld;
    uiSlider*			transfld;

    uiSpinBox*			nrstepsbox;

    void			selCB(CallBacker*);
    void			deselCB(CallBacker*);
    void			selectCB(CallBacker*);
    void			handleInterpreterSelection();

    void			displayCB(CallBacker*);

    void			attribSel(CallBacker*);
    void			transpChg(CallBacker*);

    void			undoPush(CallBacker*);
    void			redoPush(CallBacker*);
    void			updateButtonSensitivity(CallBacker* = 0);
    void			trackBackward(CallBacker*);
    void			trackForward(CallBacker*);

    void			seedMode(CallBacker*);
    void			extendMode(CallBacker*);
    void			retrackMode(CallBacker*);
    void			eraseMode(CallBacker*);
    void			mouseEraseMode(CallBacker*);
    void			trackModeChg(CallBacker*);
    void			setTrackButton(bool,bool,bool);
    void			showTracker( bool yn, int trackmode );


    int				seedidx;
    int				extendidx, retrackidx, eraseidx;
    int				selectidx, displayidx;
    int				mouseeraseridx;
    int				undoidx, redoidx;
    int				trackforwardidx, trackbackwardidx;
    bool			trackerwasonbeforemouseerase;

    BufferString		destrackertype;
};

#endif
