/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    LevelSetExtensionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __LevelSetExtensionFilter_h_
#define __LevelSetExtensionFilter_h_

#include "itkFiniteDifferenceFunction.h" 
#include "itkFiniteDifferenceImageFilter.h" 
#include "itkMutexLock.h"
#include "itkConditionVariable.h"
#include "itkBarrier.h"
#include "itkCommand.h"

/** A generic extension of a filter (intended to be a 
 * FiniteDifferenceImageFilter) that let's use control it
 * in a VCR (play-stop-step-rewind) fashion */
template <class TFilter>
class LevelSetExtensionFilter : public TFilter  
{
public:
  
  /** Standard class typedefs. */
  typedef LevelSetExtensionFilter<TFilter> Self;
  typedef TFilter Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Run-time type information. */
  itkTypeMacro(LevelSetExtensionFilter,TFilter);

  /** Capture information from the superclass. */
  typedef typename Superclass::InputImageType   InputImageType;
  typedef typename Superclass::OutputImageType  OutputImageType;

  /** Dimensionality of input and output data is assumed to be the same.
   * It is inherited from the superclass. */
  itkStaticConstMacro(ImageDimension, unsigned int,Superclass::ImageDimension);

  /** ITK new macro */
  itkNewMacro(LevelSetExtensionFilter);

  /** The pixel type of the output image will be used in computations.
   * Inherited from the superclass. */
  typedef typename Superclass::PixelType PixelType;
  typedef typename Superclass::TimeStepType TimeStepType;

protected:
  LevelSetExtensionFilter() {}
  virtual ~LevelSetExtensionFilter() {}

  /** Just a print method */
  void PrintSelf(std::ostream& os, itk::Indent indent) const
  {
    Superclass::PrintSelf(os,indent);
  }

private:
  LevelSetExtensionFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented
 
};


#endif // __LevelSetExtensionFilter_h_

