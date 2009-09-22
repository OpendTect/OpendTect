#ifndef uivisdirlightdlg_h
#define uivisdirlightdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id: uivisdirlightdlg.h,v 1.2 2009-09-22 09:55:32 cvskarthika Exp $
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

protected:

    visBase::DirectionalLight*	getCurrentDirLight() const;
    void			updateWidgets(bool);
    void			validateInput();

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			sceneSel(CallBacker*);
    void			fieldChangedCB(CallBacker*);
    virtual void		show();
    
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
