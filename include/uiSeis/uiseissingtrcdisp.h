#ifndef uiseissingtrcdisp_h
#define uiseissingtrcdisp_h

/*
________________________________________________________________________
            
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bert
Date:          May 2012
RCS:           $Id$
______________________________________________________________________
                       
*/

#include "uiseismod.h"
#include "uiflatviewer.h"
#include "datapack.h"
class Wavelet;
class SeisTrc;


mExpClass(uiSeis) uiSeisSingleTraceDisplay : public uiFlatViewer
{
public:

    		uiSeisSingleTraceDisplay(uiParent*);

		// setData will remove all refs
    void	setData(const Wavelet*);
    void	setData(const SeisTrc*,const char* nm); //!< nm=for datapack

    void	addRefZ(float);
    			//!< Wavelet automatically get 0 as ref,
    			//!< traces zref or pick if n0t 0 and not undef
    void	removeRefs();

    int			compnr_;
    DataPack::ID	curid_;

protected:

    void		cleanUp();

};


#endif

