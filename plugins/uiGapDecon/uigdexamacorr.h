#ifndef uigdexamacorr_h
#define uigdexamacorr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.h,v 1.1 2006-09-24 13:18:28 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "cubesampling.h"
#include "attribdescid.h"

using namespace Attrib;

class EngineMan;

/*! \brief GapDecon Attribute autocorrelation preview in a 2d viewer */

class GapDeconACorrView
{
public:
    			GapDeconACorrView(uiParent*);
    bool                computeAutocorr();
    void		setCubesampling( CubeSampling cs )	{ cs_ = cs; }
    void		setInputID( DescID id )			{ inpid_ = id; }
    void		setCorrWin( Interval<float> win )	{ gate_ = win; }

protected:
    EngineMan*          createEngineMan();
    void                extractAndSaveVals();

    CubeSampling	cs_;
    DescID		inpid_;
    Interval<float>	gate_;
    uiParent*		parent_;
};


#endif
