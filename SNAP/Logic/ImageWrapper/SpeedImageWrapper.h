/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    SpeedImageWrapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __SpeedImageWrapper_h_
#define __SpeedImageWrapper_h_

#include "ImageWrapper.h"
#include "SpeedImageWrapper.h"
#include "itkRGBAPixel.h"

// Forward references to ITK
namespace itk {
  template <class TInput,class TOutput,class TFunctor> 
    class UnaryFunctorImageFilter;
  template <class TOutput> class ImageSource;
};

// Disable 'inheritance by dominance' warining in VC6
#ifdef _WIN32
  #pragma warning (disable: 4250)
#endif

/**
 * \class SpeedImageWrapper
 * \brief Image wraper for speed images in SNAP
 *
 * This wrapper remaps floating point slices to byte slices differently
 * depending on if it's in InOut snake mode (some speed values are negative) or
 * in Edge snake mode (speed values are nonnegative).
 *
 * \sa ImageWrapper
 */
class SpeedImageWrapper : public ImageWrapper<float>
{
public:
  // Basics
  typedef SpeedImageWrapper Self;
  typedef ImageWrapper<float> Superclass;
  typedef Superclass::ImageType ImageType;

  // The type definition for the image used to display speed slices
  typedef itk::RGBAPixel<unsigned char> DisplayPixelType;
  typedef itk::Image<DisplayPixelType,2> DisplaySliceType;
  typedef itk::SmartPointer<DisplaySliceType> DisplaySlicePointer;

  // The type definition for the image used to display overlays based on
  // speed images
  typedef itk::RGBAPixel<unsigned char> OverlayPixelType;
  typedef itk::Image<OverlayPixelType,2> OverlaySliceType;
  typedef itk::SmartPointer<OverlaySliceType> OverlaySlicePointer;

  /**
   * Set the preview source for the slices.  This means that the slices
   * will be generated not from the internal image but from external 
   * images (outputs of some preprocessing filters)
   */
  void SetSliceSourceForPreview(unsigned int slice,ImageType *source);

  /** 
   * Unset the preview sources for all slices.  The slices will be now
   * generated from the internal image 
   */
  void RemoveSliceSourcesForPreview();

  /** Get a 'preview' voxel, i.e., a voxel from the previewing slices.  For
   * the results to be valid, the voxel has to be on one of the previewing
   * slices, and this method is intended for getting the voxel at the
   * cross-hairs position */
  float GetPreviewVoxel(const Vector3ui &point) const;

  /**
   * Indicate that this image is a In/Out speed image that has a 
   * range of -1 to +1.  
   */
  void SetModeToInsideOutsideSnake()
    {
    m_IsModeInsideOutside = true;
    }
  
  /**
   * Indicate that this image is a Edge speed image that has a 
   * range of 0 to 1.  
   */
  void SetModeToEdgeSnake()
    {
    m_IsModeInsideOutside = false;
    }

  /**
   * Check if the image is in the Inside/Outside or Edge mode
   */
  bool IsModeInsideOutsideSnake() const
    {
    return m_IsModeInsideOutside;
    }
    
  /**
   * Check if the image is in the Inside/Outside or Edge mode
   */
  bool IsModeEdgeOutsideSnake() const
    {
    return !m_IsModeInsideOutside;
    }
  
  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySliceType *GetDisplaySlice(unsigned int dim);

  /**
   * Get an overlay mask slice for displaying on top of the greylevel
   * segmentation image.  Such slices are used for example to overlay
   * the thresholding result over the grey slice to give users feedback 
   * of the segmentation
   */
  OverlaySliceType *GetOverlaySlice(unsigned int dim);

  /**
   * Set the overlay cutoff range.  The intensities above the cutoff will
   * be included in the overlay
   */
  void SetOverlayCutoff(float cutoff);  
  float GetOverlayCutoff() const;

  typedef itk::ImageSource<ImageType> PreviewFilterType;
  typedef itk::SmartPointer<PreviewFilterType> PreviewFilterPointer;
  
  /**
   * Set the color for overlay drawing
   */
  void SetOverlayColor(const OverlayPixelType &color);
  OverlayPixelType GetOverlayColor() const;
  
  /** Constructor initializes mappers */
  SpeedImageWrapper();

  /** Destructor */
  ~SpeedImageWrapper();

protected:
  /** We override this method in order to maintain the preview sources 
   * when the image gets changed */
  void UpdateImagePointer(ImageType *newImage);

private:
  /**
   * A very simple functor used to map intensities
   */
  class MappingFunctor 
  {
  public:
    MappingFunctor();
    DisplayPixelType operator()(float in);
    void SetColorMap(DisplayPixelType inPlus, 
      DisplayPixelType inMinus, DisplayPixelType inZero); 
  private:
    DisplayPixelType m_Plus, m_Minus, m_Zero;
  };  
  
  /**
   * A functor used for overlay mapping
   */
  class OverlayFunctor 
  {
  public:
    /** Operator used to map pixels to color */
    OverlayPixelType operator()(float in);

    /** Overlay cutoff */
    float m_Cutoff;

    /** Overlay color */
    OverlayPixelType m_Color;
  };  
  
  // Type of the display intensity mapping filter used when the 
  // input is a in-out image
  typedef itk::UnaryFunctorImageFilter<
    ImageWrapper<float>::SliceType,DisplaySliceType,MappingFunctor> 
    IntensityFilterType;
  typedef itk::SmartPointer<IntensityFilterType> IntensityFilterPointer;

  // A mapping filter used to construct overlay images.  This filter assigns
  // an opaque color to pixels over the cutoff threshold
  typedef itk::UnaryFunctorImageFilter<
    ImageWrapper<float>::SliceType,OverlaySliceType,OverlayFunctor> 
    OverlayFilterType;
  typedef itk::SmartPointer<OverlayFilterType> OverlayFilterPointer;
    
  /** Whether or not the image is in edge or in-out mode */
  bool m_IsModeInsideOutside;

  /** 
   * The filters used to remap internal speed image that can be 
   * in range of -1 to 1 to a display image in range 0 to 1
   */
  IntensityFilterPointer m_DisplayFilter[3];

  /**
   * The filters used to create overlay slices
   */
  OverlayFilterPointer m_OverlayFilter[3];

  /** The currently used overlay functor */
  OverlayFunctor m_OverlayFunctor;

  /** Preview sources */
  ImagePointer m_PreviewSource[3];
};

#endif // __SpeedImageWrapper_h_
