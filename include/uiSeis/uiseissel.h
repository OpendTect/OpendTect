#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseissel.h,v 1.14 2004-10-15 09:50:37 bert Exp $
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

    static const char*	standardTranslSel(int pol2d);
    			//!< returns "CBVS`2D" for pol2d == Both2DAnd3D

protected:

    uiSeisSubSel*	subsel;
    uiGenInput*		attrfld;
    BufferString	orgkeyvals;

    void		entrySel(CallBacker*);
    void		fillFlds(CallBacker*);

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
    IOPar&		iopar;
    BufferString	orgkeyvals;
    BufferString	attrnm;
    const char**	seltxts;
    mutable BufferString curusrnm;

    virtual void	updateInput();
    virtual void	newSelection(uiIOObjRetDlg*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual uiIOObjRetDlg* mkDlg();

};


#endif
