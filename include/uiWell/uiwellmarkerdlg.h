#ifndef uiwellmarkerdlg_h
#define uiwellmarkerdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: uiwellmarkerdlg.h,v 1.12 2010-12-23 11:34:02 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiStratLevelSel;
class uiCheckBox;
class uiGenInput;
class uiTable;
namespace Well { class Marker; class MarkerSet; class Track; }

/*! \brief Dialog for marker specifications */

mClass uiMarkerDlg : public uiDialog
{
public:
				uiMarkerDlg(uiParent*,const Well::Track&);

    void			setMarkerSet(const Well::MarkerSet&,
	    				     bool addtoexisting=false);
    void			getMarkerSet(Well::MarkerSet&) const;

protected:

    uiTable*			table_;
    uiGenInput*			unitfld_;
    uiCheckBox*			stratmrkfld_;
    const Well::Track&		track_;

    //TODO will go with the Strat level Sel 
    ObjectSet<const Well::Marker> markers_;

    int				getNrRows() const;
    int				rowNrFor(uiStratLevelSel*) const;
    void			mouseClick(CallBacker*);
    void			doStrat(CallBacker*);
    void			rdFile(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			stratLvlChg(CallBacker*);
    void			unitChangedCB(CallBacker*);
    void			updateFromLevel(int,uiStratLevelSel*);
};

#endif
