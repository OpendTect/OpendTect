#ifndef uigmtarray2dinterpol_h
#define uigmtarray2dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id: uigmtarray2dinterpol.h,v 1.1 2010-08-13 11:03:33 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiarray2dinterpol.h"

class BufferString;
class IOPar;
class uiGenInput;


mClass uiGMTArray2DInterpol : public uiArray2DInterpol
{
public:
				uiGMTArray2DInterpol(uiParent* p)
				    : uiArray2DInterpol( p, "GMT grid" ) {}

    bool			acceptOK()		{ return true; }
    virtual void		fillPar(IOPar&) const		= 0;
};


mClass uiGMTSurfaceGrid	: public uiGMTArray2DInterpol
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

#endif
