/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    ceExtractorConsoleBase.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include "ceExtractorConsoleBase.h"
#include "itkMinimumMaximumImageCalculator.h"

/************************************
 *
 *  Constructor
 *
 ***********************************/
ceExtractorConsoleBase 
::ceExtractorConsoleBase()
{

  m_ImageLoaded = false;

  m_Reader     = VolumeReaderType::New();
  m_Writer     = VolumeWriterType::New();

  m_RescaleIntensitySmoothed   = RescaleIntensityFilterType::New();
  m_RescaleIntensityMedialness = RescaleIntensityFilterType::New();
  m_RescaleIntensityMaxEigen   = RescaleIntensityFilterType::New();

  m_Hx   = InputGaussianFilterType::New();
  m_Hy   = InputGaussianFilterType::New();

  m_Hx->SetDirection( 0 );
  m_Hy->SetDirection( 1 );

  m_Hxy  = GaussianFilterType::New();

  m_Hxy->SetDirection( 1 );

  m_H1x  = GaussianFilterType::New();
  m_H1y  = GaussianFilterType::New();

  m_H1x->SetDirection( 0 );
  m_H1y->SetDirection( 1 );

  m_H1x->SetOrder( GaussianFilterType::FirstOrder );
  m_H1y->SetOrder( GaussianFilterType::FirstOrder );

  m_H1xy = GaussianFilterType::New();
  m_H1xy->SetDirection( 1 );
  m_H1xy->SetOrder( GaussianFilterType::FirstOrder );
    
  m_H2x  = GaussianFilterType::New();
  m_H2y  = GaussianFilterType::New();

  m_H2x->SetDirection( 0 );
  m_H2y->SetDirection( 1 );

  m_H2x->SetOrder( GaussianFilterType::SecondOrder );
  m_H2y->SetOrder( GaussianFilterType::SecondOrder );

  m_Hx->SetInputImage( m_Reader->GetOutput() );
  m_Hy->SetInputImage( m_Reader->GetOutput() );

  m_Hxy->SetInputImage( m_Hx->GetOutput() );

  m_H1x->SetInputImage( m_Hy->GetOutput() );
  m_H1y->SetInputImage( m_Hx->GetOutput() );

  m_H2x->SetInputImage( m_Hy->GetOutput() );
  m_H2y->SetInputImage( m_Hx->GetOutput() );

  m_H1xy->SetInputImage( m_H1x->GetOutput() );

  
  m_Add = AddFilterType::New();

  m_Add->SetInput1( m_H2x->GetOutput() );
  m_Add->SetInput2( m_H2y->GetOutput() );

  
  m_Modulus = ModulusFilterType::New();

  m_Modulus->SetInput1( m_H1x->GetOutput() );
  m_Modulus->SetInput2( m_H1y->GetOutput() );

  m_Gradient = GradientFilterType::New();
  
  m_Gradient->SetInput( m_Reader->GetOutput() );

  
  m_Eigen = EigenFilterType::New();
  
  m_Eigen->SetInput1( m_H2x->GetOutput() );
  m_Eigen->SetInput2( m_H1xy->GetOutput() );
  m_Eigen->SetInput3( m_H2y->GetOutput() );
      

  m_Join = JoinFilterType::New();

  m_Join->SetInput1( m_H1x->GetOutput() );
  m_Join->SetInput2( m_H1y->GetOutput() );
  
  m_ScalarProduct = ScalarProductFilterType::New();

  m_ScalarProduct->SetInput1( m_Join->GetOutput() );
  m_ScalarProduct->SetInput2( m_Eigen->GetMaxEigenVector() );

  // Normalize the parametric space
  m_RescaleIntensitySmoothed->SetInput( m_Hxy->GetOutput() );
  m_RescaleIntensitySmoothed->SetOutputMinimum( -1.0 );
  m_RescaleIntensitySmoothed->SetOutputMaximum(  1.0 );

  m_RescaleIntensityMedialness->SetInput( m_ScalarProduct->GetOutput() );
  m_RescaleIntensityMedialness->SetOutputMinimum( -1.0 );
  m_RescaleIntensityMedialness->SetOutputMaximum(  1.0 );

  m_RescaleIntensityMaxEigen->SetInput( m_Eigen->GetMaxEigenValue() );
  m_RescaleIntensityMaxEigen->SetOutputMinimum( 0.0 );
  m_RescaleIntensityMaxEigen->SetOutputMaximum( 1.0 );

  m_ParametricSpace = ParametricSpaceFilterType::New();

  m_ParametricSpace->SetInput( 0, m_RescaleIntensityMaxEigen->GetOutput() );
  m_ParametricSpace->SetInput( 1, m_RescaleIntensityMedialness->GetOutput() );
  m_ParametricSpace->SetInput( 2, m_RescaleIntensitySmoothed->GetOutput() );

  m_SpatialFunctionControl = SpatialFunctionControlType::New();

  m_SpatialFunctionFilter = SpatialFunctionFilterType::New();

  m_SpatialFunctionFilter->SetInput(  
                              m_ParametricSpace->GetOutput() );

  m_SpatialFunctionControl->SetSpatialFunction( 
                  m_SpatialFunctionFilter->GetSpatialFunction() );

#ifdef FRUSTUM_FUNCTION
  m_SpatialFunctionControl->SetAngleZ( 20.0f );
  m_SpatialFunctionControl->SetApertureAngleX( 12.0f );
  m_SpatialFunctionControl->SetApertureAngleY(  3.0f );
  m_SpatialFunctionControl->SetTopPlane( 0.1f );
  m_SpatialFunctionControl->SetBottomPlane( 2.0f );
  m_SpatialFunctionControl->SetApex( 0.4f, 0.0f, 1.0f );
#endif


#ifdef SPHERE_FUNCTION
  m_SpatialFunctionControl->SetRadius( 0.1f );
#endif


  
  m_InverseParametricFilter = InverseParametricFilterType::New();

  m_InverseParametricFilter->SetInput( 
      m_SpatialFunctionFilter->GetOutput() );

}




