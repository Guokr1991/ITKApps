/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    ImageWrapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ImageWrapper_h_
#define __ImageWrapper_h_

// Smart pointers have to be included from ITK, can't forward reference them
#include "SNAPCommon.h"
#include "ImageCoordinateTransform.h"
#include <itkMinimumMaximumImageCalculator.h>
#include <itkImageRegionIterator.h>

// Forward declarations to IRIS classes
template <class TPixel> class IRISSlicer;
class SNAPSegmentationROISettings;


/**
 * \class ImageWrapper
 * \brief Abstract parent class for all image wrappers
 * \see ImageWrapper
 */
class ImageWrapperBase
{
public:
  virtual const ImageCoordinateTransform &GetImageToDisplayTransform(
    unsigned int) const = 0;
  virtual void SetImageToDisplayTransform(
    unsigned int, const ImageCoordinateTransform &) = 0;
  virtual Vector3ui GetSliceIndex() const = 0;
  virtual void SetSliceIndex(const Vector3ui &) = 0;
  virtual itk::ImageBase<3> *GetImageBase() const = 0;
  virtual bool IsInitialized() const = 0;
};

/**
 * \class ImageWrapper
 * \brief A wrapper around an itk::Image and related pipelines.
 * 
 * Image Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of images in
 * SNaP.  
 */
template<class TPixel> class ImageWrapper : public ImageWrapperBase
{
public:

  // Basic type definitions
  typedef itk::Image<TPixel,3> ImageType;
  typedef typename itk::SmartPointer<ImageType> ImagePointer;

  // Slice image type
  typedef itk::Image<TPixel,2> SliceType;
  typedef typename itk::SmartPointer<SliceType> SlicePointer;

  // Slicer type
  typedef IRISSlicer<TPixel> SlicerType;
  typedef typename itk::SmartPointer<SlicerType> SlicerPointer;

  // MinMax calculator type
  typedef itk::MinimumMaximumImageCalculator<ImageType> MinMaxCalculatorType;
  typedef typename itk::SmartPointer<MinMaxCalculatorType> 
    MinMaxCalculatorPointer;

  // Iterator types
  typedef typename itk::ImageRegionIterator<ImageType> Iterator;
  typedef typename itk::ImageRegionConstIterator<ImageType> ConstIterator;

  /** 
   * Default constructor.  Creates an image wrapper with a blank internal 
   * image 
   */
  ImageWrapper();

  /** 
   * Copy constructor.  Copies the contents of the passed-in image wrapper. 
   */
  ImageWrapper(const ImageWrapper<TPixel> &copy);
  
  /** Destructor */
  virtual ~ImageWrapper();

  /**
   * Initialize the image wrapper to match another image wrapper, setting the
   * image pixels to a default value. The slice indices and the transforms will
   * all be updated to match the source
   */
  virtual void InitializeToWrapper(
    const ImageWrapperBase *source, const TPixel &value);

  /** 
   * Initialize the image wrapper with a combination of another image wrapper and
   * an actual image. This will update the indices and transforms from the 
   * source wrapper, otherwise, it's equivalent to SetImage()
   */ 
  virtual void InitializeToWrapper(
    const ImageWrapperBase *source, ImageType *image);

  /**
   * Initialize the internal image to a blank image of size x,y,z.
   */
  // virtual void InitializeToSize(unsigned int x, unsigned int y,unsigned int z);

  /**
   * Clear the data associated with storing an image
   */
  virtual void Reset();

  /**
   * Is the image initialized?
   */
  virtual bool IsInitialized() const;

  /**
   * Get reference to a voxel at a given position.
   */
  virtual TPixel &GetVoxelForUpdate(unsigned int x, 
                                    unsigned int y, 
                                    unsigned int z);
  
  virtual TPixel &GetVoxelForUpdate(const Vector3ui &index);
  
  /**
   * Get a constant reference to a voxel at a given position.
   */
  virtual const TPixel &GetVoxel(unsigned int x, 
                                 unsigned int y, 
                                 unsigned int z) const;

  virtual const TPixel &GetVoxel(const Vector3ui &index) const;

  /** Return some image info independently of pixel type */
  virtual itk::ImageBase<3> *GetImageBase() const
  {
    return m_Image.GetPointer();
  }

  /** 
   * Return the pointed to the ITK image encapsulated by this wrapper.
   */
  virtual ImageType *GetImage() const
  {
    return m_Image;
  }

  /**
   * Get the size of the image
   */
  virtual Vector3ui GetSize() const;

  /**
   * Get the minimum intensity value.  Call ComputeImageIntensityRange() 
   * first.
   */
  virtual TPixel GetImageMin();

  /**
   * Get the maximum intensity value.  Call ComputeImageIntensityRange() 
   * first.
   */
  virtual TPixel GetImageMax();

  /**
   * Get the scaling factor used to convert between intensities stored
   * in this image and the 'true' image intensities
   */
  virtual double GetImageScaleFactor();

  /**
   * Remap the intensity range of the image to a given range
   */
  virtual void RemapIntensityToRange(TPixel min, TPixel max);

  /**
   * Remap the intensity range to max possible range
   */
  virtual void RemapIntensityToMaximumRange();

  /** Get the current slice index */
  virtual Vector3ui GetSliceIndex() const;

  /**
   * Set the current slice index in all three dimensions.  The index should
   * be specified in the image coordinates, the slices will be generated
   * in accordance with the transforms that are specified
   */
  virtual void SetSliceIndex(const Vector3ui &cursor);

  /**
   * Set the trasforms from image space to one of the three display slices (be
   * sure to set all three, or you'll get weird looking slices!
   */
  virtual void SetImageToDisplayTransform(
    unsigned int iSlice,const ImageCoordinateTransform &transform);

  /** Get the coordinate transform for each display slice */
  virtual const ImageCoordinateTransform &GetImageToDisplayTransform(
    unsigned int iSlice) const;

  /**
   * Use a default image-slice transformation, the first slice is along z,
   * the second along y, the third, along x, all directions of traversal are 
   * positive.
   */
  virtual void SetImageToDisplayTransformsToDefault();

  /**
   * Get a slice of the image in a given direction
   */
  virtual SliceType *GetSlice(unsigned int dimension);

  /**
   * This method exposes the scalar pointer in the image
   */
  virtual const TPixel *GetVoxelPointer() const;

  /**
   * Pring debugging info
   * TODO: Delete this or make is worthwhile
   */
  virtual void PrintDebugInformation();
                       
  /**
   * Replace the image internally stored in this wrapper by another image.
   */
  virtual void SetImage(ImagePointer newImage);

  /**
   * This method is used to perform a deep copy of a region of this image 
   * into another image, potentially resampling the region to use a different
   * voxel size
   */
  ImagePointer DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                              itk::Command *progressCommand = NULL) const;

  /**
   * Get an iterator for traversing the image.  The iterator is initialized
   * to point to the beginning of the image
   */
  virtual ConstIterator GetImageConstIterator() const;
  virtual Iterator GetImageIterator();

  /** For each slicer, find out which image dimension does is slice along */
  unsigned int GetDisplaySliceImageAxis(unsigned int slice);

