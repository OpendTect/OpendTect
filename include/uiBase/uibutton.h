#ifndef uibutton_h
#define uibutton_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.h,v 1.14 2004-02-25 14:49:18 nanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
class uiButtonGroup;
class uiButtonBody;

class uiPushButtonBody;
class uiRadioButtonBody;
class uiCheckBoxBody;
class uiToolButtonBody;
class ioPixmap;


//! Button Abstract Base class
class uiButton : public uiObject
{
public:
			uiButton(uiParent*,const char*,const CallBack*,
				 uiObjectBody&);

    virtual		~uiButton()		{}

    virtual void	setText(const char*);
    const char*		text();

    Notifier<uiButton>	activated;
};

class uiPushButton : public uiButton
{
public:
				uiPushButton(uiParent*,const char* nm);
				uiPushButton(uiParent*,const char* nm,
					     const CallBack&); 
				uiPushButton(uiParent*,const char* nm,
					     const ioPixmap&);
				uiPushButton(uiParent*,const char* nm,
					     const ioPixmap&,const CallBack&);

    void			setDefault( bool yn = true);

private:

    uiPushButtonBody*		body_;
    uiPushButtonBody&		mkbody(uiParent*,const ioPixmap*,const char*);

};


class uiRadioButton : public uiButton
{                        
public:
				uiRadioButton(uiParent*,const char*);
				uiRadioButton(uiParent*,const char*,
					      const CallBack&);

    bool			isChecked() const;
    virtual void		setChecked( bool yn=true );

private:

    uiRadioButtonBody*		body_;
    uiRadioButtonBody&		mkbody(uiParent*, const char*);

};


class uiCheckBox: public uiButton
{
public:

				uiCheckBox(uiParent*,const char*);
				uiCheckBox(uiParent*,const char*,
					   const CallBack&);

    bool			isChecked () const;
    void			setChecked ( bool yn=true ) ;

    virtual void		setText(const char*);

private:

    uiCheckBoxBody*		body_;
    uiCheckBoxBody&		mkbody(uiParent*, const char*);

};


class uiToolButton : public uiButton
{
public:
				uiToolButton(uiParent*,const char*);
				uiToolButton(uiParent*,const char*,
					     const CallBack&);
				uiToolButton(uiParent*,const char*,
					     const ioPixmap&);
				uiToolButton(uiParent*,const char*,
					     const ioPixmap&,const CallBack&);

    bool			isOn();
    void			setOn( bool yn=true );

    void			setToggleButton( bool yn=true);
    bool			isToggleButton();


private:

    uiToolButtonBody*		body_;
    uiToolButtonBody&		mkbody(uiParent*,const ioPixmap*, const char*); 

};


//! Button Abstract Base class
class uiButtonBody
{
    friend class        i_ButMessenger;

public:
			uiButtonBody()				{}
    virtual		~uiButtonBody()				{}

    //! Button signals emitted by Qt.
    enum notifyTp       { clicked, pressed, released, toggled, stateChanged };

protected:
    //! Handler called from Qt.
    virtual void        notifyHandler( notifyTp )		=0;
};




#endif
