#ifndef uispinbox_h
#define uispinbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.h,v 1.4 2001-10-03 09:03:20 nanne Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <uigroup.h>

class uiSpinBoxBody;
class uiLabel;

class uiSpinBox : public uiObject
{
friend class		uiSpinBoxBody;
public:

                        uiSpinBox(uiParent*, const char* nm="SpinBox");

    const char*		text() const;
    int 		getIntValue() const;
    double 		getValue() const;

    void		setText( const char* );
    void		setValue( int );
    void		setValue( double );

    int			minValue() const;
    int			maxValue() const;
    void		setMinValue( int );
    void		setMaxValue( int );
    int			step() const;
    void		setStep( int );

    Notifier<uiSpinBox>	valueChanged;

protected:

    virtual bool	useMappers()			{ return false; }
    virtual int		mapTextToValue( bool* ok )	{ return 0; }
    virtual const char*	mapValueToText( int )		{ return 0; }

private:

    mutable BufferString result;

    uiSpinBoxBody*	body_;
    uiSpinBoxBody&	mkbody(uiParent*, const char*);

};


class uiLabeledSpinBox : public uiGroup
{
public:
                uiLabeledSpinBox( uiParent*,const char* txt,
                                   const char* nm="Labeled Spinbox");

    uiSpinBox*  	box()			{ return sb; }
    uiLabel*    	label()			{ return lbl; }

protected:

    uiSpinBox*	sb;
    uiLabel*    lbl;

};

#endif
