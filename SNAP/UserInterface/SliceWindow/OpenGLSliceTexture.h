/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    OpenGLSliceTexture.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __OpenGLSliceTexture_h_
#define __OpenGLSliceTexture_h_

#ifndef _WIN32
#ifndef GLU_VERSION_1_2
#define GLU_VERSION_1_2
#endif
#endif

#include <FL/gl.h>
#include <GL/glu.h>

#include "itkImage.h"
#include "itkConstantPadImageFilter.h"

/**
 * \class OpenGLSliceTexture
 * \brief This class is used to turn a 2D ITK image of (arbitrary) type
 * into a GL texture.  
 *
 * The calls to Update will make sure that the texture is up to date.  
 */
template<class TPixel> class OpenGLSliceTexture 
{
public:
  // Image typedefs
  typedef itk::Image<TPixel,2> ImageType;
  typedef typename itk::SmartPointer<ImageType> ImagePointer;

  /** Constructor, initializes the texture object */
  OpenGLSliceTexture();

  /** Destructor, deallocates texture memory */
  virtual ~OpenGLSliceTexture();
  
  /** Pass in a pointer to a 2D image */
  void SetImage(ImagePointer inImage);

  /** Get the dimensions of the texture image, which are powers of 2 */
  irisGetMacro(TextureSize,Vector2ui);

  /** Get the GL texture number automatically allocated by this object */
  irisGetMacro(TextureIndex,int);

  /** Set the number of components used in call to glTextureImage */
  irisSetMacro(GlComponents,GLuint);

  /** Get the format (e.g. GL_LUMINANCE) in call to glTextureImage */
  irisSetMacro(GlFormat,GLenum);

  /** Get the type (e.g. GL_UNSIGNED_INT) in call to glTextureImage */
  irisSetMacro(GlType,GLenum);

  /**
   * Make sure that the texture is up to date (reflects the image)
   */
  void Update();

  /**
   * Draw the texture in the current OpenGL context on a polygon with vertices
   * (0,0) - (size_x,size_y). Paramters are the background color of the polygon
   */
  void Draw(const Vector3d &clrBackground);

  /**
   * Draw the texture in transparent mode, with given level of alpha blending.
   */
  void DrawTransparent(unsigned char alpha);

private:
  
  // Filter typedefs
  typedef itk::ConstantPadImageFilter<ImageType,ImageType> FilterType;
  typedef typename itk::SmartPointer<FilterType> FilterPointer;

  // The dimensions of the texture as stored in memory
  Vector2ui m_TextureSize;

  // The pointer to the image from which the texture is computed
  ImagePointer m_Image;

  // The padding filter used to resize the image
  FilterPointer m_PadFilter;

  // The texture number (index)
  GLuint m_TextureIndex;

  // Has the texture been initialized?
  bool m_IsTextureInitalized;

  // The pipeline time of the source image (vs. our pipeline time)
  unsigned long m_UpdateTime;

  // The number of components for Gl op
  GLuint m_GlComponents;

  // The format for Gl op
  GLenum m_GlFormat;

  // The type for Gl op
  GLenum m_GlType;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "OpenGLSliceTexture.txx"
#endif

#endif // __OpenGLSliceTexture_h_
