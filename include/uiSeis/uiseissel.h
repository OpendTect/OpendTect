#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseissel.h,v 1.23 2007-11-23 11:59:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
class uiSeisSubSel;
namespace Seis { class SelSetup; }


class uiSeisSelDlg : public uiIOObjSelDlg
{
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     const Seis::SelSetup&);
			~uiSeisSelDlg();

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    static const char*	standardTranslSel(bool);

protected:

    uiGenInput*		attrfld_;
    bool		is2d_;
    bool		allowcnstrsabsent_;	//2D only

    void		entrySel(CallBacker*);
    void		filter2DStoredNames(BufferStringSet&) const;
};


class uiSeisSel : public uiIOObjSel
{
public:

			uiSeisSel(uiParent*,CtxtIOObj&,const Seis::SelSetup&,
				  bool wthclear=false,
				  const char** sel_labels=0);
				//!< See .cc code for sel_labels
			~uiSeisSel();

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    bool		is2D() const;
    void		setAttrNm(const char*);
    const char*		attrNm() const		{ return attrnm.buf(); }
    virtual void	processInput();
    virtual bool	existingTyped() const;
    virtual void	updateInput();

protected:

    Seis::SelSetup&	setup;
    BufferString	orgkeyvals;
    BufferString	attrnm;
    const char**	seltxts;
    mutable BufferString curusrnm;
    IOPar&		dlgiopar;
    IOPar&		orgparconstraints;

    virtual void	newSelection(uiIOObjRetDlg*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual uiIOObjRetDlg* mkDlg();
};


#endif
