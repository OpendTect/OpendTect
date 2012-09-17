#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.h,v 1.15 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uibaseobject.h"
#include "draw.h"

class uiStatusBarBody;
class QStatusBar;
class uiMainWin;


mClass uiStatusBar : public uiBaseObject
{

    friend class	uiMainWinBody;

public:
    			~uiStatusBar();

    int			addMsgFld(const char* lbltxt=0,
	    			  const char* tooltip =0,
				  Alignment::HPos al=Alignment::Left,
	    			  int stretch=1);

    int			addMsgFld(const char* tooltip,
				  Alignment::HPos al=Alignment::Left,
	    			  int stretch=1);

    void		setToolTip(int,const char*);
    void		setTxtAlign(int,Alignment::HPos);
    void		setLabelTxt(int,const char*);

    void 		message(const char*,int fldidx=0, int msecs=-1);
    void		setBGColor( int fldidx, const Color& );
    Color		getBGColor( int fldidx ) const;

protected:

                        uiStatusBar(uiMainWin*,const char*,QStatusBar&); 
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*, QStatusBar&);
};


#endif
