#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseissel.h,v 1.1 2003-10-01 12:51:42 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
class uiBinIDSubSel;


class uiSeisSelDlg : public uiIOObjSelDlg
{
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     bool withz=false,
				     const char* selectlistannot=0);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
    void		selChg(CallBacker*);

protected:

    uiBinIDSubSel*	subsel;

};


class uiSeisSel : public uiIOObjSel
{
public:

			uiSeisSel(uiParent*,CtxtIOObj&,const char* txt=0,
				  bool wthz=false,bool wthclear=false,
				  const char* selectlistannot=0);

    virtual bool	fillPar(IOPar&) const;
			//!< Extra, ctio will be filled anyway
    virtual void	usePar(const IOPar&);
			//!< Extra, ctio will be used anyway

protected:

    bool		withz;

    virtual void	newSelection(uiIOObjRetDlg*);
    virtual uiIOObjRetDlg* mkDlg();

};


#endif
