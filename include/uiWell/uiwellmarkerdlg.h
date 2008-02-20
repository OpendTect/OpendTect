#ifndef uiwellmarkerdlg_h
#define uiwellmarkerdlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: uiwellmarkerdlg.h,v 1.2 2008-02-20 04:44:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiTable;
namespace Well { class Marker; class Track; }

/*! \brief Dialog for marker specifications */

class uiMarkerDlg : public uiDialog
{
public:
				uiMarkerDlg(uiParent*,const Well::Track&);

    void			setMarkerSet(const ObjectSet<Well::Marker>&,
	    				     bool addtoexisting=false);
    void			getMarkerSet(ObjectSet<Well::Marker>&) const;

protected:

    uiTable*			table;
    uiGenInput*			unitfld;
    const Well::Track&		track;

    int				getNrRows() const;
    void			mouseClick(CallBacker*);
    void			rdFile(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			stratLvlChg(CallBacker*);
};

#endif
