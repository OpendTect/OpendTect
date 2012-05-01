#ifndef uiseissingtrcdisp_h
#define uiseissingtrcdisp_h

/*
________________________________________________________________________
            
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bert
Date:          May 2012
RCS:           $Id: uiseissingtrcdisp.h,v 1.2 2012-05-01 14:14:29 cvsbert Exp $
______________________________________________________________________
                       
*/

#include "uiflatviewer.h"
#include "datapack.h"
class Wavelet;
class SeisTrc;


mClass uiSeisSingleTraceDisplay : public uiFlatViewer
{
public:

    		uiSeisSingleTraceDisplay(uiParent*);

    void	setData(const Wavelet*);
    void	setData(const SeisTrc*,const char* nm); //!< nm=for datapack

    int			compnr_;
    DataPack::ID	curid_;

protected:

    void		cleanUp();
    void		setRefZ(float);

};


#endif
