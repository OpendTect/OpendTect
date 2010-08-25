#ifndef uigmtarray2dinterpol_h
#define uigmtarray2dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id: uigmtarray2dinterpol.h,v 1.2 2010-08-25 07:11:11 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiarray2dinterpol.h"

class BufferString;
class IOPar;
class uiGenInput;


mClass uiGMTSurfaceGrid	: public uiArray2DInterpol
{
public:
				uiGMTSurfaceGrid(uiParent*);

    static const char*		sName();
    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    void			fillPar(IOPar&) const;
    bool			acceptOK();

protected:
    uiGenInput*			tensionfld_;
};


mClass uiGMTNearNeighborGrid : public uiArray2DInterpol
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
    uiGenInput*			rediusfld_;
};

#endif
