/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    SpeedImageWrapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "SpeedImageWrapper.h"
#include "ImageWrapper.txx"

using namespace itk;

SpeedImageWrapper
::SpeedImageWrapper()
: ImageWrapper<float> ()
{
  // Intialize display filters
  for(unsigned int i=0;i<3;i++) 
    {
    // Create the intensity mapping filter
    m_DisplayFilter[i] = IntensityFilterType::New();

    // Set the corresponding slice as the input image
    m_DisplayFilter[i]->SetInput(GetSlice(i));

    // Create the overlay mapping filter
    m_OverlayFilter[i] = OverlayFilterType::New();

    // Set the corresponding slice as the input image
    m_OverlayFilter[i]->SetInput(GetSlice(i));
    }

  // Initialize the overlay functor
  m_OverlayFunctor.m_Cutoff = 0.0f;

  // Initialize to Edge mode
  m_IsModeInsideOutside = false;
}

SpeedImageWrapper
::~SpeedImageWrapper()
{
}

SpeedImageWrapper::DisplaySliceType * 
SpeedImageWrapper
::GetDisplaySlice(unsigned int iSlice)
{
  // Depending on the current mode, return the display slice or the 
  // original slice from the parent
  //return m_IsModeInsideOutside ? 
  //  m_DisplayFilter[iSlice]->GetOutput() : 
  //  GetSlice(iSlice);
  return m_DisplayFilter[iSlice]->GetOutput();
}


SpeedImageWrapper::OverlaySliceType * 
SpeedImageWrapper
::GetOverlaySlice(unsigned int dim)
{
  return m_OverlayFilter[dim]->GetOutput();
}

void
SpeedImageWrapper
::SetOverlayCutoff(float cutoff)
{
  if(cutoff != m_OverlayFunctor.m_Cutoff) 
    {
    // Update the variable
    m_OverlayFunctor.m_Cutoff = cutoff;

    // Update the filters
    for(unsigned int i=0;i<3;i++)
      m_OverlayFilter[i]->SetFunctor(m_OverlayFunctor);
    }
}

float
SpeedImageWrapper
::GetOverlayCutoff() const
{
  return m_OverlayFunctor.m_Cutoff;
}

void
SpeedImageWrapper
::SetOverlayColor(const OverlayPixelType &color)
{
  if(color != m_OverlayFunctor.m_Color) 
    {
    // Update the variable
    m_OverlayFunctor.m_Color = color;

    // Update the filters
    for(unsigned int i=0;i<3;i++)
      m_OverlayFilter[i]->SetFunctor(m_OverlayFunctor);
    }
}

void 
SpeedImageWrapper
::UpdateImagePointer(ImageType *newImage)
{
  // Call the parent's method
  ImageWrapper<float>::UpdateImagePointer(newImage);

  // Update the sources
  for(unsigned int i=0;i<3;i++)
    if(m_PreviewSource[i])
      m_Slicer[i]->SetInput(m_PreviewSource[i]);      
}


SpeedImageWrapper::OverlayPixelType 
SpeedImageWrapper
::GetOverlayColor() const
{
  return m_OverlayFunctor.m_Color;
}
  
SpeedImageWrapper::OverlayPixelType 
SpeedImageWrapper::OverlayFunctor
::operator()(float in)
{
  // Initialize with a clear pixel
  const unsigned char clear[] = {0,0,0,0};
  SpeedImageWrapper::OverlayPixelType rtn(clear);
  
  // Check the threshold and return appropriate value
  if(in<m_Cutoff)
    {
    return clear;
    } 
  return m_Color;
}

SpeedImageWrapper::MappingFunctor
::MappingFunctor()
{
  m_Plus.Set(255,255,255,255);
  m_Minus.Set(0,0,255,255);
  m_Zero.Set(0,0,0,255);
}

void
SpeedImageWrapper::MappingFunctor
::SetColorMap(DisplayPixelType inPlus, DisplayPixelType inMinus, DisplayPixelType inZero)
{
  m_Plus = inPlus;
  m_Minus = inMinus;
  m_Zero = inZero;
}

SpeedImageWrapper::DisplayPixelType
SpeedImageWrapper::MappingFunctor
::operator()(float t)
{
  // Initialize with a clear pixel
  const unsigned char clear[] = {0,0,0,255};
  SpeedImageWrapper::OverlayPixelType P(clear);

  // The red component is used when speed is positive
  if(t > 0)
    {
    float u = 1.0f - t;
    P[0] = (unsigned char)(t * m_Plus[0] + u * m_Zero[0]);
    P[1] = (unsigned char)(t * m_Plus[1] + u * m_Zero[1]);
    P[2] = (unsigned char)(t * m_Plus[2] + u * m_Zero[2]);
    }
  else
    {
    float u = 1.0f + t;
    P[0] = (unsigned char)(-t * m_Minus[0] + u * m_Zero[0]);
    P[1] = (unsigned char)(-t * m_Minus[1] + u * m_Zero[1]);
    P[2] = (unsigned char)(-t * m_Minus[2] + u * m_Zero[2]);
    }

  // Return
  return P;
}

void 
SpeedImageWrapper
::SetSliceSourceForPreview(unsigned int slice,ImageType *source)
{
  m_PreviewSource[slice] = source;
  m_Slicer[slice]->SetInput(source);    
}

void 
SpeedImageWrapper
::RemoveSliceSourcesForPreview()
{
  for(unsigned int i=0;i<3;i++)
    {
    m_PreviewSource[i] = NULL;
    m_Slicer[i]->SetInput(m_Image);
    }
}

float 
SpeedImageWrapper
::GetPreviewVoxel(const Vector3ui &point) const
{
  // Better be in preview mode
  assert(m_PreviewSource[0] && m_PreviewSource[1] && m_PreviewSource[2]);

  // Create an index
  itk::Index<3> index;
  index[0] = point[0];
  index[1] = point[1];
  index[2] = point[2];

  // Make sure the slice is current
  m_Slicer[0]->Update();

  // Make sure the voxel is in the computed region
  assert(m_Slicer[0]->GetInput()->GetBufferedRegion().IsInside(index));

  // Return the pixel
  return m_Slicer[0]->GetInput()->GetPixel(index);    
}

