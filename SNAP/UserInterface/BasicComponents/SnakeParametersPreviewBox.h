/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    SnakeParametersPreviewBox.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/

#ifndef __SnakeParametersPreviewBox_h_
#define __SnakeParametersPreviewBox_h_

#include "FLTKCanvas.h"
#include "SnakeParameters.h"
#include <vector>

template<class TPixel> class OpenGLSliceTexture;
class SnakeParametersPreviewPipeline;

/**
 * \class SnakeParametersPreviewBox
 * \brief A user interface component used to preview snake 
 * parameters in 2D.  
 * 
 * This component displays a portion of an image and a curve representing
 * an evolving snake.  Using the snake equation parameters that the user specifies,
 * forces normal to the boundary of the image are shown as vectors
 */
class SnakeParametersPreviewBox : public FLTKCanvas
{
public:
  SnakeParametersPreviewBox(int x, int y, int w, int h, const char *label=0);
  virtual ~SnakeParametersPreviewBox();

  /** A preview pipeline that has the logic of this class */
  irisSetMacro(Pipeline,SnakeParametersPreviewPipeline *);
  irisGetMacro(Pipeline,SnakeParametersPreviewPipeline *);
  
  /** An enumeration of different display modes for this widget */
  enum DisplayMode 
    {
    PROPAGATION_FORCE,CURVATURE_FORCE,ADVECTION_FORCE,TOTAL_FORCE
    };

  /** Set the display mode */
  irisSetMacro(ForceToDisplay,DisplayMode);
  
  /** Draw method - paints the widget */
  void draw();
  
protected:
  
  // Texture type for drawing speed images
  typedef OpenGLSliceTexture<float> TextureType;

  /** Preview pipeline logic */
  SnakeParametersPreviewPipeline *m_Pipeline;
  
  /** A texture object used to store the image */
  TextureType *m_Texture;
  
  /** Which force is being displayed? */
  DisplayMode m_ForceToDisplay;

  /** An interaction mode for point manipulation */
  class Interactor : public InteractionMode {
  public:
    Interactor(SnakeParametersPreviewBox *owner);

    int OnMousePress(const FLTKEvent &event);
    int OnMouseRelease(const FLTKEvent &event, const FLTKEvent &irisNotUsed(pressEvent));
    int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &irisNotUsed(pressEvent));
    void OnDraw();
  private:
    SnakeParametersPreviewBox *m_Owner;
    bool m_ControlsVisible;
    bool m_ControlPicked;
    unsigned int m_ActiveControl;
  };

  // Allow private access to the interactor
  friend class SnakeParametersPreviewBox::Interactor;

  /** The interactor */
  Interactor m_Interactor;
};





#endif // __SnakeParametersPreviewBox_h_
