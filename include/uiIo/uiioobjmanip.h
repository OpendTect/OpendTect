#ifndef uiioobjmanip_h
#define uiioobjmanip_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2003
 RCS:           $Id: uiioobjmanip.h,v 1.1 2003-05-16 15:33:43 bert Exp $
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

protected:

    IODirEntryList&	entries;
    IOObj*		ioobj;
    BufferString	defext;

    uiListBox*		box;
    uiToolButton*	locbut;
    uiToolButton*	robut;
    uiToolButton*	renbut;
    uiToolButton*	rembut;

    void		tbPush(CallBacker*);

    bool		rmEntry(bool);
    bool		renEntry(Translator*);
    bool		chgEntry(Translator*);
    bool		roEntry(Translator*);
    bool		renImpl(Translator*,IOStream&,IOStream&);

};


//! Removes the underlying file(s) that an IOObj describes, with warnings
//!< if exist_lbl set to true, " existing " is added to message.
bool uiRmIOObjImpl(IOObj&,bool exist_lbl=false);


#endif
