#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.10 2001-07-13 22:04:40 bert Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
#include <uidialog.h>
class IOObj;
class CtxtIOObj;
class IODirEntryList;
class uiListBox;
class uiGenInput;

/*! \brief Dialog for selection of IOObjs */

class uiIOObjSelDlg : public uiDialog
{
public:
			uiIOObjSelDlg(uiParent*,const CtxtIOObj&);
			~uiIOObjSelDlg();

    const IOObj*	ioObj() const		{ return ioobj; }

protected:

    const CtxtIOObj&	ctio;
    IODirEntryList*	entrylist;
    IOObj*		ioobj;

    uiListBox*		listfld;
    uiGenInput*		nmfld;

    bool		acceptOK(CallBacker*);
    void		selChg(CallBacker*);
};


/*! \brief UI element for selection of IOObjs */

class uiIOObjSel : public uiIOSelect
{
public:
			uiIOObjSel(uiParent*,CtxtIOObj&,const char* txt=0,
				      bool withclear=false);

    bool		commitInput(bool mknew);

    void		updateInput(); //!< updates from CtxtIOObj
    void		processInput(); //!< Match user typing with existing
					//!< IOObjs, then set item accordingly
    bool		existingTyped() const;
					//!< returns false is typed input is
					//!< not an existing IOObj name
    CtxtIOObj&		ctxtIOObj()		{ return ctio; }

protected:

    CtxtIOObj&		ctio;
    bool		forread;

    void		doObjSel(CallBacker*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual void	objSel();

};


#endif
