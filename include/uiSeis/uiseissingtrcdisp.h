#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiflatviewer.h"
#include "datapack.h"
class Wavelet;
class SeisTrc;


mExpClass(uiSeis) uiSeisSingleTraceDisplay : public uiFlatViewer
{
public:
			uiSeisSingleTraceDisplay(uiParent*);
			~uiSeisSingleTraceDisplay();

			// setData will remove all refs
    void		setData(const Wavelet*);
    void		setData(const SeisTrc*,const char* nm);
			//!< nm=for datapack

    void		addRefZ(float);
			//!< Wavelet automatically get 0 as ref,
			//!< traces zref or pick if n0t 0 and not undef

    int			compnr_;
    DataPackID		curid_;

protected:

    void		cleanUp();

};
