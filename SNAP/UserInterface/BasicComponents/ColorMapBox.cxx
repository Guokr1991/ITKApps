/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    ColorMapBox.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "ColorMapBox.h"
#include "SNAPOpenGL.h"

ColorMapBox
::ColorMapBox(int lx,int ly,int lw,int lh,const char *llabel)
  : FLTKCanvas(lx, ly, lw, lh, llabel)
{
  m_RangeStart = 0.0;
  m_RangeEnd = 0.0;
}

void
ColorMapBox
::draw()
{
  // The standard 'valid' business
  if(!valid())
    {
    // Set up the basic projection with a small margin
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.005,1.005,-0.05,1.05);
    glViewport(0,0,w(),h());

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();    
    }
    
  // Clear the viewport
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);

  // Disable lighting
  glDisable(GL_LIGHTING);

  // Draw the color map
  glBegin(GL_QUAD_STRIP);

  // Get the colors
  unsigned int n = 256;
  for(unsigned int ii = 0; ii <= n; ii++)
    {
    double u = ii * 1.0 / n;
    double t = m_RangeStart + u * (m_RangeEnd - m_RangeStart);
    SpeedColorMap::OutputType xColor = m_ColorMap(t);
    
    glColor3ub(xColor[0], xColor[1], xColor[2]);    
    glVertex2d(u, 0.0); glVertex2d(u, 1.0);
    }
  
  glEnd();

  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();
}
