#ifndef uigeninput_h
#define uigeninput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Oct 2000
 RCS:           $Id: uigeninput.h,v 1.2 2001-01-26 09:53:43 arend Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
class uiLineEdit;
class uiLabel;
class uiCheckBox;


/*! \brief General Input line Base class.
    The General Input line consists of a number of uiLineEdit's
*/
class uiGenInpLine : public uiGroup
{
protected:
			uiGenInpLine(uiObject* p, const char* str)
			    : uiGroup(p,str)		{}
public:

    const char*		text(int nr=0) const;
    int			getIntValue(int nr=0) const;
    double		getValue(int nr=0) const;

    void		setText(const char*,int nr=0);
    void		setValue(float,int nr=0);
    void		setValue(double,int nr=0);

    const ObjectSet<uiLineEdit>& edits() const;
    uiLineEdit*		edit( int nr=0 )		{ return leds[nr]; }
    void		setReadOnly( bool yn, int nr=-1 );
    void		clearEdits();

    virtual const char*	titleText()			=0;
    virtual void	setTitleText(const char*)	=0;

    virtual bool 	isSingleLine() const		{ return true; }


protected:

    ObjectSet<uiLineEdit> leds;
};


#define mGenInputConstructors( GenInpClss ) \
		GenInpClss(uiObject*,const char* disptxt,const char* definp=0);\
		GenInpClss(uiObject*,const char*,const char*,const char*);\
		GenInpClss(uiObject*,const char*,int);\
		GenInpClss(uiObject*,const char*,float);\
		GenInpClss(uiObject*,const char*,double);\
		GenInpClss(uiObject*,const char*,int,int);\
		GenInpClss(uiObject*,const char*,float,float);\
		GenInpClss(uiObject*,const char*,double,double);


/*! \brief General Labeled Input line.
    A uiGenInput constructs a widget with a label and 1 or 2 input
    fields, depending on the constructor used.
*/
class uiGenInput : public uiGenInpLine
{
public:

			mGenInputConstructors( uiGenInput )

    uiLabel*		label()			{ return labl; }

    virtual const char*	titleText();
    virtual void	setTitleText(const char*);

protected:

    uiLabel*		labl;
    void		init(const char*,const char*,const char*);

};


/*! \brief General Input line with accomaning checkbox (enable/disable).
    A uiGenCheckInput constructs a widget with a checkbox and 1 or 2 input
    fields, depending on the constructor used.
*/
class uiGenCheckInput : public uiGenInpLine
{
public:
			mGenInputConstructors( uiGenCheckInput )

    uiCheckBox*		checkBox()		{ return cbox; }

    virtual const char*	titleText();
    virtual void	setTitleText(const char*);

    void 		setChecked( bool yn );
    bool		isChecked();

protected:

    uiCheckBox*		cbox;
    void		init(const char*,const char*,const char*);

    void 		checkBoxSel( CallBacker* cb ) { checkBoxSel_( cb ); }
			//! allows over-ride of checkbox behavior.
    virtual void 	checkBoxSel_( CallBacker* cb );
};

#endif
