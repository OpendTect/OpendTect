#ifndef uidpsoverlayattrdlg_h
#define uidpsoverlayattrdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2010
 SVN:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiColorTable;
class uiComboBox;
class uiDataPointSetCrossPlotter;

/*!
\brief Dialog box to display properties within points in crossplot.
*/

mExpClass(uiIo) uiDPSOverlayPropDlg : public uiDialog
{
public:
			uiDPSOverlayPropDlg(uiParent*,
					    uiDataPointSetCrossPlotter&);
    uiDataPointSetCrossPlotter&	plotter()		{ return plotter_; }

protected:

    uiDataPointSetCrossPlotter&	plotter_;
    uiColorTable*		y3coltabfld_;
    uiColorTable*		y4coltabfld_;
    uiComboBox*			y3propselfld_;
    uiComboBox*			y4propselfld_;
    TypeSet<int>		colids_;

    const char* 	userName(int did) const;
    void		doApply(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		attribChanged(CallBacker*);

};

#endif

