#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "position.h"
#include "uigroup.h"

class uiLabel;

namespace PreStackView
{

mClass(uiPreStackProcessing) uiGatherDisplayInfoHeader : public uiGroup
{ mODTextTranslationClass(uiGatherDisplayInfoHeader)
public:
    				uiGatherDisplayInfoHeader(uiParent*);
				~uiGatherDisplayInfoHeader();

    void			setData(const BinID&,bool inl,bool is2d,
	    				const char* data);
    void			setData(int pos,const char* data);
    void			setOffsetRange(const Interval<float>&);
    const char*			getDataName() const;

protected:
    uiLabel*			datalbl_;
    uiLabel*			poslbl_;
};

} // namespace PreStackView
