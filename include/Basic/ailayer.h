#ifndef ailayer_h
#define aimodel_hlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: ailayer.h,v 1.1 2011-01-27 22:24:26 cvskris Exp $
________________________________________________________________________

-*/

/*!\brief Acoustic Impedance layer.  */

mStruct AILayer
{
		AILayer( float z, float vel, float den )
		    : depth_(z), vel_(vel), den_(den)	{}
    bool	operator ==( const AILayer& p ) const
		{ return depth_ == p.depth_; }

    float	depth_, vel_, den_;
};


#endif
