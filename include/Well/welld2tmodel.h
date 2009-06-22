#ifndef welld2tmodel_h
#define welld2tmodel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welld2tmodel.h,v 1.16 2009-06-22 11:49:52 cvsbert Exp $
________________________________________________________________________


-*/

#include "welldahobj.h"
#include "tableascio.h"

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

    float		getTime(float d_ah) const;
    float		getDepth(float) const;
    float		getVelocity(float d_ah) const;

    inline float	t( int idx ) const	{ return t_[idx]; }
    float		value( int idx ) const	{ return t(idx); }

    BufferString	desc;
    BufferString	datasource;

    static const char*	sKeyTimeWell(); //!< name of model for well that is only
    				      //!< known in time
    static const char*	sKeyDataSrc();

    void		add( float d_ah, float tm )
						{ dah_ += d_ah; t_ += tm; }

protected:

    TypeSet<float>	t_;

    void		removeAux( int idx )	{ t_.remove(idx); }
    void		eraseAux()		{ t_.erase(); }

};


mClass D2TModelAscIO : public Table::AscIO
{   
    public:
				D2TModelAscIO( const Table::FormatDesc& fd )
				: Table::AscIO(fd)          {}

    static Table::FormatDesc*   getDesc(bool withunitfld);
    static void                 updateDesc(Table::FormatDesc&,bool withunitfld);
    static void                 createDescBody(Table::FormatDesc*,bool unitfld);

    bool                        get(std::istream&,Well::D2TModel&,
	    			    const Well::Track&) const;
};


}; // namespace Well

#endif
