#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.h,v 1.3 2003-10-17 15:00:41 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"

class uiTable;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiLabeledListBox;

namespace Well { class Data; class LogSet; class Marker; };

/*! \brief Dialog for marker specifications */

class uiMarkerDlg : public uiDialog
{
public:
				uiMarkerDlg(uiParent*);

    void			setMarkerSet(const ObjectSet<Well::Marker>&);
    void			getMarkerSet(ObjectSet<Well::Marker>&) const;

protected:

    uiTable*			table;
    uiCheckBox*			feetfld;

    void			markerAdded(CallBacker*);
    void			mouseClick(CallBacker*);
};



/*! \brief
Dialog for loading logs from las file
*/

class uiLoadLogsDlg : public uiDialog
{
public:
    				uiLoadLogsDlg(uiParent*,Well::Data&);

protected:

    uiFileInput*		lasfld;
    uiGenInput*			intvfld;
    uiGenInput*			udffld;
    uiLabeledListBox*		logsfld;

    Well::Data&			wd;

    bool			acceptOK(CallBacker*);
    void			lasSel(CallBacker*);
};


/*! \brief
Dialog for selecting logs for display
*/

class uiLogSelDlg : public uiDialog
{
public:
    				uiLogSelDlg(uiParent*,const Well::LogSet&);

    int				logNumber() const;
    int				selectedLog() const;
    Interval<float>		selRange() const;

protected:

    const Well::LogSet&		logset;

    uiLabeledListBox*		logsfld;
    uiGenInput*			rangefld;
    uiGenInput*			dispfld;

    void			logSel(CallBacker*);

};


#endif
