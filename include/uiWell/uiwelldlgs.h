#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.h,v 1.9 2004-03-09 15:46:49 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"

class uiButtonGroup;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiLabel;
class uiLabeledListBox;
class StreamData;
class uiTable;

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
    uiGenInput*			unitfld;

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
    uiLabel*			unitlbl;
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


class uiExportLogs : public uiDialog
{
public:
    				uiExportLogs(uiParent*,const Well::Data&,
					    const BoolTypeSet&);

protected:
    const Well::Data&		wd;
    const BoolTypeSet&		logsel;

    uiGenInput*			typefld;
    uiButtonGroup*		zunitgrp;
    uiGenInput*			zrangefld;
    uiFileInput*		outfld;

    void			setDefaultRange(bool);
    void			typeSel(CallBacker*);
    virtual bool		acceptOK(CallBacker*);
    void			writeHeader(StreamData&);
    void			writeLogs(StreamData&);
};

#endif
