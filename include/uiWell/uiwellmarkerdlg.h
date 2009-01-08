#ifndef uiwellmarkerdlg_h
#define uiwellmarkerdlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: uiwellmarkerdlg.h,v 1.6 2009-01-08 09:16:21 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiStratLevelSel;
class uiGenInput;
class uiTable;
namespace Well { class Marker; class Track; }

/*! \brief Dialog for marker specifications */

mClass uiMarkerDlg : public uiDialog
{
public:
				uiMarkerDlg(uiParent*,const Well::Track&);

    void			setMarkerSet(const ObjectSet<Well::Marker>&,
	    				     bool addtoexisting=false);
    void			getMarkerSet(ObjectSet<Well::Marker>&) const;

protected:

    uiTable*			table_;
    uiGenInput*			unitfld_;
    const Well::Track&		track_;

    int				getNrRows() const;
    int				rowNrFor(uiStratLevelSel*) const;
    void			mouseClick(CallBacker*);
    void			doStrat(CallBacker*);
    void			rdFile(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			stratLvlChg(CallBacker*);
    void			updateFromLevel(int,uiStratLevelSel*);
};

#endif
