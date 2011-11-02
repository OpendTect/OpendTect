#ifndef uiwelltiecheckshotedit_h
#define uiwelltiecheckshotedit_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Oct 2011
RCS:           $Id: uiwellwelltiecheckshotedit.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "welldahobj.h"

namespace Well { class D2TModel; class Data; class DahObj;}
class uiWellDahDisplay;
class uiCheckBox;
class uiComboBox;
class uiGraphicsView;
class uiToolBar;
class uiToolButton;
class uiWellDisplayControl;

namespace WellTie
{

mClass uiCheckShotEdit : public uiDialog
{
public:
				uiCheckShotEdit(uiParent*,Well::Data&,
					    const char* vellog,bool issonic);
				~uiCheckShotEdit();

    void			fillPar(IOPar&) const 	{}
    void			usePar(const IOPar&)	{}

protected:

    mClass DriftCurve : public Well::DahObj
    {
    public:
			DriftCurve() : DahObj("Drift Curve") {}

	void		add( float dh, float val )  { dah_ += dh; val_ += val; }
	float  		value( int idx ) const      { return val_[idx]; }

    protected:
	TypeSet<float>	val_;

	void 		removeAux(int idx)	    { val_.remove( idx ); }
	void		eraseAux() 		    { val_.erase(); } 
    };

    Well::Data&			wd_;
    Well::D2TModel*		d2t_;
    Well::D2TModel*		cs_;
    Well::D2TModel*		orgcs_;
    Well::D2TModel*		orgd2t_;
    DriftCurve			driftcurve_;
    DriftCurve			csdriftcurve_;

    uiToolBar*			toolbar_;
    uiToolButton*		editbut_;

    bool			dointerpolatecs_;
    bool			dodrawpoints_;
    bool			dodrawcurves_;
    bool			isedit_;

    uiWellDahDisplay*		d2tdisplay_;
    uiWellDahDisplay*		driftdisplay_;
    uiWellDisplayControl*	control_;

    void			draw();
    void			drawDahObj(const Well::DahObj* d,bool,bool);
    void			drawDrift();

    bool			acceptOK(CallBacker*);
    void			editCSPushed(CallBacker*);

    void			setInfoMsg(CallBacker*);
    void			mousePressedCB(CallBacker*);
    void			mouseReleasedCB(CallBacker*);

    void			editCB(CallBacker*);
    void			parChg(CallBacker*);
};


}; //namespace WellTie

#endif

