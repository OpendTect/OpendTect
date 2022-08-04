#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		December 2009
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"

class Array1DInterpol;
class uiComboBox;
class uiGenInput;
class uiArray2DInterpol;

template <class T> class Array1D;


mExpClass(uiTools) uiArray1DInterpolSel : public uiDlgGroup
{ mODTextTranslationClass(uiArray1DInterpolSel);
public:
				uiArray1DInterpolSel( uiParent*,bool extrapol,
						      bool holesz);
				//extrapol enables the UI part for extrapolation
				//but yet not supported in algo.set it to false

    bool			acceptOK() override;
    Array1DInterpol*		getResult(int);
				
    void			setDistanceUnit(const uiString&);
    void			setInterpolators(int totalnr);
    void			setArraySet(ObjectSet< Array1D<float> >&);
    
protected:
					~uiArray1DInterpolSel();

    ObjectSet<Array1DInterpol>		results_;

    uiGenInput*				polatefld_;
    uiGenInput*				maxgapszfld_;
    uiComboBox*				methodsel_;
};

