#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.h,v 1.7 2004-02-19 14:02:53 bert Exp $
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
    bool			acceptOK(CallBacker*);
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
    uiGenInput*			ftfld;
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
    uiCheckBox*			autofld;
    uiGenInput*			dispfld;

    void			logSel(CallBacker*);
    virtual bool		acceptOK(CallBacker*);

};


#endif
