/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    EdgePreprocessingImageFilter.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
using namespace itk;

template<typename TInputImage,typename TOutputImage>
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::EdgePreprocessingImageFilter()
{  
  // Construct the adaptor
  m_CastFilter = CastFilterType::New();
  m_CastFilter->ReleaseDataFlagOn();
  m_CastFilter->SetInput(GetInput()); 
  
  // Construct the Gaussian filter
  m_GaussianFilter = GaussianFilterType::New();
  m_GaussianFilter->SetUseImageSpacing(false);
  m_GaussianFilter->ReleaseDataFlagOn();
  m_GaussianFilter->SetInput(m_CastFilter->GetOutput());

  // The gradient magnitude filter
  m_GradientFilter = GradientFilterType::New();
  m_GradientFilter->ReleaseDataFlagOn();
  m_GradientFilter->SetInput(m_GaussianFilter->GetOutput());  

  // The normalization filter
  m_RescaleFilter = RescaleFilterType::New();
  m_RescaleFilter->ReleaseDataFlagOn();
  m_RescaleFilter->SetOutputMinimum(0.0f);
  m_RescaleFilter->SetOutputMaximum(1.0f);
  m_RescaleFilter->SetInput(m_GradientFilter->GetOutput());
  
  // Construct the Remapping filter
  m_RemappingFilter = RemappingFilterType::New();
  m_RemappingFilter->ReleaseDataFlagOn();
  m_RemappingFilter->SetInput(m_RescaleFilter->GetOutput());

  // Create the progress accumulator
  m_ProgressAccumulator = itk::ProgressAccumulator::New();
  m_ProgressAccumulator->SetMiniPipelineFilter(this);

  // Register the filters with the progress accumulator
  m_ProgressAccumulator->RegisterInternalFilter(m_CastFilter,0.1f);
  m_ProgressAccumulator->RegisterInternalFilter(m_GaussianFilter,0.6f);
  m_ProgressAccumulator->RegisterInternalFilter(m_GradientFilter,0.1f);
  m_ProgressAccumulator->RegisterInternalFilter(m_RescaleFilter,0.1f);
  m_ProgressAccumulator->RegisterInternalFilter(m_RemappingFilter,0.1f);
}

template<typename TInputImage,typename TOutputImage>
void
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::GenerateData()
{
  // Get the input and output pointers
  const typename InputImageType::ConstPointer inputImage = GetInput();
  typename OutputImageType::Pointer outputImage = GetOutput();

  // Initialize the progress counter
  m_ProgressAccumulator->ResetProgress();

  // Allocate the output image
  outputImage->SetBufferedRegion(outputImage->GetRequestedRegion());
  outputImage->Allocate();

  // Pipe in the input image
  m_CastFilter->SetInput(inputImage);

  // Set the variance
  Vector3f variance(
    m_EdgePreprocessingSettings.GetGaussianBlurScale() * 
    m_EdgePreprocessingSettings.GetGaussianBlurScale());
  m_GaussianFilter->SetVariance(variance.data_block());

  // Construct the functor
  FunctorType functor;
  functor.SetParameters(0.0f,1.0f,
                        m_EdgePreprocessingSettings.GetRemappingExponent(),
                        m_EdgePreprocessingSettings.GetRemappingSteepness());

  // Assign the functor to the filter
  m_RemappingFilter->SetFunctor(functor);

  // Call the filter's GenerateData()
  m_RemappingFilter->GraftOutput(outputImage);
  m_RemappingFilter->Update();

  // graft the mini-pipeline output back onto this filter's output.
  // this is needed to get the appropriate regions passed back.
  GraftOutput( m_RemappingFilter->GetOutput() );

}

template<typename TInputImage,typename TOutputImage>
void
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}

template<typename TInputImage,typename TOutputImage>
void 
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::SetEdgePreprocessingSettings(const EdgePreprocessingSettings &settings)
{
  if(!(settings == m_EdgePreprocessingSettings))
    {
    m_EdgePreprocessingSettings = settings;    
    Modified();
    }
}

template<typename TInputImage,typename TOutputImage>
void 
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::GenerateInputRequestedRegion()
{
  // Make sure we call the parent's implementation
  Superclass::GenerateInputRequestedRegion();

  // Get pointers to the input and output
  InputImagePointer  inputPtr = 
    const_cast< TInputImage * >( this->GetInput() );
  OutputImagePointer outputPtr = this->GetOutput();

  // Use the largest possible region (hack)
  inputPtr->SetRequestedRegion(
    inputPtr->GetLargestPossibleRegion());
}
