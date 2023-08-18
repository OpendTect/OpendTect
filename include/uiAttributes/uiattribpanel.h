#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "attribdescid.h"
#include "datapackbase.h"
#include "trckeyzsampling.h"
class uiFlatViewMainWin;
class uiParent;

namespace Attrib { class EngineMan; class DescSet;
		   class Data2DHolder; class Processor; }

/*! \brief Attribute preview in a 2d viewer */

mExpClass(uiAttributes) uiAttribPanel
{ mODTextTranslationClass(uiAttribPanel)
public:
				uiAttribPanel(uiParent*);
    virtual			~uiAttribPanel();

    void			compAndDispAttrib(Attrib::DescSet*,
						const Attrib::DescID&,
						const TrcKeyZSampling&,
						const Pos::GeomID&);
				//<! descset becomes mine!

protected:

    RefMan<FlatDataPack>	computeAttrib();
    Attrib::EngineMan*		createEngineMan();
    virtual void		createAndDisplay2DViewer();
    RefMan<FlatDataPack>	createFDPack(const Attrib::Data2DHolder&) const;
    RefMan<FlatDataPack>	createFDPack(Attrib::EngineMan*,
					     Attrib::Processor*) const;

    virtual const char*		getProcName()	{ return "Computing attribute";}
    virtual const char*		getPackName()	{ return "Attribute pack"; }
    virtual const char*		getPanelName()	{ return "Attribute preview"; }

    uiFlatViewMainWin*		flatvwin_;
    TrcKeyZSampling		tkzs_;
    Pos::GeomID			geomid_;
    Attrib::DescID		attribid_;
    Attrib::DescSet*		dset_;
    uiParent*			parent_;
    RefMan<FlatDataPack>	vddp_;
};
