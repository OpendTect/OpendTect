#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uiflatviewcontrol.h"
#include "uidialog.h"
#include "menuhandler.h"
#include "helpview.h"

class uiCheckBox;
class uiMenuHandler;
class uiFlatViewColTabEd;
class uiGenInput;
class uiToolBar;


mExpClass(uiFlatView) uiFlatViewZoomLevelDlg : public uiDialog
{ mODTextTranslationClass(uiFlatViewZoomLevelDlg)
public:
			uiFlatViewZoomLevelDlg(uiParent*,float& x1pospercm,
					float& x2pospercm,bool isvertical);
			~uiFlatViewZoomLevelDlg();

protected:

    float&		x1pospercm_;
    float&		x2pospercm_;

    uiGenInput*		x1fld_;
    uiGenInput*		x2fld_ = nullptr;
    uiGenInput*     unitflds_;
    uiCheckBox*		saveglobalfld_;

    void		finalizeDoneCB(CallBacker*);
    void		unitChgCB(CallBacker*);
    bool		acceptOK(CallBacker*);
};


/*!
\brief The standard tools to control uiFlatViewer(s).
*/

mExpClass(uiFlatView) uiFlatViewStdControl : public uiFlatViewControl
{ mODTextTranslationClass(uiFlatViewStdControl);
public:

    struct Setup
    {
			Setup( uiParent* p=0 )
			    : parent_(p)
			    , withcoltabed_(true)
			    , withedit_(false)
			    , withhanddrag_(true)
			    , withflip_(true)
			    , withsnapshot_(true)
			    , withrubber_(true)
			    , withhomebutton_(false)
			    , withzoombut_(true)
			    , isvertical_(false)
			    , withfixedaspectratio_(false)
			    , managecoltab_(true)
			    , withcoltabinview_(true)
			    , initialx1pospercm_(mUdf(float))
			    , initialx2pospercm_(mUdf(float))
			    , withscalebarbut_(false)
			    , initialcentre_(uiWorldPoint::udf())
			    , tba_(-1)
			    {}

	mDefSetupMemb(uiParent*,parent) //!< null => viewer's parent
	mDefSetupMemb(bool,withcoltabed)
	mDefSetupMemb(bool,withedit)
	mDefSetupMemb(bool,withhanddrag)
	mDefSetupMemb(HelpKey,helpkey)
	mDefSetupMemb(bool,withflip)
	mDefSetupMemb(bool,withsnapshot)
	mDefSetupMemb(bool,withrubber)
	mDefSetupMemb(bool,withhomebutton)
	mDefSetupMemb(bool,withzoombut)
	mDefSetupMemb(bool,isvertical)
	mDefSetupMemb(bool,withfixedaspectratio)
	mDefSetupMemb(bool,managecoltab)
	mDefSetupMemb(bool,withcoltabinview)
	mDefSetupMemb(float,initialx1pospercm)
	mDefSetupMemb(float,initialx2pospercm)
	mDefSetupMemb(bool,withscalebarbut)
	mDefSetupMemb(uiWorldPoint,initialcentre);
	mDefSetupMemb(int,tba)		//!< uiToolBar::ToolBarArea preference
    };

			uiFlatViewStdControl(uiFlatViewer&,const Setup&);
    virtual		~uiFlatViewStdControl();

    virtual uiToolBar*	toolBar()		{ return tb_; }
    virtual uiToolBar*	editToolBar()		{ return edittb_; }
    virtual uiFlatViewColTabEd* colTabEd()	{ return ctabed_; }
    void		setEditMode(bool yn);
    float		getCurrentPosPerCM(bool forx1) const;

    static void		setGlobalZoomLevel(float x1pospercm, float x2pospercm,
					   bool isvertical);
    static void		getGlobalZoomLevel(float& x1pospercm, float& x2pospercm,
					   bool isvertical);
    bool		isEditModeOn() const;
    bool		isRubberBandOn() const;
    NotifierAccess*	editPushed();

protected:

    bool		mousepressed_;
    uiPoint		mousedownpt_;

    float		defx1pospercm_;
    float		defx2pospercm_;

    uiToolBar*		tb_;
    uiToolBar*		edittb_ = nullptr;
    int			rubbandzoombut_ = -1;
    int			zoominbut_ = -1;
    int			zoomoutbut_ = -1;
    int			vertzoominbut_ = -1;
    int			vertzoomoutbut_ = -1;
    int			cancelzoombut_ = -1;
    int			sethomezoombut_ = -1;
    int			gotohomezoombut_ = -1;
    int			scalebarbut_ = -1;
    int			coltabbut_ = -1;
    int			fittoscrnbut_ = -1;
    int			parsbut_ = -1;
    int			editbut_ = -1;

    uiFlatViewer&	vwr_;
    uiFlatViewColTabEd* ctabed_ = nullptr;

    const Setup		setup_;

    virtual void	finalPrepare();
    void		clearToolBar();
    void		updatePosButtonStates();
    void		updateZoomLevel(float x1pospercm,float x2pospercm);
    void		doZoom(bool zoomin,bool onlyvertzoom,uiFlatViewer&);
    void		setViewToCustomZoomLevel(uiFlatViewer&);

    virtual void	coltabChg(CallBacker*);
    virtual void	dispChgCB(CallBacker*);
    virtual void	zoomChgCB(CallBacker*);
    virtual void	rubBandUsedCB(CallBacker*);
    virtual void	dragModeCB(CallBacker*);
    void		editModeCB(CallBacker*);
    void		flipCB(CallBacker*);
    void		helpCB(CallBacker*);
    void		handDragStarted(CallBacker*);
    void		handDragging(CallBacker*);
    void		handDragged(CallBacker*);
    void		aspectRatioCB(CallBacker*);
    void		keyPressCB(CallBacker*);
    void		homeZoomOptSelCB(CallBacker*);
    void		fitToScreenCB(CallBacker*);
    void		displayScaleBarCB(CallBacker*);
    void		displayColTabCB(CallBacker*);
    virtual void	parsCB(CallBacker*);
    virtual void	vwrAdded(CallBacker*)	{}
    virtual void	wheelMoveCB(CallBacker*);
    virtual void	zoomCB(CallBacker*);
    virtual void	pinchZoomCB(CallBacker*);
    virtual void	cancelZoomCB(CallBacker*);
    virtual void	gotoHomeZoomCB(CallBacker*);

    virtual bool	handleUserClick(int vwridx);

    uiMenuHandler&	menu_;
    MenuItem		propertiesmnuitem_;
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    HelpKey		helpkey_;

    void		setVwrCursor(uiFlatViewer&,const MouseCursor&);
};

