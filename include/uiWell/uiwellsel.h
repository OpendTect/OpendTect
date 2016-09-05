#ifndef uiwellsel_h
#define uiwellsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uicompoundparsel.h"
#include "uiioobjsel.h"
#include "dbkey.h"


mExpClass(uiWell) uiWellSel : public uiIOObjSel
{ mODTextTranslationClass(uiWellSel)
public:

			uiWellSel(uiParent*,bool forread,const Setup&);
			uiWellSel(uiParent*,bool forread,
				const uiString& seltxt=uiString::emptyString(),
				bool withinserters=true);

protected:

    Setup		getSetup(bool,const uiString&,bool) const;
    IOObjContext	getContext(bool,bool) const;

};


mExpClass(uiWell) uiWellParSel : public uiCompoundParSel
{ mODTextTranslationClass(uiWellParSel);
public:
			uiWellParSel(uiParent*);
			~uiWellParSel();

    int			nrSelected() const	{ return selids_.size(); }
    void		setSelected(const TypeSet<DBKey>&);
    void		getSelected(TypeSet<DBKey>&) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    void		doDlg(CallBacker*);
    BufferString	getSummary() const;

    TypeSet<DBKey>	selids_;
    IOPar&		iopar_;
};

#endif
