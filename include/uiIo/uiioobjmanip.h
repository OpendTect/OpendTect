#ifndef uiioobjmanip_h
#define uiioobjmanip_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2003
 RCS:           $Id: uiioobjmanip.h,v 1.4 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include <uibuttongroup.h>
class IOObj;
class MultiID;
class IOStream;
class uiListBox;
class Translator;
class uiToolButton;
class IODirEntryList;


/*! \brief Buttongroup to manipulate an IODirEntryList. */

class uiIOObjManipGroup : public uiButtonGroup
{
public:
			uiIOObjManipGroup(uiListBox*,IODirEntryList&,
				      const char* default_extension);
			~uiIOObjManipGroup()		{}

    void		selChg(CallBacker*);
    void		refreshList(const MultiID& selkey);

    Notifier<uiIOObjManipGroup>	preRelocation;
    Notifier<uiIOObjManipGroup>	postRelocation;
    const char*		curRelocationMsg() const	{ return relocmsg; }

protected:

    IODirEntryList&	entries;
    IOObj*		ioobj;
    BufferString	defext;
    BufferString	relocmsg;

    uiListBox*		box;
    uiToolButton*	locbut;
    uiToolButton*	robut;
    uiToolButton*	renbut;
    uiToolButton*	rembut;

    void		tbPush(CallBacker*);
    void		relocCB(CallBacker*);

    bool		rmEntry(bool);
    bool		renameEntry(Translator*);
    bool		relocEntry(Translator*);
    bool		readonlyEntry(Translator*);

    bool		doReloc(Translator*,IOStream&,IOStream&);

};


//! Removes the underlying file(s) that an IOObj describes, with warnings
//!< if exist_lbl set to true, " existing " is added to message.
bool uiRmIOObjImpl(IOObj&,bool exist_lbl=false);


#endif
