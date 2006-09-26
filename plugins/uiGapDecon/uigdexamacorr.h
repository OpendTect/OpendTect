#ifndef uigdexamacorr_h
#define uigdexamacorr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.h,v 1.2 2006-09-26 15:43:45 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "cubesampling.h"
#include "attribdescid.h"

namespace Attrib { class EngineMan; class DescSet; }

/*! \brief GapDecon Attribute autocorrelation preview in a 2d viewer */

class GapDeconACorrView
{
public:
    			GapDeconACorrView(uiParent*);
    bool                computeAutocorr();
    void		setCubesampling( CubeSampling cs )	{ cs_ = cs; }
    void		setInputID( Attrib::DescID id )		{ inpid_ = id; }
    void		setCorrWin( Interval<float> win )	{ gate_ = win; }
    void                setDescSet( Attrib::DescSet* ds )      	{ dset_ = ds; }

protected:
    Attrib::EngineMan*	createEngineMan();
    void		extractAndSaveVals();

    CubeSampling	cs_;
    Attrib::DescID	inpid_;
    Interval<float>	gate_;
    uiParent*		parent_;
    Attrib::DescSet*    dset_;
};


#endif
