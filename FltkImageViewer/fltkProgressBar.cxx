/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    fltkProgressBar.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/



#include "fltkProgressBar.h"
#include "itkLightProcessObject.h"
#include <FL/Fl.H>


namespace fltk {


/** Constructor */
ProgressBar::ProgressBar(int x, int y, int w, int h, char * label):
    Fl_Slider( x, y, w, h, label ) {
 m_RedrawCommand = RedrawCommandType::New();
 m_RedrawCommand->SetCallbackFunction( this, &ProgressBar::ProcessEvent );
 m_RedrawCommand->SetCallbackFunction( this, &ProgressBar::ConstProcessEvent );
}


  
/** Get Command */
ProgressBar::RedrawCommandType * 
ProgressBar::GetRedrawCommand( void ) const
{
  return m_RedrawCommand.GetPointer();
}

  



/** Manage a Progress event */
void 
ProgressBar::ProcessEvent( itk::Object * caller, 
                           const itk::EventObject & event )
{
  if( typeid( itk::ProgressEvent )   ==  typeid( event ) )
    {
    ::itk::LightProcessObject::Pointer  process = 
             dynamic_cast< itk::LightProcessObject *>( caller );
    this->value( process->GetProgress() );
    this->redraw();
    Fl::check();
    }
}




void 
ProgressBar::ConstProcessEvent( const itk::Object * caller, 
                                const itk::EventObject & event )
{
  if( typeid( itk::ProgressEvent )   ==  typeid( event ) ) 
    {
    itk::LightProcessObject::ConstPointer  process = 
              dynamic_cast< const itk::LightProcessObject *>( caller );
    this->value( process->GetProgress() );
    this->redraw();
    Fl::check();
    }
}
 

/** Manage a Progress event */
void 
ProgressBar::Observe( itk::Object *caller )
{
  caller->AddObserver(  itk::ProgressEvent(), m_RedrawCommand.GetPointer() );
}

  

} // end namespace fltk


