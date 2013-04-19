#ifndef uiflatviewstdcontrol_h
#define uiflatviewstdcontrol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uiflatviewcontrol.h"
#include "menuhandler.h"

class uiMenuHandler;
class uiToolButton;
class uiFlatViewColTabEd;
class uiToolBar;

/*!
\brief The standard tools to control uiFlatViewer(s).
*/

mExpClass(uiFlatView) uiFlatViewStdControl : public uiFlatViewControl
{
public:

    struct Setup
    {
			Setup( uiParent* p=0 )
			    : parent_(p)
			    , helpid_("")
			    , withcoltabed_(true)
			    , withedit_(false)
			    , withthumbnail_(true)		      
			    , withstates_(true)
			    , withhanddrag_(true)
			    , withsnapshot_(true)
				, withflip_(true)
			    , tba_(-1)		      	{}

	mDefSetupMemb(uiParent*,parent) //!< null => viewer's parent
	mDefSetupMemb(bool,withcoltabed)
	mDefSetupMemb(bool,withedit)
	mDefSetupMemb(bool,withthumbnail)
	mDefSetupMemb(bool,withhanddrag)
	mDefSetupMemb(bool,withstates)
	mDefSetupMemb(int,tba)		//!< uiToolBar::ToolBarArea preference
	mDefSetupMemb(BufferString,helpid)
	mDefSetupMemb(bool,withflip)
	mDefSetupMemb(bool,withsnapshot)
    };

    			uiFlatViewStdControl(uiFlatViewer&,const Setup&);
    			~uiFlatViewStdControl();
    virtual uiToolBar*	toolBar()		{ return tb_; }
    virtual uiFlatViewColTabEd* colTabEd()	{ return ctabed_; }
    void		setEditMode(bool yn);

    NotifierAccess* 	editPushed();

protected:

    bool		manip_;
    bool		mousepressed_;
    uiPoint		mousedownpt_;
    uiWorldRect		mousedownwr_;
    
    bool		viewdragged_;
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
    void		doZoom(bool in,uiFlatViewer&,FlatView::ZoomMgr&);

    virtual void	coltabChg(CallBacker*);
    void		dispChgCB(CallBacker*);
    virtual void	editCB(CallBacker*);
    void		flipCB(CallBacker*);
    void		helpCB(CallBacker*);
    void		handDragStarted(CallBacker*);
    void		handDragging(CallBacker*);
    void		handDragged(CallBacker*);
    void		keyPressCB(CallBacker*);
    void		panCB(CallBacker*);
    virtual void	parsCB(CallBacker*);
    void		stateCB(CallBacker*);
    void		translateCB(CallBacker*);
    virtual void	vwrAdded(CallBacker*) 	{}
    void		vwChgCB(CallBacker*);
    virtual void	wheelMoveCB(CallBacker*);
    virtual void	zoomCB(CallBacker*);

    virtual bool		handleUserClick();

    uiMenuHandler&      menu_;
    MenuItem           	propertiesmnuitem_;
    void                createMenuCB(CallBacker*);
    void                handleMenuCB(CallBacker*);

    BufferString	helpid_;
};

#endif

