#include "vissurvobj.h"
#include "uiodscenemgr.h"
#include "uitutmod.h"
#include "vistext.h"

namespace visSurvey
{
mExpClass(uiTut) TutTextDisplay:
                 public visBase::VisualObjectImpl,  
		 public visSurvey::SurveyObject
/*gives access to parent node
via VisualObjectImpl:: parent node and
implements VisualObject::DataObject:: child node*/
{ mODTextTranslationClass( TutTextDisplay )  /*to defing tr(...)*/
mDefaultFactoryInstantiation(/*to provide create() to instantiate the class*/
				    visSurvey::SurveyObject,TutTextDisplay,
				    "TutTextDisplay",
				    toUiString( sFactoryKeyword() ) )
public:				    
				TutTextDisplay();
				TutTextDisplay(
				    const uiString& str,const Coord3& c );
				~TutTextDisplay();

private:
    visBase::Text2*             text_;
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); 
				  /*defined in mDefaultFactoryInstantiation*/
				}
};

}
