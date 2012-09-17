#ifndef uiattrtypesel_h
#define uiattrtypesel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2006
 RCS:           $Id: uiattrtypesel.h,v 1.5 2010/12/03 15:41:28 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "bufstringset.h"

class uiComboBox;

/*!\brief Selector for attribute type

  Note that every attribute belongs to a group, but the usage of that
  is not mandatory.

  */

mClass uiAttrTypeSel : public uiGroup
{
public:
				uiAttrTypeSel(uiParent*,bool sorted=true);
				~uiAttrTypeSel();
    void			fill(BufferStringSet* selgrps=0);
						//!< with factory entries

    const char*			group() const;
    const char*			attr() const;
    void			setGrp(const char*);
    void			setAttr(const char*);

    Notifier<uiAttrTypeSel>	selChg;

    void			add(const char* grp,const char* attr);
    void			update();	//!< after a number of add()'s
    void			setEmpty();

    static const char*		sKeyAllGrp;

protected:

    uiComboBox*			grpfld;
    uiComboBox*			attrfld;

    BufferStringSet		grpnms_;
    BufferStringSet		attrnms_;
    TypeSet<int>		attrgroups_;
    int*			idxs_;
    bool			sorted_;

    void			grpSel(CallBacker*);
    void			attrSel(CallBacker*);
    int				curGrpIdx() const;
    void			updAttrNms(const char* s=0);
    bool			isPrefAttrib(int,const char*) const;

    void			clear();
};


#endif
