#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.h,v 1.1 2003-10-16 15:01:37 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiLabeledListBox;
class uiTable;

namespace Well { class Data; class LogSet; class Marker; };

/*! \brief
Dialog for marker specifications
*/

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
    				uiLogSelDlg(uiParent*,
					    const ObjectSet<BufferString>&);

    int				selectedLog() const;

protected:

    uiLabeledListBox*		logsfld;

};


#endif
