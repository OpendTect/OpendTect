#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.2 2003-09-10 15:23:30 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IODirEntryList;
class IOObj;
class uiButtonGroup;
class uiIOObjManipGroup;
class uiListBox;
class uiTextEdit;
class uiToolButton;
class uiTable;

class uiFileInput;
class uiGenInput;
class uiLabeledListBox;
class uiLabel;

namespace Well { class Data; class LogSet; class Marker; };

/*! \brief
Well manager
*/

class uiWellMan : public uiDialog
{
public:
    				uiWellMan(uiParent*);
				~uiWellMan();

protected:

    IODirEntryList*		entrylist;
    uiListBox*			listfld;
    uiTextEdit*			infofld;
    uiIOObjManipGroup*		manipgrp;
    uiListBox*			logsfld;
    uiToolButton*		rembut;
    uiButtonGroup*		butgrp;

    IOObj*			ioobj;
    CtxtIOObj&			ctio;

    void			remLogPush(CallBacker*);
    void			selChg(CallBacker*);
    void			mkFileInfo();
    BufferString		getFileSize(const char*);
    void			fillLogsFld(const Well::LogSet&);

    void			addMarkers(CallBacker*);
    void			addLogs(CallBacker*);
};


class uiMarkerDlg : public uiDialog
{
public:
				uiMarkerDlg(uiParent*);

    void			setMarkerSet(const ObjectSet<Well::Marker>&);
    void			getMarkerSet(ObjectSet<Well::Marker>&) const;

protected:

    uiTable*			table;

    void			markerAdded(CallBacker*);
    void			mouseClick(CallBacker*);
};


class uiLoadLogsDlg : public uiDialog
{
public:
    				uiLoadLogsDlg(uiParent*,Well::Data&);

protected:

    uiFileInput*		lasfld;
    uiGenInput*			intvfld;
    uiLabel*			unitlbl;
    uiGenInput*			udffld;
    uiLabeledListBox*		logsfld;

    Well::Data&			wd;

    bool			acceptOK(CallBacker*);
    void			lasSel(CallBacker*);
};

#endif
