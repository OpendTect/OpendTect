#ifndef uiwellsel_h
#define uiwellsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
 RCS:		$Id: uiwellsel.h,v 1.3 2012/02/24 23:13:20 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uiioobjsel.h"
#include "multiid.h"

class IOPar;

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
			~uiWellParSel();

    int			nrSel() const		{ return selids_.size(); }
    void		setSelected(const TypeSet<MultiID>&);
    void		getSelected(TypeSet<MultiID>&) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    void		doDlg(CallBacker*);
    BufferString	getSummary() const;

    TypeSet<MultiID>	selids_;
    IOPar&		iopar_;
};

#endif

