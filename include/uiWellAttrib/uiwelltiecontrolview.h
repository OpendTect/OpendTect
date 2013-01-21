#ifndef uiwelltiecontrolview_h
#define uiwelltiecontrolview_h

/*+
  ________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uiwellattribmod.h"
#include "uiflatviewstdcontrol.h"

class uiFlatViewer;
class uiButton;
class uiToolBar;
class uiIOObjSelDlg;
class uiRect;

namespace WellTie
{
    class DispParams;
    class uiMrkDispDlg;
    class Server;

mExpClass(uiWellAttrib) uiControlView : public uiFlatViewStdControl
{
public:
			uiControlView(uiParent*,uiToolBar*,
					uiFlatViewer*,Server&);
			~uiControlView(){};
   
    bool 		isZoomAtStart() const;
    void 		setEditOn(bool);
    void		setSelView(bool isnewsel = true, bool viewall=false );

    void		usePar(const IOPar& iop);
    void		fillPar(IOPar& iop) const; 

    Notifier<uiControlView> redrawNeeded;
    Notifier<uiControlView> redrawAnnotNeeded;
    
protected:
    
    bool                manip_;
    
    uiToolBar*		toolbar_;
    uiToolButton*	horbut_;
    uiToolButton*	hormrkdispbut_;
    uiIOObjSelDlg*	selhordlg_;
    uiWorldRect		curview_;
   
    uiMrkDispDlg*	mrkrdlg_;
    Server&		server_;

    bool 		checkIfInside(double,double);
    bool 		handleUserClick();
   
    void 		altZoomCB(CallBacker*);
    void                applyProperties(CallBacker*);
    void 		keyPressCB(CallBacker*);
    void		loadHorizons(CallBacker*);
    void		dispHorMrks(CallBacker*);
    void		rubBandCB(CallBacker*);
    void		reDrawNeeded(CallBacker*);
    void 		wheelMoveCB(CallBacker*);

    friend class	uiTieWin;
};

/*
mExpClass(uiWellAttrib) uiTieClippingDlg : public uiDialog
{
public:
				uiTieClippingDlg(uiParent*);
				~uiTieClippingDlg();

protected :

    mStruct(uiWellAttrib) ClipData
    {
	float			cliprate_;			
	bool			issynthetic_;			
	Interval<float> 	timerange_;
    };

    uiGenInput*                 tracetypechoicefld_;
    uiSliderExtra*              sliderfld_;
    uiGenInput*                 timerangefld_;

    ClipData 			cd_;

    void 			getFromScreen(Callbacker*);
    void 			putToScreen(Callbacker*);
    void 			applyClipping(CallBacker*);
};
*/

}; //namespace WellTie

#endif


