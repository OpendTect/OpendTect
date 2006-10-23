#ifndef uigdexamacorr_h
#define uigdexamacorr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.h,v 1.6 2006-10-23 15:23:26 cvshelene Exp $
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

class GapDeconACorrView
{
public:
    			GapDeconACorrView(uiParent*);
    			~GapDeconACorrView();
    bool                computeAutocorr();
    void                createAndDisplay2DViewer(bool);
    void		setCubeSampling( CubeSampling cs )	{ cs_ = cs; }
    void		setAttribID( Attrib::DescID id )	{ attribid_=id;}
    void                setDescSet(Attrib::DescSet*);
    			//<! descset becomes mine!

protected:
    Attrib::EngineMan*	createEngineMan();
    void		extractAndSaveVals(const Attrib::DataCubes*);
    void		displayWiggles(bool,bool);

    uiMainWin*		examwin_;
    uiMainWin*		qcwin_;
    CubeSampling	cs_;
    Attrib::DescID	attribid_;
    Attrib::DescSet*    dset_;
    Array2D<float>*	autocorr2darr_;
    uiFlatDisp::VertViewer*	examviewer2d_;
    uiFlatDisp::VertViewer*	qcviewer2d_;
};


#endif
