/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    liFilterConsole2DBase.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#include <liFilterConsole2DBase.h>
#include <itkPNGImageIO.h>



/************************************
 *
 *  Constructor
 *
 ***********************************/
liFilterConsole2DBase 
::liFilterConsole2DBase()
{

  m_ImageFileNameAvailable = false;

  m_Reader     = VolumeReaderType::New();
  
  m_Hx   = InputGaussianFilterType::New();
  m_Hy   = InputGaussianFilterType::New();

  m_Hx->SetDirection( 0 );
  m_Hy->SetDirection( 1 );

  m_Hx->SetOrder( InputGaussianFilterType::ZeroOrder );
  m_Hy->SetOrder( InputGaussianFilterType::ZeroOrder );

  m_Hxy  = GaussianFilterType::New();

  m_Hxy->SetDirection( 1 );

  m_H1x  = GaussianFilterType::New();
  m_H1y  = GaussianFilterType::New();

  m_H1x->SetDirection( 0 );
  m_H1y->SetDirection( 1 );

  m_H1x->SetOrder( GaussianFilterType::FirstOrder );
  m_H1y->SetOrder( GaussianFilterType::FirstOrder );

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

  m_Laplacian = AddFilterType::New();

  m_Laplacian->SetInput1( m_H2x->GetOutput() );
  m_Laplacian->SetInput2( m_H2y->GetOutput() );

  m_Smoothed = GaussianFilterType::New();

  m_Smoothed->SetInputImage( m_Hx->GetOutput() );
  m_Smoothed->SetDirection( 1 );
  m_Smoothed->SetOrder( GaussianFilterType::ZeroOrder );

  m_Modulus = ModulusFilterType::New();
  m_Modulus = ModulusFilterType::New();

  m_Modulus->SetInput1( m_H1x->GetOutput() );
  m_Modulus->SetInput2( m_H1y->GetOutput() );

  m_RescaleIntensityModulus   = RescaleIntensityImageFilterType::New();
  m_RescaleIntensitySmoothed  = RescaleIntensityImageFilterType::New();
  m_RescaleIntensityLaplacian = RescaleIntensityImageFilterType::New();

  m_RescaleIntensitySmoothed->SetInput(  m_Smoothed->GetOutput()  );
  m_RescaleIntensityModulus->SetInput(   m_Modulus->GetOutput()   );
  m_RescaleIntensityLaplacian->SetInput( m_Laplacian->GetOutput() );

  m_RescaleIntensityModulus->SetOutputMinimum(      0UL );
  m_RescaleIntensityModulus->SetOutputMaximum(   1000UL );
  m_RescaleIntensitySmoothed->SetOutputMinimum(     0UL );
  m_RescaleIntensitySmoothed->SetOutputMaximum(  1000UL );
  m_RescaleIntensityLaplacian->SetOutputMinimum(    0UL );
  m_RescaleIntensityLaplacian->SetOutputMaximum( 1000UL );

  m_WriterSmoothed   = VolumeWriterType::New();
  m_WriterModulus    = VolumeWriterType::New();
  m_WriterLaplacian  = VolumeWriterType::New();
 
  m_WriterSmoothed->SetInput(  m_RescaleIntensitySmoothed->GetOutput()  );
  m_WriterModulus->SetInput(   m_RescaleIntensityModulus->GetOutput()   );
  m_WriterLaplacian->SetInput( m_RescaleIntensityLaplacian->GetOutput() );

  m_WriterSmoothed->SetImageIO(  itk::PNGImageIO::New() );
  m_WriterModulus->SetImageIO(   itk::PNGImageIO::New() );
  m_WriterLaplacian->SetImageIO( itk::PNGImageIO::New() );

}




/************************************
 *
 *  Destructor
 *
 ***********************************/
liFilterConsole2DBase 
::~liFilterConsole2DBase()
{

}



 
/************************************
 *
 *  Load
 *
 ***********************************/
void
liFilterConsole2DBase 
::Load( const char * filename )
{
  if( !filename )
  {
    return;
  }

  m_Reader->SetFileName( filename );

  m_ImageFileNameAvailable = true;

}


 
/************************************
 *
 *  Save
 *
 ***********************************/
void
liFilterConsole2DBase 
::SaveSmoothed( const char * filename )
{
  if( !filename )
  {
    return;
  }

  m_WriterSmoothed->SetFileName( filename );
  m_WriterSmoothed->Write();

}



 
/************************************
 *
 *  Save
 *
 ***********************************/
void
liFilterConsole2DBase 
::SaveLaplacian( const char * filename )
{
  if( !filename )
  {
    return;
  }

  m_WriterLaplacian->SetFileName( filename );
  m_WriterLaplacian->Write();

}



 
/************************************
 *
 *  Save
 *
 ***********************************/
void
liFilterConsole2DBase 
::SaveModulus( const char * filename )
{
  if( !filename )
  {
    return;
  }

  m_WriterModulus->SetFileName( filename );
  m_WriterModulus->Write();

}



 
/************************************
 *
 *  Show Progress
 *
 ***********************************/
void
liFilterConsole2DBase 
::ShowProgress( float )
{

}


 
/************************************
 *
 *  Show Status
 *
 ***********************************/
void
liFilterConsole2DBase 
::ShowStatus( const char * )
{

}




 
/************************************
 *
 *  Set Sigma
 *
 ***********************************/
void
liFilterConsole2DBase 
::SetSigma( ComputationType value )
{
  
  m_Hx->SetSigma( value );
  m_Hy->SetSigma( value );

  m_Hxy->SetSigma( value );

  m_H1x->SetSigma( value );
  m_H1y->SetSigma( value );

  m_H2x->SetSigma( value );
  m_H2y->SetSigma( value );
 
  m_Smoothed->SetSigma( value );

}




 
/************************************
 *
 *  Execute
 *
 ***********************************/
void
liFilterConsole2DBase 
::Execute( void )
{

  if( ! (m_ImageFileNameAvailable) ) 
  {
    ShowStatus("Please load an image first");
    return;
  }
  
  m_Laplacian->Update();
  m_Smoothed->Update();
  m_Modulus->Update();

}


 
/************************************
 *
 *  Execute
 *
 ***********************************/
void
liFilterConsole2DBase 
::Cancel( void )
{
  m_Laplacian->AbortGenerateDataOn();
  m_Smoothed->AbortGenerateDataOn();
  m_Modulus->AbortGenerateDataOn();
}




