#ifndef uiwellmarkerdlg_h
#define uiwellmarkerdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: uiwellmarkerdlg.h,v 1.8 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiStratLevelSel;
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
