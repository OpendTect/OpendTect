#ifndef uigeninput_h
#define uigeninput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Oct 2000
 RCS:           $Id: uigeninput.h,v 1.1 2000-11-27 10:19:43 bert Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
class uiLineEdit;
class uiLabel;


class uiGenInput : public uiGroup
{
public:
		uiGenInput(uiObject*,const char* disptxt,const char* definp=0);
		uiGenInput(uiObject*,const char*,const char*,const char*);
		uiGenInput(uiObject*,const char*,int);
		uiGenInput(uiObject*,const char*,float);
		uiGenInput(uiObject*,const char*,double);
		uiGenInput(uiObject*,const char*,int,int);
		uiGenInput(uiObject*,const char*,float,float);
		uiGenInput(uiObject*,const char*,double,double);

    virtual bool isSingleLine() const { return true; }

    const char*	text(int nr=0) const;
    int		getIntValue(int nr=0) const;
    double	getValue(int nr=0) const;

    void	setText(const char*,int nr=0);
    void	setValue(float,int nr=0);
    void	setValue(double,int nr=0);

    uiLineEdit*	edit( int nr=0 )	{ return le[nr]; }
    uiLabel*	label()			{ return labl; }

protected:

    uiLineEdit*	le[2];
    uiLabel*	labl;

    void	init(const char*,const char*,const char*);

};


#endif
