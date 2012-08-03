#ifndef uiarray1dinterpol_h
#define uiarray1dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          December 2009
 RCS:           $Id: uiarray1dinterpol.h,v 1.2 2012-08-03 13:01:11 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"

class Array1DInterpol;
class uiComboBox;
class uiGenInput;
class uiArray2DInterpol;

template <class T> class Array1D;


mClass(uiTools) uiArray1DInterpolSel : public uiDlgGroup
{
public:
    				uiArray1DInterpolSel( uiParent*,bool extrapol,
						      bool holesz);
				//extrapol enables the UI part for extrapolation
				//but yet not supported in algo.set it to false

    bool			acceptOK();
    Array1DInterpol*		getResult(int);
				
    void			setDistanceUnit(const char*);
    void			setInterpolators(int totalnr);
    void			setArraySet(ObjectSet< Array1D<float> >&);
    
protected:
					~uiArray1DInterpolSel();

    ObjectSet<Array1DInterpol>		results_;

    uiGenInput*				polatefld_;
    uiGenInput*				maxgapszfld_;
    uiComboBox*				methodsel_;
};

#endif

