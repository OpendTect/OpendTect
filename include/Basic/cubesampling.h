#ifndef cubesampling_h
#define cubesampling_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Feb 2002
 RCS:           $Id: cubesampling.h,v 1.1 2002-02-12 17:51:44 bert Exp $
________________________________________________________________________

-*/

#include <ranges.h>
#include <position.h>
class BinIDRange;
class BinIDSampler;


/*\brief Horizontal sampling in 3D surveys */

struct HorSampling
{
			HorSampling();
			HorSampling(const BinIDRange&);
			HorSampling(const BinIDSampler&);
    HorSampling&	set(const BinIDRange&);
    HorSampling&	set(const BinIDSampler&);

    BinID		start;
    BinID		stop;
    BinID		step;

};


/*\brief Hor+Vert sampling in 3D surveys */

struct CubeSampling
{
public:

    			CubeSampling();

    HorSampling		hrg;
    StepInterval<float>	zrg;

};


#endif