protected:

  /** The image that we are wrapping */
  ImagePointer m_Image;

  /** The associated slicer filters */
  SlicerPointer m_Slicer[3];

  /** 
   * The min-max calculator used to hold min/max values.  This should be
   * replaced by a filter, when the latter is more efficient
   */
  MinMaxCalculatorPointer m_MinMaxCalc;

  /** The intensity scaling factor */
  double m_ImageScaleFactor;

  /** The current cursor position (slice index) in image dimensions */
  Vector3ui m_SliceIndex;

  /**
   * Is the image wrapper initialized? That is a prerequisite for all 
   * operations.
   */
  bool m_Initialized;

  /** Transform from image space to display space */
  ImageCoordinateTransform m_ImageToDisplayTransform[3];

  /** Transform from image space to display space */
  ImageCoordinateTransform m_DisplayToImageTransform[3];

  /**
   * Compute the intensity range of the image if it's out of date.  
   * This is done before calling GetImateMin, GetImateMax and GetImageScaleFactor methods.
   */
  void CheckImageIntensityRange();

  /**
   * Handle a change in the image pointer (i.e., a load operation on the image or 
   * an initialization operation)
   */
  virtual void UpdateImagePointer(ImageType *);

  /** Common code for the different constructors */
  void CommonInitialization();
};

#endif // __ImageWrapper_h_
