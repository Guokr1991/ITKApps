/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    IntensityCurveBox.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "IntensityCurveBox.h"
#include "IntensityCurveUILogic.h"
#include "GreyImageWrapper.h"
#include "itkImageRegionConstIterator.h"

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>

#include <FL/Fl.H>
#include "SNAPOpenGL.h"
#include <FL/fl_draw.H>


unsigned int IntensityCurveBox::CURVE_RESOLUTION = 64;

IntensityCurveBox
::IntensityCurveBox(int lx, int ly, int lw, int lh, const char *llabel)
: FLTKCanvas(lx,ly,lw,lh,llabel), m_DefaultHandler(this)
{
  // Start with the blank curve
  m_Curve = NULL;
  m_Parent = NULL;

  // Initialize the histogram parameters
  m_HistogramBinSize = 1;
  m_HistogramMaxLevel = 1.0;
  m_HistogramLog = false;

  // Set up the default handler
  PushInteractionMode(&m_DefaultHandler);
}

void 
IntensityCurveBox
::draw() 
{       
  // The curve should have been initialized
  assert(m_Curve);

  if (!valid()) 
    {
    // Set up the basic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.025,1.025,-0.025,1.025);
    glViewport(0,0,w(),h());

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT,viewport);
    }

  // Clear the viewport
  glClearColor(.95,.95,.95,1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);

  // Disable lighting
  glDisable(GL_LIGHTING);

  // Draw the plot area. The intensities outside of the window and level
  // are going to be drawn in darker shade of gray
  float t0, t1, xDummy;
  m_Curve->GetControlPoint(0, t0, xDummy);
  m_Curve->GetControlPoint(m_Curve->GetControlPointCount() - 1, t1, xDummy);
  
  // Draw the quads
  glBegin(GL_QUADS);

  // Outer quad
  glColor3d(0.9, 0.9, 0.9);
  glVertex2d(0,0);
  glVertex2d(0,1);
  glVertex2d(1,1);
  glVertex2d(1,0);

  // Inner quad
  glColor3d(1.0, 1.0, 1.0);
  glVertex2d(t0, 0.0); 
  glVertex2d(t0, 1.0); 
  glVertex2d(t1, 1.0); 
  glVertex2d(t1, 0.0);

  glEnd();

  // Draw a histogram if it exists
  if(m_Histogram.size())
    {
    // Compute the heights of all the histogram bars
    float xWidth = 1.0f / m_Histogram.size();
    unsigned int xPixelsPerBin = w() / m_Histogram.size();
    std::vector<double> xHeight(m_Histogram.size(), 0.0);

    // Start by computing the maximum (cutoff) height
    float xHeightMax = m_HistogramMax * m_HistogramMaxLevel;
    if(m_HistogramLog)
      xHeightMax = log(xHeightMax) / log(10.0);

    // Continue by computing each bin's height
    unsigned int ii;
    for(ii=0;ii < m_Histogram.size();ii++)
      {
      // Process the histogram height based on options
      xHeight[ii] = m_Histogram[ii];
      if(m_HistogramLog)
        xHeight[ii] = (xHeight[ii] > 0) ? log(xHeight[ii]) / log(10.0) : 0;
      if(xHeight[ii] > xHeightMax)
        xHeight[ii] = xHeightMax / 0.9f;
      }

    // Draw the horizontal lines at various powers of 10 if in log mode
    if(m_HistogramLog)
      {
      glColor3d(0.75, 0.75, 0.75);
      glBegin(GL_LINES);
      for(double d = 1.0; d < xHeightMax; d+=1.0)
        { glVertex2f(0,d); glVertex2f(1,d); }
      glEnd();
      }
      
    // Paint the filled-in histogram bars. We fill in with black color if the 
    // histogram width is less than 4 pixels
    if(xPixelsPerBin < 4) 
      glColor3f(0.0f, 0.0f, 0.0f); 
    else 
      glColor3f(0.8f, 0.8f, 1.0f);

    // Paint the bars as quads
    glBegin(GL_QUADS);
    for(ii=0;ii < m_Histogram.size();ii++)
      {
      // Compute the physical height of the bin
      float xBin = xWidth * ii;
      float hBin = xHeight[ii] * 0.9f / xHeightMax;

      // Paint the bar
      glVertex2f(xBin,0);
      glVertex2f(xBin,hBin);
      glVertex2f(xBin+xWidth,hBin);
      glVertex2f(xBin+xWidth,0);
      }
    glEnd();

    // Draw lines around the quads, but only if the bins are thick
    if(xPixelsPerBin >= 4)
      {
      // Draw the vertical lines between the histogram bars
      glBegin(GL_LINE_STRIP);
      glColor3d(0.0, 0.0, 0.0);
      for(ii = 0; ii < m_Histogram.size(); ii++)
        {
        // Compute the physical height of the bin
        float xBin = xWidth * ii;
        float hBin = xHeight[ii] * 0.9f / xHeightMax;

        // Draw around the bar, starting at the lower left corner
        glVertex2f(xBin, 0.0f);
        glVertex2f(xBin, hBin);
        glVertex2f(xBin + xWidth, hBin);
        glVertex2f(xBin + xWidth, 0.0f);
        }
      glEnd();
      }
    }

  // Draw the box around the plot area
  glColor3d(0,0,0);
  glBegin(GL_LINE_LOOP);
  glVertex2d(0,0);
  glVertex2d(0,1);
  glVertex2d(1,1);
  glVertex2d(1,0);
  glEnd();

  // Set up the smooth line drawing style
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2.0);

  // Draw the curve using linear segments
  glColor3d(1.0,0.0,0.0);
  glBegin(GL_LINE_STRIP);

  float t = 0.0;
  float tStep = 1.0f / (CURVE_RESOLUTION);
  for (unsigned int ii=0;ii<=CURVE_RESOLUTION;ii++) 
    {
    glVertex2f(t,m_Curve->Evaluate(t));
    t+=tStep;
    }

  glEnd();

  // Draw the handles
  for (unsigned int c=0;c<m_Curve->GetControlPointCount();c++) 
    {
    // Get the next control point
    float tt,xx;
    m_Curve->GetControlPoint(c,tt,xx);

    // Draw a quad around the control point

    double rx = 5.0 / w();
    double ry = 5.0 / h();

    glColor3d(1,1,0.5);
    glBegin(GL_QUADS);
    glVertex2d(tt,xx-ry);
    glVertex2d(tt+rx,xx);
    glVertex2d(tt,xx+ry);
    glVertex2d(tt-rx,xx);
    glEnd();       

    glColor3d(0,0,0);
    glLineWidth(1.0);
    glColor3d(1.0,0.0,0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(tt,xx-ry);
    glVertex2d(tt+rx,xx);
    glVertex2d(tt,xx+ry);
    glVertex2d(tt-rx,xx);
    glEnd();

    }

  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();
}

