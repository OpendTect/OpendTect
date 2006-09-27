#ifndef uigdexamacorr_h
#define uigdexamacorr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.h,v 1.3 2006-09-27 15:12:41 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "cubesampling.h"
#include "attribdescid.h"
#include "uimainwin.h"

template <class T> class Array2D;
namespace Attrib { class EngineMan; class DescSet; class DataCubes; }
namespace uiFlatDisp { class VertViewer; }

/*! \brief GapDecon Attribute autocorrelation preview in a 2d viewer */

class GapDeconACorrView : public uiMainWin
{
public:
    			GapDeconACorrView(uiParent*);
    			~GapDeconACorrView();
    bool                computeAutocorr();
    void                createAndDisplay2DViewer();
    void		setCubesampling( CubeSampling cs )	{ cs_ = cs; }
    void		setInputID( Attrib::DescID id )		{ inpid_ = id; }
    void		setCorrWin( Interval<float> win )	{ gate_ = win; }
    void                setDescSet( Attrib::DescSet* ds )      	{ dset_ = ds; }

protected:
    Attrib::EngineMan*	createEngineMan();
    void		extractAndSaveVals(const Attrib::DataCubes*);
    void		displayWiggles(bool);

    CubeSampling	cs_;
    Attrib::DescID	inpid_;
    Interval<float>	gate_;
    Attrib::DescSet*    dset_;
    Array2D<float>*	autocorr2darr_;
    uiFlatDisp::VertViewer*	viewer2d_;
};


#endif
