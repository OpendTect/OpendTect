#ifndef uivisdirlightdlg_h
#define uivisdirlightdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id: uivisdirlightdlg.h,v 1.1 2009-09-21 07:10:56 cvskarthika Exp $
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

    uiLabeledComboBox*		scenefld_;
    uiSliderExtra*		azimuthsliderfld_;
    uiSliderExtra*		dipsliderfld_;
    uiGenInput*			intensityfld_;

    TypeSet<int>		sceneids_;
    bool			valchgd_;
    int				initazimuthval_;
    int				initdipval_;
    float			initintensityval_;

    void			setDirLight();
    visBase::DirectionalLight*	getCurrentDirLight() const;
    void			updateWidgets();

    bool			acceptOK(CallBacker*);
    void			sceneSel(CallBacker*);
    void			fieldChangedCB(CallBacker*);

};

#endif
