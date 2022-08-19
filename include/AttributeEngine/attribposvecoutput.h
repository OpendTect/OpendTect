#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "executor.h"
#include "bufstringset.h"

class NLAModel;
class BinIDValueSet;
class PosVecDataSet;

namespace Attrib
{

class EngineMan;
class DescSet;

/*!
\brief Positionvector output generator for attributes.
*/

mExpClass(AttributeEngine) PosVecOutputGen : public Executor
{ mODTextTranslationClass(PosVecOutputGen)
public:

		PosVecOutputGen(const DescSet&,
				const BufferStringSet& attribdefkeys,
				const ObjectSet<BinIDValueSet>& positions,
				ObjectSet<PosVecDataSet>& output,
				const NLAModel* mdl=0);
		~PosVecOutputGen();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;
    od_int64		nrDone() const override
			{ return outex_ ? outex_->nrDone() : 0; }
    od_int64		totalNr() const override
			{ return outex_ ? outex_->totalNr() : -1; }

protected:

    const BufferStringSet&	inps_;
    const DescSet&		ads_;
    const ObjectSet<BinIDValueSet>& bvss_;
    ObjectSet<PosVecDataSet>&	vdss_;
    EngineMan*			aem_;
    Executor*			outex_;
    mutable uiString	        msg_;

    int				nextStep() override;
};

} // namespace Attrib
