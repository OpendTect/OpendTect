#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

#include "uigmtmod.h"
#include "uiarray2dinterpol.h"

class uiGenInput;


mExpClass(uiGMT) uiGMTSurfaceGrid	: public uiArray2DInterpol
{ mODTextTranslationClass(uiGMTSurfaceGrid);
public:
				uiGMTSurfaceGrid(uiParent*);

    static const char*		sName();
    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    void			fillPar(IOPar&) const;
    bool			acceptOK();
    void			setValuesFrom(const Array2DInterpol&);

protected:
    void			gmtPushCB(CallBacker*);

    uiGenInput*			tensionfld_;
};


mExpClass(uiGMT) uiGMTNearNeighborGrid : public uiArray2DInterpol
{ mODTextTranslationClass(uiGMTNearNeighborGrid);
public:
				uiGMTNearNeighborGrid(uiParent*);

    static const char*		sName();
    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    BufferString		mkCommand();
    void			fillPar(IOPar&) const;
    bool			acceptOK();
    void			setValuesFrom(const Array2DInterpol&);

protected:
    void			gmtPushCB(CallBacker*);

    uiGenInput*			radiusfld_;
};