int 
  IntensityCurveBox
::GetControlPointInVincinity(float xx, float yy, int pixelRadius) 
{
  float rx = pixelRadius * 1.0f / w();
  float ry = pixelRadius * 1.0f / h();
  float fx = 1.0f / (rx * rx);
  float fy = 1.0f / (ry * ry);

  float minDistance = 1.0f;
  int nearestPoint = -1;

  for (unsigned int c=0;c<m_Curve->GetControlPointCount();c++)
    {

    // Get the next control point
    float cx,cy;
    m_Curve->GetControlPoint(c,cx,cy);

    // Check the distance to the control point
    float d = (cx - xx) * (cx - xx) * fx + (cy - yy) * (cy - yy) * fy;
    if (minDistance >= d)
      {
      minDistance = d;
      nearestPoint = c;
      }
    }

  // Negative: return -1
  return nearestPoint;
}

void 
IntensityCurveBox
::ComputeHistogram(GreyImageWrapper *source, unsigned int iMinPixelsPerBin)
{
  // Need a wrapper
  assert(source);
  assert(this->w() > 0);

  // Get 'absolute' image intensity range, i.e., the largest and smallest
  // intensity in the whole image
  GreyType iAbsMin = source->GetImageMin();
  GreyType iAbsMax = source->GetImageMax();
  unsigned int nFrequencies = (iAbsMax - iAbsMin) + 1;

  // Compute the freqencies of the intensities
  unsigned int *frequency = new unsigned int[nFrequencies];
  memset(frequency,0,sizeof(unsigned int) * nFrequencies);
  GreyImageWrapper::ConstIterator it = source->GetImageConstIterator();
  for(it.GoToBegin();!it.IsAtEnd();++it)
    {
    frequency[it.Value()-iAbsMin]++;
    }

  // Determine the bin size: no bin should be less than a single pixel wide
  if(nFrequencies * iMinPixelsPerBin > m_HistogramBinSize * this->w()) 
    m_HistogramBinSize = 
    (unsigned int) ceil(nFrequencies * iMinPixelsPerBin * 1.0 / this->w());
  unsigned int nBins = (unsigned int) ceil(nFrequencies * 1.0 / m_HistogramBinSize);

  // Allocate an array of bins
  m_Histogram.resize(nBins);
  fill(m_Histogram.begin(), m_Histogram.end(), 0);

  // Reset the max-frequency
  m_HistogramMax = 0;

  // Put the frequencies into the bins
  for(unsigned int ii=0;ii<nFrequencies;ii++)
    {
    unsigned int iBin = ii / m_HistogramBinSize;
    m_Histogram[iBin] += frequency[ii];

    // Compute the maximum frequency
    if(m_HistogramMax < m_Histogram[iBin])
      m_HistogramMax  = m_Histogram[iBin]; 
    }

  // Clear the frequencies
  delete frequency;
}

