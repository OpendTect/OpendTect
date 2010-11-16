#ifndef synthseis_h
#define synthseis_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.5 2010-11-16 09:49:10 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "gendefs.h"
class Wavelet;
class SeisTrc;
class AIModel;


namespace Seis
{

/* Generates synthetic traces.
 
   Either the Wavelet or the AIModel will be pre-FFTed.
   
   Note that the Wavelet and AIModel will not be copied, so they need to stay
   alive during the lifetime of this object.
 
 */

mClass SynthGenerator
{
public:

			SynthGenerator(const Wavelet&);
			SynthGenerator(const AIModel&);
    virtual		~SynthGenerator();

    void		generate(const Wavelet&);
    void		generate(const AIModel&);

    const SeisTrc&	result() const;

protected:

    const Wavelet*	wvlt_;
    const AIModel*	aimdl_;

    SeisTrc&		outtrc_;

    void		init();
    void		generate();
};

}

#endif
