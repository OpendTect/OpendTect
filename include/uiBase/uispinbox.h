#ifndef uispinbox_h
#define uispinbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.h,v 1.3 2001-08-24 14:23:42 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class uiSpinBoxBody;

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

#endif
