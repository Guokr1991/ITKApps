/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    ImageCoordinateTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "ImageCoordinateTransform.h"
#include "vnl/vnl_inverse.h"

#include <cmath>

ImageCoordinateTransform
::ImageCoordinateTransform()
{
  // Transform is the identity
  m_Transform.fill(0.0f);
  m_Transform(0,0) = m_Transform(1,1) = m_Transform(2,2) = 1.0f;

  // The offset is zero
  m_Offset.fill(0.0f);

  ComputeSecondaryVectors();  
}

void 
ImageCoordinateTransform
::SetTransform(const Vector3i &map, const Vector3ui &size)
{
  unsigned int i;

  // Make sure it's a legal mapping
  for(int i=0;i<3;i++)
    {
    assert(abs(map[i]) <= 3 && abs(map[i]) > 0);
    assert(abs(map[i]) != abs(map[(i+1) % 3]));
    }
  
  // Initialize the transform matrix
  m_Transform.fill(0);
  for(i=0;i<3;i++)
    {
    if(map[i] > 0)
      m_Transform(map[i]-1,i) = 1.0;
    else
      m_Transform(-1-map[i],i) = -1.0;
    }

  // Initialize the offset vector
  m_Offset = m_Transform * to_float(size);

  for(i=0;i<3;i++)
    {
    // Update the offset vector to make it right
    m_Offset[i] = m_Offset[i] < 0 ? - m_Offset[i] : 0;
    }

  ComputeSecondaryVectors();  
}

void
ImageCoordinateTransform
::ComputeSecondaryVectors()
{
  // For this calculation we need the transpose of the matrix
  MatrixType T = m_Transform.transpose();

  Vector3f map = T * Vector3f(0.0f,1.0f,2.0f);
  m_AxesIndex[0] = (unsigned int) fabs(map[0]);
  m_AxesIndex[1] = (unsigned int) fabs(map[1]);
  m_AxesIndex[2] = (unsigned int) fabs(map[2]);

  m_AxesDirection = to_int(T * Vector3f(1.0f));
}



ImageCoordinateTransform
ImageCoordinateTransform
::Inverse() const
{
  // Compute the new transform's details
  ImageCoordinateTransform inv;
  inv.m_Transform = vnl_inverse(m_Transform);
  inv.m_Offset = - inv.m_Transform * m_Offset;

  // Compute additional quantities
  inv.ComputeSecondaryVectors();

  /*
  // Compute the transpose of the transform
  int newMapping[3];
  for(int i=0;i<3;i++)
    {
    if(m_Mapping[i] > 0)
      newMapping[m_Mapping[i]-1] = i+1;
    else
      newMapping[-1-m_Mapping[i]] = -i-1;
    }

  return ImageCoordinateTransform(newMapping[0],newMapping[1],newMapping[2]);
  */

  return inv;
}

ImageCoordinateTransform 
ImageCoordinateTransform
::Product(const ImageCoordinateTransform &t1) const
{
  // Compute the new transform's details
  ImageCoordinateTransform prod;
  prod.m_Transform = m_Transform * t1.m_Transform;
  prod.m_Offset = m_Transform * t1.m_Offset + m_Offset;

  // Compute additional quantities
  prod.ComputeSecondaryVectors();
/*
  // Multiply two transforms
  int newMapping[3];
  for(int i=0;i<3;i++)
    {
    if(m_Mapping[i] > 0)
      newMapping[i] = t1.m_Mapping[m_Mapping[i]-1];
    else
      newMapping[i] = -t1.m_Mapping[-1-m_Mapping[i]];
    }

  return ImageCoordinateTransform(newMapping[0],newMapping[1],newMapping[2]);
*/

  return prod;
}

Vector3f 
ImageCoordinateTransform
::TransformPoint(const Vector3f &x) const 
{
  return m_Transform * x + m_Offset;
/*  
  // Transform the vector
  Vector3f xVec = TransformVector(x);

  // Transform the size 
  Vector3f xSpace = TransformVector(xSourceVolumeSize);

  // Compute an offset to add to the vector
  if(xVec[0] < 0) xVec[0] -= xSpace[0];
  if(xVec[1] < 0) xVec[1] -= xSpace[1];
  if(xVec[2] < 0) xVec[2] -= xSpace[2];

  // Return the result
  return xVec;
*/  
}

Vector3f 
ImageCoordinateTransform
::TransformVector(const Vector3f &x) const 
{
  return m_Transform * x;
}

Vector3ui
ImageCoordinateTransform
::TransformSize(const Vector3ui &sz) const
{
  Vector3f szSigned = m_Transform * to_float(sz);
  return Vector3ui(
    (unsigned int)fabs(szSigned(0)),
    (unsigned int)fabs(szSigned(1)),
    (unsigned int)fabs(szSigned(2)));
}

Vector3ui 
ImageCoordinateTransform
::TransformVoxelIndex(const Vector3ui &x) const 
{
  // Convert to voxel center coordinates
  Vector3f xVoxelCenter = to_float(x) + Vector3f(0.5);

  // Make sure the mapping is legit
  Vector3f xMappedCenter = TransformPoint(xVoxelCenter);
  assert(xMappedCenter(0) >= 0.0f && 
         xMappedCenter(1) >= 0.0f &&
         xMappedCenter(2) >= 0.0f);

  // Compute the result and scale down to integers
  return to_unsigned_int(xMappedCenter);
}


