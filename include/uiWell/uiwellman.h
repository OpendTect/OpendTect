#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.1 2003-09-08 13:08:02 nanne Exp $
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

namespace Well { class LogSet; class Marker; };

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


#endif
