#ifndef uiwellmarkerdlg_h
#define uiwellmarkerdlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: uiwellmarkerdlg.h,v 1.3 2008-02-26 09:17:36 cvsnanne Exp $
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

    uiTable*			table_;
    uiGenInput*			unitfld_;
    const Well::Track&		track_;

    int				getNrRows() const;
    void			mouseClick(CallBacker*);
    void			rdFile(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			stratLvlChg(CallBacker*);
};

#endif
