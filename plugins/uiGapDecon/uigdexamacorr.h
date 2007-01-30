#ifndef uigdexamacorr_h
#define uigdexamacorr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.h,v 1.7 2007-01-30 11:40:08 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "cubesampling.h"
#include "attribdescid.h"
#include "uimainwin.h"

class DataPack;
template <class T> class Array2D;
namespace Attrib { class EngineMan; class DescSet; class DataCubes; }
namespace FlatDisp { class uiFlatViewGen; }

/*! \brief GapDecon Attribute autocorrelation preview in a 2d viewer */

class GapDeconACorrView
{
public:
    			GapDeconACorrView(uiParent*);
    			~GapDeconACorrView();
    bool                computeAutocorr(bool);
    void                createAndDisplay2DViewer(bool);
    void		setCubeSampling( CubeSampling cs )	{ cs_ = cs; }
    void		setAttribID( Attrib::DescID id )	{ attribid_=id;}
    void                setDescSet(Attrib::DescSet*);
    			//<! descset becomes mine!

protected:
    Attrib::EngineMan*	createEngineMan();
    void		displayWiggles(bool,bool);

    uiMainWin*		examwin_;
    uiMainWin*		qcwin_;
    CubeSampling	cs_;
    Attrib::DescID	attribid_;
    Attrib::DescSet*    dset_;
    DataPack*		datapackexam_;
    DataPack*		datapackqc_;
    FlatDisp::uiFlatViewGen*	examviewergen_;
    FlatDisp::uiFlatViewGen*	qcviewergen_;
};


#endif
