#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.h,v 1.2 2003-10-17 14:19:01 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiTable;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class BufferStringSet;
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
    				uiLogSelDlg(uiParent*,const BufferStringSet&);

    int				selectedLog() const;

protected:

    uiLabeledListBox*		logsfld;

};


#endif
