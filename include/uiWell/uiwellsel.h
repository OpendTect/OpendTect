#ifndef uiwellsel_h
#define uiwellsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
 RCS:		$Id: uiwellsel.h,v 1.2 2012-02-17 23:09:22 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uiioobjsel.h"
#include "multiid.h"

mClass uiWellSel : public uiIOObjSel
{
public:
			uiWellSel(uiParent*,bool forread,const char* lbltxt=0);

protected:
};


mClass uiWellParSel : public uiCompoundParSel
{
public:
			uiWellParSel(uiParent*);

    void		setSelected(const TypeSet<MultiID>&);
    void		getSelected(TypeSet<MultiID>&) const;

protected:

    void		doDlg(CallBacker*);
    BufferString	getSummary() const;

    TypeSet<MultiID>	selids_;
};

#endif

