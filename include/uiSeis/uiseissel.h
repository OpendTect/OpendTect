#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseissel.h,v 1.18 2006-12-12 11:16:57 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
class uiSeisSubSel;
class SeisSelSetup;


class uiSeisSelDlg : public uiIOObjSelDlg
{
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     const SeisSelSetup&);
			~uiSeisSelDlg();

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    static const char*	standardTranslSel(Pol2D);
    			//!< returns "CBVS`2D" for pol2d == Both2DAnd3D

protected:

    uiGenInput*		attrfld;
    BufferString	orgkeyvals;
    Pol2D		p2d;

    void		entrySel(CallBacker*);

private:

    const CtxtIOObj&	getCtio(const CtxtIOObj&,const SeisSelSetup&);

};


class uiSeisSel : public uiIOObjSel
{
public:

			uiSeisSel(uiParent*,CtxtIOObj&,const SeisSelSetup&,
				  bool wthclear=false,
				  const char** sel_labels=0);
				//!< See .cc code for sel_labels
			~uiSeisSel();

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    bool		is2D() const;
    void		set2DPol(Pol2D);
    void		setAttrNm(const char*);
    const char*		attrNm() const		{ return attrnm.buf(); }
    virtual void	processInput();
    virtual bool	existingTyped() const;

protected:

    SeisSelSetup&	setup;
    BufferString	orgkeyvals;
    BufferString	attrnm;
    const char**	seltxts;
    mutable BufferString curusrnm;
    IOPar&		dlgiopar;
    IOPar&		orgparconstraints;

    virtual void	updateInput();
    virtual void	newSelection(uiIOObjRetDlg*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual uiIOObjRetDlg* mkDlg();

};


#endif
