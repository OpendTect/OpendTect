#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.h,v 1.15 2004-10-22 14:37:52 bert Exp $
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

namespace Well
{ class Data; class LogSet; class Marker; class D2TModel; class Track; };


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
};


/*! \brief Dialog for D2T Model editing. */

class uiD2TModelDlg : public uiDialog
{
public:
				uiD2TModelDlg(uiParent*,Well::Data&);
				~uiD2TModelDlg();

protected:

    Well::Data&			wd;
    Well::D2TModel&		d2t;
    Well::D2TModel*		orgd2t;

    uiTable*			table;
    uiGenInput*			unitfld;

    void			updNow(CallBacker*);
    bool			rejectOK(CallBacker*);
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
    uiGenInput*			intvunfld;
    uiGenInput*			istvdfld;
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
    bool			scaleLogarithmic() const;

protected:

    const Well::LogSet&		logset;

    uiLabeledListBox*		logsfld;
    uiGenInput*			rangefld;
    uiCheckBox*			logscfld;
    uiGenInput*			dispfld;

    void			resetPush(CallBacker*);
    void			logSel(CallBacker*);
    virtual bool		acceptOK(CallBacker*);

private:

    void			setLogRg(bool);

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
