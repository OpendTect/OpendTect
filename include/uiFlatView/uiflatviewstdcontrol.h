#ifndef uiflatviewstdcontrol_h
#define uiflatviewstdcontrol_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: uiflatviewstdcontrol.h,v 1.13 2009-06-23 04:50:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"
#include "menuhandler.h"
class uiMenuHandler;
class uiToolButton;
class uiFlatViewColTabEd;
class uiToolBar;


/*!\brief The standard tools to control uiFlatViewer(s). */

mClass uiFlatViewStdControl : public uiFlatViewControl
{
public:

    struct Setup
    {
			Setup( uiParent* p=0 )
			    : parent_(p)
			    , helpid_("")
			    , withwva_(true)
			    , withcoltabed_(true)
			    , withedit_(false)
			    , withstates_(true)		{}

	mDefSetupMemb(uiParent*,parent) //!< null => viewer's parent
	mDefSetupMemb(bool,withwva)
	mDefSetupMemb(bool,withcoltabed)
	mDefSetupMemb(bool,withedit)
	mDefSetupMemb(bool,withstates)
	mDefSetupMemb(BufferString,helpid)
    };

    			uiFlatViewStdControl(uiFlatViewer&,const Setup&);
    			~uiFlatViewStdControl();

protected:

    bool		manip_;
    bool		mousepressed_;
    uiToolBar*		tb_;
    uiToolButton*	zoominbut_;
    uiToolButton*	zoomoutbut_;
    uiToolButton*	manipdrawbut_;
    uiToolButton*	parsbut_;
    uiToolButton*	editbut_;

    uiFlatViewer&	vwr_;
    uiFlatViewColTabEd*	ctabed_;

    virtual void	finalPrepare();
    void		updatePosButtonStates();
    uiRect		getViewRect();

    void		vwChgCB(CallBacker*);
    void		vwrAdded(CallBacker*);
    void		wheelMoveCB(CallBacker*);
    void		zoomCB(CallBacker*);
    void		handDragStarted(CallBacker*);
    void		handDragging(CallBacker*);
    void		handDragged(CallBacker*);
    void		panCB(CallBacker*);
    void		flipCB(CallBacker*);
    void		parsCB(CallBacker*);
    void		stateCB(CallBacker*);
    void		editCB(CallBacker*);
    void		helpCB(CallBacker*);
    void		coltabChg(CallBacker*);
    void		dispChgCB(CallBacker*);

    bool		handleUserClick();

    uiMenuHandler&      menu_;
    MenuItem           	propertiesmnuitem_;
    void                createMenuCB(CallBacker*);
    void                handleMenuCB(CallBacker*);

    BufferString	helpid_;

};

#endif