IntensityCurveBox::DefaultHandler
::DefaultHandler(IntensityCurveBox *parent) 
: InteractionMode(parent)
{
  this->m_Parent = parent;
}

int 
IntensityCurveBox::DefaultHandler
::OnMousePress(const FLTKEvent &event)
{
  // Check the control point affected by the event
  m_MovingControlPoint = 
    m_Parent->GetControlPointInVincinity(event.XSpace[0],event.XSpace[1],5);

  SetCursor(m_MovingControlPoint);

  return 1;
}

int 
IntensityCurveBox::DefaultHandler
::OnMouseRelease(const FLTKEvent &event,
  const FLTKEvent &irisNotUsed(dragEvent))
{
  if (m_MovingControlPoint >= 0) {
    // Update the control point
    if (UpdateControl(event.XSpace))
      {
      // Repaint parent
      m_Parent->redraw();

      // Fire the update event (should this be done on drag?)
      m_Parent->GetParent()->OnCurveChange();
      }

    // Set the cursor back to normal
    SetCursor(-1);
  }

  return 1;
}

int 
IntensityCurveBox::DefaultHandler
::OnMouseDrag(const FLTKEvent &event,
  const FLTKEvent &irisNotUsed(dragEvent))
{
  if (m_MovingControlPoint >= 0) 
    {
    // Update the moving control point
    if (UpdateControl(event.XSpace)) 
      {
      // Repaint parent
      m_Parent->redraw();

      // Fire the update event (should this be done on drag?)
      m_Parent->GetParent()->OnCurveChange();
      }
    }

  return 1;
}

int 
IntensityCurveBox::DefaultHandler
::OnMouseEnter(const FLTKEvent &irisNotUsed(event))
{
  return 1;
}

int 
  IntensityCurveBox::DefaultHandler
::OnMouseLeave(const FLTKEvent &irisNotUsed(event))
{
  return 1;
}

int 
IntensityCurveBox::DefaultHandler
::OnMouseMotion(const FLTKEvent &event)
{
  int cp = 
    m_Parent->GetControlPointInVincinity(event.XSpace[0],event.XSpace[1],5);

  SetCursor(cp);

  return 1;
}

void 
IntensityCurveBox::DefaultHandler
::SetCursor(int cp) 
{
  if (cp < 0) 
    {
    fl_cursor(FL_CURSOR_DEFAULT);
    }   
  else if (cp == 0 || 
    cp == (int)(m_Parent->GetCurve()->GetControlPointCount()-1)) 
    {
    fl_cursor(FL_CURSOR_WE);
    } 
  else     
    {
    fl_cursor(FL_CURSOR_MOVE);
    }
}

bool 
IntensityCurveBox::DefaultHandler
::UpdateControl(const Vector3f &p) 
{
  // Take a pointer to the spline for convenience
  IntensityCurveInterface &curve = *m_Parent->GetCurve();
  int last = curve.GetControlPointCount()-1;

  // Check whether the motion is out of range
  if (p[0] < 0.0 || p[0] > 1.0)
    return false;

  // First and last control points are treated specially because they
  // provide windowing style behavior
  if (m_MovingControlPoint == 0 || m_MovingControlPoint == last) 
    {
    // Get the current domain
    float xMin,xMax,y;        
    curve.GetControlPoint(0,xMin,y);
    curve.GetControlPoint(last,xMax,y);

    // Check if the new domain is valid
    float epsilon = 0.02;
    if (m_MovingControlPoint == 0 && p[0] < xMax - epsilon) 
      {
      xMin = p[0];
      } 
    else if (m_MovingControlPoint == last && p[0] > xMin + epsilon) 
      {
      xMax = p[0];
      } 
    else 
      {
      // One of the conditions failed; the window has size <= 0
      return false;
      }

    // Change the domain of the curve
    curve.ScaleControlPointsToWindow(xMin,xMax);
    } 
  else if (m_MovingControlPoint > 0) 
    {
    // Check whether the Y coordinate is in range
    if (p[1] < 0.0 || p[1] > 1.0)
      return false;

    // Record the position of the curve before motion
    float x,y;
    curve.GetControlPoint(m_MovingControlPoint,x,y);

    // Update the control point
    curve.UpdateControlPoint(m_MovingControlPoint,(float) p[0],(float) p[1]);

    // Check the curve for monotonicity
    if (!curve.IsMonotonic()) {
      curve.UpdateControlPoint(m_MovingControlPoint,x,y);
      return false;
    }
    }


  return true;
}

