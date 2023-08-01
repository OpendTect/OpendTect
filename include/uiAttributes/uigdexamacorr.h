#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"

#include "attribdescid.h"
#include "trckeyzsampling.h"
#include "uistring.h"

namespace Attrib { class EngineMan; class DescSet;
		   class Data2DHolder; class Processor; }
class uiFlatViewMainWin;
class uiParent;
class FlatDataPack;

/*! \brief GapDecon Attribute autocorrelation preview in a 2d viewer */

mClass(uiAttributes) GapDeconACorrView
{ mODTextTranslationClass(GapDeconACorrView)
public:
			GapDeconACorrView(uiParent*);
			~GapDeconACorrView();

    bool                computeAutocorr(bool);
    void                createAndDisplay2DViewer(bool);
    void		setTrcKeyZSampling( const TrcKeyZSampling& cs )
			{ tkzs_ = cs; }
    void		setGeomID( const Pos::GeomID& geomid )
			{ geomid_ = geomid; }
    void		setAttribID( const Attrib::DescID& id )	{ attribid_=id;}
    void                setDescSet(Attrib::DescSet*);
			//<! descset becomes mine!

protected:

    Attrib::EngineMan*	createEngineMan();
    void		createFD2DDataPack(bool,const Attrib::Data2DHolder&);
    void		createFD3DDataPack(bool,Attrib::EngineMan*,
					   Attrib::Processor*);
    void		displayWiggles(bool,bool);
    bool		setUpViewWin(bool);

    uiFlatViewMainWin*		examwin_;
    uiFlatViewMainWin*		qcwin_;
    TrcKeyZSampling		tkzs_;
    Pos::GeomID			geomid_;
    Attrib::DescID		attribid_;
    Attrib::DescSet*		dset_;
    uiParent*			parent_;
    uiString			examtitle_;
    uiString			qctitle_;

    RefMan<FlatDataPack>	fddatapackqc_;
    RefMan<FlatDataPack>	fddatapackexam_;
};
