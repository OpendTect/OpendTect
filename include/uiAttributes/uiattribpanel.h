#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "trckeyzsampling.h"
#include "linekey.h"
#include "attribdescid.h"

class FlatDataPack;
class uiFlatViewMainWin;

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

    FlatDataPack*		computeAttrib();
    Attrib::EngineMan*		createEngineMan();
    virtual void		createAndDisplay2DViewer(FlatDataPack*);
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
};
