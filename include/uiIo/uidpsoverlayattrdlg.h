#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
