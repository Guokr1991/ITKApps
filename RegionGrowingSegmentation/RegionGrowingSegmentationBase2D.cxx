/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    RegionGrowingSegmentationBase2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <RegionGrowingSegmentationBase2D.h>
#include <FL/fl_ask.H>
#include <itkMinimumMaximumImageCalculator.h>


/************************************
 *
 *  Constructor
 *
 ***********************************/
RegionGrowingSegmentationBase2D 
::RegionGrowingSegmentationBase2D()
{

  m_ImageReader                  = ImageReaderType::New();
  m_ImageWriter                  = ImageWriterType::New();


  m_CastImageFilter = CastImageFilterType::New();
  m_CastImageFilter->SetInput( m_ImageReader->GetOutput() );

  m_BilateralImageFilter = BilateralImageFilterType::New();
  m_BilateralImageFilter->SetInput( m_CastImageFilter->GetOutput() );

  m_CurvatureFlowImageFilter = CurvatureFlowImageFilterType::New();
  m_CurvatureFlowImageFilter->SetInput( m_CastImageFilter->GetOutput() );

  m_CurvatureAnisotropicDiffusionImageFilter = CurvatureAnisotropicDiffusionImageFilterType::New();
  m_CurvatureAnisotropicDiffusionImageFilter->SetInput( m_CastImageFilter->GetOutput() );
  
  m_GradientAnisotropicDiffusionImageFilter = GradientAnisotropicDiffusionImageFilterType::New();
  m_GradientAnisotropicDiffusionImageFilter->SetInput( m_CastImageFilter->GetOutput() );
  
  m_NullImageFilter = NullImageFilterType::New();
  m_NullImageFilter->SetInput( m_CurvatureFlowImageFilter->GetOutput() );

  m_ConnectedThresholdImageFilter = ConnectedThresholdImageFilterType::New();
  m_ConnectedThresholdImageFilter->SetInput( m_NullImageFilter->GetOutput() );

  m_ConfidenceConnectedImageFilter = ConfidenceConnectedImageFilterType::New();
  m_ConfidenceConnectedImageFilter->SetInput( m_NullImageFilter->GetOutput() );

  m_FuzzyConnectedImageFilter = FuzzyConnectedImageFilterType::New();
  m_FuzzyConnectedImageFilter->SetInput( m_NullImageFilter->GetOutput() );

  m_SobelImageFilter = SobelImageFilterType::New();
  m_SobelImageFilter->SetInput( m_ConfidenceConnectedImageFilter->GetOutput() );

  m_MaximumImageFilter = MaximumImageFilterType::New();
  m_MaximumImageFilter->SetInput1( m_CastImageFilter->GetOutput() );
  m_MaximumImageFilter->SetInput2( m_SobelImageFilter->GetOutput() );

  m_InputImageIsLoaded  = false;

}




/************************************
 *
 *  Destructor
 *
 ***********************************/
RegionGrowingSegmentationBase2D 
::~RegionGrowingSegmentationBase2D()
{

}



 
/************************************
 *
 *  Load Input Image
 *
 ***********************************/
void
RegionGrowingSegmentationBase2D 
::LoadInputImage( const char * filename )
{
  if( !filename )
  {
    return;
  }

  m_ImageReader->SetFileName( filename );
  m_ImageReader->Update();
  
  m_InputImageIsLoaded = true;

  typedef  itk::MinimumMaximumImageCalculator< InternalImageType > MinimumMaximumCalculatorType;
  MinimumMaximumCalculatorType::Pointer calculator = MinimumMaximumCalculatorType::New();

  m_CastImageFilter->Update();
  calculator->SetImage( m_CastImageFilter->GetOutput() );
  calculator->ComputeMaximum();
  const float maximumValue = calculator->GetMaximum();
  const float factor = 1.1;
  OutputPixelType replacementValue = static_cast<OutputPixelType>(maximumValue * factor);
  
  m_ConnectedThresholdImageFilter->SetReplaceValue( replacementValue );
  m_ConfidenceConnectedImageFilter->SetReplaceValue( replacementValue );

}


 
/************************************
 *
 *  Write Output Image
 *
 ***********************************/
void
RegionGrowingSegmentationBase2D 
::WriteOutputImage( const char * filename )
{
  if( !filename )
  {
    return;
  }

  m_ImageWriter->SetFileName( filename );
  m_ImageWriter->Update();
  
}




/************************************
 *
 *  Select Smoothing Filter
 *
 ***********************************/
void
RegionGrowingSegmentationBase2D
::SelectSmoothingFilter( unsigned int choice )
{
  switch(choice)
    {
    case 0:
      m_NullImageFilter->SetInput( m_BilateralImageFilter->GetOutput() );
      break;
    case 1:
      m_NullImageFilter->SetInput( m_CurvatureFlowImageFilter->GetOutput() );
      break;
    case 2:
      m_NullImageFilter->SetInput( m_GradientAnisotropicDiffusionImageFilter->GetOutput() );
      break;
    case 3:
      m_NullImageFilter->SetInput( m_CurvatureAnisotropicDiffusionImageFilter->GetOutput() );
      break;
    }
}








/************************************
 *
 *  Stop Registration
 *
 ***********************************/
void
RegionGrowingSegmentationBase2D
::Stop( void )
{
  // TODO: add a Stop() method to Filters

}




