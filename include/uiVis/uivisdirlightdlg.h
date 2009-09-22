#ifndef uivisdirlightdlg_h
#define uivisdirlightdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id: uivisdirlightdlg.h,v 1.3 2009-09-22 11:27:04 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiSliderExtra;
class uiLabeledComboBox;
class uiGenInput;

namespace visBase { class DirectionalLight; }

mClass uiDirLightDlg : public uiDialog
{
public:
				uiDirLightDlg(uiParent*);

    bool			valueChanged() const;
    virtual void		show();

protected:

    visBase::DirectionalLight*	getCurrentDirLight() const;
    void			updateWidgets(bool);
    void			validateInput();

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			sceneSel(CallBacker*);
    void			fieldChangedCB(CallBacker*);
    
    uiLabeledComboBox*		scenefld_;
    uiSliderExtra*		azimuthsldrfld_;
    uiSliderExtra*		dipsldrfld_;
    uiGenInput*			intensityfld_;

    TypeSet<int>		sceneids_;
    bool			valchgd_;
    float			initazimuthval_;
    float			initdipval_;
    float			initintensityval_;

    void			setDirLight();

};

#endif
