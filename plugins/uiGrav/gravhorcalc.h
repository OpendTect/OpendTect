#ifndef gravhorcalc_h
#define gravhorcalc_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2010
 RCS:		$Id: gravhorcalc.h,v 1.3 2010-04-20 12:53:18 cvsbert Exp $
________________________________________________________________________

*/

#include "executor.h"
#include "multiid.h"
#include "grav.h"
class MultiID;
class Time2DepthStretcher;
namespace EM { class Horizon3D; }


namespace Grav
{


class HorCalc : public ::Executor
{
public:

			HorCalc(const MultiID&,const MultiID* top=0,
				const MultiID* bot=0,float ang=1);
			~HorCalc();

    void		setCutOffAngle( float a )	{ cutoffangle_ = a; }
    void		setVelModel( const MultiID& m )	{ velmid_ = m; }

protected:

    float		cutoffangle_;
    MultiID		velmid_;

    EM::Horizon3D*	calchor_;
    EM::Horizon3D*	tophor_;
    EM::Horizon3D*	bothor_;
    Time2DepthStretcher* ztransf_;

};

} // namespace Grav


#endif
