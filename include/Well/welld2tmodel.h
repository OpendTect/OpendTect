#ifndef welld2tmodel_h
#define welld2tmodel_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welld2tmodel.h,v 1.5 2003-08-22 11:21:47 nanne Exp $
________________________________________________________________________


-*/

#include "uidobj.h"
#include "color.h"

namespace Well
{

class D2TModel : public ::UserIDObject
{
public:

			D2TModel( const char* nm= 0 )
			: ::UserIDObject(nm)	{}

    float		getTime(float d_ah) const;

    int			size() const		{ return t_.size(); }
    float		t( int idx ) const	{ return t_[idx]; }
    float		dah( int idx ) const	{ return dah_[idx]; }

    BufferString	desc;
    BufferString	datasource;

    static const char*	sKeyTimeWell; //!< name of model for well that is only
    				      //!< known in time
    static const char*	sKeyDataSrc;

    void		add( float d_ah, float tm )
						{ dah_ += d_ah; t_ += tm; }

protected:

    TypeSet<float>	dah_;
    TypeSet<float>	t_;

};


}; // namespace Well

#endif
