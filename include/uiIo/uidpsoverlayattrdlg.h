#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2010
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiColorTableGroup;
class uiComboBox;
class uiDataPointSetCrossPlotter;

/*!
\brief Dialog box to display properties within points in crossplot.
*/

mExpClass(uiIo) uiDPSOverlayPropDlg : public uiDialog
{ mODTextTranslationClass(uiDPSOverlayPropDlg);
public:
			uiDPSOverlayPropDlg(uiParent*,
					    uiDataPointSetCrossPlotter&);
    uiDataPointSetCrossPlotter&	plotter()		{ return plotter_; }

protected:

    uiDataPointSetCrossPlotter&	plotter_;
    uiColorTableGroup*		y3coltabfld_;
    uiColorTableGroup*		y4coltabfld_;
    uiComboBox*			y3propselfld_;
    uiComboBox*			y4propselfld_;
    TypeSet<int>		colids_;

    const char* 	userName(int did) const;
    void		doApply(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		attribChanged(CallBacker*);
    void		scaleChanged(CallBacker*);

};

