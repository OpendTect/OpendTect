#ifndef uiwellsel_h
#define uiwellsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
 RCS:		$Id: uiwellsel.h,v 1.1 2012-02-14 23:23:12 cvsnanne Exp $
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
    void		getSelected(TypeSet<MultiID>&);

protected:

    void		doDlg(CallBacker*);
    BufferString	getSummary() const;

    BufferStringSet	selnms_;                                   
    TypeSet<MultiID>	selids_;
};

#endif

