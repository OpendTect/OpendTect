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
class uiWellDisplayControl;

namespace WellTie
{

class Server;

mExpClass(uiWellAttrib) DahObjUndoEvent : public UndoEvent
{ mODTextTranslationClass(DahObjUndoEvent);
public:
			DahObjUndoEvent(float dah,float val,
					Well::DahObj&,bool isadd);

    const char*		getStandardDesc() const override;
    bool		unDo() override;
    bool		reDo() override;

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
	float		value( int idx ) const override    { return val_[idx]; }
	bool		insertAtDah(float dh,float v) override;

	int		indexOfCurrentPoint(float dah,float val) const;

    protected:
	TypeSet<float>	val_;

	void		removeAux(int idx) override
			    { val_.removeSingle( idx ); }

	void		eraseAux() override		{ val_.erase(); }
    };

protected:
    Server&			server_;
    Well::Data&			wd_;

    Well::D2TModel*		d2t_;
    Well::D2TModel*		orgd2t_;
    Well::D2TModel*		tkzs_;
    Well::D2TModel*		orgcs_ = nullptr;
    DriftCurve			driftcurve_;
    DriftCurve			newdriftcurve_;

    uiToolBar*			toolbar_;
    int				editbut_;
    int				undobut_;
    int				redobut_;
    uiCheckBox*			viewcorrd2t_;

    uiComboBox*			driftchoicefld_;

    bool			isedit_ = false;

    uiWellDahDisplay*		d2tdisplay_;
    uiWellDahDisplay*		driftdisplay_;
    uiWellDisplayControl*	control_;
    uiPolyLineItem*		d2tlineitm_;

    Undo			undo_;
    int				movingpointidx_ = -1;

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

    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    void			reSizeCB(CallBacker*);
};

} // namespace WellTie

