#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "attribprovider.h"
#include "bufstring.h"
#include "multiid.h"

namespace EM { class Horizon; }

namespace Attrib
{

class DataHolder;

mClass(EMAttrib) Horizon : public Provider
{ mODTextTranslationClass(Horizon);
public:
    static void		initClass();
			Horizon(Desc&);
			~Horizon();

    void		prepareForComputeData() override;

    static const char*	attribName()	{ return "Horizon"; }
    static const char*	sKeyHorID()	{ return "horid"; }
    static const char*	sKeySurfDataName(){ return "surfdatanm"; }
    static const char*	sKeyType()	{ return "type"; }
    static const char*	sKeyRelZ()	{ return "relz"; }
    static const char*	outTypeNamesStr(int);

    bool		isOK() const override;

protected:
    static Provider*	createInstance( Desc& );
    static void         updateDesc( Desc& );

    bool		getInputData(const BinID&,int intv) override;
    bool		computeData(const DataHolder&,const BinID& relpos,
			    int z0,int nrsamples,int threadid) const override;

    bool		allowParallelComputation() const override
			{ return true; }

    void		fillLineID();

    MultiID		horid_;
    BufferString	surfdatanm_;
    int			outtype_;
    bool		relz_;

    EM::Horizon*	horizon_;
    const DataHolder*	inputdata_;
    int			dataidx_;
    int			horizon2dlineid_;
};

} // namespace Attrib
