#pragma once

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
class uiCheckBox;
class uiTable;
namespace Well {
    class Data;
    class Marker;
    class Track;
    class MarkerSet;
    class Reader;
}

/*! \brief Dialog for marker specifications */

mExpClass(uiWell) uiMarkerDlg : public uiDialog
{ mODTextTranslationClass(uiMarkerDlg);
public:
				uiMarkerDlg(uiParent*,const Well::Track&);
				~uiMarkerDlg();

    void			setMarkerSet(const Well::MarkerSet&,
					     bool addtoexisting=false);
    bool			getMarkerSet(Well::MarkerSet&) const;
    static void			exportMarkerSet(uiParent* p,
						const Well::MarkerSet& mset,
						const Well::Track& trck, 
						uiCheckBox* cb=0 );
protected:

    uiTable*			table_;
    uiCheckBox*			unitfld_;
    const Well::Track&		track_;
    Well::MarkerSet*		oldmrkrs_;

    //TODO will go with the Strat level Sel

    void			getColLabels(BufferStringSet&) const;
    int				getNrRows() const;
    int				rowNrFor(uiStratLevelSel*) const;
    void			mouseClick(CallBacker*);
    void			doStrat(CallBacker*);
    void			rdFile(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			stratLvlChg(CallBacker*);
    void			unitChangedCB(CallBacker*);
    void			updateFromLevel(int,uiStratLevelSel*);
    bool			getFromScreen();
    void			markerChangedCB(CallBacker*);
    void			markerAddedCB(CallBacker*);
    bool			setAsRegMarkersCB(CallBacker*);
    float			zFactor() const;
    void			exportCB(CallBacker*);
    bool			getKey(MultiID&) const;
    void			updateDisplayCB(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			updateMarkerDepths(int rowidx,bool md2tvdss);
    float			getOldMarkerVal(Well::Marker*) const;

				//This marker is now yours
    Well::Marker*		getMarker(int rowidx,bool fromname) const;

    void			assignRandomColorsCB(CallBacker*);
    void			assignRandomColors(Well::MarkerSet&);

};


/*! \brief Dialog for Viewing the markers for a well */

mExpClass(uiWell) uiMarkerViewDlg : public uiDialog
{ mODTextTranslationClass(uiMarkerViewDlg)
public:
				uiMarkerViewDlg(uiParent*,const Well::Data&);

protected:

    uiTable*			table_;
    const Well::Data*		wd_;

    void			exportCB(CallBacker*);

};


