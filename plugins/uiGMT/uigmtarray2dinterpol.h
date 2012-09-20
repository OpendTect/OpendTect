#ifndef uigmtarray2dinterpol_h
#define uigmtarray2dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigmtmod.h"
#include "uiarray2dinterpol.h"

class BufferString;
class IOPar;
class uiGenInput;


mClass(uiGMT) uiGMTSurfaceGrid	: public uiArray2DInterpol
{
public:
				uiGMTSurfaceGrid(uiParent*);

    static const char*		sName();
    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    void			fillPar(IOPar&) const;
    bool			acceptOK();

protected:
    void			gmtPushCB(CallBacker*);

    uiGenInput*			tensionfld_;
};


mClass(uiGMT) uiGMTNearNeighborGrid : public uiArray2DInterpol
{
public:
				uiGMTNearNeighborGrid(uiParent*);

    static const char*		sName();
    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    BufferString		mkCommand();
    void			fillPar(IOPar&) const;
    bool			acceptOK();

protected:
    void			gmtPushCB(CallBacker*);

    uiGenInput*			radiusfld_;
};

#endif