/************************************
 *
 *  Destructor
 *
 ***********************************/
ceExtractorConsoleBase 
::~ceExtractorConsoleBase()
{

}



 
/************************************
 *
 *  Load
 *
 ***********************************/
void
ceExtractorConsoleBase 
::Load( const char * filename )
{

  if( !filename )
  {
    return;
  }

  m_Reader->SetFileName( filename );
  m_Reader->UpdateLargestPossibleRegion();

  InputImageType::Pointer inputImage = m_Reader->GetOutput();

  inputImage->SetRequestedRegion( 
      inputImage->GetLargestPossibleRegion() );

  m_ImageLoaded = true;

}


 
/************************************
 *
 *  SaveOutput
 *
 ***********************************/
void
ceExtractorConsoleBase 
::SaveOutput( const char * filename )
{

  if( !filename )
  {
    return;
  }

  m_InverseParametricFilter->Update();

  // THIS CODE MUST BE MOVED INTO A NEW FILTER
  InputImageType::ConstPointer inputImage = m_Reader->GetOutput();

  typedef itk::MinimumMaximumImageCalculator< InputImageType > CalculatorType;
  CalculatorType::Pointer calculator = CalculatorType::New();
  calculator->SetImage( inputImage );
  calculator->Compute();

  InputPixelType minimumValue = calculator->GetMinimum();
  InputPixelType maximumValue = calculator->GetMaximum();

  InputImageType::RegionType region = inputImage->GetLargestPossibleRegion();

  OutputImageType::Pointer outputImage = OutputImageType::New();
  outputImage->CopyInformation( inputImage );
  outputImage->SetRegions( region );
  outputImage->Allocate();
  OutputPixelType black;
  black.Fill(0);
  outputImage->FillBuffer( black );

  typedef itk::ImageRegionIterator< OutputImageType > OutputIterator;
  typedef itk::ImageRegionConstIterator< InputImageType  > InputIterator;

  InputIterator it( inputImage, region );
  OutputIterator ot( outputImage, region );

  it.GoToBegin();
  ot.GoToBegin();
  
  OutputPixelType pixelValue;

  typedef OutputPixelType::ValueType  OutputValueType;

  const double factor = 
    itk::NumericTraits< OutputValueType >::max() / ( maximumValue - minimumValue );

  while( !it.IsAtEnd() )
    {
    OutputValueType normalizedValue = 
      static_cast< OutputValueType >( (it.Get() - minimumValue) * factor );
    pixelValue.Fill( normalizedValue );
    ot.Set( pixelValue );
    ++it;
    ++ot;
    }
  // END OF CODE TO MOVE INTO A NEW FILTER

  m_Writer->SetInput( outputImage );
  
  m_Writer->SetFileName( filename );

  try
    {
    m_Writer->Update();
    }
  catch( itk::ExceptionObject & excp )
    {
    std::cerr << excp << std::endl;
    return;
    }

}


 
/************************************
 *
 *  Show Progress
 *
 ***********************************/
void
ceExtractorConsoleBase 
::ShowProgress( float )
{

}


 
/************************************
 *
 *  Show Status
 *
 ***********************************/
void
ceExtractorConsoleBase 
::ShowStatus( const char * )
{

}




 
/************************************
 *
 *  Set Sigma
 *
 ***********************************/
void
ceExtractorConsoleBase 
::SetSigma( RealType value )
{
  
  m_Hx->SetSigma( value );
  m_Hy->SetSigma( value );

  m_Hxy->SetSigma( value );

  m_H1x->SetSigma( value );
  m_H1y->SetSigma( value );

  m_H1xy->SetSigma( value );

  m_H2x->SetSigma( value );
  m_H2y->SetSigma( value );

  m_Gradient->SetSigma( value );

}




 
/************************************
 *
 *  Execute
 *
 ***********************************/
void
ceExtractorConsoleBase 
::Execute( void )
{

  if( ! (m_ImageLoaded) ) 
  {
    ShowStatus("Please load an image first");
    return;
  }
  

  m_Hxy->Update();

  m_H1xy->Update();

  m_Add->Update();

  m_Modulus->Update();
  
  m_InverseParametricFilter->Update();

}



 
/************************************
 *
 *  Show Spatial Function Control
 *
 ***********************************/
void
ceExtractorConsoleBase 
::ShowSpatialFunctionControl( void )
{
  m_SpatialFunctionControl->Show();
}


 
/************************************
 *
 *  Hide Spatial Function Control
 *
 ***********************************/
void
ceExtractorConsoleBase 
::HideSpatialFunctionControl( void )
{
  m_SpatialFunctionControl->Hide();
}


