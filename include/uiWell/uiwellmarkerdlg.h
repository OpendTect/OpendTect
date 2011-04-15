#ifndef uiwellmarkerdlg_h
#define uiwellmarkerdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: uiwellmarkerdlg.h,v 1.16 2011-04-15 11:12:44 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "wellmarker.h"

class uiStratLevelSel;
class uiGenInput;
class uiCheckBox;
class uiTable;
namespace Well { class Marker; class Track; }

/*! \brief Dialog for marker specifications */

mClass uiMarkerDlg : public uiDialog
{
public:
				uiMarkerDlg(uiParent*,const Well::Track&);

    void			setMarkerSet(const Well::MarkerSet&,
	    				     bool addtoexisting=false);
    bool			getMarkerSet(Well::MarkerSet&);

protected:

    uiTable*			table_;
    uiCheckBox*			unitfld_;
    const Well::Track&		track_;
    TypeSet<float>		depths_;

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
};

#endif
