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
#include "helpview.h"

class uiMenuHandler;
class uiToolButton;
class uiFlatViewColTabEd;
class uiFlatViewThumbnail;
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
			    , helpkey_("")
			    , withcoltabed_(true)
			    , withedit_(false)
			    , withthumbnail_(true)		      
			    , withstates_(true)
			    , withhanddrag_(true)
			    , withsnapshot_(true)
			    , withflip_(true)
			    , withrubber_(true)
			    , tba_(-1)		      	{}

	mDefSetupMemb(uiParent*,parent) //!< null => viewer's parent
	mDefSetupMemb(bool,withcoltabed)
	mDefSetupMemb(bool,withedit)
	mDefSetupMemb(bool,withthumbnail)
	mDefSetupMemb(bool,withhanddrag)
	mDefSetupMemb(bool,withstates)
	mDefSetupMemb(int,tba)		//!< uiToolBar::ToolBarArea preference
	mDefSetupMemb(HelpKey,helpkey)
	mDefSetupMemb(bool,withflip)
	mDefSetupMemb(bool,withsnapshot)
	mDefSetupMemb(bool,withrubber)
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
    
    uiToolBar*		tb_;
    uiToolButton*	zoominbut_;
    uiToolButton*	zoomoutbut_;
    uiToolButton*	manipdrawbut_;
    uiToolButton*	parsbut_;
    uiToolButton*	editbut_;

    uiFlatViewer&		vwr_;
    uiFlatViewColTabEd*		ctabed_;
    uiFlatViewThumbnail*	thumbnail_;

    virtual void	finalPrepare();
    void		clearToolBar();
    void		updatePosButtonStates();
    void		doZoom(bool zoomin,uiFlatViewer&);

    virtual void	coltabChg(CallBacker*);
    virtual void	dispChgCB(CallBacker*);
    virtual void        zoomChgCB(CallBacker*);
    virtual void	editCB(CallBacker*);
    void		flipCB(CallBacker*);
    void		helpCB(CallBacker*);
    void		handDragStarted(CallBacker*);
    void		handDragging(CallBacker*);
    void		handDragged(CallBacker*);
    void		keyPressCB(CallBacker*);
    virtual void	parsCB(CallBacker*);
    void		stateCB(CallBacker*);
    virtual void	vwrAdded(CallBacker*) 	{}
    virtual void	vwChgCB(CallBacker*);
    virtual void	wheelMoveCB(CallBacker*);
    virtual void	zoomCB(CallBacker*);

    virtual bool	handleUserClick(int vwridx);

    uiMenuHandler&      menu_;
    MenuItem           	propertiesmnuitem_;
    void                createMenuCB(CallBacker*);
    void                handleMenuCB(CallBacker*);

    HelpKey		helpkey_;
};

#endif

