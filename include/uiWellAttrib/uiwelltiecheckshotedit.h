#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Oct 2011
RCS:           $Id: uiwellwelltiecheckshotedit.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "welldahobj.h"
#include "undo.h"
#include "uistring.h"

namespace Well { class D2TModel; class Data; class DahObj;}
class uiWellDahDisplay;
class uiCheckBox;
class uiComboBox;
class uiPolyLineItem;
class uiToolBar;
class uiToolButton;
class uiWellDisplayControl;

namespace WellTie
{

class Server;

mExpClass(uiWellAttrib) DahObjUndoEvent : public UndoEvent
{ mODTextTranslationClass(DahObjUndoEvent);
public:
			DahObjUndoEvent(float dah,float val,
					Well::DahObj&,bool isadd);

    const char*         getStandardDesc() const;
    bool                unDo();
    bool                reDo();

protected:

    bool		isadd_;
    Well::DahObj&	dahobj_;
    float		dah_;
    float		val_;
};


mExpClass(uiWellAttrib) uiCheckShotEdit : public uiDialog
{ mODTextTranslationClass(uiCheckShotEdit);
public:
				uiCheckShotEdit(uiParent*,Server&);
				~uiCheckShotEdit();

    mExpClass(uiWellAttrib) DriftCurve : public Well::DahObj
    {
    public:
			DriftCurve() : DahObj("Drift Curve") {}

	void		add( float dh, float val )  { dah_ += dh; val_ += val; }
	float		value( int idx ) const      { return val_[idx]; }
	bool		insertAtDah(float dh,float v);

	int		indexOfCurrentPoint(float dah,float val) const;

    protected:
	TypeSet<float>	val_;

	void		removeAux(int idx)	{ val_.removeSingle( idx ); }
	void		eraseAux()		{ val_.erase(); }
    };

protected:
    Server&			server_;
    Well::Data&			wd_;

    Well::D2TModel*		d2t_;
    Well::D2TModel*		orgd2t_;
    Well::D2TModel*		tkzs_;
    Well::D2TModel*		orgcs_;
    DriftCurve			driftcurve_;
    DriftCurve			newdriftcurve_;

    uiToolBar*			toolbar_;
    uiToolButton*		editbut_;
    uiToolButton*		undobut_;
    uiToolButton*		redobut_;
    uiCheckBox*			viewcorrd2t_;

    uiComboBox*			driftchoicefld_;

    bool			isedit_;

    uiWellDahDisplay*		d2tdisplay_;
    uiWellDahDisplay*		driftdisplay_;
    uiWellDisplayControl*	control_;
    uiPolyLineItem*		d2tlineitm_;

    Undo			undo_;
    int				movingpointidx_;

    void			draw();
    void			drawDahObj(const Well::DahObj* d,bool,bool);
    void			drawDrift();
    void			drawPoints();
    void			movePt();
    void			doInsertRemovePt();

    void			applyCB(CallBacker*);
    void			editCSPushed(CallBacker*);
    void			editCB(CallBacker*);
    void			undoCB(CallBacker*);
    void			redoCB(CallBacker*);
    void			mousePressedCB(CallBacker*);
    void			mouseReleasedCB(CallBacker*);
    void			mouseMovedCB(CallBacker*);
    void			setInfoMsg(CallBacker*);

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			reSizeCB(CallBacker*);
};

} // namespace WellTie

