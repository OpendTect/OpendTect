#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseissel.h,v 1.3 2004-06-28 07:43:06 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
class uiBinIDSubSel;
class ui2DSeisSubSel;


class uiSeisSelDlg : public uiIOObjSelDlg
{
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     const SeisSelSetup&);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    static const char*	standardTranslSel(int pol2d);
    			//!< returns "CBVS`2D" for pol2d == 0

protected:

    SeisSelSetup	setup;
    bool		is2d;

    uiBinIDSubSel*	subsel;
    ui2DSeisSubSel*	subsel2d;

    void		entrySel(CallBacker*);
    void		fillFlds(CallBacker*);

private:

    const CtxtIOObj&	getCtio(const CtxtIOObj&,const SeisSelSetup&);

};


class uiSeisSel : public uiIOObjSel
{
public:

			uiSeisSel(uiParent*,CtxtIOObj&,const char* txt,
				  const SeisSelSetup&,bool wthclear=false);

    virtual bool	fillPar(IOPar&) const;
			//!< Extra, ctio will be filled anyway
    virtual void	usePar(const IOPar&);
			//!< Extra, ctio will be used anyway

protected:

    SeisSelSetup	setup;

    virtual void	newSelection(uiIOObjRetDlg*);
    virtual uiIOObjRetDlg* mkDlg();

};


#endif
