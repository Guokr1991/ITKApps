/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    GreyImageWrapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __GreyImageWrapper_h_
#define __GreyImageWrapper_h_

#include "ImageWrapper.h"
// #include "IntensityCurveInterface.h"
// #include "UnaryFunctorCache.h"

// Forward references
namespace itk {
  template<class TInput,class TOutput> class FunctionBase;
  template<class TInput,class TOutput,class TFunctor> 
    class UnaryFunctorImageFilter;
};
template <class TInput, class TOutput, class TFunctor> 
  class UnaryFunctorCache;
template <class TInput, class TOutput, class TFunctor> 
  class CachingUnaryFunctor;

/**
 * \class GreyImageWrapper
 * \brief Image wrapper used for greyscale images in IRIS/SNAP.
 * 
 * Adds ability to remap intensity from short to byte using an
 * arbitrary function when outputing slices.
 */
class GreyImageWrapper : public ImageWrapper<GreyType>
{
public:

  // Definition of the intensity map function
  typedef itk::FunctionBase<float,float> IntensityMapType;

  // Definition for the display slice type
  typedef itk::Image<unsigned char,2> DisplaySliceType;
  typedef itk::SmartPointer<DisplaySliceType> DisplaySlicePointer;

  /**
   * Set the intensity curve to be used for mapping
   * image intensities for producing slices
   */
  void SetIntensityMapFunction(IntensityMapType *curve);   

  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim);

  /** Constructor initializes mappers */
  GreyImageWrapper();

  /** Destructor */
  ~GreyImageWrapper();

private:

  /**
   * This object is passed on to the cache for intensity mapping
   */
  class IntensityFunctor {
  public:

    /** Map a grey value */
    unsigned char operator()(const GreyType &value) const;

    // The storage for the float->float intensity map
    IntensityMapType *m_IntensityMap;

    // Intensity mapping factors
    GreyType m_IntensityMin;
    float m_IntensityFactor;

    /**
     * Set the range over which the input data is mapped to output data
     */
    void SetInputRange(GreyType intensityMin,GreyType intensityMax);
  };

  // Type of intensity function used to map 3D volume intensity into
  // 2D slice intensities
  typedef UnaryFunctorCache<GreyType,unsigned char,IntensityFunctor> CacheType;
  typedef itk::SmartPointer<CacheType> CachePointer;  
  typedef CachingUnaryFunctor<GreyType,unsigned char,IntensityFunctor>
     CacheFunctor;

  // Filters applied to slices
  typedef itk::Image<GreyType,2> GreySliceType;
  typedef itk::UnaryFunctorImageFilter<
    GreySliceType,DisplaySliceType,CacheFunctor> IntensityFilterType;
  typedef itk::SmartPointer<IntensityFilterType> IntensityFilterPointer;

  /**
   * An instance of the private intensity mapper (this mapper wraps the
   * passed in float->float function to a new function that is 
   * [min..max]->uchar)
   */
  IntensityFunctor m_IntensityFunctor;

  /**
   * A cache used for the intensity mapping function
   */
  CachePointer m_IntensityMapCache;

  /**
   * Filters used to remap the intensity of the slices in this image
   * into unsigned char images
   */
  IntensityFilterPointer m_IntensityFilter[3];
};

#endif // __GreyImageWrapper_h_
