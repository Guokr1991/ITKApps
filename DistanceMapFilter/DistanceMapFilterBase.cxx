
/**

    Illustration of the use of the class
        
    itk::DanielssonDistanceMapImageFilter

*/


#include "DistanceMapFilterBase.h"
#include "FL/fl_file_chooser.H"
#include "FL/fl_ask.H"

#include <iostream>  
#include "itkImageRegionIteratorWithIndex.h"



DistanceMapFilterBase
::DistanceMapFilterBase()
{

  m_ImageLoaded = false;

  m_Reader = VolumeReaderType::New();

  m_DistanceFilter = DistanceFilterType::New();

  m_DistanceFilter->SetInput( m_Reader->GetOutput() );

  m_DistanceImage = m_DistanceFilter->GetOutput();

  m_Display.GetGlWindow()->SetBackground( 0.7, 0.8, 0.9 );
  m_Display.GetGlWindow()->SetZoom( 1.0 );
  m_Display.SetLabel("Distance Filter");


  // Slice Drawer for the input image
  m_ImageSliceDrawer = ImageSliceDrawerType::New();

  m_ImageSliceDrawer->SetInput( m_Reader->GetOutput() );

  m_Display.GetNotifier()->AddObserver( 
             fltk::GlDrawEvent(),
             m_ImageSliceDrawer->GetDrawCommand().GetPointer() );

  m_ImageSliceDrawer->AddObserver( 
                         fltk::VolumeReslicedEvent(),
                         m_Display.GetRedrawCommand() );
 
  m_ImageSliceDrawer->SetLabel("Input Image");


  // Slice Drawer for the distance map image
  m_DistanceImageSliceDrawer = ImageSliceDrawerType::New();

  m_DistanceImageSliceDrawer->SetInput( m_DistanceFilter->GetOutput() );

  m_Display.GetNotifier()->AddObserver( 
             fltk::GlDrawEvent(),
             m_DistanceImageSliceDrawer->GetDrawCommand().GetPointer() );

  m_DistanceImageSliceDrawer->AddObserver( 
                         fltk::VolumeReslicedEvent(),
                         m_Display.GetRedrawCommand() );
 
  m_DistanceImageSliceDrawer->SetLabel("Input Image");

  m_ImageViewer = new ImageViewerType;
  m_ImageViewer->SetLabel("Input Image");
  m_ImageViewer->SetImage( m_Reader->GetOutput() );

  m_DistanceImageViewer = new DistanceImageViewerType;
  m_DistanceImageViewer->SetLabel("Distance Map");
  m_DistanceImageViewer->SetImage( m_DistanceFilter->GetOutput() );

}


DistanceMapFilterBase
::~DistanceMapFilterBase()
{
  this->HideDisplay();
  delete m_ImageViewer;
  delete m_DistanceImageViewer;
}



/**
 *    Show the Display
 */ 
void 
DistanceMapFilterBase
::ShowDisplay(void)
{
  m_Display.Show();
}





/**
 *    Hide the Display
 */ 
void 
DistanceMapFilterBase
::HideDisplay(void)
{
  m_Display.Hide();
  m_ImageSliceDrawer->Hide();
  m_DistanceImageSliceDrawer->Hide();
  m_ImageViewer->Hide();
  m_DistanceImageViewer->Hide();
}






/**
 *    Save
 */ 
void 
DistanceMapFilterBase
::Save(void) const
{
  const char * filename = fl_file_chooser("","","");

  if( !filename )
    {
    return;
    }

  this->Save( filename );

}






/**
 *    Load
 */ 
void 
DistanceMapFilterBase
::Load(void) 
{
  const char * filename = fl_file_chooser("","","");

  if( !filename )
    {
    return;
    }

  this->Load( filename );

}



/**
 *    Save
 */ 
void 
DistanceMapFilterBase
::Save(const char * filename) const
{
  std::cout << "Cannot save file: " << filename;
  std::cout << " function Save() is not implemented" << std::endl; 
}



 

/**
 *    Load
 */ 
void 
DistanceMapFilterBase
::Load(const char * filename) 
{
   
  if( !filename )
  {
    return;
  }

  m_Reader->SetFileName( filename );
  m_Reader->Update();

  m_ImageSliceDrawer->SetInput( m_Reader->GetOutput() );

  m_ImageViewer->SetImage( m_Reader->GetOutput() );
 
  m_ImageLoaded = true;

 
}


 

/**
 *    Compute Distance
 */ 
void 
DistanceMapFilterBase
::ComputeDistance(void)
{
   
  if( !m_ImageLoaded )
    {
    fl_alert("Please load an image first");
    return;
    }

  m_DistanceFilter->Update();

  m_DistanceImageSliceDrawer->SetInput( 
               m_DistanceFilter->GetOutput() );

  m_Display.Redraw();

  m_DistanceImageViewer->SetImage( 
               m_DistanceFilter->GetOutput() );

}

  
 
 


