#ifndef welld2tmodel_h
#define welld2tmodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "welldahobj.h"

namespace Well
{

class Track;

mClass D2TModel : public DahObj
{
public:

			D2TModel( const char* nm= 0 )
			: DahObj(nm)	{}
			D2TModel( const D2TModel& d2t )
			: DahObj("") 	{ *this = d2t; }
    D2TModel&		operator =(const D2TModel&);

			//Do not use,to be removed shortly
    float		getTime(float d_ah) const;
    float		getVelocity(float d_ah) const;
    float		getDah(float time) const;

    inline float	t( int idx ) const	{ return t_[idx]; }
    float		value( int idx ) const	{ return t(idx); }
    float*		valArr() 		{ return t_.arr(); }
    const float*	valArr() const		{ return t_.arr(); }

    BufferString	desc;
    BufferString	datasource;

    static const char*	sKeyTimeWell(); //!< name of model for well that is only
    				      //!< known in time
    static const char*	sKeyDataSrc();

    void		add( float d_ah, float tm )
						{ dah_ += d_ah; t_ += tm; }
    bool		insertAtDah(float d_ah,float t,bool ascendingonly=true);

protected:

    TypeSet<float>	t_;

    void		removeAux( int idx )	{ t_.remove(idx); }
    void		eraseAux()		{ t_.erase(); }

protected:

    bool		getVelocityBounds(float d_ah,const Track&,
					  Interval<double>& depths,
					  Interval<float>& times) const;

    inline float	getDepth( float time ) const { return getDah(time); }
    			//!< Legacy, misleading name. Use getDah().
    int			getDahIndex(float d_ah,const Track&) const;
    			/*!<For dah which are out of range of 
			time-depth model gives correct index!>*/
    float		getOldTime(float d_ah,const Track&) const;
    			//!<Read legacy incorrect time-depth model!>
    double		getOldVelocity(float d_ah,const Track&,
	    				Interval<double>* depths=0,
					Interval<float>* times=0) const;
    			//!<Read legacy incorrect time-depth model!>
    bool		getOldVelocityBounds(float d_ah,const Track&,
	    				Interval<double>& depths,
					Interval<float>& times ) const;
    			//!<Read legacy incorrect time-depth model!>

public:
    float		getTime(float d_ah,const Track&) const;
    double		getVelocity(float d_ah,const Track&,
				    Interval<double>* depths=0,
				    Interval<float>* times=0) const;

};

} // namespace Well

#endif
