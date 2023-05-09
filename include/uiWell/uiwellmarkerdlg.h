#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"

#include "welldata.h"
#include "wellmarker.h"

class uiCheckBox;
class DisplayProperties;
class uiStratLevelSel;
class uiTable;
namespace Well { class Reader; }

/*! \brief Dialog for marker specifications */

mExpClass(uiWell) uiMarkerDlg : public uiDialog
{ mODTextTranslationClass(uiMarkerDlg);
public:
				uiMarkerDlg(uiParent*,const Well::Track&,
					    const Well::D2TModel*);
				~uiMarkerDlg();

    void			setMarkerSet(const Well::MarkerSet&,
					     bool addtoexisting=false);
    bool			getMarkerSet(Well::MarkerSet&) const;
    static void			exportMarkerSet(uiParent* p,
						const Well::MarkerSet& mset,
						const Well::Track& trck,
						const Well::D2TModel* d2t,
						uiCheckBox* =nullptr );
private:

    uiTable*			table_;
    uiCheckBox*			unitfld_;
    const Well::Track&		track_;
    const Well::D2TModel*	d2tmodel_;
    Well::MarkerSet*		oldmrkrs_ = nullptr;
    Well::DisplayProperties*	olddisps_ = nullptr;

    //TODO will go with the Strat level Sel

    void			getColLabels(uiStringSet&) const;
    int				getNrRows() const;
    int				rowNrFor(uiStratLevelSel*) const;
    void			mouseClick(CallBacker*);
    void			doStrat(CallBacker*);
    void			rdFile(CallBacker*);
    bool			acceptOK(CallBacker*) override;
    void			stratLvlChg(CallBacker*);
    void			unitChangedCB(CallBacker*);
    void			updateFromLevel(int,uiStratLevelSel*);
    bool			getFromScreen();
    void			markerChangedCB(CallBacker*);
    void			markerAddedCB(CallBacker*);
    void			setAsRegMarkersCB(CallBacker*);
    float			zFactor() const;
    void			exportCB(CallBacker*);
    bool			getKey(MultiID&) const;
    void			getUnselMarkerNames(BufferStringSet&) const;
    void			updateDisplayCB(CallBacker*);
    bool			rejectOK(CallBacker*) override;
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
				~uiMarkerViewDlg();

private:

    uiTable*			table_;
    ConstRefMan<Well::Data>	wd_ = nullptr;

    void			exportCB(CallBacker*);

};
