#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.h,v 1.10 2008-12-19 04:45:05 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uihandle.h"
#include "draw.h"

class uiStatusBarBody;
class QStatusBar;
class uiMainWin;


class uiStatusBar : public uiObjHandle
{

    friend class	uiMainWinBody;

public:

    int			addMsgFld(const char* lbltxt=0,
	    			  const char* tooltip =0,
				  OD::Alignment al=OD::AlignLeft,
	    			  int stretch=1);

    int			addMsgFld(const char* tooltip,
				  OD::Alignment al=OD::AlignLeft,
	    			  int stretch=1);

    void		setToolTip(int,const char*);
    void		setTxtAlign(int,OD::Alignment);
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
