#ifndef uispinbox_h
#define uispinbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.h,v 1.1 2001-02-16 17:01:43 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class i_SpinBoxMessenger;
class i_QSpinBox;

class uiSpinBox : public uiWrapObj<i_QSpinBox>
{
friend class		i_QSpinBox;
public:

                        uiSpinBox(uiObject*, const char* nm="SpinBox");

    virtual bool        isSingleLine() const { return true; }

    const char*		text() const;
    int 		getIntValue() const;
    double 		getValue() const;

    void		setText( const char* );
    void		setValue( int );
    void		setValue( double );


    Notifier<uiSpinBox>	valueChanged;

protected:

    virtual bool	useMappers()			{ return false; }
    virtual int		mapTextToValue( bool* ok )	{ return 0; }
    virtual const char*	mapValueToText( int )		{ return 0; }

    const QWidget*	qWidget_() const;

private:

    i_SpinBoxMessenger& _messenger;

    mutable BufferString result;

};

#endif
