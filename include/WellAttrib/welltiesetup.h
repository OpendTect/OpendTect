#ifndef welltiesetup_h
#define welltiesetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: welltiesetup.h,v 1.1 2009-04-21 13:55:59 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

#include "attribdescid.h"
#include "multiid.h"
#include "welltieunitfactors.h"

#include "wellio.h"
#include <iosfwd>

class IOPar;
class MultiID;


mClass WellTieSetup
{
public:
			WellTieSetup()
			    : wellid_(*new MultiID())
			    , attrid_(*new Attrib::DescID())
			    , wvltid_(*new MultiID())
			    , issonic_(true)
			    , iscscorr_(true)		    
			    , factors_(this)		    
			    , corrvellognm_("Corrected ")
			    {}


			WellTieSetup( const WellTieSetup& setup ) 
			    : wellid_(setup.wellid_)
			    , attrid_(setup.attrid_)
			    , wvltid_(setup.wvltid_)
			    , issonic_(setup.issonic_)
			    , vellognm_(setup.vellognm_)
			    , denlognm_(setup.denlognm_)
			    , attrnm_(setup.attrnm_)				
			    , corrvellognm_(setup.corrvellognm_)		
			    , iscscorr_(setup.iscscorr_)		    
			    , factors_(WellTieUnitFactors(&setup))		
			    {}	
	
    MultiID			wellid_;
    Attrib::DescID         	attrid_;
    BufferString        	vellognm_;
    BufferString          	denlognm_;
    BufferString          	corrvellognm_;
    BufferString          	attrnm_;
    MultiID               	wvltid_;
    bool                	issonic_;
    bool			iscscorr_;
    WellTieUnitFactors		factors_; 	
    
    void    	      		usePar(const IOPar&);
    void          	 	fillPar(IOPar&) const;

    static WellTieSetup&	defaults();
    static void                 commitDefaults();

protected:
    
};


mClass WellTieIO : public Well::IO
{
public:
    				WellTieIO(const char* f,bool isrd)
				: Well::IO(f,isrd)
				{}

    static const char*  	sKeyWellTieSetup();
    static const char*  	sExtWellTieSetup();
};



mClass WellTieWriter : public WellTieIO
{
public:
				WellTieWriter(const char* f,const WellTieSetup&);

    bool          	        putWellTieSetup() const;  
    bool 			putWellTieSetup(std::ostream&) const;

protected:
   
    const WellTieSetup& 	wts_;
    bool                	wrHdr(std::ostream&,const char*) const;

};


mClass WellTieReader : public WellTieIO
{
public:
				WellTieReader(const char* f,WellTieSetup&);
  
    bool               		getWellTieSetup() const;	
    bool                	getWellTieSetup(std::istream&) const;


protected:
   
    WellTieSetup& 	wts_;
};

#endif
