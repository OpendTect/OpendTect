#ifndef uiwellmarkerdlg_h
#define uiwellmarkerdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "wellmarker.h"

class uiStratLevelSel;
class uiGenInput;
class uiCheckBox;
class uiTable;
namespace Well { class Marker; class Track; class MarkerSet; }

/*! \brief Dialog for marker specifications */

mClass(uiWell) uiMarkerDlg : public uiDialog
{
public:
				uiMarkerDlg(uiParent*,const Well::Track&);
				~uiMarkerDlg();

    void			setMarkerSet(const Well::MarkerSet&,
	    				     bool addtoexisting=false);
    bool			getMarkerSet(Well::MarkerSet&);

protected:

    uiTable*			table_;
    uiCheckBox*			unitfld_;
    const Well::Track&		track_;
    TypeSet<float>		depths_;
    Well::MarkerSet*		oldmrkrs_;

    //TODO will go with the Strat level Sel 

    int				getNrRows() const;
    int				rowNrFor(uiStratLevelSel*) const;
    void			mouseClick(CallBacker*);
    void			doStrat(CallBacker*);
    void			rdFile(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			stratLvlChg(CallBacker*);
    void			unitChangedCB(CallBacker*);
    void			updateFromLevel(int,uiStratLevelSel*);
    void			markerChangedCB(CallBacker*);
    void			markerAddedCB(CallBacker*);
    void			markerRemovedCB(CallBacker*);
    bool			setAsRegMarkersCB(CallBacker*);
    float			zFactor() const;
    void			exportCB(CallBacker*);
    bool			getKey(MultiID&) const;
    void			updateDisplayCB(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			updateMarkerDepths(int rowidx, bool md2tvdss);
};

#endif

