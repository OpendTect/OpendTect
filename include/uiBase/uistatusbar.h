#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibaseobject.h"
#include "draw.h"
#include "uistring.h"


mFDQtclass(QStatusBar)
class uiStatusBarBody;
class uiMainWin;


mExpClass(uiBase) uiStatusBar : public uiBaseObject
{

    friend class	uiMainWinBody;

public:
			~uiStatusBar();

    int 		addMsgFld(const uiString& lbltxt=0,
				  const uiString& tooltip =0,
				  Alignment::HPos al=Alignment::Left,
				  int stretch=1);

    int 		addMsgFld(const uiString& tooltip,
				  Alignment::HPos al=Alignment::Left,
				  int stretch=1);

    void		setToolTip(int,const uiString&);
    void		setTxtAlign(int,Alignment::HPos);
    void		setLabelTxt(int,const uiString&);

    int			nrFields() const;
    void		message(const uiString&,int fldidx=0, int msecs=-1);
    void		setEmpty(int startat=0);
    void		setBGColor(int fldidx,const Color&);
    Color		getBGColor(int fldidx) const;

protected:

                        uiStatusBar(uiMainWin*,const char*,
				    mQtclass(QStatusBar&));
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*,
			       mQtclass(QStatusBar&));
};


#endif

