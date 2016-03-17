#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibaseobject.h"
#include "draw.h"
#include "uistring.h"

mFDQtclass(QStatusBar)
class uiStatusBarBody;
class uiMainWin;
class uiObject;


mExpClass(uiBase) uiStatusBar : public uiBaseObject
{

    friend class	uiMainWinBody;

public:
			~uiStatusBar();

    int		addMsgFld(const uiString& lbltxt=uiString::emptyString(),
			  const uiString& tooltip =uiString::emptyString(),
			  OD::Alignment::HPos al=OD::Alignment::Left,
			  int stretch=1);

    int		addMsgFld(const uiString& tooltip,
				  OD::Alignment::HPos al=OD::Alignment::Left,
				  int stretch=1);
    bool		addObject(uiObject*);

    void		setToolTip(int,const uiString&);
    void		setTxtAlign(int,OD::Alignment::HPos);
    void		setLabelTxt(int,const uiString&);

    int			nrFields() const;
    void		message(const uiString&,int fldidx=0, int msecs=-1);
    void		setEmpty(int startat=0);
    void		setBGColor(int fldidx,const Color&);
    Color		getBGColor(int fldidx) const;

    int			getNrWidgets() const		{ return 1; }
    mQtclass(QWidget)*	getWidget(int);

protected:

                        uiStatusBar(uiMainWin*,const char*,
				    mQtclass(QStatusBar&));
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*,
			       mQtclass(QStatusBar&));
};


#endif
