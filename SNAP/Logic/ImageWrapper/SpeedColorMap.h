/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    SpeedColorMap.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __SpeedColorMap_h_
#define __SpeedColorMap_h_

#include "itkRGBAPixel.h"
#include "GlobalState.h"

/**
 * \class SpeedColorMap
 * \brief A very simple functor used to map intensities from 
 * the range (-1,1) to RGB color space.
 */
class SpeedColorMap 
{
public:
  typedef itk::RGBAPixel<unsigned char> OutputType;

  /** Constructor, sets default color map */
  SpeedColorMap();

  /** 
   * Mapping operator, maps range [-1, 1] into colors. This operator must
   * be very fast, because it is called a lot of times in the rendering 
   * pipeline. So we define it inline, and sacrifice generality for speed 
   */
  OutputType operator()(float t)
    {
    // We do bounds checking, just in case
    if(t < -1.0f) return m_ColorEntry.front();
    if(t >= 1.0f) return m_ColorEntry.back();

    // Project u into the array of intensities
    float u = t * m_DeltaT - m_Shift;
    unsigned int iu = (int) u;
    float a = u - iu;

    // Compute the correct color
    // (1 - a) c[iu] + a c[iu + 1] = c[iu] - a * (c[iu+1] - c[iu])
    OutputType p = m_ColorEntry[iu];
    p[0] = (unsigned char) (p[0] + a * m_ColorEntryDelta[iu][0]);
    p[1] = (unsigned char) (p[1] + a * m_ColorEntryDelta[iu][1]);
    p[2] = (unsigned char) (p[2] + a * m_ColorEntryDelta[iu][2]);

    // Return the point
    return p;
    }
  
  /** Set the color map by specifying three points (-1, 0 and 1) */
  void SetColorMap(OutputType inMinus, OutputType inZero, OutputType inPlus) 
    {
    OutputType colors[] = {inMinus, inZero, inPlus};
    SetColorMap(3, colors);
    }

  /** Set the color map for the positive range only */
  void SetColorMapPositive(OutputType inZero, OutputType inHalf, OutputType inOne)
    {
    OutputType colors[] = {inZero, inZero, inZero, inHalf, inOne};
    SetColorMap(5, colors);
    }

  /** Set the color map by specifying a set of n colors between -1 and 1 */
  void SetColorMap(unsigned int n, OutputType *colors);

  /** Generate a color map for one of the presets */
  static SpeedColorMap GetPresetColorMap(ColorMapPreset xPreset);

  bool operator!=( const SpeedColorMap & other ) const
  {
    bool value = ( ( this->m_DeltaT != other.m_DeltaT ) || 
                   ( this->m_Shift  != other.m_Shift  ) || 
                   ( this->m_ColorMapSize  != other.m_ColorMapSize ) );
      return value;
  }
    
private:
  // The colors in the color map
  std::vector<OutputType> m_ColorEntry;

  // Differences between pairs of colors
  std::vector<Vector3f> m_ColorEntryDelta;

  // Scaling factors used to reference color entries
  float m_DeltaT;
  float m_Shift;
  float m_ColorMapSize;
};  

#endif
