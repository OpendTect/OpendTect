#ifndef uislider_h
#define uislider_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.h,v 1.1 2001-02-16 17:01:43 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
class QSlider;
template <class T> class i_QObjWrapper;
mTemplTypeDefT(i_QObjWrapper,QSlider,i_QSlider)

class	i_SliderMessenger;

class uiSlider : public uiWrapObj<i_QSlider>
{
public:

                        uiSlider(uiObject*, const char* nm="Line Edit");

    virtual bool        isSingleLine() const { return true; }

    const char*		text() const;
    int 		getIntValue() const;
    double 		getValue() const;

    void		setText( const char* );
    void		setValue( int );
    void		setValue( double );
    void		setTickMarks( bool yn=true);

    Notifier<uiSlider>	valueChanged;
    Notifier<uiSlider>	sliderMoved;

protected:

    const QWidget*	qWidget_() const;

private:

    i_SliderMessenger& _messenger;

    mutable BufferString result;

};

#endif
